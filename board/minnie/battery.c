/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */
#include "battery.h"
#include "i2c.h"

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
	}
}

static int cutoff(void)
{
	/* Write SET_SHUTDOWN(0x13) to CTRL(0x00) */
	return i2c_write16(I2C_PORT_BATTERY, 0xaa, 0x0, 0x13);
}

int board_cut_off_battery(void)
{
	return cutoff();
}
