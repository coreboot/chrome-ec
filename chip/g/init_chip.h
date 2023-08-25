/* Copyright 2016 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_INIT_CHIP_H
#define __CROS_EC_INIT_CHIP_H

/**
 * This is the current state of the PMU persistent registers. There are two
 * types: long life and pwrdn scratch. Long life will persist through any
 * reset other than POR. PWRDN scratch only survives deep sleep.
 *
 * LONG_LIFE_SCRATCH0 - Rollback counter
 * LONG_LIFE_SCRATCH1 - Board properties as bit mask (see `scratch_reg1.h`)
 * LONG_LIFE_SCRATCH2 - Unused
 * LONG_LIFE_SCRATCH3 - Unused
 *
 *
 * PWRDN_SCRATCH0 - PWRDN_SCRATCH15 - Can be locked by boot ROM or RO
 *
 * PWRDN_SCRATCH 16 - 27 - Can be used by RW:
 *
 * PWRDN_SCRATCH16 - Indicator that firmware is running for debug purposes
 * PWRDN_SCRATCH17 - Deep sleep counter
 * PWRDN_SCRATCH18 - Preserving USB_DCFG through deep sleep
 * PWRDN_SCRATCH19 - Preserving USB data sequencing PID through deep sleep
 * PWRDN_SCRATCH20 - Preserving EC-EFS context
 * PWRDN_SCRATCH21 - Preserving TPM_BOARD_CFG register
 * PWRDN_SCRATCH22 - Preserve FIPS power-up test status on Cr50
 * PWRDN_SCRATCH23 - Preserve time since cold boot
 * PWRDN_SCRATCH24 - Preserve count of flash errors in low 12 bits
 * PWRDN_SCRATCH25 - Unused
 * PWRDN_SCRATCH26 - Unused
 * PWRDN_SCRATCH27 - Unused
 *
 * PWRDN_SCRATCH 28 - 31 - Reserved for boot rom
 */

#define PWRDN_SCRATCH24_FLASH_ERROR_MASK 0xFFFU

enum permission_level {
	PERMISSION_LOW = 0x00,
	PERMISSION_MEDIUM = 0x33,    /* APPS run at medium */
	PERMISSION_HIGH = 0x3C,
	PERMISSION_HIGHEST = 0x55
};

int runlevel_is_high(void);
void init_runlevel(const enum permission_level desired_level);

void init_jittery_clock(int highsec);
void init_jittery_clock_locking_optional(int highsec,
					 int enable, int lock_required);
void init_sof_clock(void);

#endif	/* __CROS_EC_INIT_CHIP_H */
