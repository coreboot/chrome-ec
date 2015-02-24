/* Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery.h"
#include "battery_smart.h"
#include "console.h"
#include "gpio.h"
#include "host_command.h"
#include "util.h"

#define	SB_SHIP_MODE_ADDR	0x3a
#define	SB_SHIP_MODE_DATA	0xc574

/* Values for 48Wh 4UAF495780-1-T1186/AC011353-PRR14G01 battery */
static const struct battery_info info = {

	.voltage_max	= 17200,
	.voltage_normal	= 15200, /* Average of max & min */
	.voltage_min	= 12000,
	/* Pre-charge values. */
	.precharge_current	= 256,	/* mA */
	.start_charging_min_c	= 0,
	.start_charging_max_c	= 50,
	.charging_min_c		= 0,
	.charging_max_c		= 60,
	.discharging_min_c	= 0,
	.discharging_max_c	= 40,
};

const struct battery_info *battery_get_info(void)
{
	return &info;
}

static int battery_command_cut_off(struct host_cmd_handler_args *args)
{
	return sb_write(SB_SHIP_MODE_ADDR, SB_SHIP_MODE_DATA);
}
DECLARE_HOST_COMMAND(EC_CMD_BATTERY_CUT_OFF, battery_command_cut_off,
		     EC_VER_MASK(0));

static int command_battcutoff(int argc, char **argv)
{
	return sb_write(SB_SHIP_MODE_ADDR, SB_SHIP_MODE_DATA);
}
DECLARE_CONSOLE_COMMAND(battcutoff, command_battcutoff,
			NULL,
			"Enable battery cutoff (ship mode)",
			NULL);
