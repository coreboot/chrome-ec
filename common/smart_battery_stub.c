/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions needed by smart battery driver.
 */

#include "battery_pack.h"
#include "common.h"
#include "smart_battery.h"
#include "i2c.h"

int sbc_read(int cmd, int *param)
	{ return i2c_read16(I2C_PORT_CHARGER, CHARGER_ADDR, cmd, param); }

int sbc_write(int cmd, int param)
	{ return i2c_write16(I2C_PORT_CHARGER, CHARGER_ADDR, cmd, param); }

int sb_read(int cmd, int *param)
{
	if (battery_is_cut_off())
		return EC_ERROR_INVAL;
	return i2c_read16(I2C_PORT_BATTERY, BATTERY_ADDR, cmd, param);
}

int sb_write(int cmd, int param)
{
	if (battery_is_cut_off())
		return EC_ERROR_INVAL;
	return i2c_write16(I2C_PORT_BATTERY, BATTERY_ADDR, cmd, param);
}
