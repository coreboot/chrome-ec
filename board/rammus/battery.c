/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Placeholder values for temporary battery pack.
 */

#include "battery.h"
#include "battery_smart.h"
#include "charge_manager.h"
#include "charge_state.h"
#include "chipset.h"
#include "console.h"
#include "ec_commands.h"
#include "extpower.h"
#include "hooks.h"
#include "gpio.h"
#include "usb_pd.h"
#include "util.h"

static enum battery_present batt_pres_prev = BP_NOT_SURE;

/* Shutdown mode parameters to write to manufacturer access register */
#define SB_SHIP_MODE_REG	SB_MANUFACTURER_ACCESS
#define SB_SHUTDOWN_DATA	0x0010

static const struct battery_info info = {
	.voltage_max = 13200,
	.voltage_normal = 11550,
	.voltage_min = 9000,
	/* Pre-charge values. */
	.precharge_current = 256, /* mA */

	.start_charging_min_c = 0,
	.start_charging_max_c = 45,
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
	rv = sb_write(SB_SHIP_MODE_REG, SB_SHUTDOWN_DATA);

	if (rv != EC_SUCCESS)
		return rv;

	return sb_write(SB_SHIP_MODE_REG, SB_SHUTDOWN_DATA);
}

enum battery_present battery_hw_present(void)
{
	/* The GPIO is low when the battery is physically present */
	return gpio_get_level(GPIO_BATT_PRES_L) ? BP_NO : BP_YES;
}

static int battery_init(void)
{
	int batt_status;

	return battery_status(&batt_status) ? 0 :
		!!(batt_status & STATUS_INITIALIZED);
}

/*
 * Check for case where both XCHG and XDSG bits are set indicating that even
 * though the FG can be read from the battery, the battery is not able to be
 * charged or discharged. This situation will happen if a battery disconnect was
 * intiaited via H1 setting the DISCONN signal to the battery. This will put the
 * battery pack into a sleep state and when power is reconnected, the FG can be
 * read, but the battery is still not able to provide power to the system. The
 * calling function returns batt_pres = BP_NO, which instructs the charging
 * state machine to prevent powering up the AP on battery alone which could lead
 * to a brownout event when the battery isn't able yet to provide power to the
 * system. .
 */
static int battery_check_disconnect(void)
{
	int rv;
	uint8_t data[6];

	/* Check if battery charging + discharging is disabled. */
	rv = sb_read_mfgacc(PARAM_OPERATION_STATUS,
			    SB_ALT_MANUFACTURER_ACCESS, data, sizeof(data));
	if (rv)
		return BATTERY_DISCONNECT_ERROR;

	if ((data[3] & (BATTERY_DISCHARGING_DISABLED |
			BATTERY_CHARGING_DISABLED)) ==
	    (BATTERY_DISCHARGING_DISABLED | BATTERY_CHARGING_DISABLED))
		return BATTERY_DISCONNECTED;

	return BATTERY_NOT_DISCONNECTED;
}

enum battery_present battery_is_present(void)
{
	enum battery_present batt_pres;

	/* Get the physical hardware status */
	batt_pres = battery_hw_present();

	/*
	 * Make sure battery status is implemented, I2C transactions are
	 * successful & the battery status is initialized to find out if it
	 * is a working battery and it is not in the cut-off mode.
	 *
	 * If battery I2C fails but VBATT is high, battery is booting from
	 * cut-off mode.
	 *
	 * FETs are turned off after Power Shutdown time.
	 * The device will wake up when a voltage is applied to PACK.
	 * Battery status will be inactive until it is initialized.
	 */
	if (batt_pres == BP_YES && batt_pres_prev != batt_pres &&
	    (battery_is_cut_off() != BATTERY_CUTOFF_STATE_NORMAL ||
	     battery_check_disconnect() != BATTERY_NOT_DISCONNECTED ||
	     battery_init() == 0)) {
		batt_pres = BP_NO;
	}

	batt_pres_prev = batt_pres;
	return batt_pres;
}

static void reduce_input_voltage_when_full(void)
{
	struct batt_params batt;
	int max_pd_voltage_mv;
	int active_chg_port;

	active_chg_port = charge_manager_get_active_charge_port();
	if (active_chg_port == CHARGE_PORT_NONE)
		return;

	battery_get_params(&batt);
	if (!(batt.flags & BATT_FLAG_BAD_STATUS)) {
		/* Lower our input voltage to 9V when battery is full. */
		if ((batt.status & STATUS_FULLY_CHARGED) &&
		    chipset_in_state(CHIPSET_STATE_ANY_OFF))
			max_pd_voltage_mv = 9000;
		else
			max_pd_voltage_mv = PD_MAX_VOLTAGE_MV;

		if (pd_get_max_voltage() != max_pd_voltage_mv)
			pd_set_external_voltage_limit(active_chg_port,
						      max_pd_voltage_mv);
	}
}
DECLARE_HOOK(HOOK_SECOND, reduce_input_voltage_when_full, HOOK_PRIO_DEFAULT);
