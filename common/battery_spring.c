/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Smart battery driver for Spring.
 */

#include "battery_pack.h"
#include "chipset.h"
#include "console.h"
#include "host_command.h"
#include "i2c.h"
#include "pmu_tpschrome.h"
#include "smart_battery.h"
#include "timer.h"
#include "util.h"

#define PARAM_CUT_OFF_LOW  0x10
#define PARAM_CUT_OFF_HIGH 0x00

#ifndef BATTERY_CUT_OFF_MAH
#define BATTERY_CUT_OFF_MAH 0
#endif

#ifndef BATTERY_CUT_OFF_DELAY
#define BATTERY_CUT_OFF_DELAY 0
#endif

static timestamp_t last_cutoff;
static int has_cutoff;

int battery_cut_off(void)
{
	int rv;
	uint8_t buf[3];

	buf[0] = SB_MANUFACTURER_ACCESS & 0xff;
	buf[1] = PARAM_CUT_OFF_LOW;
	buf[2] = PARAM_CUT_OFF_HIGH;

	i2c_lock();
	rv = i2c_xfer(I2C_PORT_BATTERY, BATTERY_ADDR, buf, 3, NULL, 0);
	rv = i2c_xfer(I2C_PORT_BATTERY, BATTERY_ADDR, buf, 3, NULL, 0);
	i2c_unlock();

	has_cutoff = 1;
	last_cutoff = get_time();

	return rv;
}

int battery_is_cut_off(void)
{
	return has_cutoff &&
	       get_time().val - last_cutoff.val < BATTERY_CUT_OFF_DELAY;
}

int battery_check_cut_off(void)
{
	int charge;

	if (!BATTERY_CUT_OFF_MAH)
		return 0;
	if (battery_is_cut_off())
		return 0;
	if (chipset_in_state(CHIPSET_STATE_ON | CHIPSET_STATE_SUSPEND))
		return 0;
	if (battery_remaining_capacity(&charge))
		return 0;
	if (charge > BATTERY_CUT_OFF_MAH)
		return 0;
	if (board_get_ac())
		return 0;

	ccprintf("[%T Cutting off battery]\n");
	cflush();
	battery_cut_off();
	return 1;
}

int battery_command_cut_off(struct host_cmd_handler_args *args)
{
	if (battery_cut_off())
		return EC_RES_ERROR;
	return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_BATTERY_CUT_OFF, battery_command_cut_off,
		     EC_VER_MASK(0));

static int command_cutoff(int argc, char **argv)
{
	int tries = 5;

	while (board_get_ac() && tries) {
		ccprintf("Remove AC power in %d seconds...\n", tries);
		tries--;
		usleep(SECOND);
	}

	if (board_get_ac())
		return EC_ERROR_UNKNOWN;

	ccprintf("Cutting off. Please wait for 10 seconds.\n");

	return battery_cut_off();
}
DECLARE_CONSOLE_COMMAND(cutoff, command_cutoff, NULL,
			"Cut off the battery", NULL);
