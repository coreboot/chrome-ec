/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery.h"
#include "battery_smart.h"
#include "console.h"
#include "extpower.h"
#include "gpio.h"
#include "hooks.h"
#include "host_command.h"
#include "system.h"
#include "util.h"

/* FET ON/OFF cammand write to fet off register */
#define SB_FET_OFF      0x34
#define SB_FETOFF_DATA1 0x0000
#define SB_FETOFF_DATA2 0x1000
#define SB_FETON_DATA1  0x2000
#define SB_FETON_DATA2  0x4000
#define BATTERY_FETOFF  0x0100

/* First use day base */
#define BATT_FUD_BASE   0x38

/*
 * Green book support parameter
 * Enable this will make battery meet JEITA standard
 */
#define GREEN_BOOK_SUPPORT      (1 << 2)

/* Shutdown mode parameter to write to manufacturer access register */
#define SB_SHUTDOWN_DATA	0x0010

static const struct battery_info info = {
	.voltage_max = 13050,/* mV */
	.voltage_normal = 11400,
	.voltage_min = 9000,
	.precharge_current = 150,/* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 45,
	.charging_min_c = 0,
	.charging_max_c = 45,
	.discharging_min_c = -20,
	.discharging_max_c = 60,
};

const struct battery_info *battery_get_info(void)
{
	return &info;
}

static void battery_wakeup(void)
{
	int d;
	int mode;

	/* Add Green Book support */
	if (sb_read(SB_BATTERY_MODE, &mode)) {
		mode |= GREEN_BOOK_SUPPORT;
		sb_write(SB_BATTERY_MODE, mode);
        }

	sb_read(SB_FET_OFF, &d);
	if (extpower_is_present() && (BATTERY_FETOFF == d)) {
		sb_write(SB_FET_OFF, SB_FETON_DATA1);
		sb_write(SB_FET_OFF, SB_FETON_DATA2);
	}
}
DECLARE_HOOK(HOOK_INIT, battery_wakeup, HOOK_PRIO_DEFAULT);

static int battery_cutoff(void)
{
	int rv;

	/* Ship mode command must be sent twice to take effect */
	rv = sb_write(SB_FET_OFF, SB_FETOFF_DATA1);

	if (rv != EC_SUCCESS)
		return rv;

	return sb_write(SB_FET_OFF, SB_FETOFF_DATA2);
}

int board_cut_off_battery(void)
{
	return battery_cutoff();
}

int battery_get_vendor_param(uint32_t param, uint32_t *value)
{
	return EC_ERROR_UNIMPLEMENTED;
}

/* parameter 0 for first use day */
int battery_set_vendor_param(uint32_t param, uint32_t value)
{
	if (param == 0) {
		int rv, ymd;

		rv = sb_read(BATT_FUD_BASE, &ymd);
		if (rv != EC_SUCCESS)
			return EC_ERROR_UNKNOWN;
		if (ymd == 0)
			return sb_write(BATT_FUD_BASE, value) ?
					EC_ERROR_UNKNOWN : EC_SUCCESS;

		rv = sb_read(BATT_FUD_BASE | 0x03, &ymd);
		if (rv != EC_SUCCESS)
			return EC_ERROR_UNKNOWN;
		if (ymd == 0)
			return sb_write(BATT_FUD_BASE | 0x03, value) ?
					EC_ERROR_UNKNOWN : EC_SUCCESS;

		rv = sb_read(BATT_FUD_BASE | 0x07, &ymd);
		if (rv != EC_SUCCESS)
			return EC_ERROR_UNKNOWN;
		if (ymd == 0)
			return sb_write(BATT_FUD_BASE | 0x07, value) ?
					EC_ERROR_UNKNOWN : EC_SUCCESS;

		return EC_ERROR_UNKNOWN;
	} else {
		return EC_ERROR_UNIMPLEMENTED;
	}
}
