/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery.h"
#include "battery_smart.h"
#include "gpio.h"
#include "host_command.h"

#define SB_SHIP_MODE_ADDR	0x3a
#define SB_SHIP_MODE_DATA	0xc574

/* Values for 48Wh 4UAF495780-1-T1186/AC011353-PRR14G01 battery */
static const struct battery_info info_4s1p = {

	.voltage_max    = 17200,
	.voltage_normal = 15200, /* Average of max & min */
	.voltage_min    = 12000,

	/* Pre-charge values. */
	.precharge_current  = 256,	/* mA */

	.start_charging_min_c = 0,
	.start_charging_max_c = 50,
	.charging_min_c       = 0,
	.charging_max_c       = 60,
	.discharging_min_c    = 0,
	.discharging_max_c    = 60,
};

/* Values for 54Wh LIS3091ACPC(SYS6)/AC011401-PRR13G01 battery */
static const struct battery_info info_3s1p = {

	.voltage_max    = 12600,
	.voltage_normal = 11100, /* Average of max & min */
	.voltage_min    = 9000,

	 /* Pre-charge values. */
	.precharge_current  = 256,      /* mA */

	.start_charging_min_c = 0,
	.start_charging_max_c = 50,
	.charging_min_c       = 0,
	.charging_max_c       = 60,
	.discharging_min_c    = 0,
	.discharging_max_c    = 75,
};

const struct battery_info *battery_get_info(void)
{
	if (gpio_get_level(GPIO_BAT_ID))
		return &info_3s1p;
	else
		return &info_4s1p;
}

int board_cut_off_battery(void)
{
	return sb_write(SB_SHIP_MODE_ADDR, SB_SHIP_MODE_DATA);
}
