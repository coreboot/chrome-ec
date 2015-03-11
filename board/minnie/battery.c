/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */
#include "battery.h"
#include "extpower.h"
#include "gpio.h"
#include "hooks.h"
#include "timer.h"

#define FORCE_PRECHG_TIME (2 * SECOND)

#ifdef PRINT_FORCE_CHARGE_MSG
#include "console.h"
#define FPC_PRINT(format, args...) cprints(CC_CHARGER, format, ## args)
#else
#define FPC_PRINT(format, args...)
#endif

enum {
	FORCE_PRECHG_IDLE,
	FORCE_PRECHG_CHARGING,
	FORCE_PRECHG_PENDING
} force_pre_charge;

static void deferred_stop_force_precharge(void);

static const struct battery_info info = {
	.voltage_max    = 4350,		/* mV */
	.voltage_normal = 4300,
	.voltage_min    = 3328,
	.precharge_current  = 256,	/* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 45,
	.charging_min_c       = 0,
	.charging_max_c       = 45,
	.discharging_min_c    = 0,
	.discharging_max_c    = 60,
};

const struct battery_info *battery_get_info(void)
{
	return &info;
}

void battery_override_params(struct batt_params *batt)
{
	int temp;

	if (!(batt->flags & BATT_FLAG_BAD_TEMPERATURE)) {
		temp = DECI_KELVIN_TO_CELSIUS(batt->temperature);

		if (temp < 0) {
			batt->desired_voltage = 4350;
			batt->desired_current = 0;
			batt->flags |= BATT_FLAG_BAD_ANY;
		} else if (temp < 12) {
			batt->desired_voltage = 4350;
			batt->desired_current = 1500;
			batt->flags |= BATT_FLAG_WANT_CHARGE;
		} else if (temp < 50) {
			batt->desired_voltage = 4350;
			batt->desired_current = 3500;
			batt->flags |= BATT_FLAG_WANT_CHARGE;
		} else if (temp < 55) {
			batt->desired_voltage = 4110;
			batt->desired_current = 3500;
			batt->flags |= BATT_FLAG_WANT_CHARGE;
		} else {
			batt->desired_voltage = 4110;
			batt->desired_current = 0;
			batt->flags |= BATT_FLAG_BAD_ANY;
		}

		switch (force_pre_charge) {
		case FORCE_PRECHG_PENDING:
			FPC_PRINT("FPC: From pending to charging");
			force_pre_charge = FORCE_PRECHG_CHARGING;
			hook_call_deferred(deferred_stop_force_precharge, FORCE_PRECHG_TIME);

		case FORCE_PRECHG_CHARGING:
			FPC_PRINT("FPC: Force pre-charging");
			batt->state_of_charge = 99;
			batt->desired_current = 256;
			break;

		case FORCE_PRECHG_IDLE:
			break;
		}
	}
	else {
		if (force_pre_charge == FORCE_PRECHG_CHARGING) {
			/* We need force pre-charge, but battery error,
			   so set pending state, wait for battery is OK. */
			force_pre_charge = FORCE_PRECHG_PENDING;
			hook_call_deferred(deferred_stop_force_precharge, -1);
		}
	}
}

static int cutoff(void)
{
	gpio_set_level(GPIO_BAT_CUT_OFF, 0);
	return EC_SUCCESS;
}

int board_cut_off_battery(void)
{
	return cutoff();
}

static void hook_ac_change(void)
{
	struct batt_params batt;
	battery_get_params(&batt);

	if (extpower_is_present() && (batt.state_of_charge == 100)) {
		FPC_PRINT("FPC: Start force pre-charge");
		force_pre_charge = FORCE_PRECHG_CHARGING;
		hook_call_deferred(deferred_stop_force_precharge, FORCE_PRECHG_TIME);
	}
	else {
		force_pre_charge = FORCE_PRECHG_IDLE;
	}
}
DECLARE_HOOK(HOOK_AC_CHANGE, hook_ac_change, HOOK_PRIO_DEFAULT);

static void deferred_stop_force_precharge(void)
{
	FPC_PRINT("FPC: Stop force pre-charge");
	force_pre_charge = FORCE_PRECHG_IDLE;
}
DECLARE_DEFERRED(deferred_stop_force_precharge);
