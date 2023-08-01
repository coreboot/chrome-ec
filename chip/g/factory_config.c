/* Copyright 2023 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.h"
#include "board_id.h"
#include "endian.h"
#include "extension.h"
#include "flash_info.h"
#include "system.h"
#include "util.h"

#define CPRINTS(format, args...) cprints(CC_SYSTEM, format, ## args)
#define CPRINTF(format, args...) cprintf(CC_SYSTEM, format, ## args)

static int factory_config_is_blank(uint64_t fc)
{
	return fc == 0;
}

/**
 * Read the INFO1 factory config value into fc.
 *
 * @return EC_SUCCESS or an error code in cases of various failures to read the
 *		      flash space.
 */
int read_factory_config(uint64_t *fc)
{
	uint32_t *fc_p;
	int i;

	/*
	 * The factory config offset is guaranteed to be divisible by 4, and it
	 * is guaranteed to be aligned at 8 bytes.
	 */

	fc_p = (uint32_t *)fc;

	for (i = 0; i < sizeof(*fc); i += sizeof(uint32_t)) {
		int rv;

		rv = flash_physical_info_read_word
			(INFO_FACTORY_CFG_OFFSET + i, fc_p);
		if (rv != EC_SUCCESS) {
			CPRINTF("%s: failed to read word %d, error %d\n",
				__func__, i, rv);
			return rv;
		}
		fc_p++;
	}
	/* The config is stored inverted. Invert the value. */
	*fc = ~(*fc);
	return EC_SUCCESS;
}

void print_factory_config(void)
{
	uint64_t fc;
	int rv;

	rv = read_factory_config(&fc);
	ccprintf("fc = ");
	if (rv)
		ccprintf("invalid (%d)\n", rv);
	else
		ccprintf("0x%016llx\n", fc);
	ccprintf("\n");
}

/**
 * Write the factory config into the flash INFO1 space.
 *
 * @param fc	Pointer to the factory config to copy into info1
 *
 * @return EC_SUCCESS or an error code in cases of various failures to read or
 *              if the space has been already initialized.
 */
static int write_factory_config(uint64_t *new_fc)
{
	uint64_t fc;
	uint32_t rv;
#ifndef CR50_DEV
	struct board_id id;

	/* Fail if Board ID Type is already programmed */
	if (read_board_id(&id) || !board_id_type_is_blank(&id))
		return EC_ERROR_ACCESS_DENIED;
#endif

	rv = read_factory_config(&fc);
	if (rv != EC_SUCCESS) {
		CPRINTS("%s: error reading cfg", __func__);
		return rv;
	}

	if (*new_fc == fc) {
		CPRINTS("%s: ok.", __func__);
		return EC_SUCCESS;
	}
	if (!factory_config_is_blank(fc)) {
		CPRINTS("%s: factory cfg already programmed", __func__);
		return EC_ERROR_ACCESS_DENIED;
	}

	CPRINTS("Set FC - 0x%llx ", *new_fc);
	/* The config is stored inverted. */
	*new_fc = ~(*new_fc);

	flash_info_write_enable();

	/* Write Board ID */
	rv = flash_info_physical_write(INFO_FACTORY_CFG_OFFSET,
				       sizeof(*new_fc), (const char *)new_fc);
	if (rv != EC_SUCCESS)
		CPRINTS("%s: write failed", __func__);

	flash_info_write_disable();

	return rv;
}

static enum vendor_cmd_rc vc_set_factory_config(enum vendor_cmd_cc code,
						void *buf,
						size_t input_size,
						size_t *response_size)
{
	uint64_t fc;
	uint8_t *pbuf = buf;
	int rv;

	*response_size = 0;

	if (input_size != INFO_FACTORY_CFG_SIZE)
		return VENDOR_RC_BOGUS_ARGS;

	memcpy(&fc, pbuf, INFO_FACTORY_CFG_SIZE);
	/* Convert to line representation. */
	fc = be64toh(fc);

	rv = write_factory_config(&fc);
	if (rv == EC_ERROR_ACCESS_DENIED)
		return VENDOR_RC_NOT_ALLOWED;
	else if (rv)
		return VENDOR_RC_INTERNAL_ERROR;
	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_SET_FACTORY_CONFIG, vc_set_factory_config);

static enum vendor_cmd_rc vc_get_factory_config(enum vendor_cmd_cc code,
						void *buf,
						size_t input_size,
						size_t *response_size)
{
	uint64_t fc;

	*response_size = 0;
	if (read_factory_config(&fc))
		return VENDOR_RC_READ_FLASH_FAIL;

	fc = htobe64(fc);
	memcpy(buf, &fc, sizeof(fc));
	*response_size = sizeof(fc);

	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_GET_FACTORY_CONFIG, vc_get_factory_config);
