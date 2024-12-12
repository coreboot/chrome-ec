/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Board re-init for Telith board.
 * Telith has convertible and clamshell config,
 * and shares the same firmware.
 * So some functions should be disabled for clamshell.
 */
#include "accelgyro.h"
#include "battery.h"
#include "common.h"
#include "cros_cbi.h"
#include "driver/accel_bma4xx.h"
#include "driver/accel_lis2dw12_public.h"
#include "driver/accelgyro_bmi323.h"
#include "driver/accelgyro_icm42607.h"
#include "driver/accelgyro_lsm6dsm.h"
#include "fan.h"
#include "gpio/gpio_int.h"
#include "hooks.h"
#include "motion_sense.h"
#include "motionsense_sensors.h"
#include "tablet_mode.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <ap_power/ap_power.h>

LOG_MODULE_REGISTER(board_init, LOG_LEVEL_ERR);

enum base_sensor_type {
	base_none = 0,
	base_bmi323,
	base_lsm6dsm,
	base_icm42607,
};

enum lid_sensor_type {
	lid_none = 0,
	lid_bma422,
	lid_lis2dw12,
};

static int base_use_alt_sensor;
static int lid_use_alt_sensor;

test_export_static void board_setup_init(void)
{
	int ret;
	uint32_t val;

	ret = cros_cbi_get_fw_config(FORM_FACTOR, &val);
	if (ret != 0) {
		LOG_ERR("Error retrieving CBI FW_CONFIG field %d", FORM_FACTOR);
		return;
	}
	if (val == CLAMSHELL) {
		motion_sensor_count = 0;
		gmr_tablet_switch_disable();
		gpio_disable_dt_interrupt(GPIO_INT_FROM_NODELABEL(int_imu));
		gpio_pin_configure_dt(GPIO_DT_FROM_NODELABEL(gpio_imu_int_l),
				      GPIO_INPUT | GPIO_PULL_UP);
		gpio_disable_dt_interrupt(GPIO_INT_FROM_NODELABEL(int_lid_imu));
		gpio_pin_configure_dt(GPIO_DT_FROM_NODELABEL(gpio_acc_int_l),
				      GPIO_INPUT | GPIO_PULL_UP);
	}
}
DECLARE_HOOK(HOOK_INIT, board_setup_init, HOOK_PRIO_PRE_DEFAULT);

void motion_interrupt(enum gpio_signal signal)
{
	if (base_use_alt_sensor == base_lsm6dsm)
		lsm6dsm_interrupt(signal);
	else if (base_use_alt_sensor == base_icm42607)
		icm42607_interrupt(signal);
	else
		bmi3xx_interrupt(signal);
}

void lid_accel_interrupt(enum gpio_signal signal)
{
	if (lid_use_alt_sensor == lid_lis2dw12)
		lis2dw12_interrupt(signal);
	else
		bma4xx_interrupt(signal);
}

#define INT_RECHECK_US 5000

static void check_audio_jack(void);
DECLARE_DEFERRED(check_audio_jack);

static void check_audio_jack(void)
{
	int value;
	if (chipset_in_state(CHIPSET_STATE_ON)) {
		if (gpio_pin_get_dt(
			    GPIO_DT_FROM_NODELABEL(gpio_jack_detect_l))) {
			value = 0;
			gpio_pin_set_dt(
				GPIO_DT_FROM_NODELABEL(gpio_loading_enable_l),
				value);
		} else {
			value = 1;
			gpio_pin_set_dt(
				GPIO_DT_FROM_NODELABEL(gpio_loading_enable_l),
				value);
		}
	} else {
		value = 0;
		gpio_pin_set_dt(GPIO_DT_FROM_NODELABEL(gpio_loading_enable_l),
				value);
	}
}

void audio_jack_interrupt(enum gpio_signal signal)
{
	hook_call_deferred(&check_audio_jack_data, INT_RECHECK_US);
}

static void jack_detect_init(void)
{
	gpio_enable_dt_interrupt(GPIO_INT_FROM_NODELABEL(int_jack_detect));
}
DECLARE_HOOK(HOOK_INIT, jack_detect_init, HOOK_PRIO_DEFAULT);

static void check_audio_jack_handler(struct ap_power_ev_callback *cb,
				     struct ap_power_ev_data data)
{
	int value;

	switch (data.event) {
	default:
		return;

	case AP_POWER_RESUME:
		/* Called on AP S3 -> S0 transition */
		if (gpio_pin_get_dt(
			    GPIO_DT_FROM_NODELABEL(gpio_jack_detect_l))) {
			value = 0;
			gpio_pin_set_dt(
				GPIO_DT_FROM_NODELABEL(gpio_loading_enable_l),
				value);
		} else {
			value = 1;
			gpio_pin_set_dt(
				GPIO_DT_FROM_NODELABEL(gpio_loading_enable_l),
				value);
		}
		break;

	case AP_POWER_SUSPEND:
	case AP_POWER_SHUTDOWN:
		value = 0;
		/* Called on AP S0 -> S3 transition */
		gpio_pin_set_dt(GPIO_DT_FROM_NODELABEL(gpio_loading_enable_l),
				value);
		break;
	}
}

static int jack_detect_handler(void)
{
	static struct ap_power_ev_callback cb;
	/*
	 * Add a callback for suspend/resume/shutdowm to
	 * control the jack_detect VCC.
	 */
	ap_power_ev_init_callback(&cb, check_audio_jack_handler,
				  AP_POWER_RESUME | AP_POWER_SUSPEND |
					  AP_POWER_SHUTDOWN);
	ap_power_ev_add_callback(&cb);

	return 0;
}
SYS_INIT(jack_detect_handler, APPLICATION, 1);

test_export_static void alt_sensor_init(void)
{
	int ret;
	uint32_t val;

	/* Check if it's clamshell or convertible */
	ret = cros_cbi_get_fw_config(FORM_FACTOR, &val);
	if (ret != 0) {
		LOG_ERR("Error retrieving CBI FW_CONFIG field %d", FORM_FACTOR);
		return;
	}
	if (val == CLAMSHELL)
		return;

	/* Check which motion sensors are used */
	if (cros_cbi_ssfc_check_match(
		    CBI_SSFC_VALUE_ID(DT_NODELABEL(base_sensor_lsm6dsm))))
		base_use_alt_sensor = base_lsm6dsm;
	else if (cros_cbi_ssfc_check_match(
			 CBI_SSFC_VALUE_ID(DT_NODELABEL(base_sensor_icm42607))))
		base_use_alt_sensor = base_icm42607;
	else
		base_use_alt_sensor = base_bmi323;

	if (cros_cbi_ssfc_check_match(
		    CBI_SSFC_VALUE_ID(DT_NODELABEL(lid_sensor_lis2dw12))))
		lid_use_alt_sensor = lid_lis2dw12;
	else
		lid_use_alt_sensor = lid_bma422;

	motion_sensors_check_ssfc();
}
DECLARE_HOOK(HOOK_INIT, alt_sensor_init, HOOK_PRIO_POST_I2C);

enum battery_present battery_hw_present(void)
{
	const struct gpio_dt_spec *batt_pres;

	batt_pres = GPIO_DT_FROM_NODELABEL(gpio_ec_battery_pres_odl);

	/* The GPIO is low when the battery is physically present */
	return gpio_pin_get_dt(batt_pres) ? BP_NO : BP_YES;
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
	} else {
		/* Configure the fan enable GPIO */
		gpio_pin_configure_dt(
			GPIO_DT_FROM_NODELABEL(gpio_en_pp5000_fan),
			GPIO_OUTPUT);
	}
}
DECLARE_HOOK(HOOK_INIT, fan_init, HOOK_PRIO_POST_FIRST);

static bool key_bl = FW_KB_BL_NOT_PRESENT;
static bool key_numpad = FW_KB_NUMPAD_NOT_PRESENT;
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
		key_bl = FW_KB_BL_PRESENT;
	} else {
		key_bl = FW_KB_BL_NOT_PRESENT;
	}

	ret = cros_cbi_get_fw_config(FW_KB_NUMPAD, &val);
	if (val == FW_KB_NUMPAD_PRESENT) {
		key_numpad = FW_KB_NUMPAD_PRESENT;
	} else {
		key_numpad = FW_KB_NUMPAD_NOT_PRESENT;
	}
}
DECLARE_HOOK(HOOK_INIT, kb_init, HOOK_PRIO_POST_FIRST);

int8_t board_vivaldi_keybd_idx(void)
{
	int kb_status;

	kb_status = (key_numpad << 1) | key_bl;

	switch (kb_status) {
	case 0:
		return DT_NODE_CHILD_IDX(DT_NODELABEL(kbd_config_0));
	case 1:
		return DT_NODE_CHILD_IDX(DT_NODELABEL(kbd_config_1));
	case 2:
		return DT_NODE_CHILD_IDX(DT_NODELABEL(kbd_config_2));
	case 3:
		return DT_NODE_CHILD_IDX(DT_NODELABEL(kbd_config_3));
	default:
		return DT_NODE_CHILD_IDX(DT_NODELABEL(kbd_config_0));
	}
}

/*
 * Enable interrupts
 */
static void board_init(void)
{
	gpio_pin_set_dt(GPIO_DT_FROM_NODELABEL(gpio_en_pp5000_s5), 1);
}
DECLARE_HOOK(HOOK_INIT, board_init, HOOK_PRIO_DEFAULT);
