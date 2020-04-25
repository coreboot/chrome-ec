/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Code supporting AP RO verification.
 */

#include "console.h"
#include "crypto_api.h"
#include "extension.h"
#include "flash.h"
#include "flash_info.h"
#include "cryptoc/sha256.h"
#include "stddef.h"
#include "stdint.h"
#include "timer.h"
#include "usb_spi.h"

#define CPRINTS(format, args...) cprints(CC_SYSTEM, format, ##args)
#define CPRINTF(format, args...) cprintf(CC_SYSTEM, format, ##args)

/* A flash range included in hash calculations. */
struct flash_range {
	uint32_t flash_offset;
	uint32_t range_size;
} __packed;

/* Values used for sanity check of the flash_range structure fields. */
#define MAX_SUPPORTED_FLASH_SIZE (32 * 1024 * 1024)
#define MAX_SUPPORTED_RANGE_SIZE (4 * 1024 * 1024)

/* Page offset for H1 flash operations. */
static const uint32_t h1_flash_offset_ =
	AP_RO_DATA_SPACE_ADDR - CONFIG_PROGRAM_MEMORY_BASE;

/*
 * Payload of the vendor command communicating a variable number of flash
 * ranges to be checked and the total sha256.
 *
 * The actual number of ranges is determined based on the actual payload size.
 */
struct ap_ro_check_payload {
	uint8_t digest[SHA256_DIGEST_SIZE];
	struct flash_range ranges[0];
} __packed;

/* Version of the AP RO check information saved in the H1 flash page. */
#define AP_RO_HASH_LAYOUT_VERSION 0

/*
 * Header added for storing of the AP RO check information in the H1 flash
 * page. The checksum is a 4 byte truncated sha256 of the saved payload, just
 * a sanity check.
 */
struct ap_ro_check_header {
	uint16_t version;
	uint16_t num_ranges;
	uint32_t checksum;
};

/* Format of the AP RO check information saved in the H1 flash page. */
struct ap_ro_check {
	struct ap_ro_check_header header;
	struct ap_ro_check_payload payload;
};

/* Fixed pointer at the H1 flash page storing the AP RO check information. */
static const struct ap_ro_check *p_chk =
	(const struct ap_ro_check *)AP_RO_DATA_SPACE_ADDR;

/* Errors recognized and returned by the vendor command handler. */
enum ap_ro_check_vc_errors {
	ARCVE_TOO_SHORT = 1,
	ARCVE_BAD_PAYLOAD_SIZE = 2,
	ARCVE_BAD_OFFSET = 3,
	ARCVE_BAD_RANGE_SIZE = 4,
	ARCVE_ALREADY_PROGRAMMED = 5,
};

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

	*response_size = 1; /* Just in case there is an error. */

	/* There should be at least one range and the hash. */
	if (input_size < (SHA256_DIGEST_SIZE + sizeof(struct flash_range))) {
		*response = ARCVE_TOO_SHORT;
		return VENDOR_RC_BOGUS_ARGS;
	}

	/* There should be an integer number of ranges. */
	if (((input_size - SHA256_DIGEST_SIZE) % sizeof(struct flash_range)) !=
	    0) {
		*response = ARCVE_BAD_PAYLOAD_SIZE;
		return VENDOR_RC_BOGUS_ARGS;
	}

	vc_num_of_ranges =
		(input_size - SHA256_DIGEST_SIZE) / sizeof(struct flash_range);

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

	check_header.version = AP_RO_HASH_LAYOUT_VERSION;
	check_header.num_ranges = vc_num_of_ranges;
	app_compute_hash(buf, input_size, &check_header.checksum,
			 sizeof(check_header.checksum));

	flash_open_ro_window(h1_flash_offset_, prog_size);
	flash_physical_write(h1_flash_offset_, sizeof(check_header),
			     (char *)&check_header);
	flash_physical_write(h1_flash_offset_ + sizeof(check_header),
			     input_size, buf);

	*response_size = 0;
	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_SEED_AP_RO_CHECK, vc_seed_ap_ro_check);

static int verify_ap_ro_check_space(void)
{
	uint32_t checksum;
	size_t data_size;

	data_size = p_chk->header.num_ranges * sizeof(struct flash_range) +
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

int validate_ap_ro(void)
{
	uint32_t i;
	HASH_CTX ctx;
	uint8_t digest[SHA256_DIGEST_SIZE];
	int rv;

	if (p_chk->header.num_ranges == (uint16_t)~0) {
		CPRINTS("%s: RO verification not programmed", __func__);
		return EC_ERROR_INVAL;
	}

	/* Is the contents intact? */
	if (verify_ap_ro_check_space() != EC_SUCCESS)
		return EC_ERROR_INVAL; /* No verification possible. */

	enable_ap_spi_hash_shortcut();
	usb_spi_sha256_start(&ctx);
	for (i = 0; i < p_chk->header.num_ranges; i++) {
		CPRINTS("%s: %x:%x", __func__,
			p_chk->payload.ranges[i].flash_offset,
			p_chk->payload.ranges[i].range_size);
		/* Make sure the message gets out before verification starts. */
		cflush();
		usb_spi_sha256_update(&ctx,
				      p_chk->payload.ranges[i].flash_offset,
				      p_chk->payload.ranges[i].range_size);
	}

	usb_spi_sha256_final(&ctx, digest, sizeof(digest));
	if (memcmp(digest, p_chk->payload.digest, sizeof(digest))) {
		CPRINTS("AP RO verification FAILED!");
		CPRINTS("Calculated digest %ph",
			HEX_BUF(digest, sizeof(digest)));
		CPRINTS("Stored digest %ph",
			HEX_BUF(p_chk->payload.digest,
				sizeof(p_chk->payload.digest)));
		rv = EC_ERROR_CRC;
	} else {
		rv = EC_SUCCESS;
		CPRINTS("AP RO verification SUCCEEDED!");
	}
	disable_ap_spi_hash_shortcut();

	return rv;
}

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
		/*
		 * TODO(vbendeb): Make this a partial erase, use refactored
		 * Board ID space partial erase.
		 */
		flash_open_ro_window(h1_flash_offset_, AP_RO_DATA_SPACE_SIZE);
		flash_physical_erase(h1_flash_offset_, AP_RO_DATA_SPACE_SIZE);
	}
#endif
	if ((p_chk->header.num_ranges == (uint16_t)~0) &&
	    (p_chk->header.checksum == ~0)) {
		ccprintf("AP RO check space is not programmed\n");
		return EC_SUCCESS;
	}

	rv = verify_ap_ro_check_space();
	if (rv != EC_SUCCESS)
		return rv; /* No verification possible. */

	ccprintf("sha256 hash %ph\n",
		 HEX_BUF(p_chk->payload.digest, sizeof(p_chk->payload.digest)));
	ccprintf("Covered ranges:\n");
	for (i = 0; i < p_chk->header.num_ranges; i++)
		ccprintf("%08x...%08x\n", p_chk->payload.ranges[i].flash_offset,
			 p_chk->payload.ranges[i].flash_offset +
				 p_chk->payload.ranges[i].range_size - 1);

	return EC_SUCCESS;
}
DECLARE_SAFE_CONSOLE_COMMAND(ap_ro_info, ap_ro_info_cmd,
#ifdef CR50_DEV
			     "[erase]", "Display or erase AP RO check space"
#else
			     "", "Display AP RO check space"
#endif
);
