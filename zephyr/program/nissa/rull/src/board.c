/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "battery.h"
#include "charge_state.h"
#include "common.h"
#include "cros_board_info.h"
#include "cros_cbi.h"
#include "ec_commands.h"
#include "fan.h"
#include "gpio.h"
#include "gpio/gpio_int.h"
#include "hooks.h"
#include "keyboard_backlight.h"
#include "keyboard_customization.h"
#include "util.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(board_init, LOG_LEVEL_INF);
#define CPRINTS(format, args...) cprints(CC_SYSTEM, format, ##args)

#define SB_AP23A7L 0x00
#define CONFIG_BATTERY_ACTIVATION_TIMEOUT (10 * SECOND)

enum battery_present battery_hw_present(void)
{
	const struct gpio_dt_spec *batt_pres;

	batt_pres = GPIO_DT_FROM_NODELABEL(gpio_ec_battery_pres_odl);

	/* The GPIO is low when the battery is physically present */
	return gpio_pin_get_dt(batt_pres) ? BP_NO : BP_YES;
}

enum battery_present battery_is_present(void)
{
	int state;

	if (gpio_pin_get_dt(GPIO_DT_FROM_NODELABEL(gpio_ec_battery_pres_odl)))
		return BP_NO;

	/*
	 *  According to the battery manufacturer's reply:
	 *  To detect a bad battery, need to read the 0x00 register.
	 *  If the 12th bit(Permanently Failure) is 1, it means a bad battery.
	 */
	if (sb_read(SB_MANUFACTURER_ACCESS, &state))
		return BP_NO;

	/* Detect the 12th bit value */
	if (state & BIT(12))
		return BP_NO;

	return BP_YES;
}

static const struct battery_info *batt_info;
int charger_profile_override(struct charge_state_data *curr)
{
	batt_info = battery_get_info();

	if (get_time().val < CONFIG_BATTERY_ACTIVATION_TIMEOUT &&
	    (battery_hw_present() == BP_YES) &&
	    curr->batt.voltage <= batt_info->voltage_min) {
		int current = batt_info->precharge_current;

		curr->requested_current = MAX(curr->requested_current, current);

		return -1;
	}

	return 0;
}

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

test_export_static void fan_init(void)
{
	int ret;
	uint32_t val;
	/*
	 * Retrieve the fan config.
	 */
	ret = cros_cbi_get_fw_config(FW_FAN, &val);
	if (ret != 0) {
		LOG_ERR("Error retrieving CBI FW_CONFIG field %d", FW_FAN);
		return;
	}
	if (val != FW_FAN_PRESENT) {
		/* Disable the fan */
		fan_set_count(0);
	}
}
DECLARE_HOOK(HOOK_INIT, fan_init, HOOK_PRIO_POST_FIRST);

static bool has_backlight = FW_KB_BL_NOT_PRESENT;
static bool has_numeric_pad = FW_KB_NUMPAD_NOT_PRESENT;

int8_t board_vivaldi_keybd_idx(void)
{
	int config_index;

	CPRINTS("Keyboard configuration: %s backlight.",
		has_backlight ? "with" : "without");
	CPRINTS("Keyboard configuration: %s numeric pad.",
		has_numeric_pad ? "with" : "without");

	config_index = (has_backlight << 1) | has_numeric_pad;

	switch (config_index) {
	case 0: // No backlight, no numeric pad
		return DT_NODE_CHILD_IDX(DT_NODELABEL(kbd_config_0));
	case 1: // No backlight, with numeric pad
		return DT_NODE_CHILD_IDX(DT_NODELABEL(kbd_config_1));
	case 2: // With backlight, no numeric pad
		return DT_NODE_CHILD_IDX(DT_NODELABEL(kbd_config_2));
	case 3: // With backlight, with numeric pad
		return DT_NODE_CHILD_IDX(DT_NODELABEL(kbd_config_3));
	default: // Default to configuration 0
		return DT_NODE_CHILD_IDX(DT_NODELABEL(kbd_config_0));
	}
}

/*
 * Keyboard function decided by FW config.
 */
test_export_static void kb_init(void)
{
	int ret;
	uint32_t val;

	ret = cros_cbi_get_fw_config(FW_KB_BL, &val);
	if (ret != 0) {
		LOG_ERR("Error retrieving CBI FW_CONFIG field %d", FW_KB_BL);
		return;
	}

	if (val == FW_KB_BL_PRESENT) {
		has_backlight = FW_KB_BL_PRESENT;
	} else {
		has_backlight = FW_KB_BL_NOT_PRESENT;
		kblight_enable(0);
	}

	ret = cros_cbi_get_fw_config(FW_KB_NUMPAD, &val);
	if (ret != 0) {
		LOG_ERR("Error retrieving CBI FW_CONFIG field %d, "
			"assuming FW_KB_NUMERIC_PAD_PRESENT",
			FW_KB_NUMPAD);
		val = FW_KB_NUMPAD_NOT_PRESENT;
	}

	if (val == FW_KB_NUMPAD_PRESENT) {
		has_numeric_pad = FW_KB_NUMPAD_PRESENT;
	} else {
		has_numeric_pad = FW_KB_NUMPAD_NOT_PRESENT;
	}
}
DECLARE_HOOK(HOOK_INIT, kb_init, HOOK_PRIO_POST_I2C);
