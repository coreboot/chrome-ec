/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery_pack.h"
#include "gpio.h"
#include "host_command.h"
#include "smart_battery.h"

#define SB_SHIP_MODE_ADDR	0x00
#define SB_SHIP_MODE_DATA	0x0010

const struct battery_temperature_ranges bat_temp_ranges = {
	.start_charging_min_c = 0,
	.start_charging_max_c = 50,
	.charging_min_c       = 0,
	.charging_max_c       = 50,
	.discharging_min_c    = 0,
	.discharging_max_c    = 65,
};

static const struct battery_info info = {
/* Change to the real specification  */

	.voltage_max    = 13050,
	.voltage_normal = 11400,
	.voltage_min    = 9000,

	/* Pre-charge values. */
	.precharge_current  = 256,	/* mA */
};

const struct battery_info *battery_get_info(void)
{
	return &info;
}

/* Triger the command twice to make sure the battery can be cut-off.
 * Need to confirm with vendor why the command should be trigered twice.
 * By the result of experiment, it's ok and can be cut-off. */
int battery_command_cut_off(struct host_cmd_handler_args *args)
{
	int rv;
	rv = sb_write(SB_SHIP_MODE_ADDR, SB_SHIP_MODE_DATA);
	if (!rv)
		rv = sb_write(SB_SHIP_MODE_ADDR, SB_SHIP_MODE_DATA);
	return rv;
}
DECLARE_HOST_COMMAND(EC_CMD_BATTERY_CUT_OFF, battery_command_cut_off,
		     EC_VER_MASK(0));

/* FIXME: The smart battery should do the right thing - that's why it's
 * called "smart". Do we really want to second-guess it? For now, let's not. */
void battery_vendor_params(struct batt_params *batt)
{
}

int battery_is_connected(void)
{
	return (gpio_get_level(GPIO_BAT_DETECT_L) == 0);
}

