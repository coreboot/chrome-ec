/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* NCT7717 temperature sensor module for Chrome EC */

#ifndef __CROS_EC_TEMP_SENSOR_NCT7717_H
#define __CROS_EC_TEMP_SENSOR_NCT7717_H

#define NCT7717_I2C_ADDR		0x90 /* 7-bit address is 0x48 */

#define NCT7717_IDX_INTERNAL		0

/* Chip-specific commands */
#define NCT7717_TEMP_LOCAL		0x00
#define NCT7717_STATUS			0x02
#define NCT7717_CONFIGURATION_R		0x03
#define NCT7717_CONVERSION_RATE_R	0x04
#define NCT7717_LOCAL_TEMP_HIGH_LIMIT_R	0x05
#define NCT7717_CONFIGURATION_W		0x09
#define NCT7717_CONVERSION_RATE_W	0x0a
#define NCT7717_LOCAL_TEMP_HIGH_LIMIT_W	0x0b
#define NCT7717_ONESHOT			0x0f

#define NCT7717_ALERT_MODE		0xbf
#define NCT7717_CHIP_ID			0xfd
#define NCT7717_MANUFACTURER_ID		0xfe
#define NCT7717_DEVICE_ID		0xff

/* Config register bits */
#define NCT7717_CONFIGURATION_STANDBY		(1 << 6)
#define NCT7717_CONFIGURATION_ALERT_MASK	(1 << 7)

/* Status register bits */
#define NCT7717_STATUS_LOCAL_TEMP_HIGH_ALARM	(1 << 6)
#define NCT7717_STATUS_BUSY			(1 << 7)

/**
 * Get the last polled value of a sensor.
 *
 * @param idx		Index to read. Idx indicates whether to read die
 *			temperature or external temperature.
 * @param temp_ptr	Destination for temperature in K.
 *
 * @return EC_SUCCESS if successful, non-zero if error.
 */
int nct7717_get_val(int idx, int *temp_ptr);

#endif  /* __CROS_EC_TEMP_SENSOR_NCT7717_H */
