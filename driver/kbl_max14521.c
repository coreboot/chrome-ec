/* Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Maxim MAX14521 EL lamp driver
 */

#include "common.h"
#include "console.h"
#include "driver/kbl_max14521.h"
#include "hooks.h"
#include "host_command.h"
#include "i2c.h"
#include "lid_switch.h"
#include "pwm.h"
#include "system.h"
#include "util.h"


/* I2C interface */
#define MAX14521_I2C_ADDR       0xF0
#define MAX14521_REG_DEV_ID     0x00
#define MAX14251_DEVICE_ID      0xB2
#define MAX14521_REG_PWR_MODE   0x01
#define MAX14521_REG_EL_FREQ	0x02
#define MAX14521_REG_EL_SHAPE   0x03
#define MAX14521_REG_BST_FREQ   0x04
#define MAX14521_REG_AUDIO      0x05
#define MAX14521_REG_EL1_T_V    0x06
#define MAX14521_REG_EL2_T_V    0x07
#define MAX14521_REG_EL3_T_V    0x08
#define MAX14521_REG_EL4_T_V    0x09
#define MAX14521_REG_EL_UPDATE  0x0A

#define MAX14521_PWR_MODE_EN    0x01
#define MAX14521_PWR_MODE_DIS   0x00
#define MAX14521_BST_FREQ_1KHZ  0x04
#define MAX14521_EL_FREQ_180HZ  0x73
/* Ramping Time Configuration : 2 seconds */
#define MAX14521_RT_2SEC        (0x07 << 5)

#define MAX14521_VSTEP_MAX      31
static int curr_percent;
static int g_pwm_duty;

static int max14521_set_kblight(int percent)
{
        int rv, step;

        if(percent == 0) {
                curr_percent = 0;

                return i2c_write8(I2C_PORT_KBLIGHT, MAX14521_I2C_ADDR,
                                MAX14521_REG_PWR_MODE, MAX14521_PWR_MODE_DIS);
        }

        if(percent < 0)
                return EC_ERROR_PARAM1;

        if(percent > 100)
                return EC_ERROR_PARAM1;

        step = (percent * (MAX14521_VSTEP_MAX - 1)) / 100 + 1;

        if(curr_percent == 0) {
                rv = i2c_write8(I2C_PORT_KBLIGHT, MAX14521_I2C_ADDR,
                                MAX14521_REG_PWR_MODE, MAX14521_PWR_MODE_EN);

                if(rv)
                        return rv;
        }

        /* Write ramping time and voltage */
        rv = i2c_write8(I2C_PORT_KBLIGHT, MAX14521_I2C_ADDR,
                        MAX14521_REG_EL1_T_V, MAX14521_RT_2SEC | step);

        if(rv)
                return rv;

        /* It is needed that write update register to change output level of EL.*/
        rv = i2c_write8(I2C_PORT_KBLIGHT, MAX14521_I2C_ADDR,
                        MAX14521_REG_EL_UPDATE, MAX14521_RT_2SEC | step);

        if(rv)
                return rv;

        curr_percent = percent;

        return EC_SUCCESS;
}

int max14521_init(void)
{
	int rv;

        rv = i2c_write8(I2C_PORT_KBLIGHT, MAX14521_I2C_ADDR,
                        MAX14521_REG_BST_FREQ, MAX14521_BST_FREQ_1KHZ);

        if(rv)
                return rv;

        rv = i2c_write8(I2C_PORT_KBLIGHT, MAX14521_I2C_ADDR,
                        MAX14521_REG_EL_FREQ, MAX14521_EL_FREQ_180HZ);

        if(rv)
                return rv;

        return max14521_set_kblight(0);
}

static int max14521_get_kblight(void)
{
        return curr_percent;
}

/*****************************************************************************/
/* Commands to control keyboard backlight by ACPI */
static void update_kblight(void)
{
        max14521_set_kblight(g_pwm_duty);
}
DECLARE_DEFERRED(update_kblight);

int pwm_get_duty(enum pwm_channel ch)
{
        return max14521_get_kblight();
}

void pwm_set_duty(enum pwm_channel ch, int data)
{
        g_pwm_duty = data;
        hook_call_deferred(update_kblight, 0);
}

int pwm_get_enabled(enum pwm_channel ch)
{
	return (curr_percent != 0);
}
