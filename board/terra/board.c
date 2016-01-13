/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* Terra board-specific configuration */

#include "charge_state.h"
#include "driver/charger/bq24773.h"
#include "driver/temp_sensor/tmp432.h"
#include "extpower.h"
#include "i2c.h"
#include "lid_switch.h"
#include "power.h"
#include "power_button.h"
#include "temp_sensor.h"
#include "thermal.h"
#include "uart.h"
#include "util.h"

#define GPIO_KB_INPUT (GPIO_INPUT | GPIO_PULL_UP)
#define GPIO_KB_OUTPUT (GPIO_ODR_HIGH)
#define GPIO_KB_OUTPUT_COL2 (GPIO_OUT_LOW)

#include "gpio_list.h"

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
	GPIO_AC_PRESENT,
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

uint32_t board_get_gpio_hibernate_state(uint32_t port, uint32_t pin)
{
	int i;
	const uint32_t out_low_gpios[][2] = {
		GPIO_TO_PORT_MASK_PAIR(GPIO_POWER_LED),
		GPIO_TO_PORT_MASK_PAIR(GPIO_SMC_SHUTDOWN),
	};

	/* Some GPIOs should be driven low in hibernate */
	for (i = 0; i < ARRAY_SIZE(out_low_gpios); ++i) {
		if (out_low_gpios[i][0] == port && out_low_gpios[i][1] == pin)
			return GPIO_OUTPUT | GPIO_LOW;
	}

	/* Other GPIOs should be put in a low-power state */
	return GPIO_INPUT | GPIO_PULL_UP;
}

int board_charger_post_init(void)
{
	int ret;

	ret = raw_write16(REG_CHARGE_OPTION0, 0x014f);
	if (ret)
		return ret;

	ret = raw_write16(REG_CHARGE_OPTION1, 0x0211);
	if (ret)
		return ret;

	ret = raw_write16(REG_CHARGE_OPTION2, 0x0000);
	if (ret)
		return ret;

	ret = raw_write16(REG_PROCHOT_OPTION0, 0x4b4e);
	if (ret)
		return ret;

	return raw_write16(REG_PROCHOT_OPTION1, 0x813C);
}

int board_get_version(void)
{
	int v = 0;

	if (gpio_get_level(GPIO_BOARD_VERSION1))
		v |= 0x01;
	if (gpio_get_level(GPIO_BOARD_VERSION2))
		v |= 0x02;
	if (gpio_get_level(GPIO_BOARD_VERSION3))
		v |= 0x04;
	if (gpio_get_level(GPIO_IMAGE_SEL))
		v |= 0x08;

	return v;
}
