/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Code supporting AP RO verification.
 */

#include "ap_ro_integrity_check.h"
#include "board_id.h"
#include "byteorder.h"
#include "console.h"
#include "crypto_api.h"
#include "extension.h"
#include "extension.h"
#include "flash.h"
#include "flash_info.h"
#include "shared_mem.h"
#include "stddef.h"
#include "stdint.h"
#include "timer.h"
#include "tpm_registers.h"
#include "usb_spi.h"
#include "usb_spi_board.h"

#define CPRINTS(format, args...) cprints(CC_SYSTEM, format, ##args)
#define CPRINTF(format, args...) cprintf(CC_SYSTEM, format, ##args)

#define VB2_KEYBLOCK_MAGIC	"CHROMEOS"
#define VB2_KEYBLOCK_MAGIC_SIZE (sizeof(VB2_KEYBLOCK_MAGIC) - 1)

/* FMAP must be aligned at 4K or larger power of 2 boundary. */
#define LOWEST_FMAP_ALIGNMENT  (4 * 1024)
#define FMAP_SIGNATURE	       "__FMAP__"
#define GSCVD_AREA_NAME	       "RO_GSCVD"
#define FMAP_AREA_NAME	       "FMAP"
#define FMAP_SIGNATURE_SIZE    (sizeof(FMAP_SIGNATURE) - 1)
#define FMAP_NAMELEN	       32
#define FMAP_MAJOR_VERSION     1
#define FMAP_MINOR_VERSION     1
#define KEYBLOCK_MAJOR_VERSION 2
#define KEYBLOCK_MINOR_VERSION 1

#define LOWEST_ACCEPTABLE_GVD_ROLLBACK 1

/*
 * A somewhat arbitrary maximum number of AP RO hash ranges to save. There are
 * 27 regions in a FMAP layout. The AP RO ranges should only be from the RO
 * region. It's unlikely anyone will need more than 32 ranges.
 * If there are AP RO hash issues, the team will likely need to look at the
 * value of each range what part of the FMAP it corresponds to. Enforce a limit
 * to the number of ranges, so it's easier to debug and to make people consider
 * why they would need more than 32 ranges.
 */
#define APRO_MAX_NUM_RANGES 32
/* Values used for sanity check of the flash_range structure fields. */
#define MAX_SUPPORTED_FLASH_SIZE (32 * 1024 * 1024)
#define MAX_SUPPORTED_RANGE_SIZE (4 * 1024 * 1024)

/* Version of the AP RO check information saved in the H1 flash page. */
#define AP_RO_HASH_LAYOUT_VERSION_0 0
#define AP_RO_HASH_LAYOUT_VERSION_1 1

/* Verification scheme V1. */
#define AP_RO_HASH_TYPE_FACTORY 0
/* Verification scheme V2. */
#define AP_RO_HASH_TYPE_GSCVD	1

/* A flash range included in hash calculations. */
struct ro_range {
	uint32_t flash_offset;
	uint32_t range_size;
};

/* Maximum number of RO ranges this implementation supports. */
struct ro_ranges {
	struct ro_range ranges[APRO_MAX_NUM_RANGES];
};

/*
 * Payload of the vendor command communicating a variable number of flash
 * ranges to be checked and the total sha256.
 *
 * The actual number of ranges is determined based on the actual payload size.
 */
struct ap_ro_check_payload {
	uint8_t digest[SHA256_DIGEST_SIZE];
	struct ro_range ranges[0];
} __packed;

/*
 * Hash of previously read and validated gsc verification data, stored in the
 * local cache.
 */
struct gvd_descriptor {
	uint32_t fmap_offset; /* Offsets in SPI flash. */
	uint32_t gvd_offset;
	uint32_t rollback;
	uint8_t digest[SHA256_DIGEST_SIZE];
};

/*
 * Header added for storing of the AP RO check information in the H1 flash
 * page. The checksum is a 4 byte truncated sha256 of the saved payload, just
 * a sanity check.
 */
struct ap_ro_check_header {
	uint8_t version;
	uint8_t type;
	 /* This field is ignored when type is AP_RO_HASH_TYPE_GSCVD. */
	uint16_t num_ranges;
	uint32_t checksum;
};

/* Format of the AP RO check information saved in the H1 flash page. */
struct ap_ro_check {
	struct ap_ro_check_header header;
	union {
		/* Used by the V1 scheme. */
		struct ap_ro_check_payload payload;
		/* Used by the V2 scheme. */
		struct gvd_descriptor descriptor;
	};
};

/* FMAP structures borrowed from host/lib/include/fmap.h in vboot_reference. */
struct fmap_header {
	char fmap_signature[FMAP_SIGNATURE_SIZE];
	uint8_t fmap_ver_major;
	uint8_t fmap_ver_minor;
	uint64_t fmap_base;
	uint32_t fmap_size;
	char fmap_name[FMAP_NAMELEN];
	uint16_t fmap_nareas;
} __packed;

struct fmap_area_header {
	uint32_t area_offset;
	uint32_t area_size;
	char area_name[FMAP_NAMELEN];
	uint16_t area_flags;
} __packed;

/*
 * Header of GSC Verification data saved in AP RO flash. The variable element
 * of range_count RO ranges is placed adjacent to this structure in the AP RO
 * flash.
 */
#define GSC_VD_MAGIC 0x65666135 /* Little endian '5 a f e' */
struct gsc_verification_data {
	uint32_t gv_magic;
	uint16_t size; /* Size of this struct in bytes inclusive */
	uint16_t major_version; /* Version of this struct layout. Starts at 0 */
	uint16_t minor_version;
	/*
	 * GSC will cache the counter value and will not accept verification
	 * data blobs with a lower value.
	 */
	uint16_t rollback_counter;
	uint32_t gsc_board_id; /* Locks blob to certain platform. */
	uint32_t gsc_flags; /* A field for future enhancements. */
	/*
	 * The location of fmap that points to this blob. This location must
	 * also be in one of the verified sections, expressed as offset in
	 * flash
	 */
	uint32_t fmap_location;
	uint32_t hash_alg; /* one of enum vb2_hash_algorithm alg. */
	/*
	 * SHAxxx(ranges[0].offset..ranges[0].size || ... ||
	 *        ranges[n].offset..ranges[n].size)
	 *
	 * Let the digest space allow to accommodate the largest possible one.
	 */
	uint8_t ranges_digest[SHA512_DIGEST_SIZE];
	uint32_t range_count; /* Number of VerifiedSection entries. */
	struct ro_range ranges[0];
};

/* Cryptographic entities defined in vboot_reference. */
struct vb2_signature {
	/* Offset of signature data from start of this struct */
	uint32_t sig_offset;
	uint32_t reserved0;

	/* Size of signature data in bytes */
	uint32_t sig_size;
	uint32_t reserved1;

	/* Size of the data block which was signed in bytes */
	uint32_t data_size;
	uint32_t reserved2;
};

struct vb2_packed_key {
	/* Offset of key data from start of this struct */
	uint32_t key_offset;
	uint32_t reserved0;

	/* Size of key data in bytes (NOT strength of key in bits) */
	uint32_t key_size;
	uint32_t reserved1;

	/* Signature algorithm used by the key (enum vb2_crypto_algorithm) */
	uint32_t algorithm;
	uint32_t reserved2;

	/* Key version */
	uint32_t key_version;
	uint32_t reserved3;
};

struct vb2_keyblock {
	/* Magic number */
	uint8_t magic[VB2_KEYBLOCK_MAGIC_SIZE];

	/* Version of this header format */
	uint32_t header_version_major;
	uint32_t header_version_minor;

	/*
	 * Length of this entire keyblock, including keys, signatures, and
	 * padding, in bytes
	 */
	uint32_t keyblock_size;
	uint32_t reserved0;

	/*
	 * Signature for this keyblock (header + data pointed to by data_key)
	 * For use with signed data keys
	 */
	struct vb2_signature keyblock_signature;

	/*
	 * SHA-512 hash for this keyblock (header + data pointed to by
	 * data_key) For use with unsigned data keys.
	 *
	 * Only supported for kernel keyblocks, not firmware keyblocks.
	 */
	struct vb2_signature keyblock_hash;

	/* Flags for key (VB2_KEYBLOCK_FLAG_*) */
	uint32_t keyblock_flags;
	uint32_t reserved1;

	/* Key to verify the chunk of data */
	struct vb2_packed_key data_key;
};

/*
 * The layout of RO_GSCVD area of AP RO flash is as follows:
 * struct gsc_verication_data,
 * ro_ranges, number of ranges is found in gsc verification data,
 * vb2_signature  signature of the two objects above
 * vb2_keyblock   contains the key used to generate the signature and
 *		  the signature of the key
 * vb2_packed_key RSA public key to use to verify the vb2_keyblock above.
 *		  The hash of this key is saved in the root_key_hash array
 *		  above.
 */

/*
 * Supported combination for signature and hashing algorithms used to wrap the
 * platform key, a subset of the values defined in vboot_reference.
 */
enum vb2_crypto_algorithm {
	VB2_ALG_RSA4096_SHA256 = 7,
};

/*
 * Containers for various objects, including the offsets of the objects in the
 * AP RO flash.
 */
struct gvd_container {
	uint32_t offset;
	struct gsc_verification_data gvd;
};

struct sig_container {
	uint32_t offset;
	struct vb2_signature sigh;
};
struct kb_container {
	uint32_t offset;
	struct vb2_keyblock *kb;
};
struct rootk_container {
	uint32_t offset;
	struct vb2_packed_key *rootk;
};

/*
 * Local representation of the RSA key and hashing mode, necessary for
 * verifying RSA signatures.
 */
struct vb_rsa_pubk {
	struct RSA rsa;
	enum hashing_mode hashing;
};

/* A helper structure representing a memory block in the GSC address space. */
struct memory_block {
	const void *base;
	size_t size;
};

/* Page offset for H1 flash operations. */
static const uint32_t h1_flash_offset_ =
	AP_RO_DATA_SPACE_ADDR - CONFIG_PROGRAM_MEMORY_BASE;

/* Fixed pointer at the H1 flash page storing the AP RO check information. */
static const struct ap_ro_check *p_chk =
	(const struct ap_ro_check *)AP_RO_DATA_SPACE_ADDR;

/*
 * Track if the AP RO hash was validated this boot. Must be cleared every AP
 * reset.
 */
static uint8_t validated_ap_ro_boot;

/*
 * In dev signed Cr50 images this is the hash of
 * tests/devkeys/kernel_subkey.vbpubk from vboot_reference tree. Will be
 * replaced with the hash of the real root prod key by the signer, before prod
 * signing.
 */
const __attribute__((section(".rodata.root_key_hash")))
uint8_t root_key_hash[] = {
#include "ap_ro_root_key_hash.inc"
};

/**
 * Read AP flash area into provided buffer.
 *
 * Expects AP flash access to be provisioned. Max size to read is limited.
 *
 * @param buf pointer to the buffer to read to.
 * @param offset offset into the flash to read from.
 * @param size number of bytes to read.
 * @param code_line line number where this function was invoked from.
 *
 * @return zero on success, -1 on failure.
 */
static int read_ap_spi(void *buf, uint32_t offset, size_t size, int code_line)
{
	if (size > MAX_SUPPORTED_RANGE_SIZE) {
		CPRINTS("%s: request to read %d bytes in line %d", __func__,
			size, code_line);
		return -1;
	}

	if (usb_spi_read_buffer(buf, offset, size)) {
		CPRINTS("Failed to read %d bytes at offset 0x%x in line %d",
			size, offset, code_line);
		return -1;
	}

	return 0;
}

/*
 **
 * Convert RSA public key representation between vb2 and dcrypto.
 *
 * Note that for signature verification the only required parameters are
 * exponent, N, and hashing type used to prepare the digest for signing. This
 * function ignores the d component of the key.
 *
 * Some basic validity checks are performed on input data.
 *
 * @param packedk vb2 packed RSA key read from AP flash.
 * @param pubk dcrypto representation of the RSA key, used for signature
 *             verification.
 *
 * @return zero on success, -1 on failure.
 */
static int unpack_pubk(const struct vb2_packed_key *packedk,
		       struct vb_rsa_pubk *pubk)
{
	const uint32_t *buf32;
	uint32_t exp_key_size;
	uint32_t exp_sig_size;
	uint32_t arr_size;

	switch (packedk->algorithm) {
	case VB2_ALG_RSA4096_SHA256:
		exp_sig_size = 512;
		pubk->hashing = HASH_SHA256;
		break;
	default:
		CPRINTS("unsupported algorithm %d", packedk->algorithm);
		return -1;
	}

	exp_key_size = exp_sig_size * 2 + 8;
	if (packedk->key_size != exp_key_size) {
		CPRINTS("key size mismatch %d %d", packedk->key_size,
			exp_key_size);
		return -1;
	}

	buf32 = (uint32_t *)((uintptr_t)packedk + packedk->key_offset);

	arr_size = buf32[0];

	if (arr_size != (exp_sig_size / sizeof(uint32_t))) {
		CPRINTS("array size mismatch %d %d", arr_size,
			(exp_sig_size / sizeof(uint32_t)));
		return -1;
	}

	pubk->rsa.e = 65537; /* This is the only exponent we support. */
	pubk->rsa.N.dmax = arr_size;
	pubk->rsa.N.d = (struct access_helper *)(buf32 + 2);
	pubk->rsa.d.dmax = 0; /* Not needed for signature verification. */

	return 0;
}

/**
 * Verify signature of the requested memory space.
 *
 * Memory space is represented as one or more memory_block structures.
 *
 * @param blocks a pointer to array of memory_block structures, the last entry
 *		 in the array has .base set to NULL.
 * @param pubk public RSA key used to verify the signature
 * @param sig_body pointer to the signature blob
 * @param sig_size size of the signature blob
 *
 * @return zero on success, non zero of failure (either incorrect hashing
 *	   algorithm or signature mismatch)
 */
static int verify_signature(struct memory_block *blocks,
			    const struct vb_rsa_pubk *pubk,
			    const void *sig_body, size_t sig_size)
{
	const void *digest;
	uint32_t digest_size;
	size_t i;
	union hash_ctx ctx;

	digest_size = DCRYPTO_hash_size(pubk->hashing);

	if (!digest_size ||
	    DCRYPTO_hw_hash_init(&ctx, pubk->hashing) != DCRYPTO_OK)
		return -1; /* Will never happen, inputs have been verified. */

	for (i = 0; blocks[i].base; i++)
		HASH_update(&ctx, blocks[i].base, blocks[i].size);

	digest = HASH_final(&ctx);

	return DCRYPTO_rsa_verify(&pubk->rsa, digest, digest_size, sig_body,
				  sig_size, PADDING_MODE_PKCS1, pubk->hashing) -
	       DCRYPTO_OK;
}

/**
 * Verify that the passed in key block is signed with the passed in key.
 *
 * @param kbc container of the signed key block
 * @param pubk RSA public key to validate the key block signature
 *
 * @return zero on success, non zero on failure,
 */
static int verify_keyblock(const struct kb_container *kbc,
			   const struct vb_rsa_pubk *pubk)
{
	int rv;
	struct memory_block blocks[2];
	const void *sig_body;

	blocks[1].base = NULL;

	blocks[0].size = kbc->kb->keyblock_signature.data_size;
	blocks[0].base = kbc->kb;

	sig_body = (const void *)((uintptr_t)&kbc->kb->keyblock_signature +
				  kbc->kb->keyblock_signature.sig_offset);
	rv = verify_signature(blocks, pubk, sig_body,
			      kbc->kb->keyblock_signature.sig_size);

	CPRINTS("Keyblock %sOK", rv ? "NOT " : "");

	return rv;
}

/* Clear validate_ap_ro_boot state. */
void ap_ro_device_reset(void)
{
	if (validated_ap_ro_boot)
		CPRINTS("%s: clear validated state", __func__);
	validated_ap_ro_boot = 0;
}

/* Erase flash page containing the AP RO verification data hash. */
static int ap_ro_erase_hash(void)
{
	int rv;

	/*
	 * TODO(vbendeb): Make this a partial erase, use refactored
	 * Board ID space partial erase.
	 */
	flash_open_ro_window(h1_flash_offset_, AP_RO_DATA_SPACE_SIZE);
	rv = flash_physical_erase(h1_flash_offset_, AP_RO_DATA_SPACE_SIZE);
	flash_close_ro_window();

	return rv;
}

/*
 * Leaving this function available for testing, will not be necessary in prod
 * signed images.
 */
static enum vendor_cmd_rc vc_seed_ap_ro_check(enum vendor_cmd_cc code,
					      void *buf, size_t input_size,
					      size_t *response_size)
{
	struct ap_ro_check_header check_header;
	const struct ap_ro_check_payload *vc_payload = buf;
	uint32_t vc_num_of_ranges;
	uint32_t i;
	uint8_t *response = buf;
	size_t prog_size;
	int rv;

	*response_size = 1; /* Just in case there is an error. */

	/* Neither write nor erase are allowed once Board ID is programmed. */
#ifndef CR50_DEV
	if (!board_id_is_erased()) {
		*response = ARCVE_BID_PROGRAMMED;
		return VENDOR_RC_NOT_ALLOWED;
	}
#endif

	if (input_size == 0) {
		/* Empty payload is a request to erase the hash. */
		if (ap_ro_erase_hash() != EC_SUCCESS) {
			*response = ARCVE_FLASH_ERASE_FAILED;
			return VENDOR_RC_INTERNAL_ERROR;
		}

		*response_size = 0;
		return EC_SUCCESS;
	}

	/* There should be at least one range and the hash. */
	if (input_size < (SHA256_DIGEST_SIZE + sizeof(struct ro_range))) {
		*response = ARCVE_TOO_SHORT;
		return VENDOR_RC_BOGUS_ARGS;
	}

	/* There should be an integer number of ranges. */
	if (((input_size - SHA256_DIGEST_SIZE) % sizeof(struct ro_range)) !=
	    0) {
		*response = ARCVE_BAD_PAYLOAD_SIZE;
		return VENDOR_RC_BOGUS_ARGS;
	}

	vc_num_of_ranges =
		(input_size - SHA256_DIGEST_SIZE) / sizeof(struct ro_range);

	if (vc_num_of_ranges > APRO_MAX_NUM_RANGES) {
		*response = ARCVE_TOO_MANY_RANGES;
		return VENDOR_RC_BOGUS_ARGS;
	}
	for (i = 0; i < vc_num_of_ranges; i++) {
		if (vc_payload->ranges[i].range_size >
		    MAX_SUPPORTED_RANGE_SIZE) {
			*response = ARCVE_BAD_RANGE_SIZE;
			return VENDOR_RC_BOGUS_ARGS;
		}
		if ((vc_payload->ranges[i].flash_offset +
		     vc_payload->ranges[i].range_size) >
		    MAX_SUPPORTED_FLASH_SIZE) {
			*response = ARCVE_BAD_OFFSET;
			return VENDOR_RC_BOGUS_ARGS;
		}
	}

	prog_size = sizeof(struct ap_ro_check_header) + input_size;
	for (i = 0; i < (prog_size / sizeof(uint32_t)); i++)
		if (((uint32_t *)p_chk)[i] != ~0) {
			*response = ARCVE_ALREADY_PROGRAMMED;
			return VENDOR_RC_NOT_ALLOWED;
		}

	check_header.version = AP_RO_HASH_LAYOUT_VERSION_1;
	check_header.type = AP_RO_HASH_TYPE_FACTORY;
	check_header.num_ranges = vc_num_of_ranges;
	app_compute_hash(buf, input_size, &check_header.checksum,
			 sizeof(check_header.checksum));

	flash_open_ro_window(h1_flash_offset_, prog_size);
	rv = flash_physical_write(h1_flash_offset_, sizeof(check_header),
				  (char *)&check_header);
	if (rv == EC_SUCCESS)
		rv = flash_physical_write(h1_flash_offset_ +
						  sizeof(check_header),
					  input_size, buf);
	flash_close_ro_window();

	if (rv != EC_SUCCESS) {
		*response = ARCVE_FLASH_WRITE_FAILED;
		return VENDOR_RC_WRITE_FLASH_FAIL;
	}

	*response_size = 0;
	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_SEED_AP_RO_CHECK, vc_seed_ap_ro_check);

static int verify_ap_ro_check_space(void)
{
	uint32_t checksum;
	size_t data_size;

	if (p_chk->header.type != AP_RO_HASH_TYPE_FACTORY)
		return EC_ERROR_CRC;

	data_size = p_chk->header.num_ranges * sizeof(struct ro_range) +
		    sizeof(struct ap_ro_check_payload);
	if (data_size > CONFIG_FLASH_BANK_SIZE) {
		CPRINTS("%s: bogus number of ranges %d", __func__,
			p_chk->header.num_ranges);
		return EC_ERROR_CRC;
	}

	app_compute_hash(&p_chk->payload, data_size, &checksum,
			 sizeof(checksum));

	if (memcmp(&checksum, &p_chk->header.checksum, sizeof(checksum))) {
		CPRINTS("%s: AP RO Checksum corrupted", __func__);
		return EC_ERROR_CRC;
	}

	return EC_SUCCESS;
}

/**
 * Check if v2 gsc verification data hash is present in the flash page.
 *
 * @return pointer to the valid gvd_descriptor, NULL if not found.
 */
static const struct gvd_descriptor *find_v2_entry(void)
{
	struct sha256_ctx ctx;

	if ((p_chk->header.version < AP_RO_HASH_LAYOUT_VERSION_1) ||
	    (p_chk->header.type != AP_RO_HASH_TYPE_GSCVD))
		return NULL;

	/* Verify entry integrity. */
	if (DCRYPTO_hw_sha256_init(&ctx) != DCRYPTO_OK)
		return NULL;

	SHA256_update(&ctx, &p_chk->descriptor, sizeof(p_chk->descriptor));
	if (DCRYPTO_equals(SHA256_final(&ctx), &p_chk->header.checksum,
		   sizeof(p_chk->header.checksum)) != DCRYPTO_OK) {
		CPRINTS("Descriptor checksum mismatch!");
		return NULL;
	}

	return &p_chk->descriptor;
}

/*
 * ap_ro_check_unsupported: Returns non-zero value if AP RO verification is
 *                          unsupported.
 *
 * Returns:
 *
 *  ARCVE_OK if AP RO verification is supported.
 *  ARCVE_NOT_PROGRAMMED if the hash is not programmed.
 *  ARCVE_FLASH_READ_FAILED if there was an error reading the hash.
 *  ARCVE_BOARD_ID_BLOCKED if ap ro verification is disabled for the board's rlz
 */
static enum ap_ro_check_vc_errors ap_ro_check_unsupported(int add_flash_event)
{

	if (ap_ro_board_id_blocked()) {
		CPRINTS("%s: BID blocked", __func__);
		return ARCVE_BOARD_ID_BLOCKED;
	}

	if (p_chk->header.num_ranges == (uint16_t)~0) {
		CPRINTS("%s: RO verification not programmed", __func__);
		if (add_flash_event)
			ap_ro_add_flash_event(APROF_SPACE_NOT_PROGRAMMED);
		return ARCVE_NOT_PROGRAMMED;
	}

	/* Is the contents intact? */
	if (!find_v2_entry() && (verify_ap_ro_check_space() != EC_SUCCESS)) {
		CPRINTS("%s: unable to read ap ro space", __func__);
		if (add_flash_event)
			ap_ro_add_flash_event(APROF_SPACE_INVALID);
		return ARCVE_FLASH_READ_FAILED; /* No verification possible. */
	}
	return ARCVE_OK;
}

/**
 * Find FMAP and RO_GSCVD areas in the FMAP table in AP flash.
 *
 * @param offset offset of the fmap in the flash
 * @param nareas number of areas in fmap
 * @param fmap container to save FMAP area information in
 * @param gscvd container to save RO_GSCVD area information in
 *
 * @return zero on success, -1 if both areas not found.
 */
static int find_areas(uint32_t offset, uint16_t nareas,
		      struct fmap_area_header *fmap,
		      struct fmap_area_header *gscvd)
{
	uint16_t i;
	struct fmap_area_header fmah;

	fmap->area_offset = 0;
	gscvd->area_offset = 0;

	if (nareas > 64) {
		CPRINTS("%s: too many areas: %d", __func__, nareas);
		return -1;
	}

	for (i = 0; i < nareas; i++) {
		if (read_ap_spi(&fmah, offset, sizeof(fmah), __LINE__))
			return -1;

		if (!memcmp(fmah.area_name, GSCVD_AREA_NAME,
			    sizeof(GSCVD_AREA_NAME)))
			memcpy(gscvd, &fmah, sizeof(*gscvd));
		else if (!memcmp(fmah.area_name, FMAP_AREA_NAME,
				 sizeof(FMAP_AREA_NAME)))
			memcpy(fmap, &fmah, sizeof(*fmap));

		if (fmap->area_offset && gscvd->area_offset)
			return 0;

		offset += sizeof(fmah);
	}

	CPRINTS("Could not find %s or %s area", GSCVD_AREA_NAME,
		FMAP_AREA_NAME);

	return -1;
}

/**
 * Find FMAP header in AP flash and copy it into the passed in structure.
 *
 * Verify validity of the found header.
 *
 * @param fmh pointer to the header to copy to
 *
 * @return offset of FMAP in AP flash, or zero, if not found.
 */
static uint32_t find_fmap(struct fmap_header *fmh)
{
	uint32_t offset;
	uint32_t step = MAX_SUPPORTED_FLASH_SIZE / 2;
	uint32_t skip_mask = ~(MAX_SUPPORTED_FLASH_SIZE - 1);
	bool fmap_found = false;

	do {
		for (offset = 0; offset < MAX_SUPPORTED_FLASH_SIZE;
		     offset += step) {
			if ((offset & skip_mask) == 0)
				continue;

			if (read_ap_spi(fmh->fmap_signature, offset,
					sizeof(fmh->fmap_signature), __LINE__))
				return 0;

			if (!memcmp(fmh->fmap_signature, FMAP_SIGNATURE,
				    sizeof(fmh->fmap_signature))) {
				/*
				 * TODO(vbendeb): address the possibility of a
				 * fake FMAP placed in addition to the real
				 * one.
				 */
				fmap_found = true;
				break;
			}
		}
		step >>= 1;
		skip_mask >>= 1;

	} while ((step >= LOWEST_FMAP_ALIGNMENT) && !fmap_found);

	if (!fmap_found) {
		CPRINTS("Could not find FMAP");
		return 0;
	}

	/* Read the rest of fmap header. */
	if (read_ap_spi(
		    &fmh->fmap_ver_major, offset + sizeof(fmh->fmap_signature),
		    sizeof(*fmh) - sizeof(fmh->fmap_signature),
		    __LINE__))
		return 0;

	/* Verify fmap validity. */
	if ((fmh->fmap_ver_major != FMAP_MAJOR_VERSION) ||
	    (fmh->fmap_ver_minor != FMAP_MINOR_VERSION) ||
	    (fmh->fmap_size > MAX_SUPPORTED_FLASH_SIZE)) {
		CPRINTS("invalid FMAP contents");
		return 0;
	}

	return offset;
}

/**
 * Read gsc verification data from AP flash.
 *
 * @param fmap_offset offset of FMAP in AP flash, used for validity check
 * @param gvdc pointer to the gvd container, the offset field initialized.
 *
 * @return zero on successful read, -1 otherwise.
 */
static int read_gscvd_header(uint32_t fmap_offset, struct gvd_container *gvdc)
{
	if (read_ap_spi(&gvdc->gvd, gvdc->offset, sizeof(gvdc->gvd), __LINE__))
		return -1;

	if ((gvdc->gvd.gv_magic != GSC_VD_MAGIC) ||
	    (gvdc->gvd.size !=
	     (sizeof(gvdc->gvd) +
	      sizeof(struct ro_range) * gvdc->gvd.range_count)) ||
	    (gvdc->gvd.fmap_location != fmap_offset)) {
		CPRINTS("Inconsistent GSCVD contents");
		return -1;
	}

	return 0;
}

/**
 * Read signature structure header.
 *
 * This function does not yet read the entire signature, it reads the header
 * to determine the size of the signature to be able to gain access to the
 * next field in the RO_GSCVD layout. This is done to save memory on the heap,
 * and defer signature memory allocation until the signature is necessary,
 * leaving the heap available for root key and key block.
 *
 * @param sigc pointer to the signature container.
 *
 * @return zero on success, -1 on failure.
 */
static int read_signature_header(struct sig_container *sigc)
{
	if (read_ap_spi(&sigc->sigh, sigc->offset, sizeof(sigc->sigh),
			__LINE__) ||
	    (sigc->sigh.sig_offset != sizeof(sigc->sigh))) {
		CPRINTS("Failed to read signature at %x", sigc->offset);
		return -1;
	}

	return 0;
}

/**
 * Check if an element fits into the keyblock.
 *
 * @param kb  keyblock to check against
 * @param el  address of the element
 * @param data_offset  element's data base offset from the element address
 * @param data_size  element's data size
 *
 * @return true if the element fits, false otherwise
 */
static bool element_fits(const struct vb2_keyblock *kb,
			 const void *el,
			 uint32_t data_offset,
			 uint32_t data_size)
{
	uintptr_t kb_base;
	uint32_t headroom;
	uintptr_t el_base;
	uint32_t size;

	kb_base = (uintptr_t)kb;
	size = kb->keyblock_size;
	el_base = (uintptr_t) el;
	headroom = kb_base + size - el_base;

	return (((el_base > kb_base) && (el_base < (kb_base + size))) &&
		(data_offset < headroom) &&
		(data_size <= (headroom - data_offset)));
}

/*
 * Read keyblock from AP flash.
 *
 * First read the header of the keyblock to determine the amount of memory it
 * needs, then allocated the necessary memory and read the full keyblock into
 * it. The caller will free allocated memory even if keyblock verification
 * fails and this function returns the error.
 *
 * Verify validity of the read keyblock by checking the version fields and
 * verifying that the component structures fit into the keyblock.
 *
 * @param kbc container to read the keyblock into.
 *
 * @return zero on success, -1 on failure.
 */
static int read_keyblock(struct kb_container *kbc)
{
	struct vb2_keyblock kb;

	if (read_ap_spi(&kb, kbc->offset, sizeof(kb), __LINE__) ||
	    (memcmp(kb.magic, VB2_KEYBLOCK_MAGIC, sizeof(kb.magic)))) {
		CPRINTS("Failed to read keyblock at %x", kbc->offset);
		return -1;
	}

	/* Let's allocate memory for the full keyblock. */
	if (shared_mem_acquire(kb.keyblock_size, (char **)&kbc->kb) !=
	    EC_SUCCESS) {
		kbc->kb = NULL;
		CPRINTS("Failed to allocate %d bytes for keyblock",
			kb.keyblock_size);
		return -1;
	}

	/* Copy keyblock header into the allocated buffer. */
	memcpy(kbc->kb, &kb, sizeof(kb));

	/* Read the rest of the keyblock. */
	if (read_ap_spi(kbc->kb + 1, kbc->offset + sizeof(kb),
			kb.keyblock_size - sizeof(kb), __LINE__))
		return -1;

	/*
	 * Check keyblock version and verify that all incorporated structures
	 * fit in.
	 */
	if ((kb.header_version_major != KEYBLOCK_MAJOR_VERSION) ||
	    (kb.header_version_minor != KEYBLOCK_MINOR_VERSION) ||
	    !element_fits(kbc->kb,
			  &kbc->kb->keyblock_signature,
			  kbc->kb->keyblock_signature.sig_offset,
			  kbc->kb->keyblock_signature.sig_size) ||
	    !element_fits(kbc->kb,
			  &kbc->kb->keyblock_hash,
			  kbc->kb->keyblock_hash.sig_offset,
			  kbc->kb->keyblock_hash.sig_size) ||
	    !element_fits(kbc->kb,
			  &kbc->kb->data_key,
			  kbc->kb->data_key.key_offset,
			  kbc->kb->data_key.key_size)) {
		CPRINTS("Invalid keyblock contents");
		return -1;
	}
	return 0;
}

/**
 * Read root key from AP flash.
 *
 * Allocate memory for the key, the caller will free the memory even if this
 * function returns error. Once the key is read verify its validity by
 * comparing its hash against the known value.
 *
 * @param rootkc container to place the root key into
 *
 * @return zero on success, -1 on failure.
 */
static int read_rootk(struct rootk_container *rootkc)
{
	struct sha256_ctx ctx;
	struct vb2_packed_key rootk;
	size_t total_size;

	if (read_ap_spi(&rootk, rootkc->offset, sizeof(rootk), __LINE__) ||
	    (rootk.key_offset != sizeof(rootk))) {
		CPRINTS("Failed to read root key at %x", rootkc->offset);
		return -1;
	}

	/* Let's read the root key body. */
	total_size = sizeof(rootk) + rootk.key_size + rootk.key_offset;
	if (shared_mem_acquire(total_size, (char **)&rootkc->rootk) !=
	    EC_SUCCESS) {
		rootkc->rootk = NULL;
		CPRINTS("Failed to allocated %d bytes",
			rootk.key_size + rootk.key_offset);
		return -1;
	}

	/* Copy key rootk header. */
	memcpy(rootkc->rootk, &rootk, sizeof(rootk));
	if (read_ap_spi(rootkc->rootk + 1, rootkc->offset + sizeof(rootk),
			total_size - sizeof(rootk), __LINE__))
		return -1;

	if (DCRYPTO_hw_sha256_init(&ctx) != DCRYPTO_OK)
		return -1;

	SHA256_update(&ctx, rootkc->rootk, sizeof(rootk) + rootk.key_size);
	if (DCRYPTO_equals(SHA256_final(&ctx), root_key_hash,
			   sizeof(root_key_hash)) != DCRYPTO_OK) {
		CPRINTS("Root key digest mismatch");
		return -1;
	}

	return 0;
}

/**
 * Validate hash of AP flash ranges.
 *
 * Invoke service function to sequentially calculate sha256 hash of the AP
 * flash memory ranges, and compare the final hash with the expected value.
 *
 * @param ranges array of ranges to include in hash calculation
 * @param count number of ranges in the array
 * @param expected_digest pointer to the expected sha256 digest value.
 *
 * @return zero if digest matches, EC_ERROR_CRC if it does not. This value
 *	   is used by the caller to decide if AP boot should be allowed or
 *	   not.
 */
static int validate_ranges_sha(const struct ro_range *ranges, size_t count,
			       const uint8_t *expected_digest)
{
	int8_t digest[SHA256_DIGEST_SIZE];
	size_t i;
	struct sha256_ctx ctx;

	usb_spi_sha256_start(&ctx);
	for (i = 0; i < count; i++) {
		CPRINTS("%s: %x:%x", __func__, ranges[i].flash_offset,
			ranges[i].range_size);
		/* Make sure the message gets out before verification starts. */
		cflush();
		usb_spi_sha256_update(&ctx, ranges[i].flash_offset,
				      ranges[i].range_size);
	}

	usb_spi_sha256_final(&ctx, digest, sizeof(digest));
	if (DCRYPTO_equals(digest, expected_digest, sizeof(digest)) !=
	    DCRYPTO_OK) {
		CPRINTS("AP RO verification FAILED!");
		CPRINTS("Calculated digest %ph",
			HEX_BUF(digest, sizeof(digest)));
		CPRINTS("Stored digest %ph",
			HEX_BUF(expected_digest, sizeof(digest)));
		return EC_ERROR_CRC;
	}

	return EC_SUCCESS;
}

/**
 * Read ranges as defined in gsc_verification_data structure.
 *
 * @param gvdc pointer to the gsc_verifcation_data container
 * @param ranges pointer to the array of ro_ranges structure to fill up
 *
 * @return zero on success, non zero on failure.
 */
static int read_ranges(const struct gvd_container *gvdc,
		       struct ro_ranges *ranges)
{
	size_t range_count = gvdc->gvd.range_count;

	if (range_count > ARRAY_SIZE(ranges->ranges)) {
		CPRINTS("Too many ranges in gvd (%d)", range_count);
		return -1;
	}

	return read_ap_spi(ranges->ranges, gvdc->offset + sizeof(gvdc->gvd),
			   sizeof(struct ro_range) * range_count, __LINE__);
}

/**
 * Verify validity of the gsc_verification_data
 *
 * The signature covers the structure itself and the ranges array describing
 * which AP flash area are covered.
 *
 * This function allocates and frees memory to read the actual signature blob
 * from AP flash, based on signature container information.
 *
 * @param gvd pointer to the gsc_verification_data header
 * @param ranges pointer to the array of ranges, AP flash offset:size pairs
 * @param key pointer RSA key used for signing, vb2 representation
 * @param sig_container pointer to signature container, vb2 representation
 *
 * return 0 on success, nonzero on failure.
 */
static int verify_gvd_signature(const struct gsc_verification_data *gvd,
				const struct ro_ranges *ranges,
				const struct vb2_packed_key *key,
				const struct sig_container *sigc)
{
	struct vb_rsa_pubk rsa_key;
	void *sig_body;
	int rv = -1;
	struct memory_block blocks[3];

	if (unpack_pubk(key, &rsa_key))
		return -1;

	if (shared_mem_acquire(sigc->sigh.sig_size, (char **)&sig_body) !=
	    EC_SUCCESS) {
		CPRINTS("Failed to allocate %d bytes for sig body",
			sigc->sigh.sig_size);
		return EC_ERROR_HW_INTERNAL;
	}

	if (read_ap_spi(sig_body, sigc->offset + sigc->sigh.sig_offset,
			sigc->sigh.sig_size, __LINE__))
		goto exit;

	blocks[0].base = gvd;
	blocks[0].size = sizeof(*gvd);
	blocks[1].base = ranges;
	blocks[1].size = gvd->range_count * sizeof(ranges->ranges[0]);
	blocks[2].base = NULL;

	rv = verify_signature(blocks, &rsa_key, sig_body, sigc->sigh.sig_size);

exit:
	CPRINTS("GVDC %sOK", rv ? "NOT " : "");

	shared_mem_release(sig_body);
	return rv;
}

/**
 * Calculate and save GVD hash in the dedicated flash page.
 *
 * Attempts to save gsc_verification_data of previous generations are rejected.
 *
 * The GVD hash is saved along with a 4 byte checksum (truncated sha256 of the
 * hash) which allows to confirm validity of the saved hash on the following
 * verification attempts.
 *
 * If the dedicated page is not empty, it is erased.
 *
 * @param gvdc pointer to the gsc_verification_data container
 * @param ranges pointer to the ranges structure, gvd stores the range count
 *
 * @return 0 on success, non-zero on failure.
 */
static int save_gvd_hash(struct gvd_container *gvdc, struct ro_ranges *ranges)
{
	struct ap_ro_check ro_check;
	struct sha256_ctx ctx;
	int rv;

	if (gvdc->gvd.rollback_counter < LOWEST_ACCEPTABLE_GVD_ROLLBACK) {
		CPRINTS("Rejecting GVD rollback %d",
			gvdc->gvd.rollback_counter);
		return -1;
	}

	ro_check.header.version = AP_RO_HASH_LAYOUT_VERSION_1;
	ro_check.header.type = AP_RO_HASH_TYPE_GSCVD;
	 /*
	  * Not used, but set this field to make sure
	  * ap_ro_check_unsupported() is not
	  * tripped.
	  */
	ro_check.header.num_ranges = 0;

	ro_check.descriptor.fmap_offset = gvdc->gvd.fmap_location;
	ro_check.descriptor.gvd_offset = gvdc->offset;
	ro_check.descriptor.rollback = gvdc->gvd.rollback_counter;

	/* Calculate SHA256 of the GVD header and ranges. */
	if (DCRYPTO_hw_sha256_init(&ctx) != DCRYPTO_OK)
		return EC_ERROR_HW_INTERNAL;

	SHA256_update(&ctx, &gvdc->gvd, sizeof(gvdc->gvd));
	SHA256_update(&ctx, ranges->ranges,
		      sizeof(ranges->ranges[0]) * gvdc->gvd.range_count);
	memcpy(ro_check.descriptor.digest, SHA256_final(&ctx),
	       sizeof(ro_check.descriptor.digest));

	/* Now truncated sha256 of the descriptor. */
	if (DCRYPTO_hw_sha256_init(&ctx) != DCRYPTO_OK)
		return EC_ERROR_HW_INTERNAL;
	SHA256_update(&ctx, &ro_check.descriptor, sizeof(ro_check.descriptor));
	memcpy(&ro_check.header.checksum, SHA256_final(&ctx),
	       sizeof(ro_check.header.checksum));

	if (p_chk->header.num_ranges != (uint16_t)~0) {
		CPRINTS("Erasing GVD cache page");
		ap_ro_erase_hash();
	}

	flash_open_ro_window(h1_flash_offset_, sizeof(ro_check));
	rv = flash_physical_write(h1_flash_offset_, sizeof(ro_check),
				  (char *)&ro_check);
	flash_close_ro_window();

	CPRINTS("GVD HASH saving %ssucceeded", rv ? "NOT " : "");
	return rv;
}

/**
 * Verify gsc_verification_data cache.
 *
 * @param gvd pointer to gsc_verification_data
 * @param ranges pointer to ro_ranges, gvd stores the range_count
 * @param descriptor pointer to the descriptor containing cached hash value to
 *        compare against.
 *
 * @return zero on success, non zero on failure/
 */
static int gvd_cache_check(const struct gsc_verification_data *gvd,
			   const struct ro_ranges *ranges,
			   const struct gvd_descriptor *descriptor)
{
	struct sha256_ctx ctx;

	if (DCRYPTO_hw_sha256_init(&ctx) != DCRYPTO_OK)
		return EC_ERROR_HW_INTERNAL;

	SHA256_update(&ctx, gvd, sizeof(*gvd));
	SHA256_update(&ctx, ranges->ranges,
		      gvd->range_count * sizeof(ranges->ranges[0]));

	return DCRYPTO_equals(SHA256_final(&ctx), descriptor->digest,
			      SHA256_DIGEST_SIZE) != DCRYPTO_OK;
}

/**
 * Validate cached AP RO GVD entry.
 *
 * If a non NULL descriptor value is passed, the function does not try to
 * verify the gsc_verification_data signature, it just verifies that the
 * locally cached hash of gsc_verification_data matches.
 *
 * @param descriptor  points to locally cached hash of gsc_verification_data.
 *
 * @return zero on success, non zero on failure.
 */
static int8_t validate_cached_ap_ro_v2(const struct gvd_descriptor *descriptor)
{

	uint32_t fmap_offset;
	struct gvd_container gvdc;
	struct ro_ranges ranges;

	fmap_offset = descriptor->fmap_offset;
	gvdc.offset = descriptor->gvd_offset;

	if (read_gscvd_header(fmap_offset, &gvdc))
		return -1;

	if (read_ranges(&gvdc, &ranges))
		return -1;

	if (gvd_cache_check(&gvdc.gvd, &ranges, descriptor)) {
		CPRINTS("GVD HASH MISMATCH!!");
		return -1;
	}
	return validate_ranges_sha(ranges.ranges, gvdc.gvd.range_count,
				   gvdc.gvd.ranges_digest) ==
		EC_SUCCESS ? 0 : -1;
}

/*
 **
 * Try validating AP RO.
 *
 * This function looks for gsc_verification_data structure in AP flash through
 * FMAP, and then verifies cryptographically the validity of the contents,
 * starting with the hash of the root key, then signature of the key block,
 * and then signature of gsc_verification_data and the hash of the RO ranges.
 *
 * @return zero on success, non zero on failure.
 */
static int8_t validate_and_cache_ap_ro_v2_from_flash(void)
{
	uint32_t fmap_offset;
	struct fmap_header fmh;
	struct gvd_container gvdc;
	struct sig_container sigc;
	struct kb_container kbc;
	struct rootk_container rootkc;
	struct vb_rsa_pubk pubk;
	struct ro_ranges ranges;
	struct fmap_area_header fmap;
	struct fmap_area_header gscvd;

	int rv = -1;

	fmap_offset = find_fmap(&fmh);
	if (!fmap_offset)
		return -1;

	if (find_areas(fmap_offset + sizeof(fmh), fmh.fmap_nareas,
		       &fmap, &gscvd))
		return -1;
	gvdc.offset = gscvd.area_offset;

	if (read_gscvd_header(fmap_offset, &gvdc))
		return -1;

	if (read_ranges(&gvdc, &ranges))
		return -1;

	/* Signature comes after gscvd. */
	sigc.offset = gvdc.offset + gvdc.gvd.size;
	if (read_signature_header(&sigc))
		return -1;

	kbc.offset = sigc.offset + sigc.sigh.sig_offset + sigc.sigh.sig_size;
	if (read_keyblock(&kbc))
		return -1;

	rootkc.offset = kbc.offset + kbc.kb->keyblock_size;
	if (read_rootk(&rootkc))
		goto exit;

	/* Root key hash matches, let's verify the platform key. */
	if (unpack_pubk(rootkc.rootk, &pubk))
		goto exit;

	if (verify_keyblock(&kbc, &pubk))
		goto exit;

	shared_mem_release(rootkc.rootk);
	rootkc.rootk = NULL;

	if (verify_gvd_signature(&gvdc.gvd, &ranges, &kbc.kb->data_key, &sigc))
		return -1;

	rv = validate_ranges_sha(ranges.ranges, gvdc.gvd.range_count,
				 gvdc.gvd.ranges_digest);
	if (!rv) {
		/* Verification succeeded, save the hash for the next time. */
		rv = save_gvd_hash(&gvdc, &ranges);
	}
exit:
	if (kbc.kb)
		shared_mem_release(kbc.kb);

	if (rootkc.rootk)
		shared_mem_release(rootkc.rootk);

	return rv;
}

static uint8_t do_ap_ro_check(void)
{
	int rv;
	enum ap_ro_check_vc_errors support_status;
	bool v1_record_found;

	support_status = ap_ro_check_unsupported(true);
	if ((support_status == ARCVE_BOARD_ID_BLOCKED) ||
	    (support_status == ARCVE_FLASH_READ_FAILED))
		return EC_ERROR_UNIMPLEMENTED;

	enable_ap_spi_hash_shortcut();

	rv = EC_ERROR_CRC;
	v1_record_found = (support_status == ARCVE_OK) &&
		(p_chk->header.type == AP_RO_HASH_TYPE_FACTORY);
	if (v1_record_found) {
		rv = validate_ranges_sha(p_chk->payload.ranges,
					 p_chk->header.num_ranges,
					 p_chk->payload.digest);
	}

	/*
	 * If a V2 entry is found, or V1 check failed, which could be because
	 * there is a new RO with a V2 structure.
	 */
	if ((support_status == ARCVE_NOT_PROGRAMMED) ||
	    (p_chk->header.type == AP_RO_HASH_TYPE_GSCVD) ||
	    (v1_record_found && (rv != EC_SUCCESS))) {

		const struct gvd_descriptor *descriptor;

		descriptor = find_v2_entry();

		if (descriptor)
			rv = validate_cached_ap_ro_v2(descriptor);

		if (rv || !descriptor)
			/* There could have been a legitimate RO change. */
			rv = validate_and_cache_ap_ro_v2_from_flash();
	}

	disable_ap_spi_hash_shortcut();

	if (rv != EC_SUCCESS) {
		/* Failure reason has already been reported. */
		ap_ro_add_flash_event(APROF_CHECK_FAILED);

		/*
		 * Map all errors into EC_ERROR_CRC, this will make sure that
		 * in case this was invoked by the operator keypress, the
		 * device will not continue booting.
		 */
		rv = EC_ERROR_CRC;
	} else {
		ap_ro_add_flash_event(APROF_CHECK_SUCCEEDED);
		validated_ap_ro_boot = 1;
		CPRINTS("AP RO verification SUCCEEDED!");
	}

	return rv;
}

/*
 * Invoke AP RO verification on TPM task context.
 *
 * Verification functions calls into dcrypto library, which requires large
 * amounts of stack, this is why this function must run on TPM task context.
 *
 */
static enum vendor_cmd_rc ap_ro_check_callback(enum vendor_cmd_cc code,
						  void *buf, size_t input_size,
						  size_t *response_size)
{
	*response_size = 1;
	*((int8_t *)buf) = do_ap_ro_check();

	return VENDOR_RC_SUCCESS;
}

DECLARE_VENDOR_COMMAND(VENDOR_CC_AP_RO_VALIDATE, ap_ro_check_callback);

int validate_ap_ro(void)
{
	struct {
		struct tpm_cmd_header tpmh;
		/* Need one byte for the response code. */
		uint8_t rv;
	} __packed pack;

	/* Fixed fields of the validate AP RO command. */
	pack.tpmh.tag = htobe16(0x8001); /* TPM_ST_NO_SESSIONS */
	pack.tpmh.size = htobe32(sizeof(pack));
	pack.tpmh.command_code = htobe32(TPM_CC_VENDOR_BIT_MASK);
	pack.tpmh.subcommand_code = htobe16(VENDOR_CC_AP_RO_VALIDATE);

	tpm_alt_extension(&pack.tpmh, sizeof(pack));

	/* The last byte is the response code. */
	return pack.rv;
}

void ap_ro_add_flash_event(enum ap_ro_verification_ev event)
{
	struct ap_ro_entry_payload ev;

	ev.event = event;
	flash_log_add_event(FE_LOG_AP_RO_VERIFICATION, sizeof(ev), &ev);
}

static enum vendor_cmd_rc vc_get_ap_ro_hash(enum vendor_cmd_cc code,
					    void *buf, size_t input_size,
					    size_t *response_size)
{
	int rv;
	uint8_t *response = buf;

	*response_size = 0;
	if (input_size)
		return VENDOR_RC_BOGUS_ARGS;

	rv = ap_ro_check_unsupported(false);
	if (rv) {
		*response_size = 1;
		*response = rv;
		return VENDOR_RC_INTERNAL_ERROR;
	}
	*response_size = SHA256_DIGEST_SIZE;
	memcpy(buf, p_chk->payload.digest, *response_size);

	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_GET_AP_RO_HASH, vc_get_ap_ro_hash);

static int ap_ro_info_cmd(int argc, char **argv)
{
	int rv;
	int i;
#ifdef CR50_DEV
	int const max_args = 2;
#else
	int const max_args = 1;
#endif

	if (argc > max_args)
		return EC_ERROR_PARAM_COUNT;
#ifdef CR50_DEV
	if (argc == max_args) {
		if (strcasecmp(argv[1], "erase"))
			return EC_ERROR_PARAM1;
		ap_ro_erase_hash();
	}
#endif
	rv = ap_ro_check_unsupported(false);
	if (rv == ARCVE_FLASH_READ_FAILED)
		return EC_ERROR_CRC; /* No verification possible. */
	/* All other AP RO verificaiton unsupported reasons are fine */
	if (rv)
		return EC_SUCCESS;

	ccprintf("boot validated: %s\n", validated_ap_ro_boot ? "yes" : "no");
	ccprintf("sha256 hash %ph\n",
		 HEX_BUF(p_chk->payload.digest, sizeof(p_chk->payload.digest)));
	ccprintf("Covered ranges:\n");
	for (i = 0; i < p_chk->header.num_ranges; i++) {
		ccprintf("%08x...%08x\n", p_chk->payload.ranges[i].flash_offset,
			 p_chk->payload.ranges[i].flash_offset +
				 p_chk->payload.ranges[i].range_size - 1);
		cflush();
	}

	return EC_SUCCESS;
}
DECLARE_SAFE_CONSOLE_COMMAND(ap_ro_info, ap_ro_info_cmd,
#ifdef CR50_DEV
			     "[erase]", "Display or erase AP RO check space"
#else
			     "", "Display AP RO check space"
#endif
);

static enum vendor_cmd_rc vc_get_ap_ro_status(enum vendor_cmd_cc code,
					      void *buf, size_t input_size,
					      size_t *response_size)
{
	uint8_t rv = AP_RO_NOT_RUN;
	uint8_t *response = buf;

	CPRINTS("Check AP RO status");

	*response_size = 0;
	if (input_size)
		return VENDOR_RC_BOGUS_ARGS;

	if (ap_ro_check_unsupported(false))
		rv = AP_RO_UNSUPPORTED;
	else if (ec_rst_override())
		rv = AP_RO_FAIL;
	else if (validated_ap_ro_boot)
		rv = AP_RO_PASS;

	*response_size = 1;
	response[0] = rv;
	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_GET_AP_RO_STATUS, vc_get_ap_ro_status);
