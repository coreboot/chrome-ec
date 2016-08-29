/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* Veyron board-specific configuration */

#include "battery.h"
#include "charger.h"
#include "chipset.h"
#include "common.h"
#include "driver/accel_kionix.h"
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

/* kxcj9 mutex and local/private data*/
static struct mutex g_kx022_mutex[2];
struct kionix_accel_data g_kx022_data[2];

/* For Motion sensors */
/* Matrix to rotate accelrator into standard reference frame */
const matrix_3x3_t base_standard_ref = {
	{ FLOAT_TO_FP(-1), 0,  0},
	{ 0, FLOAT_TO_FP(1), 0},
	{ 0, 0, FLOAT_TO_FP(-1)}
};

/* Matrix to rotate accelrator into standard reference frame */
const matrix_3x3_t lid_standard_ref = {
	{ FLOAT_TO_FP(1), 0,  0},
	{ 0, FLOAT_TO_FP(-1), 0},
	{ 0, 0, FLOAT_TO_FP(-1)}
};

struct motion_sensor_t motion_sensors[] = {
	[BASE_ACCEL] = {
	 .name = "Base",
	 .active_mask = SENSOR_ACTIVE_S0_S3,
	 .chip = MOTIONSENSE_CHIP_KX022,
	 .type = MOTIONSENSE_TYPE_ACCEL,
	 .location = MOTIONSENSE_LOC_BASE,
	 .drv = &kionix_accel_drv,
	 .mutex = &g_kx022_mutex[0],
	 .drv_data = &g_kx022_data[0],
	 .addr = KX022_ADDR1,
	 .rot_standard_ref = &base_standard_ref,
	 .default_range = 8,  /* g, use hifi requirements */
	 .config = {
		 /* AP: by default shutdown all sensors */
		 [SENSOR_CONFIG_AP] = {
			 .odr = 0,
			 .ec_rate = 0,
		 },
		 /* EC use accel for angle detection */
		 [SENSOR_CONFIG_EC_S0] = {
			 .odr = 10000 | ROUND_UP_FLAG,
			 .ec_rate = 0,
		 },
		 /* Sensor off in S3/S5 */
		 [SENSOR_CONFIG_EC_S3] = {
			 .odr = 0,
			 .ec_rate = 0
		 },
		 /* Sensor off in S3/S5 */
		 [SENSOR_CONFIG_EC_S5] = {
			 .odr = 0,
			 .ec_rate = 0
		 },
	 }
	},
	[LID_ACCEL] = {
	 .name = "Lid",
	 .active_mask = SENSOR_ACTIVE_S0_S3,
	 .chip = MOTIONSENSE_CHIP_KX022,
	 .type = MOTIONSENSE_TYPE_ACCEL,
	 .location = MOTIONSENSE_LOC_LID,
	 .drv = &kionix_accel_drv,
	 .mutex = &g_kx022_mutex[1],
	 .drv_data = &g_kx022_data[1],
	 .addr = KX022_ADDR0,
	 .rot_standard_ref = &lid_standard_ref,
	 .default_range = 8,  /* g, enough for laptop. */
	 .config = {
		 /* AP: by default shutdown all sensors */
		 [SENSOR_CONFIG_AP] = {
			 .odr = 0,
			 .ec_rate = 0,
		 },
		 /* EC use accel for angle detection */
		 [SENSOR_CONFIG_EC_S0] = {
			 .odr = 10000 | ROUND_UP_FLAG,
			 .ec_rate = 0,
		 },
		 /* Sensor off in S3/S5 */
		 [SENSOR_CONFIG_EC_S3] = {
			 .odr = 0,
			 .ec_rate = 0
		 },
		 /* Sensor off in S3/S5 */
		 [SENSOR_CONFIG_EC_S5] = {
			 .odr = 0,
			 .ec_rate = 0
		 },
	 },
	},
};
const unsigned int motion_sensor_count = ARRAY_SIZE(motion_sensors);
