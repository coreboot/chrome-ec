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
#include "system.h"
#include "util.h"

/* Shutdown mode parameter to write to manufacturer access register */
#define SB_SHUTDOWN_DATA	0x0010

/* FET ON/OFF cammand write to fet off register */
#define	SB_FET_OFF	0x34
#define	SB_FETOFF_DATA1	0x0000
#define	SB_FETOFF_DATA2	0x1000
#define	SB_FETON_DATA1	0x2000
#define	SB_FETON_DATA2	0x4000
#define	BATTERY_FETOFF	0x0100

/*
 * Green book support parameter
 * Enable this will make battery meet JEITA standard
 */
#define GREEN_BOOK_SUPPORT	(1 << 2)

/* N21 battery board ID */
#define BOARD_VERSION_WITH_N21_BATTERY	0x02

static const struct battery_info info_nl6 = {
	.voltage_max    = 12600,	/* mV */
	.voltage_normal = 11100,
	.voltage_min    = 9000,
	.precharge_current  = 256,	/* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 45,
	.charging_min_c       = 0,
	.charging_max_c       = 45,
	.discharging_min_c    = 0,
	.discharging_max_c    = 60,
};

static const struct battery_info info_n21 = {
	/* New battery, use BOARD_ID to separate it. */
	.voltage_max    = 12600,	/* mV */
	.voltage_normal = 11100,
	.voltage_min    = 9000,
	.precharge_current  = 200,      /* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 60,
	.charging_min_c       = 0,
	.charging_max_c       = 60,
	.discharging_min_c    = -20,
	.discharging_max_c    = 60,
};

const struct battery_info *battery_get_info(void)
{
	if (BOARD_VERSION_WITH_N21_BATTERY == system_get_board_version())
		return &info_n21;
	else
		return &info_nl6;
}

static void wakeup_deferred(void)
{
	int d;
	int mode;

	/* Add Green Book support */
	sb_read(SB_BATTERY_MODE, &mode);
	mode |= GREEN_BOOK_SUPPORT;
	sb_write(SB_BATTERY_MODE, mode);

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
	/* Only n21 battery needs call wakeup_deferred  */
	if (BOARD_VERSION_WITH_N21_BATTERY == system_get_board_version())
		hook_call_deferred(wakeup_deferred, 0);
}
DECLARE_HOOK(HOOK_INIT, wakeup, HOOK_PRIO_DEFAULT);

static int cutoff(void)
{
	int rv;

	/* Ship mode command must be sent twice to take effect */
	if (BOARD_VERSION_WITH_N21_BATTERY == system_get_board_version())
		rv = sb_write(SB_FET_OFF, SB_FETOFF_DATA1);
	else
		rv = sb_write(SB_MANUFACTURER_ACCESS, SB_SHUTDOWN_DATA);

	if (rv != EC_SUCCESS)
		return rv;

	if (BOARD_VERSION_WITH_N21_BATTERY == system_get_board_version())
		return sb_write(SB_FET_OFF, SB_FETOFF_DATA2);
	else
		return sb_write(SB_MANUFACTURER_ACCESS, SB_SHUTDOWN_DATA);
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
	if ((BOARD_VERSION_WITH_N21_BATTERY == system_get_board_version()) &&
	    ((charge_get_state() == PWR_STATE_INIT) ||
	    (charge_get_state() == PWR_STATE_REINIT))) {
		int option;

		charger_get_option(&option);

		/* Disable LDO mode */
		option &= ~OPT_LDO_MODE_MASK;
		/* Enable SYSOVP */
		option |= OPT_SYSOVP_MASK;

		charger_set_option(option);
	}
}
DECLARE_HOOK(HOOK_CHARGE_STATE_CHANGE, charger_init, HOOK_PRIO_DEFAULT);
