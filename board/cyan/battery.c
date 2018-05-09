/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery.h"
#include "battery_smart.h"
#include "util.h"

#define	SB_SHIP_MODE_ADDR	0x3a
#define	SB_SHIP_MODE_DATA	0xc574

static const struct battery_info info_AC15A3J = {
	.voltage_max = 12600,/* mV */
	.voltage_normal = 10800,
	.voltage_min = 8250,
	.precharge_current = 340,/* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 50,
	.charging_min_c = 0,
	.charging_max_c = 60,
	.discharging_min_c = 0,
	.discharging_max_c = 60,
};

static const struct battery_info info_AC15A8J= {
	.voltage_max = 13200,/* mV */
	.voltage_normal = 11520,
	.voltage_min = 9000,
	.precharge_current = 256,/* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 50,
	.charging_min_c = 0,
	.charging_max_c = 60,
	.discharging_min_c = 0,
	.discharging_max_c = 60,
};

const struct battery_info *battery_get_info(void)
{
	char device[10];

	if (!battery_device_name(device, sizeof(device))) {
		if (!strcasecmp(device, "AC15A3J"))
			return &info_AC15A3J;
		else if (!strcasecmp(device, "AC15A8J"))
			return &info_AC15A8J;
	}
	return &info_AC15A8J;
}

int board_cut_off_battery(void)
{
	return sb_write(SB_SHIP_MODE_ADDR, SB_SHIP_MODE_DATA);
}
