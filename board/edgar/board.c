/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* Edgar board-specific configuration */

#include "adc.h"
#include "als.h"
#include "button.h"
#include "charger.h"
#include "charge_state.h"
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
#include "adc_chip.h"

#define GPIO_KB_INPUT (GPIO_INPUT | GPIO_PULL_UP)
#define GPIO_KB_OUTPUT (GPIO_ODR_HIGH)
#define GPIO_KB_OUTPUT_COL2 (GPIO_OUT_LOW)

#include "gpio_list.h"

static int adc_get_val(int idx, int *temp_ptr);

/* power signal list.  Must match order of enum power_signal. */
const struct power_signal_info power_signal_list[] = {
	{GPIO_ALL_SYS_PGOOD,     1, "ALL_SYS_PWRGD"},
	{GPIO_RSMRST_L_PGOOD,    1, "RSMRST_N_PWRGD"},
	{GPIO_PCH_SLP_S3_L,      1, "SLP_S3#_DEASSERTED"},
	{GPIO_PCH_SLP_S4_L,      1, "SLP_S4#_DEASSERTED"},
};
BUILD_ASSERT(ARRAY_SIZE(power_signal_list) == POWER_SIGNAL_COUNT);

const struct adc_t adc_channels[] = {
	{"ADC0", 1, 1, 0, 0},
};
BUILD_ASSERT(ARRAY_SIZE(adc_channels) == ADC_CH_COUNT);

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
	{"TMP432_Sensor_2", TEMP_SENSOR_TYPE_BOARD, tmp432_get_val,
		TMP432_IDX_REMOTE2, 4},
	{"ADC_Sensor_1", TEMP_SENSOR_TYPE_BOARD, adc_get_val,
		ADC_CH_0, 4},
	{"Battery", TEMP_SENSOR_TYPE_BATTERY, charge_temp_sensor_get_val,
		0, 4},
};
BUILD_ASSERT(ARRAY_SIZE(temp_sensors) == TEMP_SENSOR_COUNT);

/* Thermal limits for each temp sensor. All temps are in degrees K. Must be in
 * same order as enum temp_sensor_id. To always ignore any temp, use 0.
 */
struct ec_thermal_config thermal_params[] = {
	{{0, 0, 0}, 0, 0}, /* TMP432_Internal */
	{{0, 0, 0}, 0, 0}, /* TMP432_Sensor_2 */
	{{0, 0, 0}, 0, 0}, /* ADC_Sensor_1 */
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

static const int adc_temp[] = {
	158780, 150690, 143060, 135870, 129070, 122660, 116600, 110870,
	105450, 100330,  95488,  90904,  86565,  82456,  78565,  74878,
	 71384,  68071,  64930,  61951,  59124,  56442,  53895,  51478,
	 49181,  47000,  44927,  42957,  41084,  39303,  37609,  35997,
	 34463,  33004,  31614,  30290,  29029,  27828,  26683,  25592,
	 24551,  23559,  22613,  21710,  20848,  20025,  19239,  18489,
	 17772,  17087,  16432,  15807,  15208,  14636,  14088,  13564,
	 13063,  12582,  12123,  11682,  11260,  10856,  10468,  10097,
	  9704,   9398,   9070,   8756,   8454,   8164,   7885,   7618,
	  7361,   7114,   6876,   6648,   6429,   6217,   6014,   5819,
	  5631,   5450,   5276,   5108,   4947,   4791,   4641,   4496,
	  4357,   4223,   4093,   3968,   3848,   3732,   3620,   3511,
	  3407,   3306,   3209,   3115,   3024,   2936,   2851,   2769,
};

static int adc_get_val(int idx, int *temp_ptr)
{
	int head, tail, mid;
	int voltage_value = adc_read_channel(idx);
	int ohm_value = (voltage_value == 1023)
		? 158780 : (24900 * voltage_value) / (1023 - voltage_value);

	/* Binary search to find proper table entry */
	head = 0;
	tail = ARRAY_SIZE(adc_temp)-1;
	while (head != tail) {
		mid = (head + tail) / 2;
		if (adc_temp[mid] >= ohm_value &&
		    adc_temp[mid+1] < ohm_value)
			break;
		if (adc_temp[mid] > ohm_value)
			head = mid + 1;
		else
			tail = mid;
	}

	/* Offset 5 dergee */
	*temp_ptr = C_TO_K(mid) + 5;
	return EC_SUCCESS;
}
static void THM_tmp432(void)
{
	i2c_write8(4, 0x98, 0x1A, 0x48);
	i2c_write8(4, 0x98, 0x20, 0x48);
}
DECLARE_HOOK(HOOK_INIT, THM_tmp432, HOOK_PRIO_DEFAULT);
DECLARE_HOOK(HOOK_CHIPSET_STARUP, THM_tmp432, HOOK_PRIO_DEFAULT);
DECLARE_HOOK(HOOK_CHIPSET_SUSPEND, THM_tmp432, HOOK_PRIO_DEFAULT);
DECLARE_HOOK(HOOK_CHIPSET_SHUTDOWN, THM_tmp432, HOOK_PRIO_DEFAULT);
