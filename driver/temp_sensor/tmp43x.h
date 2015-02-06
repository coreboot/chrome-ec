/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* TMP431/TMP432 temperature sensor module for Chrome EC */

#ifndef __CROS_EC_TEMP_SENSOR_TMP43X_H
#define __CROS_EC_TEMP_SENSOR_TMP43X_H

#define TMP432_I2C_ADDR		0x98 /* 7-bit address is 0x4C (TMP432A)*/
#define TMP431_I2C_ADDR		0x9A /* 7-bit address is 0x4D (TMP431B)*/

#define TMP431_DEV_SEL		0
#define TMP432_DEV_SEL		1

#define TMP431_IDX_LOCAL	10
#define TMP431_IDX_REMOTE1	11

#define TMP432_IDX_LOCAL	0
#define TMP432_IDX_REMOTE1	1
#define TMP432_IDX_REMOTE2	2

/*
 * Chip-specific registers
 *  TMP43x_<...> - Common registers between TMP431 and TMP432
 *  TMP431_<...> - TMP431 specific registers
 *  TMP432_<...> - TMP432 specific registers
 */
#define TMP43X_LOCAL			0x00
#define TMP43X_REMOTE1			0x01
#define TMP43X_STATUS			0x02
#define TMP43X_CONFIGURATION1_R		0x03
#define TMP43X_CONVERSION_RATE_R	0x04
#define TMP43X_LOCAL_HIGH_LIMIT_R	0x05
#define TMP43X_LOCAL_LOW_LIMIT_R	0x06
#define TMP43X_REMOTE1_HIGH_LIMIT_R	0x07
#define TMP43X_REMOTE1_LOW_LIMIT_R	0x08
#define TMP43X_CONFIGURATION1_W		0x09
#define TMP43X_CONVERSION_RATE_W	0x0a
#define TMP43X_LOCAL_HIGH_LIMIT_W	0x0b
#define TMP43X_LOCAL_LOW_LIMIT_W	0x0c
#define TMP43X_REMOTE1_HIGH_LIMIT_W	0x0d
#define TMP43X_REMOTE1_LOW_LIMIT_W	0x0e
#define TMP43X_ONESHOT			0x0f
#define TMP43X_REMOTE1_EXTD		0x10
#define TMP43X_REMOTE1_HIGH_LIMIT_EXTD	0x13
#define TMP43X_REMOTE1_LOW_LIMIT_EXTD	0x14

#define TMP431_LOCAL_EXTD		0x15
#define TMP431_LOCAL_HIGH_LIMIT_EXTD	0x16
#define TMP431_LOCAL_LOW_LIMIT_EXTD	0x17
#define TMP431_NFACTOR_REMOTE1		0x18

#define TMP432_REMOTE2_HIGH_LIMIT_R	0x15
#define TMP432_REMOTE2_HIGH_LIMIT_W	0x15
#define TMP432_REMOTE2_LOW_LIMIT_R	0x16
#define TMP432_REMOTE2_LOW_LIMIT_W	0x16
#define TMP432_REMOTE2_HIGH_LIMIT_EXTD	0x17
#define TMP432_REMOTE2_LOW_LIMIT_EXTD	0x18

#define TMP43X_REMOTE1_THERM_LIMIT	0x19

#define TMP431_CONFIGURATION2_R		0x1a
#define TMP431_CONFIGURATION2_W		0x1a

#define TMP432_REMOTE2_THERM_LIMIT	0x1a
#define TMP432_STATUS_FAULT		0x1b

#define TMP43X_CHANNEL_MASK		0x1f
#define TMP43X_LOCAL_THERM_LIMIT	0x20
#define TMP43X_THERM_HYSTERESIS		0x21
#define TMP43X_CONSECUTIVE_ALERT	0x22

#define TMP432_REMOTE2			0x23
#define TMP432_REMOTE2_EXTD		0x24

#define TMP43X_BETA_RANGE_CH1		0x25

#define TMP432_BETA_RANGE_CH2		0x26
#define TMP432_NFACTOR_REMOTE1		0x27
#define TMP432_NFACTOR_REMOTE2		0x28
#define TMP432_LOCAL_EXTD		0x29
#define TMP432_STATUS_LIMIT_HIGH	0x35
#define TMP432_STATUS_LIMIT_LOW		0x36
#define TMP432_STATUS_THERM		0x37
#define TMP432_LOCAL_HIGH_LIMIT_EXTD	0x3d
#define TMP432_LOCAL_LOW_LIMIT_EXTD	0x3e
#define TMP432_CONFIGURATION2_R		0x3f
#define TMP432_CONFIGURATION2_W		0x3f

#define TMP43X_RESET_W			0xfc
#define TMP43X_DEVICE_ID		0xfd
#define TMP43X_MANUFACTURER_ID		0xfe

/* Config register bits */
#define TMP43X_CONFIG1_TEMP_RANGE		(1 << 2)
#define TMP43X_CONFIG1_MODE			(1 << 5)
#define TMP43X_CONFIG1_RUN_L			(1 << 6)
#define TMP43X_CONFIG1_ALERT_MASK_L		(1 << 7)
#define TMP43X_CONFIG2_RESISTANCE_CORRECTION	(1 << 2)
#define TMP43X_CONFIG2_LOCAL_ENABLE		(1 << 3)
#define TMP43X_CONFIG2_REMOTE1_ENABLE		(1 << 4)

#define TMP432_CONFIG2_REMOTE2_ENABLE		(1 << 5)

/* Status register bits */
#define TMP431_STATUS_LOCAL_THERM_ALARM		(1 << 0)
#define TMP431_STATUS_REMOTE_THERM_ALARM	(1 << 1)

#define TMP432_STATUS_TEMP_THERM_ALARM		(1 << 1)

#define TMP43X_STATUS_OPEN			(1 << 2)

#define TMP431_STATUS_REMOTE_LOW_ALARM		(1 << 3)
#define TMP431_STATUS_REMOTE_HIGH_ALARM		(1 << 4)
#define TMP431_STATUS_LOCAL_LOW_ALARM		(1 << 5)
#define TMP431_STATUS_LOCAL_HIGH_ALARM		(1 << 6)

#define TMP432_STATUS_TEMP_LOW_ALARM		(1 << 3)
#define TMP432_STATUS_TEMP_HIGH_ALARM		(1 << 4)

#define TMP43X_STATUS_BUSY			(1 << 7)

/*
 * Get the last polled value of a sensor.
 *
 * @param idx		Index to read. Idx indicates whether to read die
 *			temperature or external temperature.
 * @param temp_ptr	Destination for temperature in K.
 *
 * @return EC_SUCCESS if successful, non-zero if error.
 */

int tmp43x_get_val(int idx, int *temp_ptr);

#endif /* __CROS_EC_TEMP_SENSOR_TMP43X_H */
