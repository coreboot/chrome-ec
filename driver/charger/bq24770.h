/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * TI BQ24770 battery charger driver.
 */

#ifndef __CROS_EC_CHARGER_BQ24770_H
#define __CROS_EC_CHARGER_BQ24770_H

/* I2C address */
#define BQ24770_ADDR (0x6a << 1)

/* Chip specific commands */
#define BQ24770_CHARGE_OPTION0          0x12
#define BQ24770_CHARGE_OPTION1          0x3B
#define BQ24770_CHARGE_OPTION2          0x38
#define BQ24770_PROTECT_OPTION0         0x3C
#define BQ24770_PROTECT_OPTION1         0x3D
#define BQ24770_PROTECT_STATUS          0x3A
#define BQ24770_DEVICE_ADDRESS          0xff
#define BQ24770_CHARGE_CURRENT          0x14
#define BQ24770_MAX_CHARGE_VOLTAGE      0x15
#define BQ24770_MIN_SYSTEM_VOLTAGE      0x3E
#define BQ24770_INPUT_CURRENT           0x3F
#define BQ24770_MANUFACTURER_ID         0xfe



/* Option bits */
#define OPTION0_CHARGE_INHIBIT          (1 << 0)
#define OPTION0_LEARN_ENABLE            (1 << 5)

#define OPTION1_AUTOWAKE_EN                (1 << 0)

#define OPTION2_EN_EXTILIM              (1 << 7)

/* ChargeCurrent Register - 0x14 (mA) */
#define CHARGE_I_OFF                    0
#define CHARGE_I_MIN                    128
#define CHARGE_I_MAX                    8128
#define CHARGE_I_STEP                   64

/* MaxChargeVoltage Register - 0x15 (mV) */
#define CHARGE_V_MIN                    1024
#define CHARGE_V_MAX                    19200
#define CHARGE_V_STEP                   16

/* InputCurrent Register - 0x3f (mA) */
#define INPUT_I_MIN                    128
#define INPUT_I_MAX                    8128
#define INPUT_I_STEP                   64

#endif /* __CROS_EC_CHARGER_BQ24770_H */
