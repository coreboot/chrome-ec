/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Eve board configuration */

#ifndef __CROS_EC_BOARD_H
#define __CROS_EC_BOARD_H

#include "baseboard.h"

/* EC console commands */
#define CONFIG_CMD_ACCELS
#define CONFIG_CMD_ACCEL_INFO
#define CONFIG_CMD_BUTTON


/* Sensor */
#define CONFIG_TEMP_SENSOR
#define CONFIG_TEMP_SENSOR_F75303

#define CONFIG_ACCELGYRO_BMI160
#define CONFIG_ACCELGYRO_BMI160_INT_EVENT TASK_EVENT_CUSTOM(4)
#define CONFIG_ACCELGYRO_BMI160_INT2_OUTPUT
#define CONFIG_ACCEL_BMA255
#define CONFIG_ACCEL_KX022
#define CONFIG_ACCEL_INTERRUPTS

#ifndef __ASSEMBLER__

enum oem_id {
	PROJECT_AKALI = 1,
	PROJECT_VAYNE = 3,
	PROJECT_SONA,
	PROJECT_PANTHEON,
	PROJECT_NAMI,
	PROJECT_COUNT,
	PROJECT_SYLAS,
};

enum model_id {
	/* Sona variants */
	MODEL_SYNDRA = 1,
	/* Akali variants */
	MODEL_EKKO = 1,
	MODEL_BARD = 2,
};

enum temp_sensor_id {
	TEMP_SENSOR_LOCAL = 0,
	TEMP_SENSOR_REMOTE1,
	TEMP_SENSOR_REMOTE2,
	TEMP_SENSOR_COUNT,
};

/*
 * Motion sensors:
 * When reading through IO memory is set up for sensors (LPC is used),
 * the first 2 entries must be accelerometers, then gyroscope.
 * For BMI160, accel, gyro and compass sensors must be next to each other.
 */

enum sensor_id {
	LID_ACCEL = 0,
	BASE_ACCEL,
	BASE_GYRO,
	LID_ALS,
};

enum adc_channel {
	ADC_BASE_DET,
	ADC_VBUS,
	ADC_AMON_BMON,
	ADC_CH_COUNT,
};

enum pwm_channel {
	PWM_CH_LED1,
	PWM_CH_LED2,
	PWM_CH_FAN,
	PWM_CH_KBLIGHT,
	/* Number of PWM channels */
	PWM_CH_COUNT,
};

enum fan_channel {
	FAN_CH_0 = 0,
	/* Number of FAN channels */
	FAN_CH_COUNT,
};

enum mft_channel {
	MFT_CH_0 = 0,
	/* Number of MFT channels */
	MFT_CH_COUNT,
};

#endif /* !__ASSEMBLER__ */

#endif /* __CROS_EC_BOARD_H */
