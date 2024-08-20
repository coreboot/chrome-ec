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

/* Don't clear the UNSUPPORTED_TRIGGERED status for 10 seconds */
#define UNSUPPORTED_DEVICE_RST_WINDOW (10 * SECOND)

/*
 * A somewhat arbitrary maximum number of AP RO hash ranges to save. There are
 * 27 regions in a FMAP layout. The AP RO ranges should only be from the RO
 * region. It's unlikely anyone will need more than 32 ranges.
 * If there are AP RO hash issues, the team will likely need to look at the
 * value of each range what part of the FMAP it corresponds to. Enforce a limit
 * to the number of ranges, so it's easier to debug and to make people consider
 * why they would need more than 32 ranges.
 */
#define AP_RO_MAX_NUM_RANGES 32
/* Values used for validity check of the flash_range structure fields. */
#define MAX_SUPPORTED_FLASH_SIZE (32 * 1024 * 1024)
#define MAX_SUPPORTED_RANGE_SIZE (4 * 1024 * 1024)

/* Version of the AP RO check information saved in the H1 flash page. */
#define AP_RO_HASH_LAYOUT_VERSION_0 0
#define AP_RO_HASH_LAYOUT_VERSION_1 1
#define AP_RO_GBBD_LAYOUT_VERSION_2 2 /* Used to store the gbb descriptor */

/* Verification scheme V1. */
#define AP_RO_HASH_TYPE_FACTORY 0
/* Verification scheme V2 - disabled in cr50 */
/* #define AP_RO_HASH_TYPE_GSCVD 1 */
/* Use the factory gbb flags to generate the V1 hash */
#define AP_RO_HASH_TYPE_GBBD	2

/* A flash range included in hash calculations. */
struct ro_range {
	uint32_t flash_offset;
	uint32_t range_size;
};

/*
 * Payload of the vendor command communicating a variable number of flash
 * ranges to be checked and the total sha256.
 *
 * The actual number of ranges is determined based on the actual payload size.
 */
struct ap_ro_check_payload {
	uint8_t digest[SHA256_DIGEST_SIZE];
	/* Maximum number of RO ranges this implementation supports. */
	struct ro_range ranges[AP_RO_MAX_NUM_RANGES];
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

/* One of the AP RO verification outcomes, internal representation. */
enum gbbd_status {
	GS_INJECT_FLAGS       = BIT(0), /* Generate the hash using a factory */
					/* flag value. */
	GS_FLAGS_IN_HASH      = BIT(1), /* The gbb flags are covered by the */
					/* hash. */
};

struct gbb_descriptor {
	/*
	 * If validate_flags is true, verify the gbb flags are set to 0 during
	 * verification. If the gbb flags are in the hash, AP RO verification
	 * will need to validate them.
	 */
	enum gbbd_status status;
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

	/* Flags used to generate the hash */
	uint32_t injected_flags;
};

/*****************************************************************************/
/* V1 Factory Support (AP_RO_HASH_TYPE_FACTORY) */

/* Format of the AP RO check information saved in the H1 flash page. */
struct ap_ro_check {
	/* AP_RO_HASH_TYPE_FACTORY data */
	struct ap_ro_check_header header;
	/* Used by the V1 scheme. */
	struct ap_ro_check_payload payload;

	/* Optional GBB flag data. */
	struct ap_ro_check_header gbbd_header;
	/* Used to save the injected gbb flags. */
	struct gbb_descriptor gbbd;
};
/*
 * Make sure all saved AP RO data can fit in the AP RO space. This includes
 * two ap ro check headers, the sha digest of the firmware, the maximum number
 * of AP RO ranges, and a gbb descriptor.
 */
BUILD_ASSERT(sizeof(struct ap_ro_check) <= AP_RO_DATA_SPACE_SIZE);

/* One of the AP RO verification outcomes, internal representation. */
enum ap_ro_check_result {
	ROV_NOT_FOUND = 1, /* Control structures not found. */
	ROV_FAILED,	    /* Verification failed. */
	ROV_SUCCEEDED	    /* Verification succeeded. */
};

/* Page offset for H1 flash operations. */
static const uint32_t h1_flash_offset_ =
	AP_RO_DATA_SPACE_ADDR - CONFIG_PROGRAM_MEMORY_BASE;

/*
 * Enforce flash_write checks at build to ensure the chip can write the AP RO
 * data to flash. Check all structs.
 */
/* Verify the chip supports writing the struct sizes. */
BUILD_ASSERT(sizeof(struct ap_ro_check_payload) % CONFIG_FLASH_WRITE_SIZE == 0);
BUILD_ASSERT(sizeof(struct ap_ro_check_header) % CONFIG_FLASH_WRITE_SIZE == 0);
BUILD_ASSERT(sizeof(struct gbb_descriptor) % CONFIG_FLASH_WRITE_SIZE == 0);
BUILD_ASSERT(sizeof(struct ap_ro_check) % CONFIG_FLASH_WRITE_SIZE == 0);
/* Verify the chip will be able to write to the header offsets. */
BUILD_ASSERT((AP_RO_DATA_SPACE_ADDR - CONFIG_PROGRAM_MEMORY_BASE) %
	     CONFIG_FLASH_WRITE_SIZE == 0);
BUILD_ASSERT((AP_RO_DATA_SPACE_ADDR - CONFIG_PROGRAM_MEMORY_BASE +
	     offsetof(struct ap_ro_check, gbbd_header)) %
	     CONFIG_FLASH_WRITE_SIZE == 0);

/* Fixed pointer at the H1 flash page storing the AP RO check information. */
static const struct ap_ro_check *p_chk =
	(const struct ap_ro_check *)AP_RO_DATA_SPACE_ADDR;

/*
 * Track if the AP RO hash was validated this boot. Must be cleared every AP
 * reset.
 */
static enum ap_ro_status apro_result = AP_RO_NOT_RUN;
static timestamp_t ignore_device_rst_deadline;

/*
 * Verify the saved hash has a supported type, correct number of ranges, and
 * a valid checksum. Return EC_ERROR_CRC if any of these checks fail.
 *
 * Returns:
 *
 *  EC_SUCCESS if the saved hash contents are ok
 *  EC_ERROR_CRC if there's an issue with the hash contents
 */
static int verify_ap_ro_check_space(void)
{
	uint32_t checksum;
	size_t data_size;

	if (p_chk->header.type != AP_RO_HASH_TYPE_FACTORY)
		return EC_ERROR_CRC;

	data_size = p_chk->header.num_ranges * sizeof(struct ro_range) +
		    offsetof(struct ap_ro_check_payload, ranges);
	if (p_chk->header.num_ranges > AP_RO_MAX_NUM_RANGES) {
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
 * ap_ro_check_unsupported: Returns non-zero value if the AP RO hash is not
 *                          saved or is invalid.
 *
 * Returns:
 *
 *  ARCVE_DISABLED if the saved hash is ok, but verification is
 *                             disabled.
 *  ARCVE_NOT_PROGRAMMED if the hash is not programmed.
 *  ARCVE_FLASH_READ_FAILED if there was an error reading the hash.
 */
static enum ap_ro_check_vc_errors ap_ro_check_unsupported(int add_flash_event)
{
	/* Validate the saved hash contents */
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
	return ARCVE_DISABLED;
}

void update_device_rst_deadline(uint32_t delay)
{
	timestamp_t tmp = get_time();

	tmp.val += delay;
	ignore_device_rst_deadline = tmp;
}

/* The AP reset. Clear the apro_result. */
void ap_ro_device_reset(void)
{
	if (apro_result == AP_RO_NOT_RUN)
		return;
	/*
	 * Don't clear the AP RO state on device reset until the device reset
	 * window has passed.
	 */
	if (ignore_device_rst_deadline.val &&
	    !timestamp_expired(ignore_device_rst_deadline, 0)) {
		CPRINTS("%s: ignored", __func__);
		return;
	}
	CPRINTS("%s: clear apro result", __func__);
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

/**
 * Write data to the AP RO data space of H1 flash.
 *
 * @param version the payload version
 * @param type payload type
 * @param num_ranges number of ranges in the AP RO payload.
 * @param data_offset offset to write AP RO data to. It must be in the AP RO
 *                    flash region.
 * @param p_data pointer to the saved data. This is used to make sure the
 *               region is empty.
 * @param buf data to write to AP RO flash.
 * @param input_size the amount of data to write.
 *
 * @return ARCVE_OK on success, ARCVE_ALREADY_PROGRAMMED if there's already
 *         data stored in the region, or ARCVE_FLASH_WRITE_FAILED if writing
 *         flash failed..
 */
static enum ap_ro_check_vc_errors write_ap_ro_check_data(uint8_t version,
							 uint8_t type,
							 uint16_t num_ranges,
							 uint32_t data_offset,
							 uint32_t *p_data,
							 void *buf,
							 size_t input_size)
{
	uint32_t i;
	struct ap_ro_check_header check_header;
	int rv;
	size_t prog_size = sizeof(struct ap_ro_check_header) + input_size;

	/* Verify the ap ro data will fit in the ap ro data flash */
	if ((data_offset < h1_flash_offset_) ||
	    ((data_offset + prog_size) >
	     (h1_flash_offset_ + AP_RO_DATA_SPACE_SIZE)))
		return ARCVE_BAD_PAYLOAD_SIZE;

	/*
	 * If prog_size isn't divisible by 4, then the remaining bytes won't get
	 * checked. The payload should be divisible by 4. Reject the data if it
	 * isn't.
	 */
	if (prog_size % sizeof(uint32_t))
		return ARCVE_BAD_PAYLOAD_SIZE;
	/* Verify the entire data region is erased. */
	for (i = 0; i < (prog_size / sizeof(uint32_t)); i++)
		if (p_data[i] != ~0)
			return ARCVE_ALREADY_PROGRAMMED;

	check_header.version = version;
	check_header.type = type;
	check_header.num_ranges = num_ranges;
	app_compute_hash(buf, input_size, &check_header.checksum,
			 sizeof(check_header.checksum));

	flash_open_ro_window(data_offset, prog_size);
	rv = flash_physical_write(data_offset, sizeof(check_header),
		(char *)&check_header);
	if (rv == EC_SUCCESS)
		rv = flash_physical_write(data_offset + sizeof(check_header),
			input_size, buf);
	flash_close_ro_window();
	if (rv != EC_SUCCESS)
		return ARCVE_FLASH_WRITE_FAILED;
	return ARCVE_OK;
}

/*
 * Leaving this function available for testing, will not be necessary in prod
 * signed images.
 */
static enum vendor_cmd_rc vc_seed_ap_ro_check(enum vendor_cmd_cc code,
					      void *buf, size_t input_size,
					      size_t *response_size)
{
	const struct ap_ro_check_payload *vc_payload = buf;
	uint32_t vc_num_of_ranges;
	uint32_t i;
	uint8_t *response = buf;
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

	if (vc_num_of_ranges > AP_RO_MAX_NUM_RANGES) {
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

	rv = write_ap_ro_check_data(AP_RO_HASH_LAYOUT_VERSION_1,
				    AP_RO_HASH_TYPE_FACTORY,
				    vc_num_of_ranges,
				    h1_flash_offset_,
				    (uint32_t *)p_chk,
				    buf,
				    input_size);
	if (rv != ARCVE_OK) {
		*response = rv;
		return VENDOR_RC_WRITE_FLASH_FAIL;
	}

	*response_size = 0;
	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_SEED_AP_RO_CHECK, vc_seed_ap_ro_check);

static int verify_ap_ro_gbb_space(void)
{
	uint32_t checksum;

	if ((p_chk->gbbd_header.type != AP_RO_HASH_TYPE_GBBD) ||
	    (p_chk->gbbd_header.version != AP_RO_GBBD_LAYOUT_VERSION_2))
		return EC_ERROR_CRC;
	/* The GBB descriptor is only valid for FACTORY hashes. */
	if (p_chk->header.type != AP_RO_HASH_TYPE_FACTORY)
		return EC_ERROR_CRC;

	/* The V1 header saved too many ranges. The stored GBBD is invalid */
	if (p_chk->header.num_ranges > AP_RO_MAX_NUM_RANGES) {
		CPRINTS("%s: V1 stored too many ranges", __func__);
		return EC_ERROR_CRC;
	}
	if (p_chk->gbbd_header.num_ranges != p_chk->header.num_ranges) {
		CPRINTS("%s: gbbd doesn't match v1 header", __func__);
		return EC_ERROR_CRC;
	}

	app_compute_hash(&p_chk->gbbd, sizeof(struct gbb_descriptor),
			 &checksum, sizeof(checksum));

	if (memcmp(&checksum, &p_chk->gbbd_header.checksum, sizeof(checksum))) {
		CPRINTS("%s: AP RO GBB Checksum corrupted", __func__);
		return EC_ERROR_CRC;
	}

	return EC_SUCCESS;
}

/*
 * Set the AP RO verification result to UNSUPPORTED_TRIGGERED, so shimless
 * RMA and factory scripts can tell that AP RO verification was purposefully
 * skipped.
 * This result is cleared when the AP resets after UNSUPPORTED_DEVICE_RST_WINDOW
 * has passed.
 */
static uint8_t do_ap_ro_check(void)
{
	CPRINTS("%s: universally unsupported", __func__);
	update_device_rst_deadline(UNSUPPORTED_DEVICE_RST_WINDOW);
	apro_result = AP_RO_UNSUPPORTED_TRIGGERED;
	ap_ro_add_flash_event(APROF_CHECK_UNSUPPORTED);
	return EC_ERROR_UNIMPLEMENTED;
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
	if (rv != ARCVE_OK) {
		if (ARCVE_DISABLED) {
			CPRINTS("%s: hash ok", __func__);
			CPRINTS("%s: apro verification is disabled", __func__);
		} else {
			*response_size = 1;
			*response = rv;
			return VENDOR_RC_INTERNAL_ERROR;
		}
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
	ccprintf("supported : %s (%d)\n", rv ? "no" : "yes", rv);
	if (rv == ARCVE_FLASH_READ_FAILED)
		return EC_ERROR_CRC; /* No verification possible. */
	/*
	 * All other AP RO verificaiton unsupported reasons are fine.
	 * ARCVE_DISABLED means the hash is ok. Print it.
	 */
	if (rv != ARCVE_OK && rv != ARCVE_DISABLED)
		return EC_SUCCESS;
	rv = verify_ap_ro_gbb_space();
	ccprintf("gbbd      : ");
	if (rv == EC_SUCCESS) {
		ccprintf("ok (%d)\n", p_chk->gbbd.status);
		ccprintf("flags     : ");
		if (p_chk->gbbd.status & GS_FLAGS_IN_HASH)
			ccprintf("0x%x\n", p_chk->gbbd.injected_flags);
		else
			ccprintf("na\n");
	} else {
		ccprintf("na (%d)\n", rv);
	}
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

	*response_size = 1;
	response[0] = rv;
	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_GET_AP_RO_STATUS, vc_get_ap_ro_status);
