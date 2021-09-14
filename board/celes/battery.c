/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery.h"
#include "battery_smart.h"
#include "charge_state.h"
#include "ec_commands.h"
#include "gpio.h"
#include "system.h"

/* Shutdown mode parameter to write to manufacturer access register */
#define SB_SHUTDOWN_DATA	0x0010
#define CHARGING_VOLTAGE_LIMIT	8500
#define CHARGING_CURRENT_LIMIT	1930

static const struct battery_info info = {
	.voltage_max = 8700,/* mV */
	.voltage_normal = 7600,
	.voltage_min = 6000,
	.precharge_current = 150,/* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 45,
	.charging_min_c = 0,
	.charging_max_c = 60,
	.discharging_min_c = -20,
	.discharging_max_c = 60,
};

#ifdef CONFIG_BATTERY_PRESENT_CUSTOM
/**
 * Custom physical check of battery presence.
 */
enum battery_present battery_is_present(void)
{
	if (system_get_board_version() < 5)
		return BP_YES;
	else
		return gpio_get_level(GPIO_BAT_PRESENT_L) ? BP_NO : BP_YES;
}
#endif

const struct battery_info *battery_get_info(void)
{
	return &info;
}

int board_cut_off_battery(void)
{
	int rv;

	/* Ship mode command must be sent twice to take effect */
	rv = sb_write(SB_MANUFACTURER_ACCESS, SB_SHUTDOWN_DATA);

	if (rv != EC_SUCCESS)
		return rv;

	return sb_write(SB_MANUFACTURER_ACCESS, SB_SHUTDOWN_DATA);
}

int charger_profile_override(struct charge_state_data *curr)
{
	if (curr->requested_voltage > CHARGING_VOLTAGE_LIMIT)
		curr->requested_voltage = CHARGING_VOLTAGE_LIMIT;
	if (curr->requested_current > CHARGING_CURRENT_LIMIT)
		curr->requested_current = CHARGING_CURRENT_LIMIT;
	return 0;
}

/* Customs options controllable by host command. */
#define PARAM_FASTCHARGE (CS_PARAM_CUSTOM_PROFILE_MIN + 0)

enum ec_status charger_profile_override_get_param(uint32_t param,
		uint32_t *value)
{
	return EC_RES_INVALID_PARAM;
}

enum ec_status charger_profile_override_set_param(uint32_t param,
		uint32_t value)
{
	return EC_RES_INVALID_PARAM;
}
