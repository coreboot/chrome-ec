/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * EC-EFS (Early Firmware Selection)
 */
#include "common.h"
#include "console.h"
#include "ec_comm.h"
#include "ec_commands.h"
#include "hooks.h"
#include "registers.h"
#include "system.h"
#include "vboot.h"

#ifdef CR50_DEV
#define CPRINTS(format, args...) cprints(CC_TASK, "EC-EFS: " format, ## args)
#else
#define CPRINTS(format, args...) do { } while (0)
#endif

/*
 * Context of EC-EFS
 */
static struct ec_efs_context_ {
	uint32_t boot_mode:8;	        /* enum ec_efs_boot_mode */
	uint32_t hash_is_loaded:1;	/* Is EC hash loaded from nvmem */
	uint32_t reserved:23;

	uint32_t secdata_error_code;

	uint8_t hash[SHA256_DIGEST_SIZE];	/* EC-RW digest */
} ec_efs_ctx;

/*
 * Change the boot mode
 *
 * @param mode_val New boot mode value to change
 */
static void set_boot_mode_(uint8_t mode_val)
{
	CPRINTS("boot_mode: 0x%02x -> 0x%02x", ec_efs_ctx.boot_mode, mode_val);

	ec_efs_ctx.boot_mode = mode_val;

	/* Backup some ec_efs context to scratch register */
	GREG32(PMU, PWRDN_SCRATCH20) &= ~0xff;
	GREG32(PMU, PWRDN_SCRATCH20) |= mode_val;
}

/*
 * Initialize EC-EFS context.
 */
static void ec_efs_init_(void)
{
	if (!board_has_ec_cr50_comm_support())
		return;

	/*
	 * If it is a wakeup from deep sleep, then recover some core EC-EFS
	 * context values, including the boot_mode value, from a PWRD_SCRATCH
	 * register. Otherwise, reset boot_mode.
	 */
	if (system_get_reset_flags() & EC_RESET_FLAG_HIBERNATE)
		set_boot_mode_(GREG32(PMU, PWRDN_SCRATCH20) & 0xff);
	else
		ec_efs_reset();

	/* TODO(crbug/1020578): Read Hash from Kernel NV Index */
}
DECLARE_HOOK(HOOK_INIT, ec_efs_init_, HOOK_PRIO_DEFAULT);

void ec_efs_reset(void)
{
	set_boot_mode_(EC_EFS_BOOT_MODE_NORMAL);
}
