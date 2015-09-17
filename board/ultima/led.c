/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Power/Battery LED control for Strago
 */

#include "charge_state.h"
#include "chipset.h"
#include "console.h"
#include "extpower.h"
#include "gpio.h"
#include "hooks.h"
#include "host_command.h"
#include "led_common.h"
#include "pwm.h"
#include "registers.h"
#include "util.h"

#define CPRINTF(format, args...) cprintf(CC_PWM, format, ## args)
#define CPRINTS(format, args...) cprints(CC_PWM, format, ## args)

#define LED_TOTAL_TICKS 2
#define LED_ON_TICKS 1

#define TICKS_STEP1_BRIGHTER 0
#define TICKS_STEP2_DIMMER 20
#define TICKS_STEP3_OFF 40

#define FULL_BATTERY_PERMILLAGE 795

static int led_debug;
static int ticks;

const enum ec_led_id supported_led_ids[] = {
	EC_LED_ID_POWER_LED, EC_LED_ID_BATTERY_LED};
const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

enum led_color {
	LED_OFF = 0,
	LED_RED,
	LED_AMBER,
	LED_GREEN,

	/* Number of colors, not a color itself */
	LED_COLOR_COUNT
};

/* Brightness vs. color, in the order of off, red, amber, and green */
static const uint8_t color_brightness[LED_COLOR_COUNT][3] = {
	/* {Green, Blue, Red}, */
	[LED_OFF]   = {100, 100,   0},
	[LED_RED]   = {  0,   0, 100},
	[LED_AMBER] = {100,   0,   0},
	[LED_GREEN] = {  0, 100,   0},
};

/**
 * Set LED color
 *
 * @param color		Enumerated color value
 */
static void set_battery_color(enum led_color color)
{
	pwm_set_duty(PWM_CH_LED_GREEN, color_brightness[color][0]);
	pwm_set_duty(PWM_CH_LED_BLUE, color_brightness[color][1]);
}

static void set_power_color(enum led_color color)
{
	pwm_set_duty(PWM_CH_LED_RED, color_brightness[color][2]);
}

void led_get_brightness_range(enum ec_led_id led_id, uint8_t *brightness_range)
{
	brightness_range[EC_LED_COLOR_GREEN] = 100;
	brightness_range[EC_LED_COLOR_BLUE] = 100;
	brightness_range[EC_LED_COLOR_RED] = 100;
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
	pwm_set_duty(PWM_CH_LED_GREEN, brightness[EC_LED_COLOR_GREEN]);
	pwm_set_duty(PWM_CH_LED_BLUE, brightness[EC_LED_COLOR_BLUE]);
	pwm_set_duty(PWM_CH_LED_RED, brightness[EC_LED_COLOR_RED]);
	return EC_SUCCESS;
}

static void suspend_led_update(void)
{
	int delay = 50 * MSEC;

	ticks++;

	/* 1s gradual on, 1s gradual off, 3s off */
	if (ticks <= TICKS_STEP2_DIMMER) {
		pwm_set_duty(PWM_CH_LED_RED, ticks*5);
	} else if (ticks <= TICKS_STEP3_OFF) {
		pwm_set_duty(PWM_CH_LED_RED, (TICKS_STEP3_OFF - ticks)*5);
	} else {
		ticks = TICKS_STEP1_BRIGHTER;
		delay = 3000 * MSEC;
	}

	hook_call_deferred(suspend_led_update, delay);
}
DECLARE_DEFERRED(suspend_led_update);

static void suspend_led_init(void)
{
	ticks = TICKS_STEP2_DIMMER;

	hook_call_deferred(suspend_led_update, 0);
}
DECLARE_HOOK(HOOK_CHIPSET_SUSPEND, suspend_led_init, HOOK_PRIO_DEFAULT);

static void suspend_led_deinit(void)
{
	hook_call_deferred(suspend_led_update, -1);
}
DECLARE_HOOK(HOOK_CHIPSET_RESUME, suspend_led_deinit, HOOK_PRIO_DEFAULT);
DECLARE_HOOK(HOOK_CHIPSET_SHUTDOWN, suspend_led_deinit, HOOK_PRIO_DEFAULT);

static void ultima_led_set_power(void)
{
	static int power_ticks;
	static int previous_state_suspend;
	static int blink_ticks;

	power_ticks++;

	if (extpower_is_present()) {
		blink_ticks++;
		if (!previous_state_suspend)
			power_ticks = 0;

		while (blink_ticks < 7) {
			set_power_color(
				(power_ticks % LED_TOTAL_TICKS) < LED_ON_TICKS ?
				LED_RED : LED_OFF);

			previous_state_suspend = 1;
			return;
		}
	}
	if (!extpower_is_present())
		blink_ticks = 0;
	if (chipset_in_state(CHIPSET_STATE_SUSPEND | CHIPSET_STATE_SOFT_OFF)) {
		/* Reset ticks if entering suspend so LED turns red
		 * as soon as possible. */
		if (!previous_state_suspend)
			power_ticks = 0;

		/* Blink twice every one seconds. */
		set_power_color(
			(power_ticks % LED_TOTAL_TICKS) < LED_ON_TICKS ?
			LED_RED : LED_OFF);

		previous_state_suspend = 1;
		return;
	}

	previous_state_suspend = 0;

	if (chipset_in_state(CHIPSET_STATE_ANY_OFF))
		set_power_color(LED_OFF);
	if (chipset_in_state(CHIPSET_STATE_ON))
		set_power_color(LED_RED);
}

static void ultima_led_set_battery(void)
{
	static int battery_ticks;
	int remaining_capacity;
	int full_charge_capacity;
	int permillage;

	battery_ticks++;

	remaining_capacity = *(int *)host_get_memmap(EC_MEMMAP_BATT_CAP);
	full_charge_capacity = *(int *)host_get_memmap(EC_MEMMAP_BATT_LFCC);
	permillage = !full_charge_capacity ? 0 :
		(1000 * remaining_capacity) / full_charge_capacity;

	switch (charge_get_state()) {
	case PWR_STATE_CHARGE:
		set_battery_color(permillage <
			FULL_BATTERY_PERMILLAGE ? LED_AMBER : LED_GREEN);
		break;
	default:
		set_battery_color(LED_OFF);
		break;
	}
}

static void led_init(void)
{
	/* Configure GPIOs */
	gpio_config_module(MODULE_PWM_LED, 1);

	/*
	 * Enable PWMs and set to 0% duty cycle.  If they're disabled,
	 * seems to ground the pins instead of letting them float.
	 */
	pwm_enable(PWM_CH_LED_RED, 1);
	pwm_enable(PWM_CH_LED_GREEN, 1);
	pwm_enable(PWM_CH_LED_BLUE, 1);

	set_battery_color(LED_OFF);
	set_power_color(LED_OFF);
}
DECLARE_HOOK(HOOK_INIT, led_init, HOOK_PRIO_DEFAULT);

/**
 * Called by hook task every 250 ms
 */
static void led_tick(void)
{
	if (led_debug)
		return;

	if (led_auto_control_is_enabled(EC_LED_ID_BATTERY_LED))
		ultima_led_set_battery();
	if (led_auto_control_is_enabled(EC_LED_ID_POWER_LED))
		ultima_led_set_power();
}
DECLARE_HOOK(HOOK_TICK, led_tick, HOOK_PRIO_DEFAULT);

static void dump_pwm_channels(void)
{
	int ch;
	for (ch = 0; ch < 4; ch++) {
		CPRINTF("channel = %d\n", ch);
		CPRINTF("0x%04X 0x%04X 0x%04X\n",
			MEC1322_PWM_CFG(ch),
			MEC1322_PWM_ON(ch),
			MEC1322_PWM_OFF(ch));
	}
}
/******************************************************************/
/* Console commands */
static int command_led_color(int argc, char **argv)
{
	if (argc > 1) {
		if (!strcasecmp(argv[1], "debug")) {
			led_debug ^= 1;
			CPRINTF("led_debug = %d\n", led_debug);
		} else if (!strcasecmp(argv[1], "off")) {
			set_power_color(LED_OFF);
			set_battery_color(LED_OFF);
		} else if (!strcasecmp(argv[1], "red")) {
			set_power_color(LED_RED);
		} else if (!strcasecmp(argv[1], "green")) {
			set_battery_color(LED_GREEN);
		} else if (!strcasecmp(argv[1], "amber")) {
			set_battery_color(LED_AMBER);
		} else {
			/* maybe handle charger_discharge_on_ac() too? */
			return EC_ERROR_PARAM1;
		}
	}

	if (led_debug == 1)
		dump_pwm_channels();
	return EC_SUCCESS;
}
DECLARE_CONSOLE_COMMAND(ledcolor, command_led_color,
			"[debug|red|green|amber|off]",
			"Change LED color",
			NULL);

