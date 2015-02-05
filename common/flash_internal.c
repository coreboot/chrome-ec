/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Flash memory module for Flash internal to Chrome EC - common functions */

#include "common.h"
#include "console.h"
#include "flash.h"
#include "gpio.h"
#include "host_command.h"
#include "shared_mem.h"
#include "system.h"
#include "util.h"
#include "vboot_hash.h"

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
	return (char *)((uintptr_t)CONFIG_FLASH_BASE + offset);
}

/**
 * Read persistent state into pstate.
 *
 * @param pstate	Destination for persistent state
 */
void flash_read_pstate(struct persist_state *pstate)
{
	memcpy(pstate, flash_physical_dataptr(PSTATE_OFFSET), sizeof(*pstate));

	/* Sanity-check data and initialize if necessary */
	if (pstate->version != PERSIST_STATE_VERSION) {
		memset(pstate, 0, sizeof(*pstate));
		pstate->version = PERSIST_STATE_VERSION;
#ifdef CONFIG_WP_ALWAYS
		pstate->flags |= PERSIST_FLAG_PROTECT_RO;
#endif
	}
}

/**
 * Write persistent state from pstate, erasing if necessary.
 *
 * @param pstate	Source persistent state
 * @return EC_SUCCESS, or nonzero if error.
 */
int flash_write_pstate(const struct persist_state *pstate)
{
	struct persist_state current_pstate;
	int rv;

	/* Check if pstate has actually changed */
	flash_read_pstate(&current_pstate);
	if (!memcmp(&current_pstate, pstate, sizeof(*pstate)))
		return EC_SUCCESS;

	/* Erase pstate */
	rv = flash_physical_erase(PSTATE_OFFSET, PSTATE_SIZE);
	if (rv)
		return rv;

	/*
	 * Note that if we lose power in here, we'll lose the pstate contents.
	 * That's ok, because it's only possible to write the pstate before
	 * it's protected.
	 */

	/* Rewrite the data */
	return flash_physical_write(PSTATE_OFFSET, sizeof(*pstate),
				    (const char *)pstate);
}

static int flash_command_region_info(struct host_cmd_handler_args *args)
{
	const struct ec_params_flash_region_info *p = args->params;
	struct ec_response_flash_region_info *r = args->response;

	switch (p->region) {
	case EC_FLASH_REGION_RO:
		r->offset = CONFIG_FW_RO_OFF;
		r->size = CONFIG_FW_RO_SIZE;
		break;
	case EC_FLASH_REGION_RW:
		r->offset = CONFIG_FW_RW_OFF;
		r->size = CONFIG_FW_RW_SIZE;
		break;
	case EC_FLASH_REGION_WP_RO:
		r->offset = CONFIG_FW_WP_RO_OFF;
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
