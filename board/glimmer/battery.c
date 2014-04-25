/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery.h"
#include "battery_smart.h"
#include "charger.h"
#include "charge_state.h"
#include "console.h"
#include "driver/charger/bq24715.h"
#include "extpower.h"
#include "gpio.h"
#include "hooks.h"
#include "host_command.h"
#include "util.h"

/* FET ON/OFF cammand write to fet off register */
#define	SB_FET_OFF	0x34
#define	SB_FETOFF_DATA1	0x0000
#define	SB_FETOFF_DATA2	0x1000
#define	SB_FETON_DATA1	0x2000
#define	SB_FETON_DATA2	0x4000
#define	BATTERY_FETOFF	0x0100

/* First use day base */
#define BATT_FUD_BASE	0x38

static const struct battery_info info = {
	.voltage_max    = 8400,		/* mV */
	.voltage_normal = 7400,
	.voltage_min    = 6000,
	.precharge_current  = 200,	/* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 45,
	.charging_min_c       = 0,
	.charging_max_c       = 45,
	.discharging_min_c    = -20,
	.discharging_max_c    = 60,
};

const struct battery_info *battery_get_info(void)
{
	return &info;
}

static void wakeup_deferred(void)
{
	int d;

	sb_read(SB_FET_OFF, &d);

	if (extpower_is_present() && (BATTERY_FETOFF == d)) {
		sb_write(SB_FET_OFF, SB_FETON_DATA1);
		sb_write(SB_FET_OFF, SB_FETON_DATA2);
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
	int rv;

	/* Ship mode command must be sent sequentially to take effect */
	rv = sb_write(SB_FET_OFF, SB_FETOFF_DATA1);

	if (rv != EC_SUCCESS)
		return rv;

	return sb_write(SB_FET_OFF, SB_FETOFF_DATA2);
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

/**
 * Initialize charger additional option value
 */
static void charger_init(void)
{
	if ((charge_get_state() == PWR_STATE_INIT) ||
	    (charge_get_state() == PWR_STATE_REINIT)) {
		int option;

		charger_get_option(&option);

		/* Disable LDO mode */
		option &= ~OPT_LDO_MODE_MASK;

		charger_set_option(option);
	}
}
DECLARE_HOOK(HOOK_CHARGE_STATE_CHANGE, charger_init, HOOK_PRIO_DEFAULT);

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
