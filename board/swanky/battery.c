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
#include "gpio.h"
#include "host_command.h"
#include "util.h"
#include "hooks.h"

/* Console output macros */
#define CPRINTF(format, args...) cprintf(CC_CHARGER, format, ## args)

/* Battery FET ON/OFF command write to FET off register */
#define SB_CELLS_VOLTAGE_MAX	4000	/* mV */
#define SB_CELL_1_VOLTAGE_REG	0x3F
#define SB_CELL_2_VOLTAGE_REG	0x3E
#define SB_CELL_3_VOLTAGE_REG	0x3D
#define SB_FET_ONOFF_REG	0x35
#define SB_FET_ON_DATA		0x0003
#define SB_FET_OFF_DATA		0x0004
#define SB_FET_STATUS_REG	0x34
#define SB_FET_ONOFF_STATUS	0x0004
#define SB_FET_OFF_STATUS	0x0004
#define SB_FET_ON_STATUS	0x0000


static const struct battery_info info = {
	.voltage_max    = 12600,	/* mV */
	.voltage_normal = 10860,
	.voltage_min    = 9000,
	.precharge_current  = 256,	/* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 50,
	.charging_min_c       = 0,
	.charging_max_c       = 60,
	.discharging_min_c    = 0,
	.discharging_max_c    = 75,
};

const struct battery_info *battery_get_info(void)
{
	return &info;
}

static void wakeup_deferred(void)
{
	int tmp;
	if (extpower_is_present()) {
		sb_read(SB_FET_STATUS_REG, &tmp);
		if ((tmp & SB_FET_ONOFF_STATUS) == SB_FET_OFF_STATUS) {
			sb_write(SB_FET_ONOFF_REG, SB_FET_ON_DATA);
			CPRINTF("[%T Battery wakeup]\n");
		}
	}
}
DECLARE_DEFERRED(wakeup_deferred);

static void wakeup(void)
{
	/*
	 * The deferred call ensures that wakeup_deferred is called from a
	 * task. This is required to talk to the battery over I2C.
	 */
	hook_call_deferred(wakeup_deferred, 0);
}
DECLARE_HOOK(HOOK_INIT, wakeup, HOOK_PRIO_DEFAULT);

static int cutoff(void)
{
	int rv, tmp, cell_voltage;

	/* To check AC adaptor is present or not */
	if (!extpower_is_present()) {
		CPRINTF("[%T AC Adaptor is not present]\n");
		return EC_ERROR_UNKNOWN;
	}

	/* To check the cell voltage of battery pack */
	sb_read(SB_CELL_1_VOLTAGE_REG, &cell_voltage);
	if (cell_voltage >= SB_CELLS_VOLTAGE_MAX) {
		CPRINTF("[%T Battery cell 1 voltage is too high]\n");
		return EC_ERROR_UNKNOWN;
	}

	sb_read(SB_CELL_2_VOLTAGE_REG, &cell_voltage);
	if (cell_voltage >= SB_CELLS_VOLTAGE_MAX) {
		CPRINTF("[%T Battery cell 2 voltage is too high]\n");
		return EC_ERROR_UNKNOWN;
	}

	sb_read(SB_CELL_3_VOLTAGE_REG, &cell_voltage);
	if (cell_voltage >= SB_CELLS_VOLTAGE_MAX) {
		CPRINTF("[%T Battery cell 3 voltage is too high]\n");
		return EC_ERROR_UNKNOWN;
	}

	/* Ship mode command must be sent to take effect */
	rv = sb_write(SB_FET_ONOFF_REG, SB_FET_OFF_DATA);

	if (rv != EC_SUCCESS)
		return rv;

	sb_read(SB_FET_STATUS_REG, &tmp);
	if ((tmp & SB_FET_ONOFF_STATUS) == SB_FET_OFF_STATUS) {
		CPRINTF("[%T Battery cut off]\n");
		return EC_SUCCESS;
	}

	return EC_ERROR_UNKNOWN;
}

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
