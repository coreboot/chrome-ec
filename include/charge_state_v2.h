/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "battery.h"
#include "charger.h"
#include "chipset.h"
#include "timer.h"

#ifndef __CROS_EC_CHARGE_STATE_V2_H
#define __CROS_EC_CHARGE_STATE_V2_H

/*
 * The values exported by charge_get_state() and charge_get_flags() are used
 * only to control the LEDs (with one not-quite-correct exception). For V2
 * we use a different set of states internally.
 */
enum charge_state_v2 {
	ST_IDLE = 0,
	ST_DISCHARGE,
	ST_CHARGE,
	ST_PRECHARGE,

	NUM_STATES_V2
};

struct charge_state_data {
	timestamp_t ts;
	int ac;
	int batt_is_charging;
	struct charger_params chg;
	struct batt_params batt;
	enum charge_state_v2 state;
	int requested_voltage;
	int requested_current;
	int desired_input_current;
};

/*
 * Optional customization.
 *
 * On input, the struct reflects the default behavior. The function can make
 * changes to the state, requested_voltage, or requested_current.
 *
 * Return value:
 *   >0    Desired time in usec for this poll period.
 *   0     Use the default poll period (which varies with the state).
 *  <0     An error occurred. The poll time will be shorter than usual. Too
 *           many errors in a row may trigger some corrective action.
 */
int charger_profile_override(struct charge_state_data *);

/*
 * Access to custom profile params through host commands.
 * What this does is up to the implementation.
 */
enum ec_status charger_profile_override_get_param(uint32_t param,
						  uint32_t *value);
enum ec_status charger_profile_override_set_param(uint32_t param,
						  uint32_t value);

/**
 * Set the charge input current limit. This value is stored and sent every
 * time AC is applied.
 *
 * @param ma New input current limit in mA
 * @return EC_SUCCESS or error
 */
int charge_set_input_current_limit(int ma);

/**
 * Callback with which boards determine action on critical low battery
 *
 * The default implementation is provided in charge_state_v2.c. Overwrite it
 * to customize it.
 *
 * @param curr Pointer to struct charge_state_data
 * @return Action to take.
 */
enum critical_shutdown board_critical_shutdown_check(
		struct charge_state_data *curr);

/**
 * Callback to set battery level for shutdown
 *
 * A board can implement this to customize shutdown battery level at runtime.
 *
 * @return battery level for shutdown
 */
uint8_t board_set_battery_level_shutdown(void);

#endif /* __CROS_EC_CHARGE_STATE_V2_H */
