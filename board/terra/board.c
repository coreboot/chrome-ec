/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* Terra board-specific configuration */

#include "adc.h"
#include "als.h"
#include "button.h"
#include "charger.h"
#include "charge_state.h"
#include "driver/accel_kxcj9.h"
#include "driver/als_isl29035.h"
#include "driver/gyro_l3gd20h.h"
#include "driver/temp_sensor/tmp432.h"
#include "extpower.h"
#include "gpio.h"
#include "hooks.h"
#include "host_command.h"
#include "i2c.h"
#include "lid_switch.h"
#include "math_util.h"
#include "motion_lid.h"
#include "motion_sense.h"
#include "power.h"
#include "power_button.h"
#include "pwm.h"
#include "pwm_chip.h"
#include "registers.h"
#include "temp_sensor.h"
#include "temp_sensor_chip.h"
#include "thermal.h"
#include "uart.h"
#include "util.h"

#define GPIO_KB_INPUT (GPIO_INPUT | GPIO_PULL_UP)
#define GPIO_KB_OUTPUT (GPIO_ODR_HIGH)
#define GPIO_KB_OUTPUT_COL2 (GPIO_OUT_LOW)

#include "gpio_list.h"

/* PWM channels. Must be in the exactly same order as in enum pwm_channel. */
const struct pwm_t pwm_channels[] = {
	{0, PWM_CONFIG_ACTIVE_LOW},
	{1, PWM_CONFIG_ACTIVE_LOW},
	{3, PWM_CONFIG_ACTIVE_LOW},
};

BUILD_ASSERT(ARRAY_SIZE(pwm_channels) == PWM_CH_COUNT);

/* power signal list.  Must match order of enum power_signal. */
const struct power_signal_info power_signal_list[] = {
	{GPIO_ALL_SYS_PGOOD,     1, "ALL_SYS_PWRGD"},
	{GPIO_RSMRST_L_PGOOD,    1, "RSMRST_N_PWRGD"},
	{GPIO_PCH_SLP_S3_L,      1, "SLP_S3#_DEASSERTED"},
	{GPIO_PCH_SLP_S4_L,      1, "SLP_S4#_DEASSERTED"},
};
BUILD_ASSERT(ARRAY_SIZE(power_signal_list) == POWER_SIGNAL_COUNT);

const struct i2c_port_t i2c_ports[]  = {
	{"batt_chg", MEC1322_I2C0_0, 100,
		GPIO_I2C_PORT0_0_SCL, GPIO_I2C_PORT0_0_SDA},
	{"thermal", MEC1322_I2C3, 100,
		GPIO_I2C_PORT3_SCL, GPIO_I2C_PORT3_SDA}
};
const unsigned int i2c_ports_used = ARRAY_SIZE(i2c_ports);

const enum gpio_signal hibernate_wake_pins[] = {
	GPIO_POWER_BUTTON_L,
};

const int hibernate_wake_pins_used = ARRAY_SIZE(hibernate_wake_pins);

/*
 * Temperature sensors data; must be in same order as enum temp_sensor_id.
 * Sensor index and name must match those present in coreboot:
 *     src/mainboard/google/${board}/acpi/dptf.asl
 */
const struct temp_sensor_t temp_sensors[] = {
	{"TMP432_Internal", TEMP_SENSOR_TYPE_BOARD, tmp432_get_val,
		TMP432_IDX_LOCAL, 4},
	{"TMP432_Sensor_1", TEMP_SENSOR_TYPE_BOARD, tmp432_get_val,
		TMP432_IDX_REMOTE1, 4},
	{"TMP432_Sensor_2", TEMP_SENSOR_TYPE_BOARD, tmp432_get_val,
		TMP432_IDX_REMOTE2, 4},
	{"Battery", TEMP_SENSOR_TYPE_BATTERY, charge_temp_sensor_get_val,
		0, 4},
};
BUILD_ASSERT(ARRAY_SIZE(temp_sensors) == TEMP_SENSOR_COUNT);

/* Thermal limits for each temp sensor. All temps are in degrees K. Must be in
 * same order as enum temp_sensor_id. To always ignore any temp, use 0.
 */
struct ec_thermal_config thermal_params[] = {
	{{0, 0, 0}, 0, 0}, /* TMP432_Internal */
	{{0, 0, 0}, 0, 0}, /* TMP432_Sensor_1 */
	{{0, 0, 0}, 0, 0}, /* TMP432_Sensor_2 */
	{{0, 0, 0}, 0, 0}, /* Battery Sensor */
};
BUILD_ASSERT(ARRAY_SIZE(thermal_params) == TEMP_SENSOR_COUNT);
