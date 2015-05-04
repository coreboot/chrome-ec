/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* Veyron board-specific configuration */

#include "battery.h"
#include "chipset.h"
#include "common.h"
#include "driver/accel_kx022.h"
#include "extpower.h"
#include "gpio.h"
#include "i2c.h"
#include "keyboard_raw.h"
#include "lid_switch.h"
#include "math_util.h"
#include "motion_lid.h"
#include "motion_sense.h"
#include "power.h"
#include "power_button.h"
#include "power.h"
#include "pwm.h"
#include "pwm_chip.h"
#include "registers.h"
#include "spi.h"
#include "task.h"
#include "util.h"
#include "timer.h"
#include "charger.h"

#define GPIO_KB_INPUT  (GPIO_INPUT | GPIO_PULL_UP | GPIO_INT_BOTH)
#define GPIO_KB_OUTPUT GPIO_ODR_HIGH

#include "gpio_list.h"


/* power signal list.  Must match order of enum power_signal. */
const struct power_signal_info power_signal_list[] = {
	{GPIO_SOC_POWER_GOOD, 1, "POWER_GOOD"},
	{GPIO_SUSPEND_L,      1, "SUSPEND#_ASSERTED"},
};
BUILD_ASSERT(ARRAY_SIZE(power_signal_list) == POWER_SIGNAL_COUNT);

/* I2C ports */
const struct i2c_port_t i2c_ports[] = {
	{"master", I2C_PORT_MASTER, 100},
};
const unsigned int i2c_ports_used = ARRAY_SIZE(i2c_ports);

/* PWM channels. Must be in the exactly same order as in enum pwm_channel. */
const struct pwm_t pwm_channels[] = {
	{STM32_TIM(2), STM32_TIM_CH(3),
	 PWM_CONFIG_ACTIVE_LOW},
};
BUILD_ASSERT(ARRAY_SIZE(pwm_channels) == PWM_CH_COUNT);

/**
 * Discharge battery when on AC power for factory test.
 */
int board_discharge_on_ac(int enable)
{
	return charger_discharge_on_ac(enable);
}

void board_config_pre_init(void)
{
	/* enable SYSCFG clock */
	STM32_RCC_APB2ENR |= 1 << 0;

	/* Remap USART DMA to match the USART driver */
	/*
	 * the DMA mapping is :
	 *  Chan 2 : TIM1_CH1
	 *  Chan 3 : SPI1_TX
	 *  Chan 4 : USART1_TX
	 *  Chan 5 : USART1_RX
	 */
	STM32_SYSCFG_CFGR1 |= (1 << 9) | (1 << 10); /* Remap USART1 RX/TX DMA */
}

/* Base Sensor mutex */
static struct mutex g_base_mutex;

/* Lid Sensor mutex */
static struct mutex g_lid_mutex;

/* kxcj9 local/private data */
struct kx022_data g_kx022_data0;

struct kx022_data g_kx022_data1;

/* Four Motion sensors */
/* Matrix to rotate accelrator into standard reference frame */
const matrix_3x3_t base_standard_ref = {
	{ 0, FLOAT_TO_FP(-1), 0},
	{ FLOAT_TO_FP(1), 0,  0},
	{ 0, 0, FLOAT_TO_FP(-1)}
};

const matrix_3x3_t lid_standard_ref = {
	{ 0, FLOAT_TO_FP(1), 0},
	{ FLOAT_TO_FP(1), 0, 0},
	{ 0, 0, FLOAT_TO_FP(1)}
};

struct motion_sensor_t motion_sensors[] = {

	/*
	 * Note: lsm6ds0: supports accelerometer and gyro sensor
	 * Requriement: accelerometer sensor must init before gyro sensor
	 * DO NOT change the order of the following table.
	 */
	{SENSOR_ACTIVE_S0_S3, "Base", MOTIONSENSE_CHIP_KX022,
		MOTIONSENSE_TYPE_ACCEL, MOTIONSENSE_LOC_BASE,
		&kx022_drv, &g_base_mutex, &g_kx022_data0,
		KX022_ADDR1, &base_standard_ref, 119000, 2},

	{SENSOR_ACTIVE_S0_S3, "Lid",  MOTIONSENSE_CHIP_KX022,
		MOTIONSENSE_TYPE_ACCEL, MOTIONSENSE_LOC_LID,
		&kx022_drv, &g_lid_mutex, &g_kx022_data1,
		KX022_ADDR0, &lid_standard_ref, 100000, 2},
};
const unsigned int motion_sensor_count = ARRAY_SIZE(motion_sensors);

/* Define the accelerometer orientation matrices. */
const struct accel_orientation acc_orient = {
	/* Hinge aligns with y axis. */
	.rot_hinge_90 = {
		{  0, 0, FLOAT_TO_FP(1)},
		{  0, FLOAT_TO_FP(1), 0},
		{ FLOAT_TO_FP(-1), 0, 0}
	},
	.rot_hinge_180 = {
		{ FLOAT_TO_FP(-1), 0, 0},
		{  0, FLOAT_TO_FP(1), 0},
		{  0, 0, FLOAT_TO_FP(-1)}
	},
	.hinge_axis = {0, 1, 0},
};
