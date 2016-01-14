/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* Terra board-specific configuration */

#include "adc.h"
#include "adc_chip.h"
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

const struct adc_t adc_channels[] = {
	[ADC_TEMP] = {"ADC1", 1, 1, 0, 0},
};
BUILD_ASSERT(ARRAY_SIZE(adc_channels) == ADC_CH_COUNT);

/* Forward declaration */
static int terra_get_temp_sensor2(int index, int *temp_ptr);

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
	{"TMP432_Sensor_2", TEMP_SENSOR_TYPE_BOARD, terra_get_temp_sensor2,
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

struct adc_temp_t {
	int adc;
	int temp;
};

const struct adc_temp_t adc_temp[] = {
	{ 65, 125 },
	{ 66, 124 },
	{ 68, 123 },
	{ 69, 122 },
	{ 71, 121 },
	{ 73, 120 },
	{ 75, 119 },
	{ 77, 118 },
	{ 79, 117 },
	{ 81, 116 },
	{ 83, 115 },
	{ 85, 114 },
	{ 87, 113 },
	{ 89, 112 },
	{ 92, 111 },
	{ 94, 110 },
	{ 96, 109 },
	{ 99, 108 },
	{ 102, 107 },
	{ 104, 106 },
	{ 107, 105 },
	{ 110, 104 },
	{ 113, 103 },
	{ 116, 102 },
	{ 119, 101 },
	{ 122, 100 },
	{ 125, 99 },
	{ 129, 98 },
	{ 132, 97 },
	{ 136, 96 },
	{ 139, 95 },
	{ 143, 94 },
	{ 147, 93 },
	{ 151, 92 },
	{ 155, 91 },
	{ 159, 90 },
	{ 163, 89 },
	{ 168, 88 },
	{ 172, 87 },
	{ 177, 86 },
	{ 182, 85 },
	{ 187, 84 },
	{ 192, 83 },
	{ 197, 82 },
	{ 202, 81 },
	{ 208, 80 },
	{ 213, 79 },
	{ 219, 78 },
	{ 225, 77 },
	{ 231, 76 },
	{ 237, 75 },
	{ 244, 74 },
	{ 250, 73 },
	{ 257, 72 },
	{ 264, 71 },
	{ 271, 70 },
	{ 278, 69 },
	{ 285, 68 },
	{ 293, 67 },
	{ 301, 66 },
	{ 309, 65 },
	{ 317, 64 },
	{ 325, 63 },
	{ 333, 62 },
	{ 342, 61 },
	{ 351, 60 },
	{ 360, 59 },
	{ 369, 58 },
	{ 378, 57 },
	{ 388, 56 },
	{ 397, 55 },
	{ 407, 54 },
	{ 417, 53 },
	{ 427, 52 },
	{ 437, 51 },
	{ 448, 50 },
	{ 458, 49 },
	{ 469, 48 },
	{ 480, 47 },
	{ 491, 46 },
	{ 502, 45 },
	{ 513, 44 },
	{ 525, 43 },
	{ 536, 42 },
	{ 548, 41 },
	{ 559, 40 },
	{ 571, 39 },
	{ 583, 38 },
	{ 594, 37 },
	{ 606, 36 },
	{ 618, 35 },
	{ 630, 34 },
	{ 642, 33 },
	{ 654, 32 },
	{ 666, 31 },
	{ 678, 30 },
	{ 690, 29 },
	{ 701, 28 },
	{ 713, 27 },
	{ 725, 26 },
	{ 736, 25 },
	{ 748, 24 },
	{ 759, 23 },
	{ 770, 22 },
	{ 782, 21 },
	{ 793, 20 },
	{ 803, 19 },
	{ 814, 18 },
	{ 825, 17 },
	{ 835, 16 },
	{ 845, 15 },
	{ 855, 14 },
	{ 865, 13 },
	{ 875, 12 },
	{ 884, 11 },
	{ 893, 10 },
	{ 902, 9 },
	{ 911, 8 },
	{ 920, 7 },
	{ 928, 6 },
	{ 936, 5 },
	{ 944, 4 },
	{ 952, 3 },
	{ 959, 2 },
	{ 967, 1 },
	{ 974, 0 },
	{ 981, -1 },
	{ 987, -2 },
	{ 993, -3 },
	{ 1000, -4 },
	{ 1006, -5 },
	{ 1011, -6 },
	{ 1017, -7 },
	{ 1022, -8 },
	{ 1027, -9 },
	{ 1032, -10 },
	{ 1037, -11 },
	{ 1042, -12 },
	{ 1046, -13 },
	{ 1050, -14 },
	{ 1054, -15 },
	{ 1058, -16 },
	{ 1062, -17 },
	{ 1065, -18 },
	{ 1069, -19 },
	{ 1072, -20 },
	{ 1075, -21 },
	{ 1078, -22 },
	{ 1081, -23 },
	{ 1083, -24 },
	{ 1086, -25 },
	{ 1088, -26 },
	{ 1090, -27 },
	{ 1093, -28 },
	{ 1095, -29 },
	{ 1096, -30 },
	{ 1098, -31 },
	{ 1100, -32 },
	{ 1102, -33 },
	{ 1103, -34 },
	{ 1105, -35 },
	{ 1106, -36 },
	{ 1107, -37 },
	{ 1109, -38 },
	{ 1110, -39 },
	{ 1111, -40 },
};

static int adc_to_temp(int adc)
{
	static int i;

	if (adc < adc_temp[0].adc)
		return 125;
	else if (adc > adc_temp[ARRAY_SIZE(adc_temp)-1].adc)
		return -40;

	while (1) {
		if (adc < adc_temp[i].adc)
			--i;
		else if (adc > adc_temp[i+1].adc)
			++i;
		else
			return adc_temp[i].temp;
	}
}

static int terra_adc_get_val(int idx, int *temp_ptr)
{
	*temp_ptr = C_TO_K(adc_to_temp(adc_read_channel(idx)));
	return EC_SUCCESS;
}

static int terra_get_temp_sensor2(int idx, int *temp_ptr)
{
	if (gpio_get_level(GPIO_IMAGE_SEL))
		return tmp432_get_val(idx, temp_ptr);
	else
		return terra_adc_get_val(ADC_TEMP, temp_ptr);
}

