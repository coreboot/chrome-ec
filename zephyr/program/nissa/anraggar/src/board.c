/* Copyright 2023 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Board re-init for Anraggar board.
 * Anraggar has convertible and clamshell config,
 * and shares the same firmware.
 * So some functions should be disabled for clamshell.
 */
#include "accelgyro.h"
#include "common.h"
#include "cros_cbi.h"
#include "driver/accelgyro_bmi323.h"
#include "driver/accelgyro_lsm6dsm.h"
#include "gpio/gpio_int.h"
#include "hooks.h"
#include "motion_sense.h"
#include "motionsense_sensors.h"
#include "tablet_mode.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(board_init, LOG_LEVEL_ERR);

static bool board_is_clamshell;

static void board_setup_init(void)
{
	int ret;
	uint32_t val;

	ret = cros_cbi_get_fw_config(FORM_FACTOR, &val);
	if (ret != 0) {
		LOG_ERR("Error retrieving CBI FW_CONFIG field %d", FORM_FACTOR);
		return;
	}
	if (val == CLAMSHELL) {
		board_is_clamshell = true;
		motion_sensor_count = 0;
		gmr_tablet_switch_disable();
		gpio_disable_dt_interrupt(GPIO_INT_FROM_NODELABEL(int_imu));
		gpio_pin_configure_dt(GPIO_DT_FROM_NODELABEL(gpio_imu_int_l),
				      GPIO_INPUT | GPIO_PULL_UP);
	}
}
DECLARE_HOOK(HOOK_INIT, board_setup_init, HOOK_PRIO_PRE_DEFAULT);

static bool base_use_alt_sensor;

void motion_interrupt(enum gpio_signal signal)
{
	if (base_use_alt_sensor) {
		lsm6dsm_interrupt(signal);
	} else {
		bmi3xx_interrupt(signal);
	}
}

static void alt_sensor_init(void)
{
	base_use_alt_sensor = cros_cbi_ssfc_check_match(
		CBI_SSFC_VALUE_ID(DT_NODELABEL(base_sensor_lsm6dsm)));

	motion_sensors_check_ssfc();
}
DECLARE_HOOK(HOOK_INIT, alt_sensor_init, HOOK_PRIO_POST_I2C);
