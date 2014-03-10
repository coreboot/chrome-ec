/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* EC for Glimmer board configuration */

#include "adc.h"
#include "adc_chip.h"
#include "backlight.h"
#include "button.h"
#include "charge_state.h"
#include "charger.h"
#include "common.h"
#include "console.h"
#include "driver/accel_kxcj9.h"
#include "driver/temp_sensor/tmp432.h"
#include "extpower.h"
#include "fan.h"
#include "gpio.h"
#include "host_command.h"
#include "i2c.h"
#include "jtag.h"
#include "keyboard_scan.h"
#include "lid_switch.h"
#include "math_util.h"
#include "motion_sense.h"
#include "peci.h"
#include "power.h"
#include "power_button.h"
#include "pwm.h"
#include "pwm_chip.h"
#include "registers.h"
#include "switch.h"
#include "temp_sensor.h"
#include "temp_sensor_chip.h"
#include "thermal.h"
#include "timer.h"
#include "uart.h"
#include "util.h"

#ifdef CONFIG_FAN_RPM_CUSTOM
struct fan_step {
	int lv0;
	int lv1;
	int lv2;
	int lv3;
};

static struct fan_step fan_table[2] = {
	{.lv0 = 0, .lv1 = 4000, .lv2 = 4350, .lv3 = 5150,} ,
	{.lv0 = 0, .lv1 = 3950, .lv2 = 4550, .lv3 = 5600,}
};
#endif  /* CONFIG_FAN_RPM_CUSTOM */

/* GPIO signal list.  Must match order from enum gpio_signal. */
const struct gpio_info gpio_list[] = {
	/* Inputs with interrupt handlers are first for efficiency */
	{"POWER_BUTTON_L",       LM4_GPIO_A, (1<<2), GPIO_INT_BOTH_DSLEEP,
	 power_button_interrupt},
	{"LID_OPEN",             LM4_GPIO_A, (1<<3), GPIO_INT_BOTH_DSLEEP,
	 lid_interrupt},
	{"AC_PRESENT",           LM4_GPIO_H, (1<<3), GPIO_INT_BOTH_DSLEEP,
	 extpower_interrupt},
	{"PCH_SLP_S3_L",         LM4_GPIO_G, (1<<7), GPIO_INT_BOTH_DSLEEP |
							GPIO_PULL_UP,
	 power_signal_interrupt},
	{"PCH_SLP_S4_L",         LM4_GPIO_H, (1<<1), GPIO_INT_BOTH_DSLEEP |
							GPIO_PULL_UP,
	 power_signal_interrupt},
	{"PP1050_PGOOD",         LM4_GPIO_H, (1<<4), GPIO_INT_BOTH,
	 power_signal_interrupt},
	{"PP3300_PCH_PGOOD",     LM4_GPIO_C, (1<<4), GPIO_INT_BOTH,
	 power_signal_interrupt},
	{"PP5000_PGOOD",         LM4_GPIO_N, (1<<0), GPIO_INT_BOTH,
	 power_signal_interrupt},
	{"S5_PGOOD",             LM4_GPIO_G, (1<<0), GPIO_INT_BOTH,
	 power_signal_interrupt},
	{"VCORE_PGOOD",          LM4_GPIO_C, (1<<6), GPIO_INT_BOTH,
	 power_signal_interrupt},
	{"WP_L",                 LM4_GPIO_A, (1<<4), GPIO_INT_BOTH,
	 switch_interrupt},
	{"JTAG_TCK",             LM4_GPIO_C, (1<<0), GPIO_DEFAULT,
	 jtag_interrupt},
	{"UART0_RX",             LM4_GPIO_A, (1<<0), GPIO_INT_BOTH_DSLEEP |
							GPIO_PULL_UP,
	 uart_deepsleep_interrupt},
	{"BUTTON_VOLUME_DOWN_L", LM4_GPIO_B, (1<<1), GPIO_INT_BOTH,
	 button_interrupt},
	{"BUTTON_VOLUME_UP_L",   LM4_GPIO_B, (1<<0), GPIO_INT_BOTH,
	 button_interrupt},
	{"ACCEL_INT_LID",        LM4_GPIO_F, (1<<2), GPIO_INT_RISING,
	 accel_int_lid},
	{"ACCEL_INT_BASE",       LM4_GPIO_N, (1<<5), GPIO_INT_RISING,
	 accel_int_base},

	/* Other inputs */
	{"BOARD_VERSION1",       LM4_GPIO_Q, (1<<5), GPIO_INPUT, NULL},
	{"BOARD_VERSION2",       LM4_GPIO_Q, (1<<6), GPIO_INPUT, NULL},
	{"BOARD_VERSION3",       LM4_GPIO_Q, (1<<7), GPIO_INPUT, NULL},
#ifdef CONFIG_CHIPSET_DEBUG
	{"PCH_SLP_SX_L",         LM4_GPIO_G, (1<<3), GPIO_INPUT|GPIO_PULL_UP,
	 NULL},
	{"PCH_SUS_STAT_L",       LM4_GPIO_G, (1<<6), GPIO_INPUT|GPIO_PULL_UP,
	 NULL},
	{"PCH_SUSPWRDNACK",      LM4_GPIO_G, (1<<2), GPIO_INPUT|GPIO_PULL_UP,
	 NULL},
#endif
	{"PP1000_S0IX_PGOOD",    LM4_GPIO_H, (1<<6), GPIO_INPUT, NULL},
	{"USB1_OC_L",            LM4_GPIO_E, (1<<7), GPIO_INPUT, NULL},
	{"USB2_OC_L",            LM4_GPIO_E, (1<<0), GPIO_INPUT, NULL},
	{"BAT_PRESENT_L",        LM4_GPIO_B, (1<<4), GPIO_INPUT, NULL},

	/* Outputs; all unasserted by default except for reset signals */
	{"CPU_PROCHOT",          LM4_GPIO_B, (1<<5), GPIO_OUT_LOW, NULL},
	{"ENABLE_BACKLIGHT",     LM4_GPIO_M, (1<<7), GPIO_ODR_HIGH, NULL},
	{"ENABLE_TOUCHPAD",      LM4_GPIO_N, (1<<1), GPIO_OUT_LOW, NULL},
	{"ENTERING_RW",          LM4_GPIO_D, (1<<6), GPIO_OUT_LOW, NULL},
	{"LPC_CLKRUN_L",         LM4_GPIO_M, (1<<2), GPIO_ODR_HIGH, NULL},
	{"PCH_CORE_PWROK",       LM4_GPIO_F, (1<<5), GPIO_OUT_LOW, NULL},
	{"PCH_PWRBTN_L",         LM4_GPIO_H, (1<<0), GPIO_ODR_HIGH, NULL},
	{"PCH_RCIN_L",           LM4_GPIO_F, (1<<3), GPIO_ODR_HIGH, NULL},
	{"PCH_RSMRST_L",         LM4_GPIO_F, (1<<1), GPIO_OUT_LOW, NULL},
	{"PCH_SMI_L",            LM4_GPIO_F, (1<<4), GPIO_ODR_HIGH, NULL},
	{"PCH_SOC_OVERRIDE",     LM4_GPIO_G, (1<<1), GPIO_OUT_LOW, NULL},
	{"PCH_SYS_PWROK",        LM4_GPIO_J, (1<<1), GPIO_OUT_LOW, NULL},
	{"PCH_WAKE_L",           LM4_GPIO_F, (1<<0), GPIO_ODR_HIGH, NULL},
	{"PP1350_EN",            LM4_GPIO_H, (1<<5), GPIO_OUT_LOW, NULL},
	{"PP3300_DX_EN",         LM4_GPIO_J, (1<<2), GPIO_OUT_LOW, NULL},
	{"PP3300_LTE_EN",        LM4_GPIO_D, (1<<4), GPIO_OUT_LOW, NULL},
	{"PP3300_WLAN_EN",       LM4_GPIO_J, (1<<0), GPIO_OUT_LOW, NULL},
	{"PP5000_EN",            LM4_GPIO_H, (1<<7), GPIO_OUT_LOW, NULL},
	{"PPSX_EN",              LM4_GPIO_L, (1<<6), GPIO_OUT_LOW, NULL},
	{"SUSP_VR_EN",           LM4_GPIO_C, (1<<7), GPIO_OUT_LOW, NULL},
	{"TOUCHSCREEN_RESET_L",  LM4_GPIO_N, (1<<7), GPIO_OUT_LOW, NULL},
	{"USB_CTL1",             LM4_GPIO_E, (1<<6), GPIO_OUT_LOW, NULL},
	{"USB_ILIM_SEL",         LM4_GPIO_E, (1<<5), GPIO_OUT_LOW, NULL},
	{"USB1_ENABLE",          LM4_GPIO_E, (1<<4), GPIO_OUT_LOW, NULL},
	{"USB2_ENABLE",          LM4_GPIO_D, (1<<5), GPIO_OUT_LOW, NULL},
	{"VCORE_EN",             LM4_GPIO_C, (1<<5), GPIO_OUT_LOW, NULL},
	{"WLAN_OFF_L",           LM4_GPIO_J, (1<<4), GPIO_OUT_LOW, NULL},
	{"PCH_SCI_L",            LM4_GPIO_M, (1<<1), GPIO_ODR_HIGH, NULL},
	{"KBD_IRQ_L",            LM4_GPIO_M, (1<<3), GPIO_ODR_HIGH, NULL},
	{"I2C2_SCL",             LM4_GPIO_F, (1<<6), GPIO_ODR_HIGH, NULL},
	{"I2C2_SDA",             LM4_GPIO_F, (1<<7), GPIO_ODR_HIGH, NULL},
};
BUILD_ASSERT(ARRAY_SIZE(gpio_list) == GPIO_COUNT);

/* Pins with alternate functions */
const struct gpio_alt_func gpio_alt_funcs[] = {
	{GPIO_A, 0x03, 1, MODULE_UART},			/* UART0 */
	{GPIO_B, 0x04, 3, MODULE_I2C},			/* I2C0 SCL */
	{GPIO_B, 0x08, 3, MODULE_I2C, GPIO_OPEN_DRAIN},	/* I2C0 SDA */
	{GPIO_B, 0x40, 3, MODULE_I2C},			/* I2C5 SCL */
	{GPIO_B, 0x80, 3, MODULE_I2C, GPIO_OPEN_DRAIN},	/* I2C5 SDA */
	{GPIO_F, 0x40, 3, MODULE_I2C},			/* I2C2 SCL */
	{GPIO_F, 0x80, 3, MODULE_I2C, GPIO_OPEN_DRAIN},	/* I2C2 SDA */
	{GPIO_D, 0x0f, 2, MODULE_SPI},			/* SPI1 */
	{GPIO_L, 0x3f, 15, MODULE_LPC},			/* LPC */
	{GPIO_M, 0x21, 15, MODULE_LPC},			/* LPC */
	{GPIO_M, 0x40, 1, MODULE_PWM_LED},		/* FAN0PWM0 */
	{GPIO_N, 0x0c, 1, MODULE_PWM_FAN},		/* FAN0PWM2 */
};
const int gpio_alt_funcs_count = ARRAY_SIZE(gpio_alt_funcs);

/* power signal list.  Must match order of enum power_signal. */
const struct power_signal_info power_signal_list[] = {
	{GPIO_PP1050_PGOOD,      1, "PGOOD_PP1050"},
	{GPIO_PP3300_PCH_PGOOD,  1, "PGOOD_PP3300_PCH"},
	{GPIO_PP5000_PGOOD,      1, "PGOOD_PP5000"},
	{GPIO_S5_PGOOD,          1, "PGOOD_S5"},
	{GPIO_VCORE_PGOOD,       1, "PGOOD_VCORE"},
	{GPIO_PP1000_S0IX_PGOOD, 1, "PGOOD_PP1000_S0IX"},
	{GPIO_PCH_SLP_S3_L,      1, "SLP_S3#_DEASSERTED"},
	{GPIO_PCH_SLP_S4_L,      1, "SLP_S4#_DEASSERTED"},
#ifdef CONFIG_CHIPSET_DEBUG
	{GPIO_PCH_SLP_SX_L,      1, "SLP_SX#_DEASSERTED"},
	{GPIO_PCH_SUS_STAT_L,    0, "SUS_STAT#_ASSERTED"},
	{GPIO_PCH_SUSPWRDNACK,   1, "SUSPWRDNACK_ASSERTED"},
#endif
};
BUILD_ASSERT(ARRAY_SIZE(power_signal_list) == POWER_SIGNAL_COUNT);

/* ADC channels. Must be in the exactly same order as in enum adc_channel. */
const struct adc_t adc_channels[] = {
	/* EC internal temperature is calculated by
	 * 273 + (295 - 450 * ADC_VALUE / ADC_READ_MAX) / 2
	 * = -225 * ADC_VALUE / ADC_READ_MAX + 420.5
	 */
	{"ECTemp", LM4_ADC_SEQ0, -225, ADC_READ_MAX, 420,
	 LM4_AIN_NONE, 0x0e /* TS0 | IE0 | END0 */, 0, 0},

	/* IOUT == ICMNT is on PE3/AIN0 */
	/* We have 0.01-ohm resistors, and IOUT is 40X the differential
	 * voltage, so 1000mA ==> 400mV.
	 * ADC returns 0x000-0xFFF, which maps to 0.0-3.3V (as configured).
	 * mA = 1000 * ADC_VALUE / ADC_READ_MAX * 3300 / 400
	 */
	{"ChargerCurrent", LM4_ADC_SEQ1, 33000, ADC_READ_MAX * 4, 0,
	 LM4_AIN(0), 0x06 /* IE0 | END0 */, LM4_GPIO_E, (1<<3)},
};
BUILD_ASSERT(ARRAY_SIZE(adc_channels) == ADC_CH_COUNT);

/* PWM channels. Must be in the exactly same order as in enum pwm_channel. */
const struct pwm_t pwm_channels[] = {
	{0, PWM_CONFIG_ACTIVE_LOW},
};

BUILD_ASSERT(ARRAY_SIZE(pwm_channels) == PWM_CH_COUNT);

/* Physical fans. These are logically separate from pwm_channels. */
const struct fan_t fans[] = {
	{.flags = FAN_USE_RPM_MODE,
	 .rpm_min = 3680,
	 .rpm_max = 4600,
	 .ch = 2,
	 .pgood_gpio = GPIO_PP5000_PGOOD,
	 .enable_gpio = -1,
	},
};
BUILD_ASSERT(ARRAY_SIZE(fans) == CONFIG_FANS);

/* I2C ports */
/* Ports that define an SCL and SDA pin will automatically unwedge. */
const struct i2c_port_t i2c_ports[] = {
	{"batt_chg", 0, 100},
	{"accel", 2, 400, GPIO_I2C2_SCL, GPIO_I2C2_SDA},
	{"thermal",  5, 100},
};
const unsigned int i2c_ports_used = ARRAY_SIZE(i2c_ports);

/*
 * Temperature sensors data; must be in same order as enum temp_sensor_id.
 * Sensor index and name must match those present in coreboot:
 *     src/mainboard/google/${board}/acpi/dptf.asl
 */
const struct temp_sensor_t temp_sensors[] = {
	{"ECInternal", TEMP_SENSOR_TYPE_BOARD, chip_temp_sensor_get_val, 0, 4},
	{"TMP432_Power_bottom", TEMP_SENSOR_TYPE_BOARD, tmp432_get_val,
		TMP432_IDX_LOCAL, 4},
	{"TMP432_RAM_bottom", TEMP_SENSOR_TYPE_BOARD, tmp432_get_val,
		TMP432_IDX_REMOTE1, 4},
	{"TMP432_CPU_bottom", TEMP_SENSOR_TYPE_BOARD, tmp432_get_val,
		TMP432_IDX_REMOTE2, 4},
	{"Battery", TEMP_SENSOR_TYPE_BATTERY, charge_temp_sensor_get_val, 0, 4},
};
BUILD_ASSERT(ARRAY_SIZE(temp_sensors) == TEMP_SENSOR_COUNT);

/* Thermal limits for each temp sensor. All temps are in degrees K. Must be in
 * same order as enum temp_sensor_id. To always ignore any temp, use 0.
 */
struct ec_thermal_config thermal_params[] = {
	{{0, 0, 0}, 0, 0},
	{{0, 0, 0}, 0, 0},
	{{0, 0, 0}, 0, 0},
	{{0, 0, C_TO_K(70)}, C_TO_K(20), C_TO_K(70)},
	{{0, 0, 0}, 0, 0},
};
BUILD_ASSERT(ARRAY_SIZE(thermal_params) == TEMP_SENSOR_COUNT);

const struct button_config buttons[] = {
	{"Volume Down", KEYBOARD_BUTTON_VOLUME_DOWN, GPIO_BUTTON_VOLUME_DOWN_L,
	30 * MSEC, 0},
	{"Volume Up", KEYBOARD_BUTTON_VOLUME_UP, GPIO_BUTTON_VOLUME_UP_L,
	30 * MSEC, 0},
};
BUILD_ASSERT(ARRAY_SIZE(buttons) == CONFIG_BUTTON_COUNT);

const int accel_addr[] = {
	KXCJ9_ADDR0,	/* ACCEL_LID */
	KXCJ9_ADDR1	/* ACCEL_BASE */
};
BUILD_ASSERT(ARRAY_SIZE(accel_addr) == ACCEL_COUNT);

/**
 * Discharge battery when on AC power for factory test.
 */
int board_discharge_on_ac(int enable)
{
	return charger_discharge_on_ac(enable);
}

#ifdef HAS_TASK_MOTIONSENSE

#ifndef CONFIG_ACCEL_CALIBRATE
const
#endif
struct accel_orientation acc_orient = {
	.rot_align = {
		{ 0,  1,  0},
		{-1,  0,  0},
		{ 0,  0,  1}
	},
	.rot_hinge_90 = {
		{ 1,  0,  0},
		{ 0,  0,  1},
		{ 0, -1,  0}
	},
	.rot_hinge_180 = {
		{ 1,  0,  0},
		{ 0, -1,  0},
		{ 0,  0, -1}
	},
	.rot_standard_ref = {
		{ 0,  1,  0},
		{-1,  0,  0},
		{ 0,  0, -1}
	},
	.hinge_axis = {1, 0, 0},
};
#endif /* HAS_TASK_MOTIONSENSE */

#ifdef CONFIG_FAN_RPM_CUSTOM
int fan_percent_to_rpm(int fan, int pct)
{
	int id;

	int current_rpm_target = fan_get_rpm_target(fans[fan].ch);
	int new_rpm_target = -1;

	if (gpio_get_level(GPIO_BOARD_VERSION3))
		id = 1;
	else
		id = 0;

	if (pct < 30)
		new_rpm_target = fan_table[id].lv0;
	else if (pct < 38)
		new_rpm_target = (current_rpm_target > fan_table[id].lv0) ?
				  fan_table[id].lv1 : fan_table[id].lv0;
	else if (pct < 44)
		new_rpm_target = current_rpm_target;
	else if (pct < 56)
		new_rpm_target = (current_rpm_target > fan_table[id].lv0) ?
				  current_rpm_target : fan_table[id].lv1;
	else if (pct < 66)
		new_rpm_target = fan_table[id].lv2;
	else if (pct < 86)
		new_rpm_target = (current_rpm_target > fan_table[id].lv0) ?
				  current_rpm_target : fan_table[id].lv2;
	else
		new_rpm_target = fan_table[id].lv3;

	if (new_rpm_target != current_rpm_target)
		cprintf(CC_THERMAL, "[%T Setting fan RPM to %d]\n",
			new_rpm_target);

	return new_rpm_target;
}
#endif  /* CONFIG_FAN_RPM_CUSTOM */
