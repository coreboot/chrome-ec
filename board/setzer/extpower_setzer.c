/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "adc.h"
#include "charge_state.h"
#include "chipset.h"
#include "console.h"
#include "extpower.h"
#include "hooks.h"
#include "throttle_ap.h"
#include "util.h"

#define ADAPTER_PLUGGED_TIME   (200 * MSEC)
/* Console output macros */
#define CPRINTF(format, args...) cprintf(CC_USBCHARGE, format, ## args)

/* Adapter identification values */
struct adapter_id_vals {
	int lo, hi;
};

enum adapter_type {
	ADAPTER_45W = 0,
	ADAPTER_65W,
	NUM_ADAPTER_TYPES
};

/* Adapter-specific parameters. */
struct adapter_limits {
	int hi_val, lo_val;         /* current thresholds (mA) */
	int hi_cnt, lo_cnt;         /* count needed to trigger */
	int count;                  /* samples past the limit */
	int triggered;              /* threshold reached */
};

/* Values for our supported adapters */
static const char * const ad_name[] = {
	"45W",
	"65W"
};
BUILD_ASSERT(ARRAY_SIZE(ad_name) == NUM_ADAPTER_TYPES);

test_export_static
const struct adapter_id_vals ad_id_vals[] = {
	/* mV low, mV high */
	{434,     554},                  /* ADAPTER_45W */
	{561,     717}                   /* ADAPTER_65W */
};
BUILD_ASSERT(ARRAY_SIZE(ad_id_vals) == NUM_ADAPTER_TYPES);

test_export_static
const int ad_input_current[] = {
	/*
	 * Current limits in mA for each adapter.
	 * Values are in hex to avoid roundoff, because the BQ24773 Input
	 * Current Register masks off bits 6-0.
	 *
	 * Note that this is very specific to the combinations of adapters and
	 * BQ24773 charger chip on Setzer.
	 */

	0x0800,			/* ADAPTER_45W ~ 2.0 A */
	0x0c00			/* ADAPTER_65W ~ 3.0 A */
};
BUILD_ASSERT(ARRAY_SIZE(ad_input_current) == NUM_ADAPTER_TYPES);

test_export_static
struct adapter_limits ad_limits[][NUM_AC_THRESHOLDS] = {
	/* ADAPTER_45W */
	{
		{ 2308, 1385, 6, 32, },
		{ 2359, 1385, 1, 32, }
	},
	/* ADAPTER_65W */
	{
		{ 3333, 2000, 6, 32, },
		{ 3385, 2000, 1, 32, }
	}
};
BUILD_ASSERT(ARRAY_SIZE(ad_limits) == NUM_ADAPTER_TYPES);

/*
 * The battery current limits are independent of adapter rating.
 * hi_val and lo_val are DISCHARGE current in mA.
 */
test_export_static
struct adapter_limits batt_limits[NUM_BATT_THRESHOLDS] = {
	{ 5460, 3790, 6, 32, },
	{ 8400, 3790, 1, 32, }
};
BUILD_ASSERT(ARRAY_SIZE(batt_limits) == NUM_BATT_THRESHOLDS);

static int last_mv;
static enum adapter_type identify_adapter(void)
{
	int i;
	last_mv = adc_read_channel(ADC_AC_ADAPTER_ID_VOLTAGE);

	/* ADAPTER_45W matches everything, so search backwards */
	for (i = NUM_ADAPTER_TYPES - 1; i >= 0; i--)
		if (last_mv >= ad_id_vals[i].lo && last_mv <= ad_id_vals[i].hi)
			return i;

	return ADAPTER_45W;			/* should never get here */
}

test_export_static enum adapter_type ac_adapter;
void set_ad_input_current(void)
{
	ac_adapter = identify_adapter();
	CPRINTF("[%T AC Adapter is %s (%dmv)]\n",
		ad_name[ac_adapter], last_mv);
	charger_set_input_current(ad_input_current[ac_adapter]);
}
DECLARE_DEFERRED(set_ad_input_current);

static void ac_change_callback(void)
{
	if (extpower_is_present()) {
		hook_call_deferred(set_ad_input_current,
			(ADAPTER_PLUGGED_TIME));
	} else {
		ac_adapter = ADAPTER_45W;
		CPRINTF("[%T AC Adapter is not present]\n");
		/* Charger unavailable. Clear local flags */
	}
}
DECLARE_HOOK(HOOK_AC_CHANGE, ac_change_callback, HOOK_PRIO_DEFAULT);

/*
 * We need to OR all the possible reasons to throttle in order to decide
 * whether it should happen or not. Use one bit per reason.
 */
#define BATT_REASON_OFFSET 0
#define AC_REASON_OFFSET NUM_BATT_THRESHOLDS
BUILD_ASSERT(NUM_BATT_THRESHOLDS + NUM_AC_THRESHOLDS < 32);

test_export_static uint32_t ap_is_throttled;
static void set_throttle(int on, int whosays)
{
	if (on)
		ap_is_throttled |= (1 << whosays);
	else
		ap_is_throttled &= ~(1 << whosays);

	throttle_ap(ap_is_throttled ? THROTTLE_ON : THROTTLE_OFF,
		    THROTTLE_HARD, THROTTLE_SRC_POWER);
}

test_export_static
void check_threshold(int current, struct adapter_limits *lim, int whoami)
{
	if (lim->triggered) {
		/* watching for current to drop */
		if (current < lim->lo_val) {
			if (++lim->count >= lim->lo_cnt) {
				set_throttle(0, whoami);
				lim->count = 0;
				lim->triggered = 0;
			}
		} else {
			lim->count = 0;
		}
	} else {
		/* watching for current to rise */
		if (current > lim->hi_val) {
			if (++lim->count >= lim->hi_cnt) {
				set_throttle(1, whoami);
				lim->count = 0;
				lim->triggered = 1;
			}
		} else {
			lim->count = 0;
		}
	}
}

test_export_static
void watch_battery_closely(int current)
{
	int i;

	/*
	 * The values in batt_limits[] indicate DISCHARGE current (mA).
	 * However, the value returned from charger_get_params() is CHARGE
	 * current: postive for charging and negative for discharging.
	 *
	 * Turbo mode can discharge the battery even while connected to the
	 * charger. The spec says not to turn throttling off until the battery
	 * drain has been below the threshold for 5 seconds. That means we
	 * still need to check while on AC, or else just plugging the adapter
	 * in and out would mess up that 5-second timeout. Since the threshold
	 * logic uses signed numbers to compare the limits, everything Just
	 * works.
	 */

	/* Check limits against DISCHARGE current, not CHARGE current! */
	for (i = 0; i < NUM_BATT_THRESHOLDS; i++)
		check_threshold(-current, &batt_limits[i],
				/* invert sign! */
				i + BATT_REASON_OFFSET);
}

void watch_adapter_closely(void)
{
	int current, i;
	struct batt_params batt;
	battery_get_params(&batt);

	/* We always watch the battery current drain, even when on AC. */
	watch_battery_closely(batt.current);

	/* If the AP is off, we won't need to throttle it. */
	if (chipset_in_state(CHIPSET_STATE_ANY_OFF |
			     CHIPSET_STATE_SUSPEND))
		return;

	/* Check all the thresholds. */
	current = adc_read_channel(ADC_CH_CHARGER_CURRENT);
	for (i = 0; i < NUM_AC_THRESHOLDS; i++)
		check_threshold(current, &ad_limits[ac_adapter][i],
			i + AC_REASON_OFFSET);
}
DECLARE_HOOK(HOOK_TICK, watch_adapter_closely, HOOK_PRIO_DEFAULT);

static int command_adapter(int argc, char **argv)
{
	enum adapter_type v = identify_adapter();
	ccprintf("Adapter %s (%dmv), ap_is_throttled 0x%08x\n",
		 ad_name[v], last_mv, ap_is_throttled);
	return EC_SUCCESS;
}
DECLARE_CONSOLE_COMMAND(adapter, command_adapter,
			NULL,
			"Display AC adapter information",
			NULL);
