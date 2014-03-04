/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery.h"
#include "battery_smart.h"
#include "console.h"
#include "extpower.h"
#include "host_command.h"
#include "util.h"
#include "hooks.h"

/* Console output macros */
#define CPRINTF(format, args...) cprintf(CC_CHARGER, format, ## args)

/* Shutdown mode parameter to write to manufacturer access register */
#define SB_SHUTDOWN_REG		0x34
#define SB_SHUTDOWN_DATA1	0x0000
#define SB_SHUTDOWN_DATA2	0x1000
#define SB_WAKE_DATA1		0x2000
#define SB_WAKE_DATA2		0x4000
#define SB_SHIPMODE		0x0100

static const struct battery_info info = {
	.voltage_max    = 12600,	/* mV */
	.voltage_normal = 11100,
	.voltage_min    = 9000,
	.precharge_current  = 157,	/* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 45,
	.charging_min_c       = 0,
	.charging_max_c       = 60,
	.discharging_min_c    = 0,
	.discharging_max_c    = 73,
};

const struct battery_info *battery_get_info(void)
{
	return &info;
}

static int cutoff(void)
{
	int rv, tmp;

	/* Ship mode command must be sent sequentially to take effect */
	rv = sb_write(SB_SHUTDOWN_REG, SB_SHUTDOWN_DATA1);
	if (rv != EC_SUCCESS)
		return rv;

	rv = sb_write(SB_SHUTDOWN_REG, SB_SHUTDOWN_DATA2);
	if (rv != EC_SUCCESS)
		return rv;

	sb_read(SB_SHUTDOWN_REG, &tmp);
	if (tmp == SB_SHIPMODE) {
		rv = EC_SUCCESS;
		CPRINTF("[%T Battery cut off]\n");
	} else
		rv = EC_ERROR_UNKNOWN;

	return rv;
}

/* This is triggered when the AC state changes */
static void wakeup(void)
{
	int tmp;

	if (extpower_is_present()) {
		sb_read(SB_SHUTDOWN_REG, &tmp);
		if (tmp == SB_SHIPMODE) {
			sb_write(SB_SHUTDOWN_REG, SB_WAKE_DATA1);
			sb_write(SB_SHUTDOWN_REG, SB_WAKE_DATA2);
			CPRINTF("[%T Battery wake]\n");
		}
	}
}
DECLARE_HOOK(HOOK_AC_CHANGE, wakeup, HOOK_PRIO_DEFAULT);

static int battery_command_cut_off(struct host_cmd_handler_args *args)
{
	return cutoff() ? EC_RES_ERROR : EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_BATTERY_CUT_OFF, battery_command_cut_off,
		     EC_VER_MASK(0));

static int command_battcutoff(int argc, char **argv)
{
	return cutoff();
}
DECLARE_CONSOLE_COMMAND(battcutoff, command_battcutoff,
			NULL,
			"Enable battery cutoff (ship mode)",
			NULL);
