/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery.h"
#include "battery_smart.h"
#include "util.h"

/* Shutdown mode parameter to write to manufacturer access register */
#define SB_SHIP_MODE_REG	0x34
#define SB_SHUTDOWN_DATA1	0x0000
#define SB_SHUTDOWN_DATA2       0x1000

static const struct battery_info info = {
	.voltage_max = 12600,
	.voltage_normal = 11100,
	.voltage_min = 9100,
	/* Pre-charge values. */
	.precharge_current = 256, /* mA */

	.start_charging_min_c = 0,
	.start_charging_max_c = 50,
	.charging_min_c = 0,
	.charging_max_c = 60,
	.discharging_min_c = 0,
	.discharging_max_c = 60,
};

const struct battery_info *battery_get_info(void)
{
	return &info;
}

int board_cut_off_battery(void)
{
	int rv;

	/* Ship mode command must be sent twice to take effect */
	rv = sb_write(SB_SHIP_MODE_REG, SB_SHUTDOWN_DATA1);

	if (rv != EC_SUCCESS)
		return rv;

	return sb_write(SB_SHIP_MODE_REG, SB_SHUTDOWN_DATA2);
}
