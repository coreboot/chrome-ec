/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery.h"
#include "battery_smart.h"
#include "system.h"

/* Shutdown mode parameter to write to manufacturer access register */
#define SB_SHUTDOWN_DATA	0x0010

#define	SB_SHIP_MODE_ADDR	0x3a
#define	SB_SHIP_MODE_DATA	0xc574

/* Padova battery board ID */
#define BOARD_VERSION_WITH_TIFA_BATTERY	0x07

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

int board_cut_off_battery(void)
{
	int rv;

	/* Ship mode for Tifa */
	if (BOARD_VERSION_WITH_TIFA_BATTERY == system_get_board_version()) {
		rv = sb_write(SB_SHIP_MODE_ADDR, SB_SHIP_MODE_DATA);
		return rv;
	} else {
		/* Ship mode command must be sent twice to take effect */
		rv = sb_write(SB_MANUFACTURER_ACCESS, SB_SHUTDOWN_DATA);

		if (rv != EC_SUCCESS)
			return rv;

		return sb_write(SB_MANUFACTURER_ACCESS, SB_SHUTDOWN_DATA);
	}
}
