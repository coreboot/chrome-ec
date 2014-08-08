/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery.h"
#include "battery_smart.h"
#include "charge_state.h"
#include "console.h"
#include "gpio.h"
#include "host_command.h"
#include "util.h"

/* TODO(crosbug.com/p/25416): Set correct battery parameters. */


/* Shutdown mode parameter to write to manufacturer access register */
#define SB_SHUTDOWN_DATA	0x0010

static const struct battery_info info = {
	.voltage_max    = 9000,		/* mV */
	.voltage_normal = 8592,
	.voltage_min    = 5888,
	.precharge_current  = 256,	/* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 45,
	.charging_min_c       = 0,
	.charging_max_c       = 60,
	.discharging_min_c    = -20,
	.discharging_max_c    = 70,
};

const struct battery_info *battery_get_info(void)
{
	return &info;
}

static int cutoff(void)
{
	int rv;

	/* Ship mode command must be sent twice to take effect */
	rv = sb_write(SB_MANUFACTURER_ACCESS, SB_SHUTDOWN_DATA);

	if (rv != EC_SUCCESS)
		return rv;

	return sb_write(SB_MANUFACTURER_ACCESS, SB_SHUTDOWN_DATA);
}

static int battery_command_cut_off(struct host_cmd_handler_args *args)
{
	return cutoff() ? EC_RES_SUCCESS : EC_RES_ERROR;
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

#ifdef CONFIG_BATTERY_OVERRIDE_PARAMS
static timestamp_t start_lowvoltage_time;
static int oem_battery_state;
#define OEM_BATTERY_STATE_DEFAULT 0x00
#define OEM_BATTERY_STATE_ERROR 0x01
#define OEM_BATTERY_STATE_STOP_CHARGE 0x02

inline void board_battery_not_connected(void)
{
	oem_battery_state = OEM_BATTERY_STATE_DEFAULT;
}

void battery_override_params(struct batt_params *batt)
{
	int bat_temp_c = DECI_KELVIN_TO_CELSIUS(batt->temperature);
	int chstate = charge_get_state();

	/* Check battery temperature */
	if((chstate == PWR_STATE_CHARGE) ||
	   (chstate == PWR_STATE_CHARGE_NEAR_FULL)) {
		if((bat_temp_c < info.charging_min_c) ||
		   (bat_temp_c >= info.charging_max_c)) {
			oem_battery_state |= OEM_BATTERY_STATE_STOP_CHARGE;
		}
	} else {
		if((bat_temp_c < info.start_charging_min_c) ||
		   (bat_temp_c >= info.start_charging_max_c)) {
			oem_battery_state |= OEM_BATTERY_STATE_STOP_CHARGE;
		} else {
			oem_battery_state &= ~OEM_BATTERY_STATE_STOP_CHARGE;
		}
	}

	if(!(oem_battery_state & OEM_BATTERY_STATE_ERROR)) {
		if((chstate == PWR_STATE_CHARGE) ||
		   (chstate == PWR_STATE_CHARGE_NEAR_FULL)) {
			/* Check battery overvoltage */
			if(batt->voltage > info.voltage_max) {
				oem_battery_state |= OEM_BATTERY_STATE_ERROR;
			} else if(batt->voltage < info.voltage_min) {
				if((get_time().val - start_lowvoltage_time.val) > 2*HOUR) {
					oem_battery_state |= OEM_BATTERY_STATE_ERROR;
				}
			} else {
				start_lowvoltage_time = get_time();
			}
		} else if((chstate == PWR_STATE_IDLE) || (chstate == PWR_STATE_IDLE0)) {
			start_lowvoltage_time = get_time();
		}
	}

	if(oem_battery_state & OEM_BATTERY_STATE_ERROR) {
		batt->flags |= BATT_FLAG_BAD_VOLTAGE;
		batt->desired_voltage = 0;
		batt->desired_current = 0;

		return;
	}

	if(oem_battery_state & OEM_BATTERY_STATE_STOP_CHARGE) {
		batt->flags &= ~BATT_FLAG_WANT_CHARGE;
		batt->desired_voltage = 0;
		batt->desired_current = 0;

		return;
	}

	if(chstate == PWR_STATE_CHARGE) {
		batt->desired_current = (batt->full_capacity)*6/10;
	}
}
#endif /* CONFIG_BATTERY_OVERRIDE_PARAMS */
