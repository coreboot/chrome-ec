/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_DRIVER_TOUCHPAD_ELAN_H
#define __CROS_EC_DRIVER_TOUCHPAD_ELAN_H

#define ELAN_VENDOR_ID 0x04f3

#define ETP_I2C_RESET 0x0100
#define ETP_I2C_WAKE_UP 0x0800
#define ETP_I2C_SLEEP 0x0801

#define ETP_I2C_STAND_CMD 0x0005
#define ETP_I2C_PATTERN_CMD 0x0100
#define ETP_I2C_UNIQUEID_CMD 0x0101
#define ETP_I2C_FW_VERSION_CMD 0x0102
#define ETP_I2C_IC_TYPE_CMD 0x0103
#define ETP_I2C_OSM_VERSION_CMD 0x0103
#define ETP_I2C_XY_TRACENUM_CMD 0x0105
#define ETP_I2C_MAX_X_AXIS_CMD 0x0106
#define ETP_I2C_MAX_Y_AXIS_CMD 0x0107
#define ETP_I2C_RESOLUTION_CMD 0x0108
#define ETP_I2C_IAP_VERSION_CMD 0x0110
#define ETP_I2C_IC_TYPE_P0_CMD 0x0110
#define ETP_I2C_IAP_VERSION_P0_CMD 0x0111
#define ETP_I2C_PRESSURE_CMD 0x010A
#define ETP_I2C_SET_CMD 0x0300
#define ETP_I2C_IAP_TYPE_CMD 0x0304
#define ETP_I2C_POWER_CMD 0x0307
#define ETP_I2C_FW_CHECKSUM_CMD 0x030F

#define ETP_ENABLE_ABS 0x0001

#define ETP_DISABLE_POWER 0x0001

#define ETP_I2C_REPORT_LEN 34

#define ETP_MAX_FINGERS 5
#define ETP_FINGER_DATA_LEN 5

#define ETP_PRESSURE_OFFSET 25
#define ETP_FWIDTH_REDUCE 90

#define ETP_REPORT_ID 0x5D
#define ETP_REPORT_ID_OFFSET 2
#define ETP_TOUCH_INFO_OFFSET 3
#define ETP_FINGER_DATA_OFFSET 4
#define ETP_HOVER_INFO_OFFSET 30
#define ETP_MAX_REPORT_LEN 34

#define ETP_IAP_START_ADDR 0x0083

#define ETP_I2C_IAP_RESET_CMD 0x0314
#define ETP_I2C_IAP_RESET 0xF0F0
#define ETP_I2C_IAP_CTRL_CMD 0x0310
#define ETP_I2C_MAIN_MODE_ON BIT(9)
#define ETP_I2C_IAP_CMD 0x0311
#define ETP_I2C_IAP_PASSWORD 0x1EA5

#define ETP_I2C_IAP_REG_L 0x01
#define ETP_I2C_IAP_REG_H 0x06
#define ETP_I2C_IAP_REG 0x0601

#define ETP_FW_IAP_PAGE_ERR BIT(5)
#define ETP_FW_IAP_INTF_ERR BIT(4)

#endif /* __CROS_EC_DRIVER_TOUCHPAD_ELAN_H */
