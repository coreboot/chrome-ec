/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Smart battery driver for Skate.
 */

#include "battery_pack.h"
#include "chipset.h"
#include "clock.h"
#include "console.h"
#include "hooks.h"
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

#ifndef BATTERY_CELL_CUT_OFF_MV
#define BATTERY_CELL_CUT_OFF_MV 0
#endif

static timestamp_t last_cutoff;
static int has_cutoff;
static int pending_cutoff;

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

int battery_cell_in_undervoltage(void)
{
	int i, v;

	if (BATTERY_CELL_CUT_OFF_MV) {
		for (i = 0x3d; i <= 0x3f; ++i) {
			if (sb_read(i, &v))
				continue;
			if (v < BATTERY_CELL_CUT_OFF_MV)
				return 1;
		}
	}

	return 0;
}

static int battery_want_cut_off(void)
{
	int charge;

	if (battery_is_cut_off())
		return 0;
	if (board_get_ac())
		return 0;

	if (BATTERY_CUT_OFF_MAH)
		if (!battery_remaining_capacity(&charge) &&
		    charge < BATTERY_CUT_OFF_MAH)
			return 1;

	if (battery_cell_in_undervoltage())
		return 1;

	return 0;
}

int battery_check_cut_off(void)
{
	if (!battery_want_cut_off())
		return 0;
	ccprintf("[%T Cutting off battery]\n");
	cflush();
	battery_cut_off();
	return 1;
}

static void cut_off_wrapper(void)
{
	battery_cut_off();
}
DECLARE_DEFERRED(cut_off_wrapper);

static void check_pending_cutoff(void)
{
	if (pending_cutoff)
		hook_call_deferred(cut_off_wrapper, 5);
}
DECLARE_HOOK(HOOK_CHIPSET_SHUTDOWN, check_pending_cutoff, HOOK_PRIO_LAST);

int battery_command_cut_off(struct host_cmd_handler_args *args)
{
	pending_cutoff = 1;

	/*
	 * When cutting off the battery, the AP is off and AC is not present.
	 * This makes serial console unresponsive and hard to verify battery
	 * cut-off. Let's disable sleep here so one can check cut-off status
	 * if needed. This shouldn't matter because we are about to cut off
	 * the battery.
	 */
	disable_sleep(SLEEP_MASK_FORCE);

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
