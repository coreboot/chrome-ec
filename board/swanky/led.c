/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery LED control for Swanky
 */

#include "charge_state.h"
#include "chipset.h"
#include "gpio.h"
#include "hooks.h"
#include "led_common.h"
#include "lid_switch.h"
#include "pwm.h"
#include "util.h"

#define LED_TOTAL_TICKS	16
#define LED_ON_TICKS	4

const enum ec_led_id supported_led_ids[] = {
	EC_LED_ID_BATTERY_LED, EC_LED_ID_POWER_LED};
const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

enum led_color {
	LED_OFF = 0,
	LED_WHITE,
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
	{20, 60},
	{0, 100},
};

/**
 * Set Battery LED color
 *
 * @param color		Enumerated color value
 */
static void set_battery_led_color(enum led_color color)
{
	pwm_set_duty(PWM_CH_LED_ORANGE, color_brightness[color][0]);
	pwm_set_duty(PWM_CH_LED_GREEN, color_brightness[color][1]);
}

static void set_power_led_color(enum led_color color)
{
	pwm_set_duty(PWM_CH_POWER_LED_WHITE, color_brightness[color][0]);
}

void led_get_brightness_range(enum ec_led_id led_id, uint8_t *brightness_range)
{
	if (led_id == EC_LED_ID_POWER_LED)
		brightness_range[EC_LED_COLOR_WHITE] = 100;
	else {
		brightness_range[EC_LED_COLOR_YELLOW] = 100;
		brightness_range[EC_LED_COLOR_GREEN] = 100;
	}
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
	if (led_id == EC_LED_ID_POWER_LED) {
		pwm_set_duty(PWM_CH_POWER_LED_WHITE,
			     brightness[EC_LED_COLOR_WHITE]);
	} else {
		pwm_set_duty(PWM_CH_LED_ORANGE,
			     brightness[EC_LED_COLOR_YELLOW]);
		pwm_set_duty(PWM_CH_LED_GREEN,
			     brightness[EC_LED_COLOR_GREEN]);
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
	pwm_enable(PWM_CH_LED_ORANGE, 1);
	pwm_enable(PWM_CH_LED_GREEN, 1);
	set_battery_led_color(LED_OFF);
	set_power_led_color(LED_OFF);
}
DECLARE_HOOK(HOOK_INIT, led_init, HOOK_PRIO_DEFAULT);

/**
 * Called by hook task every 250 ms
 */
static void battery_led_tick(void)
{
	static unsigned ticks;
	int chstate = charge_get_state();

	ticks++;

	/* If we don't control the LED, nothing to do */
	if (!led_auto_control_is_enabled(EC_LED_ID_BATTERY_LED))
		return;

	/* If charging error, blink orange, 25% duty cycle, 4 sec period */
	if (chstate == PWR_STATE_ERROR) {
		set_battery_led_color((ticks % 16) < 4 ? LED_ORANGE : LED_OFF);
		return;
	}

	/* If charge-force-idle, blink green, 50% duty cycle, 2 sec period */
	if (chstate == PWR_STATE_IDLE &&
	    (charge_get_flags() & CHARGE_FLAG_FORCE_IDLE)) {
		set_battery_led_color((ticks & 0x4) ? LED_GREEN : LED_OFF);
		return;
	}

	/* If the system is charging, solid orange */
	if (chstate == PWR_STATE_CHARGE) {
		set_battery_led_color(LED_ORANGE);
		return;
	}

	/* If AC connected and fully charged (or close to it), solid green */
	if (chstate == PWR_STATE_CHARGE_NEAR_FULL ||
	    chstate == PWR_STATE_IDLE) {
		set_battery_led_color(LED_GREEN);
		return;
	}

	/* Otherwise, system is off and AC not connected, LED off */
	set_battery_led_color(LED_OFF);
}

static void power_led_tick(void)
{

	static int power_ticks;
	static int previous_state_suspend;


	/* If we don't control the LED, nothing to do */
	if (!led_auto_control_is_enabled(EC_LED_ID_POWER_LED))
		return;

	power_ticks++;

	/* If lid close, LED turn off */
	if (!lid_is_open()) {
		set_power_led_color(LED_OFF);
		return;
	}

	if (chipset_in_state(CHIPSET_STATE_SUSPEND)) {

		/* Reset ticks if entering suspend, so LED turns amber
		 * as soon as possible. */
		if (!previous_state_suspend)
			power_ticks = 0;

		/* Blink once every four seconds. */
		set_power_led_color(
			(power_ticks % LED_TOTAL_TICKS < LED_ON_TICKS) ?
			LED_WHITE : LED_OFF);

		previous_state_suspend = 1;
		return;
	}

	previous_state_suspend = 0;

	if (chipset_in_state(CHIPSET_STATE_ANY_OFF))
		set_power_led_color(LED_OFF);
	else if (chipset_in_state(CHIPSET_STATE_ON))
		set_power_led_color(LED_WHITE);

}

static void led_tick(void)
{
	battery_led_tick();
	power_led_tick();
}
DECLARE_HOOK(HOOK_TICK, led_tick, HOOK_PRIO_DEFAULT);
