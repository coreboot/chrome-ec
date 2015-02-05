/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Flash memory module for Chrome EC - common functions */

#include "common.h"
#include "console.h"
#include "flash.h"
#include "host_command.h"
#include "spi.h"
#include "spi_flash.h"
#include "system.h"
#include "util.h"
#include "watchdog.h"

/*
  * Buffer allocated to read data from spi Flash
  *
  */
#define FLASH_EXT_DATACHUNK_SIZE 256
static uint8_t flash_data[FLASH_EXT_DATACHUNK_SIZE];


/**
 * Return the base pointer of the SPI Flash addr for the image copy, or 0xffffffff if error.
 */
uintptr_t flash_get_image_base_spi(enum system_image_copy_t copy)
{
	switch (copy) {
	case SYSTEM_IMAGE_RO:
		return CONFIG_RO_IMAGE_FLASHADDR;
	case SYSTEM_IMAGE_RW:
		return CONFIG_RW_IMAGE_FLASHADDR;
	default:
		return 0xffffffff;
	}
}


uint32_t flash_get_image_used_spi(enum system_image_copy_t copy)
{
	uint32_t image;
	int size = 0;
	uint32_t data_size = 0;
	uint16_t i = 0;

	image = flash_get_image_base_spi(copy);
	size = system_get_image_size(copy);

	CPRINTS("image addr %x\n!!!", image);

	/*
	 * Scan backwards looking for 0xea byte, which is by definition the
	 * last byte of the image.  See ec.lds.S for how this is inserted at
	 * the end of the image.
	 */
	while (size > 0) {

		watchdog_reload();
		data_size = MIN(FLASH_EXT_DATACHUNK_SIZE, size);

		flash_physical_read((image + size - data_size), data_size,
			(uint8_t *)flash_data);


		for (i = data_size; i > 0; i--) {

			if (flash_data[i - 1] == 0xea) {

				/* 0xea byte IS part of the image */
				image -= (data_size - i);
				CPRINTS("size = 0x%x\n",
					size);
				return size;
			}
		}

		size -= data_size;
	}

	CPRINTS("Did not find 0xea size = 0x%x\n", size);
	return size;
}

/*****************************************************************************/
/* Physical layer APIs */

/**
 * Get the physical memory address of a flash offset
 *
 * This is used for direct flash access. We assume that the flash is
 * contiguous from this start address through to the end of the usable
 * flash.
 *
 * @param offset	Flash offset to get address of
 * @param dataptrp	Returns pointer to memory address of flash offset
 * @return pointer to flash memory offset, if ok, else NULL
 */
const char *flash_physical_dataptr(int offset)
{
	return (char *)((uintptr_t)CONFIG_FLASH_BASE_SPI + offset);
}

int flash_physical_write(int offset, int size, const char *data)
{
	int i;

	offset += CONFIG_FLASH_BASE_SPI;

	/* Fail if offset, size, and data aren't at least word-aligned */
	if ((offset | size | (uint32_t)(uintptr_t)data) & 3)
		return EC_ERROR_INVAL;

	spi_enable(1);

	for (i = 0; i < size; i += 16)
		spi_flash_write(offset+i, 16, &data[i]);

	spi_enable(0);

	return EC_SUCCESS;
}

int flash_physical_read(int offset, int size, char *data)
{

	offset += CONFIG_FLASH_BASE_SPI;

	/* Fail if offset, size, and data aren't at least word-aligned */
	if ((offset | size | (uint32_t)(uintptr_t)data) & 3)
		return EC_ERROR_INVAL;

	spi_enable(1);

	spi_flash_read((uint8_t *)data, offset, size);

	spi_enable(0);

	return EC_SUCCESS;
}


int flash_physical_erase(int offset, int size)
{


	offset += CONFIG_FLASH_BASE_SPI;

	spi_enable(1);

	for (; size > 0; size -= CONFIG_FLASH_ERASE_SIZE,
		     offset += CONFIG_FLASH_ERASE_SIZE) {

		/* Do nothing if already erased */
		if (spi_flash_erase(offset, CONFIG_FLASH_ERASE_SIZE))
			return EC_ERROR_UNKNOWN;

		/*
		 * Reload the watchdog timer, so that erasing many flash pages
		 * doesn't cause a watchdog reset.  May not need this now that
		 * we're using msleep() below.
		 */
		watchdog_reload();
	}
	spi_enable(0);

	return EC_SUCCESS;
}

/**
 * Read persistent state into pstate.
 *
 * @param pstate	Destination for persistent state
 */
void flash_read_pstate(struct persist_state *pstate)
{
	/* TODO(crosbug.com/p/36076): IMPLEMENT ME ! */
}

/**
 * Write persistent state from pstate, erasing if necessary.
 *
 * @param pstate	Source persistent state
 * @return EC_SUCCESS, or nonzero if error.
 */
int flash_write_pstate(const struct persist_state *pstate)
{

	/* TODO(crosbug.com/p/36076): IMPLEMENT ME ! */
	return 1;
}

int flash_physical_get_protect(int bank)
{
	/* TODO(crosbug.com/p/36076): IMPLEMENT ME ! */
	return 1;
}

int flash_physical_protect_now(int bank)
{
	/* TODO(crosbug.com/p/36076): IMPLEMENT ME ! */
	return 1;
}

uint32_t flash_physical_get_protect_flags(void)
{
	/* TODO(crosbug.com/p/36076): IMPLEMENT ME ! */
	return 1;

}

uint32_t flash_physical_get_valid_flags(void)
{
	/* TODO(crosbug.com/p/36076): IMPLEMENT ME ! */
	return 1;

}

uint32_t flash_physical_get_writable_flags(uint32_t cur_flags)
{
	/* TODO(crosbug.com/p/36076): IMPLEMENT ME ! */
	return 1;

}

/*****************************************************************************/
/* Host commands */

static int flash_command_region_info(struct host_cmd_handler_args *args)
{
	const struct ec_params_flash_region_info *p = args->params;
	struct ec_response_flash_region_info *r = args->response;

	switch (p->region) {
	case EC_FLASH_REGION_RO:
		r->offset = CONFIG_RO_SPI_OFF;
		r->size = CONFIG_FW_RO_SIZE;
		break;
	case EC_FLASH_REGION_RW:
		r->offset = CONFIG_RW_SPI_OFF;
		r->size = CONFIG_FW_RW_SIZE;
		break;
	case EC_FLASH_REGION_WP_RO:
		r->offset = CONFIG_RO_WP_SPI_OFF;
		r->size = CONFIG_FW_WP_RO_SIZE;
		break;
	default:
		return EC_RES_INVALID_PARAM;
	}

	args->response_size = sizeof(*r);
	return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_FLASH_REGION_INFO,
		     flash_command_region_info,
		     EC_VER_MASK(EC_VER_FLASH_REGION_INFO));
