/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery LED control for Cranky
 */

#include "charge_state.h"
#include "chipset.h"
#include "gpio.h"
#include "hooks.h"
#include "led_common.h"
#include "pwm.h"
#include "util.h"

const enum ec_led_id supported_led_ids[] = {
	EC_LED_ID_BATTERY_LED, EC_LED_ID_POWER_LED};
const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

enum led_color {
	LED_OFF = 0,
	LED_BLUE,
	LED_RED,
	LED_ORANGE,
	LED_YELLOW,
	LED_GREEN,

	/* Number of colors, not a color itself */
	LED_COLOR_COUNT
};

/* Brightness vs. color, for {red, green} LEDs */
static const uint8_t color_brightness[LED_COLOR_COUNT][2] = {
	{0, 0},
	{100, 0},
	{100, 0},
	{30, 45},
	{20, 60},
	{0, 100},
};

/**
 * Set LED color
 *
 * @param color		Enumerated color value
 */
static void set_color_battery_led(enum led_color color)
{
	pwm_set_duty(PWM_CH_LED_RED, color_brightness[color][0]);
	pwm_set_duty(PWM_CH_LED_GREEN, color_brightness[color][1]);
}

static void set_color_power_led(enum led_color color)
{
	pwm_set_duty(PWM_CH_LED_BLUE_POWER_LED, color_brightness[color][0]);
}

void led_get_brightness_range(enum ec_led_id led_id, uint8_t *brightness_range)
{
	switch (led_id) {
	case EC_LED_ID_BATTERY_LED:
		brightness_range[EC_LED_COLOR_RED] = 100;
		brightness_range[EC_LED_COLOR_GREEN] = 100;
		break;
	case EC_LED_ID_POWER_LED:
		brightness_range[EC_LED_COLOR_BLUE] = 100;
		break;
	default:
		/* Nothing to do */
		break;
	}
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
	switch (led_id) {
	case EC_LED_ID_BATTERY_LED:
		pwm_set_duty(PWM_CH_LED_RED,
			     brightness[EC_LED_COLOR_RED]);
		pwm_set_duty(PWM_CH_LED_GREEN,
			     brightness[EC_LED_COLOR_GREEN]);
		break;
	case EC_LED_ID_POWER_LED:
		pwm_set_duty(PWM_CH_LED_BLUE_POWER_LED,
			     brightness[EC_LED_COLOR_BLUE]);
		break;
	default:
		break;
	}
	return EC_SUCCESS;
}

static void led_init(void)
{
	/* Configure GPIOs */
	gpio_config_module(MODULE_PWM_LED, 1);

	/*
	 * Enable PWMs and set to 0% duty cycle.  If they're disabled, the LM4
	 * seems to ground the pins instead of letting them float.
	 */
	pwm_enable(PWM_CH_LED_RED, 1);
	pwm_enable(PWM_CH_LED_GREEN, 1);
	pwm_enable(PWM_CH_LED_BLUE_POWER_LED, 1);
	set_color_battery_led(LED_OFF);
	set_color_power_led(LED_OFF);
}
DECLARE_HOOK(HOOK_INIT, led_init, HOOK_PRIO_DEFAULT);

/**
 * Called by hook task every 250 ms
 */
static void power_led_tick(void)
{
	static unsigned power_ticks;
	static int previous_state_suspend;

	power_ticks++;

	/* If we don't control the LED, nothing to do */
	if (!led_auto_control_is_enabled(EC_LED_ID_POWER_LED))
		return;

	if (chipset_in_state(CHIPSET_STATE_ON)) {
		set_color_power_led(LED_BLUE);
	} else if (chipset_in_state(CHIPSET_STATE_SUSPEND)) {
		if (!previous_state_suspend) {
			previous_state_suspend = 1;
			power_ticks = 0;
		}
		/* Flashing 1 sec on, 1 sec off */
		set_color_power_led((power_ticks % 8) < 4 ? LED_BLUE : LED_OFF);
		return;
	} else {
		set_color_power_led(LED_OFF);
	}

	previous_state_suspend = 0;
}

/**
 * Called by hook task every 250 ms
 */
static void battery_led_tick(void)
{
	static unsigned battery_ticks;
	int chstate = charge_get_state();

	battery_ticks++;

	/* If we don't control the LED, nothing to do */
	if (!led_auto_control_is_enabled(EC_LED_ID_BATTERY_LED))
		return;

	/* If charging error, blink orange, 25% duty cycle, 4 sec period */
	if (chstate == PWR_STATE_ERROR) {
		set_color_battery_led((battery_ticks % 16) < 4 ?
		LED_ORANGE : LED_OFF);
		return;
	}

	/* If charge-force-idle, blink green, 50% duty cycle, 2 sec period */
	if (chstate == PWR_STATE_IDLE &&
	    (charge_get_flags() & CHARGE_FLAG_FORCE_IDLE)) {
		set_color_battery_led((battery_ticks & 0x4) ?
		LED_GREEN : LED_OFF);
		return;
	}

	/* If the system is charging, solid orange */
	if (chstate == PWR_STATE_CHARGE) {
		set_color_battery_led(LED_ORANGE);
		return;
	}

	/* If AC connected and fully charged (or close to it), solid green */
	if (chstate == PWR_STATE_CHARGE_NEAR_FULL ||
	    chstate == PWR_STATE_IDLE) {
		set_color_battery_led(LED_GREEN);
		return;
	}

	/* Otherwise, system is off and AC not connected, LED off */
	set_color_battery_led(LED_OFF);
}

static void led_tick(void)
{
	battery_led_tick();
	power_led_tick();
}
DECLARE_HOOK(HOOK_TICK, led_tick, HOOK_PRIO_DEFAULT);
