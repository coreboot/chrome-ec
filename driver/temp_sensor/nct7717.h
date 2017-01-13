/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* NCT7717 temperature sensor module for Chrome EC */

#ifndef __CROS_EC_NCT7717_H
#define __CROS_EC_NCT7717_H

#define NCT7717_I2C_ADDR		0x90 /* 7-bit address is 0x48 */

#define NCT7717_TEMP_LOCAL		0x00
#define NCT7717_ALERT_STATUS		0x02
#define NCT7717_CONFIGURATION_R		0x03
#define NCT7717_CONVERSION_RATE_R	0x04
#define NCT7717_LT_HIGH_ALERT_TEMP_R	0x05
#define NCT7717_CONFIGURATION_W		0x09
#define NCT7717_CONVERSION_RATE_W	0x0A
#define NCT7717_LT_HIGH_ALERT_TEMP_W	0x0B
#define NCT7717_ONE_SHOT_CONVERSION	0x0F
#define NCT7717_CUSTOMER_DATA_LOG_REG_1	0x2D
#define NCT7717_CUSTOMER_DATA_LOG_REG_2	0x2E
#define NCT7717_CUSTOMER_DATA_LOG_REG_3 0x2F
#define NCT7717_ALERT_MODE		0xBF
#define NCT7717_CHIP_ID			0xFD
#define NCT7717_VENDOR_ID		0xFE
#define NCT7717_DEVICE_ID		0xFF

#define NCT7717_ALERT_STATUS_STS_LTHA	(1 << 6)
#define NCT7717_ALERT_STATUS_ADC_BUSY	(1 << 7)

#define NCT7717_CONFIGURATION_EN_FAULTQ	(1 << 0)
#define NCT7717_CONFIGURATION_STOP_MNT	(1 << 6)
#define NCT7717_CONFIGURATION_ALERT_MSK	(1 << 7)

/**
 * Get the last polled value of a sensor.
 *
 * @param idx		Index to read. Always 0 for NCT7717.
 * @param temp_ptr	Destination for temperature in K.
 *
 * @return EC_SUCCESS if successful, non-zero if error.
 */
int nct7717_get_val(int idx, int *temp_ptr);

#endif  /* __CROS_EC_NCT7717_H */
