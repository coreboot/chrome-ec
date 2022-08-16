/* Copyright 2020 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Code supporting AP RO verification.
 */

#include "ap_ro_integrity_check.h"
#include "board_id.h"
#include "byteorder.h"
#include "ccd_config.h"
#include "console.h"
#include "crypto_api.h"
#include "extension.h"
#include "extension.h"
#include "flash.h"
#include "flash_info.h"
#include "shared_mem.h"
#include "stddef.h"
#include "stdint.h"
#include "system.h"
#include "timer.h"
#include "tpm_registers.h"
#include "usb_spi.h"
#include "usb_spi_board.h"

#define CPRINTS(format, args...) cprints(CC_SYSTEM, format, ##args)
#define CPRINTF(format, args...) cprintf(CC_SYSTEM, format, ##args)

/* FMAP must be aligned at 4K or larger power of 2 boundary. */
#define LOWEST_FMAP_ALIGNMENT  (4 * 1024)
#define FMAP_SIGNATURE	       "__FMAP__"
#define FMAP_AREA_NAME	       "FMAP"
#define FMAP_GBB_AREA_NAME     "GBB"
#define FMAP_SIGNATURE_SIZE    (sizeof(FMAP_SIGNATURE) - 1)
#define FMAP_NAMELEN	       32
#define FMAP_MAJOR_VERSION     1
#define FMAP_MINOR_VERSION     1

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
/* Values used for validity check of the flash_range structure fields. */
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
 * Header added for storing of the AP RO check information in the H1 flash
 * page. The checksum is a 4 byte truncated sha256 of the saved payload, just
 * a validity check.
 */
struct ap_ro_check_header {
	uint8_t version;
	uint8_t type;
	uint16_t num_ranges;
	uint32_t checksum;
};

/*
 * Saved AP RO data includes the ap ro check header, the sha digest of the
 * firmware and the RO ranges. Make sure the header, digest, and maximum number
 * of ranges fit in the AP RO space.
 */
BUILD_ASSERT(AP_RO_DATA_SPACE_SIZE >=
	sizeof(struct ap_ro_check_header) + SHA256_DIGEST_SIZE +
	APRO_MAX_NUM_RANGES * sizeof(struct ro_range));
/* Format of the AP RO check information saved in the H1 flash page. */
struct ap_ro_check {
	struct ap_ro_check_header header;
	struct ap_ro_check_payload payload;
};

/*****************************************************************************/
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

/*****************************************************************************/
/* GBB information from vboot_reference/firmware/2lib/include/2struct.h */
/*
 * Signature at start of the GBB
 */
#define VB2_GBB_SIGNATURE "$GBB"
#define VB2_GBB_SIGNATURE_SIZE (sizeof(VB2_GBB_SIGNATURE) - 1)

#define VB2_GBB_HWID_DIGEST_SIZE 32

/* Supported VB2 GBB struct version */
#define VB2_GBB_MAJOR_VER 1
#define VB2_GBB_MINOR_VER 2

#define VB2_GBB_HEADER_SIZE 128
#define VB2_GBB_HEADER_FLAG_OFFSET 12
#define VB2_GBB_HEADER_FLAG_SIZE 4
/*
 * GBB header saved in AP RO flash.
 * v1.2 - added fields for sha256 digest of the HWID
 */
struct vb2_gbb_header {
	/* Fields present in version 1.1 */
	uint8_t  signature[VB2_GBB_SIGNATURE_SIZE]; /* VB2_GBB_SIGNATURE */
	uint16_t major_version;   /* See VB2_GBB_MAJOR_VER */
	uint16_t minor_version;   /* See VB2_GBB_MINOR_VER */
	uint32_t header_size;     /* Size of GBB header in bytes */

	/* Flags.
	 * b/236844541 - APRO verification v1 has to confirm the flags are set
	 *               to 0.
	 */
	uint32_t flags;

	/* Offsets (from start of header) and sizes (in bytes) of components */
	uint32_t hwid_offset;		/* HWID */
	uint32_t hwid_size;
	uint32_t rootkey_offset;	/* Root key */
	uint32_t rootkey_size;
	uint32_t bmpfv_offset;		/* BMP FV; deprecated in current FW */
	uint32_t bmpfv_size;
	uint32_t recovery_key_offset;	/* Recovery key */
	uint32_t recovery_key_size;

	/* Added in version 1.2 */
	uint8_t  hwid_digest[VB2_GBB_HWID_DIGEST_SIZE];	/* SHA-256 of HWID */

	/* Pad to match VB2_GBB_HEADER_SIZE.  Initialize to 0. */
	uint8_t  pad[48];
} __packed;
BUILD_ASSERT(sizeof(struct vb2_gbb_header) == VB2_GBB_HEADER_SIZE);
BUILD_ASSERT(offsetof(struct vb2_gbb_header, flags) ==
	VB2_GBB_HEADER_FLAG_OFFSET);

struct gbb_descriptor {
	/*
	 * If validate_flags is true, verify the gbb flags are set to 0 during
	 * verification. If the gbb flags are in the hash, AP RO verification
	 * will need to validate them.
	 */
	bool validate_flags;
	/*
	 * The FMAP range information. The FMAP must be in the hash ranges for
	 * verification to pass.
	 */
	struct ro_range fmap;
	/* The full GBB range information. */
	struct ro_range gbb;
	/*
	 * The GBB flag range information. The flags are only verified if
	 * they're in the hash.
	 */
	struct ro_range gbb_flags;
} __packed;

/*****************************************************************************/
/* V1 Factory Support (AP_RO_HASH_TYPE_FACTORY) */

/* One of the AP RO verification outcomes, internal representation. */
enum ap_ro_check_result {
	ROV_NOT_FOUND = 1, /* Control structures not found. */
	ROV_FAILED,	    /* Verification failed. */
	ROV_SUCCEEDED	    /* Verification succeeded. */
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
static enum ap_ro_status apro_result = AP_RO_NOT_RUN;
static uint8_t apro_fail_status_cleared;

/* Clear validate_ap_ro_boot state. */
void ap_ro_device_reset(void)
{
	if (apro_result == AP_RO_NOT_RUN || apro_result == AP_RO_IN_PROGRESS ||
	    ec_rst_override())
		return;
	CPRINTS("%s: clear apro result", __func__);
	apro_fail_status_cleared = 0;
	apro_result = AP_RO_NOT_RUN;
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

	/*
	 * Neither write nor erase are allowed once Board ID type is programmed.
	 *
	 * Check the board id type insead of board_id_is_erased, because the
	 * board id flags may be written before finalization. Board id type is
	 * a better indicator for when RO is finalized and when to lock out
	 * setting the hash.
	 */
#ifndef CR50_DEV
	{
		struct board_id bid;

		if (read_board_id(&bid) != EC_SUCCESS ||
		    !board_id_type_is_blank(&bid)) {
			*response = ARCVE_BID_PROGRAMMED;
			return VENDOR_RC_NOT_ALLOWED;
		}
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

	/* Are the v1 contents intact? */
	if (verify_ap_ro_check_space() != EC_SUCCESS) {
		CPRINTS("%s: unable to read ap ro space", __func__);
		if (add_flash_event)
			ap_ro_add_flash_event(APROF_SPACE_INVALID);
		return ARCVE_FLASH_READ_FAILED; /* No verification possible. */
	}
	return ARCVE_OK;
}

/* Returns true if part_range is fully within full_range. */
static bool is_in_range(const struct ro_range part_range,
			const struct ro_range full_range)
{
	return (part_range.flash_offset >= full_range.flash_offset) &&
		(part_range.flash_offset + part_range.range_size <=
		 full_range.flash_offset + full_range.range_size);
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
 * @return ROV_SUCCEEDED if succeeded, ROV_FAILED otherwise.
 */
static
enum ap_ro_check_result validate_ranges_sha(const struct ro_range *ranges,
					    size_t count,
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
		return ROV_FAILED;
	}

	return ROV_SUCCEEDED;
}

/*****************************************************************************/
/* V1 Factory Verify GBB Support */
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

/**
 * Find GBB FMAP area in the FMAP table.
 *
 * @param offset offset of the fmap in the flash
 * @param nareas number of areas in fmap
 * @param gbbah fmap area header to save GBB area information in
 *
 * @return zero on success, -1 if the GBB is not found.
 */
static int find_gbb_fmah(uint32_t offset, uint16_t nareas,
			 struct fmap_area_header *gbbah)
{
	uint16_t i;
	struct fmap_area_header fmah;

	if (nareas > 64) {
		CPRINTS("%s: too many areas: %d", __func__, nareas);
		return -1;
	}

	for (i = 0; i < nareas; i++) {
		if (read_ap_spi(&fmah, offset, sizeof(fmah), __LINE__))
			return -1;

		if (!memcmp(fmah.area_name, FMAP_GBB_AREA_NAME,
			    sizeof(FMAP_GBB_AREA_NAME))) {
			memcpy(gbbah, &fmah, sizeof(*gbbah));
			return 0;
		}
		offset += sizeof(fmah);
	}

	CPRINTS("Could not find %s area", FMAP_GBB_AREA_NAME);

	return -1;
}

/*
 * If the range is covered by one of the AP RO Verification ranges, then the
 * range is in the hash.
 *
 * Returns:
 *   EC_SUCCESS if if the range is in one of the AP RO ranges.
 *   EC_ERROR_CRC if p_chk is invalid.
 *   EC_ERROR_INVAL if p_chk is valid, but range isn't in one of the AP RO
 *                  ranges.
 */
static int range_is_in_hash(const struct ro_range range)
{
	uint16_t i;

	for (i = 0; i < p_chk->header.num_ranges; i++) {
		if (is_in_range(range, p_chk->payload.ranges[i]))
			return EC_SUCCESS;
	}
	return EC_ERROR_INVAL;
}

/*
 * Verify the GBB contents and validate the GBB flags are set to 0.
 *
 * @param gbbd pointer to the gbb descriptor with the fmap, gbb, gbb_flags,
 *             and validate_flags information from init_gbbd.
 *
 * Returns:
 *   ROV_SUCCEEDED if the flags are outside of the hash or if the flags
 *                 are in the hash and set to 0.
 *   ROV_FAILED if the gbb format is invalid.
 */
static enum ap_ro_check_result validate_gbbd(struct gbb_descriptor *gbbd)
{
	struct vb2_gbb_header gbb;

	/*
	 * The GBB flags are outside of the hash, so there's no need to validate
	 * them.
	 * The location was found in the FMAP which will be verified. If it's
	 * pointing to the wrong place, AP RO verification will fail.
	 */
	if (!gbbd->validate_flags) {
		CPRINTS("%s: GBB flags not in hash", __func__);
		return ROV_SUCCEEDED;
	}

	if (gbbd->gbb.range_size < VB2_GBB_HEADER_SIZE) {
		CPRINTS("%s: invalid GBB size %u", __func__,
			gbbd->gbb.range_size);
		return ROV_FAILED;
	}

	/* Read and verify the contents of the GBB */
	if (read_ap_spi(&gbb, gbbd->gbb.flash_offset, sizeof(gbb), __LINE__))
		return ROV_FAILED;

	/*
	 * Verify the gbb version and signature to make sure it looks
	 * reasonable.
	 */
	if (gbb.header_size != VB2_GBB_HEADER_SIZE) {
		CPRINTS("%s: inconsistent contents", __func__);
		return ROV_FAILED;
	}
	if ((gbb.major_version != VB2_GBB_MAJOR_VER) ||
	    (gbb.minor_version != VB2_GBB_MINOR_VER)) {
		CPRINTS("%s: unsupported ver %d %d", __func__,
			gbb.major_version, gbb.minor_version);
		return ROV_FAILED;
	}
	if (memcmp(gbb.signature, VB2_GBB_SIGNATURE,
		   VB2_GBB_SIGNATURE_SIZE)) {
		CPRINTS("%s: invalid signature", __func__);
		return ROV_FAILED;
	}

	/* Verify the GBB flags are set to 0. */
	if (gbb.flags) {
		CPRINTS("%s: invalid flags %x", __func__, gbb.flags);
		return ROV_FAILED;
	}

	CPRINTS("%s: ok", __func__);
	return ROV_SUCCEEDED;
}
/*
 * Initialize the gbb descriptor using the FMAP and GBB found in RO flash.
 *
 * Iterate through AP flash at 4K intervals looking for FMAP. Once FMAP is
 * found call find the FMAP GBB section. Populate the FMAP and GBB information
 * in the gbb descriptor.
 *
 * @param gbbd pointer to the gbb descriptor.
 *
 * Returns:
 *   ROV_SUCCEEDED if a valid GBB was found using FMAP information from a
 *                 section covered by the AP RO hash.
 *   ROV_FAILED if no FMAP section was found in the AP RO hash or if the
 *              GBB wasn't found in FMAP.
 */
static enum ap_ro_check_result init_gbbd(struct gbb_descriptor *gbbd)
{
	uint32_t offset;
	int rv;

	for (offset = 0; offset < MAX_SUPPORTED_FLASH_SIZE;
	     offset += LOWEST_FMAP_ALIGNMENT) {
		struct fmap_header fmh;
		struct fmap_area_header gbbah;

		if (read_ap_spi(fmh.fmap_signature, offset,
				sizeof(fmh.fmap_signature), __LINE__))
			return ROV_FAILED;

		if (memcmp(fmh.fmap_signature, FMAP_SIGNATURE,
			   sizeof(fmh.fmap_signature)))
			continue; /* Not an FMAP candidate. */

		/* Read the rest of fmap header. */
		if (read_ap_spi(&fmh.fmap_ver_major, offset +
				sizeof(fmh.fmap_signature),
				sizeof(fmh) - sizeof(fmh.fmap_signature),
				__LINE__))
			return ROV_FAILED;

		/* Verify fmap validity. */
		if ((fmh.fmap_ver_major != FMAP_MAJOR_VERSION) ||
		    (fmh.fmap_ver_minor != FMAP_MINOR_VERSION) ||
		    (fmh.fmap_size > MAX_SUPPORTED_FLASH_SIZE)) {
			CPRINTS("invalid FMAP contents at %x", offset);
			continue;
		}

		/* Find the GBB area header in FMAP. */
		if (find_gbb_fmah(offset + sizeof(struct fmap_header),
				  fmh.fmap_nareas, &gbbah))
			continue;

		gbbd->fmap.flash_offset = offset;
		gbbd->fmap.range_size = sizeof(struct fmap_header) +
			sizeof(struct fmap_area_header) * fmh.fmap_nareas;
		/*
		 * The FMAP isn't in the AP RO hash, so the GBB location can't
		 * be trusted. Continue searching for a fmap in the hash.
		 */
		rv = range_is_in_hash(gbbd->fmap);
		if (rv != EC_SUCCESS) {
			CPRINTS("%s: FMAP(%x:%x) not in hash.", __func__,
				gbbd->fmap.flash_offset, gbbd->fmap.range_size);
			continue;
		}

		gbbd->gbb.flash_offset = gbbah.area_offset;
		gbbd->gbb.range_size = gbbah.area_size;
		gbbd->gbb_flags.flash_offset = gbbah.area_offset +
			VB2_GBB_HEADER_FLAG_OFFSET;
		gbbd->gbb_flags.range_size = VB2_GBB_HEADER_FLAG_SIZE;
		gbbd->validate_flags = range_is_in_hash(gbbd->gbb_flags) ==
			EC_SUCCESS;

		return ROV_SUCCEEDED;
	}

	return ROV_FAILED;
}

/*
 * A hook used to keep the EC in reset, no matter what keys the user presses,
 * the only way out is the Cr50 reboot, most likely through power cycle by
 * battery cutoff.
 *
 * Cr50 console over SuzyQ would still be available in case the user has the
 * cable and wants to see what happens with the system. The easiest way to see
 * the system is in this state to run the 'flog' command and examine the flash
 * log.
 */
static void keep_ec_in_reset(void);

DECLARE_DEFERRED(keep_ec_in_reset);

static void keep_ec_in_reset(void)
{
	disable_sleep(SLEEP_MASK_AP_RO_VERIFICATION);
	assert_ec_rst();
	hook_call_deferred(&keep_ec_in_reset_data, 100 * MSEC);
}

static void release_ec_reset_override(void)
{
	hook_call_deferred(&keep_ec_in_reset_data, -1);
	deassert_ec_rst();
	/* b/229974371 Give AP_FLASH_SELECT at least 500us to discharge */
	delay_sleep_by(1 * SECOND);
	enable_sleep(SLEEP_MASK_AP_RO_VERIFICATION);
}

/* Only call this through a key combo. */
void ap_ro_clear_ec_rst_override(void)
{
	if (!ec_rst_override())
		return;
	apro_fail_status_cleared = 1;
	release_ec_reset_override();
	ap_ro_add_flash_event(APROF_FAIL_CLEARED);
	CPRINTS("%s: done", __func__);
}

int ec_rst_override(void)
{
	return !apro_fail_status_cleared && apro_result == AP_RO_FAIL;
}


static uint8_t do_ap_ro_check(void)
{
	enum ap_ro_check_result rv;
	struct gbb_descriptor gbbd;

	apro_result = AP_RO_IN_PROGRESS;
	apro_fail_status_cleared = 0;
	if (ap_ro_check_unsupported(true) != ARCVE_OK ||
	    p_chk->header.type != AP_RO_HASH_TYPE_FACTORY) {
		apro_result = AP_RO_UNSUPPORTED_TRIGGERED;
		ap_ro_add_flash_event(APROF_CHECK_UNSUPPORTED);
		return EC_ERROR_UNIMPLEMENTED;
	}

	enable_ap_spi_hash_shortcut();

	/* Find the GBB and FMAP locations. */
	rv = init_gbbd(&gbbd);
	/* Check the GBB flags are 0 before validating any AP RO ranges. */
	if (rv == ROV_SUCCEEDED)
		rv = validate_gbbd(&gbbd);
	if (rv == ROV_SUCCEEDED)
		rv = validate_ranges_sha(p_chk->payload.ranges,
					 p_chk->header.num_ranges,
					 p_chk->payload.digest);

	disable_ap_spi_hash_shortcut();

	/* Failure reason has already been reported. */
	if (rv != ROV_SUCCEEDED) {
		CPRINTS("AP RO FAILED!");
		apro_result = AP_RO_FAIL;
		ap_ro_add_flash_event(APROF_CHECK_FAILED);
		keep_ec_in_reset();
		/*
		 * Map failures into EC_ERROR_CRC, this will make sure
		 * that in case this was invoked by the operator
		 * keypress, the device will not continue booting.
		 *
		 * Both explicit failure to verify OR any error if
		 * cached descriptor was found should block the
		 * booting.
		 */
		return EC_ERROR_CRC;
	}
	apro_result = AP_RO_PASS;
	ap_ro_add_flash_event(APROF_CHECK_SUCCEEDED);
	CPRINTS("AP RO PASS!");
	release_ec_reset_override();
	return EC_SUCCESS;
}

/*
 * Invoke AP RO verification on TPM task context.
 *
 * Verification functions calls into dcrypto library, which requires large
 * amounts of stack, this is why this function must run on TPM task context.
 *
 */
static enum vendor_cmd_rc ap_ro_check_callback(struct vendor_cmd_params *p)
{
	uint8_t *response = p->buffer;

	p->out_size = 0;

	if (!(p->flags & VENDOR_CMD_FROM_ALT_IF) &&
	    !(ccd_is_cap_enabled(CCD_CAP_AP_RO_CHECK_VC)))
		return VENDOR_RC_NOT_ALLOWED;

	p->out_size = 1;
	response[0] = do_ap_ro_check();

	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND_P(VENDOR_CC_AP_RO_VALIDATE, ap_ro_check_callback);

void validate_ap_ro(void)
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
	ccprintf("result    : %d\n", apro_result);
	ccprintf("supported : %s\n", rv ? "no" : "yes");
	if (rv == ARCVE_FLASH_READ_FAILED)
		return EC_ERROR_CRC; /* No verification possible. */
	/* All other AP RO verificaiton unsupported reasons are fine */
	if (rv)
		return EC_SUCCESS;

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
	uint8_t rv = apro_result;
	uint8_t *response = buf;

	CPRINTS("Check AP RO status");

	*response_size = 0;
	if (input_size)
		return VENDOR_RC_BOGUS_ARGS;

	if ((apro_result != AP_RO_UNSUPPORTED_TRIGGERED) &&
	    (ap_ro_check_unsupported(false) != ARCVE_OK))
		rv = AP_RO_UNSUPPORTED_NOT_TRIGGERED;

	*response_size = 1;
	response[0] = rv;
	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_GET_AP_RO_STATUS, vc_get_ap_ro_status);
