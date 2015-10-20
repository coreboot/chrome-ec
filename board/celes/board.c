/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* Celes board-specific configuration */

#include "adc.h"
#include "adc_chip.h"
#include "als.h"
#include "button.h"
#include "charger.h"
#include "charge_state.h"
#include "driver/charger/bq24773.h"
#include "driver/temp_sensor/ec_adc.h"
#include "driver/temp_sensor/tmp432.h"
#include "extpower.h"
#include "gpio.h"
#include "hooks.h"
#include "host_command.h"
#include "i2c.h"
#include "lid_switch.h"
#include "math_util.h"
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
#ifdef CONFIG_KEYBOARD_COL2_INVERTED
#define GPIO_KB_OUTPUT_COL2 (GPIO_OUT_LOW)
#endif

#include "gpio_list.h"

/* PWM channels. Must be in the exactly same order as in enum pwm_channel. */
const struct pwm_t pwm_channels[] = {
	{0, PWM_CONFIG_ACTIVE_LOW},
	{1, PWM_CONFIG_ACTIVE_LOW},
	{3, PWM_CONFIG_ACTIVE_LOW},
};

BUILD_ASSERT(ARRAY_SIZE(pwm_channels) == PWM_CH_COUNT);

/* ADC channels */
const struct adc_t adc_channels[] = {
       /* NAME, MUL, DIV, SHIFT, CHANNEL */
       [ADC_CH_CPU_TEMP] = {"ADC_NCP15WB_CPU", 1, 1, 0, 0},
       [ADC_CH_DIMM_TEMP] = {"ADC_NCP15WB_DIMM", 1, 1, 0, 1},
       [ADC_CH_PMIC_TEMP] = {"ADC_NCP15WB_PMIC", 1, 1, 0, 4},
};

BUILD_ASSERT(ARRAY_SIZE(adc_channels) == ADC_CH_COUNT);

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
	{"muxes", MEC1322_I2C0_1, 100,
		GPIO_I2C_PORT0_1_SCL, GPIO_I2C_PORT0_1_SDA},
	{"pd_mcu", MEC1322_I2C1, 1000,
		GPIO_I2C_PORT1_SCL, GPIO_I2C_PORT1_SDA},
	{"sensors", MEC1322_I2C2, 100,
		GPIO_I2C_PORT2_SCL, GPIO_I2C_PORT2_SDA},
	{"thermal",  MEC1322_I2C3,   100,
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
	{"NCP15WB_CPU", TEMP_SENSOR_TYPE_BOARD, ec_adc_get_val,
		ADC_CH_CPU_TEMP, 4},
	{"NCP15WB_DIMM", TEMP_SENSOR_TYPE_BOARD, ec_adc_get_val,
		ADC_CH_DIMM_TEMP, 4},
	{"NCP15WB_PMIC", TEMP_SENSOR_TYPE_BOARD, ec_adc_get_val,
		ADC_CH_PMIC_TEMP, 4},
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
	{{0, 0, 0}, 0, 0}, /* NCP15WB_CPU */
	{{0, 0, 0}, 0, 0}, /* NCP15WB_DIMM */
	{{0, 0, 0}, 0, 0}, /* NCP15WB_PMIC */
	{{0, 0, 0}, 0, 0}, /* Battery Sensor */
};
BUILD_ASSERT(ARRAY_SIZE(thermal_params) == TEMP_SENSOR_COUNT);

/* init ADC ports to avoid floating state due to thermistors */
static void adc_pre_init(void)
{
       /* Configure GPIOs */
	gpio_config_module(MODULE_ADC, 1);
}
DECLARE_HOOK(HOOK_INIT, adc_pre_init, HOOK_PRIO_INIT_ADC - 1);
