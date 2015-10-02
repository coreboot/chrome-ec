/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
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

/*
 * Green book support parameter
 * Enable this will make battery meet JEITA standard
 */
#define GREEN_BOOK_SUPPORT      (1 << 2)

static const struct battery_info info = {
	.voltage_max = 12600,/* mV */
	.voltage_normal = 11100,
	.voltage_min = 9000,
	.precharge_current = 204,/* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 60,
	.charging_min_c = 0,
	.charging_max_c = 60,
	.discharging_min_c = -20,
	.discharging_max_c = 60,
};

const struct battery_info *battery_get_info(void)
{
	return &info;
}

static void battery_init(void)
{
	int d, rv;
	int mode;

	/* Add Green Book support */
	rv = sb_read(SB_BATTERY_MODE, &mode);
	if (rv == EC_SUCCESS) {
		mode |= GREEN_BOOK_SUPPORT;
		sb_write(SB_BATTERY_MODE, mode);
	}

	rv = sb_read(SB_FET_OFF, &d);
	if (rv == EC_SUCCESS && extpower_is_present() && BATTERY_FETOFF == d) {
		sb_write(SB_FET_OFF, SB_FETON_DATA1);
		sb_write(SB_FET_OFF, SB_FETON_DATA2);
	}
}
DECLARE_HOOK(HOOK_INIT, battery_init, HOOK_PRIO_DEFAULT);

static int cutoff(void)
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
	return cutoff();
}
