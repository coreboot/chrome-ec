/* Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery LED control for Banjo
 */

#include "charge_state.h"
#include "chipset.h"
#include "gpio.h"
#include "hooks.h"
#include "led_common.h"
#include "pwm.h"
#include "util.h"
#include "extpower.h"

const enum ec_led_id supported_led_ids[] = {
	EC_LED_ID_BATTERY_LED , EC_LED_ID_POWER_LED};
const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

enum led_color {
	LED_OFF = 0,
	LED_ORANGE,
	LED_RED,
	LED_YELLOW,
	LED_BLUE,

	/* Number of colors, not a color itself */
	LED_COLOR_COUNT
};

/* Brightness vs. color, for {red, green} LEDs */
static const uint8_t color_brightness[LED_COLOR_COUNT][2] = {
	{0, 0},
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
	pwm_set_duty(PWM_CH_LED_ORANGE, color_brightness[color][0]);
	pwm_set_duty(PWM_CH_LED_BLUE, color_brightness[color][1]);
}

static void set_color_power_led(enum led_color color)
{
	pwm_set_duty(PWM_CH_LED_ORANGE_POWER_LED, color_brightness[color][0]);
	pwm_set_duty(PWM_CH_LED_BLUE_POWER_LED, color_brightness[color][1]);
}

void led_get_brightness_range(enum ec_led_id led_id, uint8_t *brightness_range)
{
	brightness_range[EC_LED_COLOR_YELLOW] = 100;
	brightness_range[EC_LED_COLOR_BLUE] = 100;
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
	if (led_id == EC_LED_ID_POWER_LED) {
		pwm_set_duty(PWM_CH_LED_BLUE_POWER_LED,
			     brightness[EC_LED_COLOR_BLUE]);
		pwm_set_duty(PWM_CH_LED_ORANGE_POWER_LED,
			     brightness[EC_LED_COLOR_YELLOW]);
	} else {
		pwm_set_duty(PWM_CH_LED_BLUE,
			     brightness[EC_LED_COLOR_BLUE]);
		pwm_set_duty(PWM_CH_LED_ORANGE,
			     brightness[EC_LED_COLOR_YELLOW]);
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
	pwm_enable(PWM_CH_LED_BLUE, 1);
	pwm_enable(PWM_CH_LED_ORANGE_POWER_LED, 1);
	pwm_enable(PWM_CH_LED_BLUE_POWER_LED, 1);
	set_color_battery_led(LED_OFF);
	set_color_power_led(LED_OFF);
}
DECLARE_HOOK(HOOK_INIT, led_init, HOOK_PRIO_DEFAULT);

/**
 * Called by hook task every 250 ms
 */
static void power_led(void)
{
	static unsigned power_ticks;
	static int previous_state_suspend;

	power_ticks++;

	/* If we don't control the LED, nothing to do */
	if (!led_auto_control_is_enabled(EC_LED_ID_POWER_LED))
		return;

	if (chipset_in_state(CHIPSET_STATE_SUSPEND)) {
		if (!previous_state_suspend)
			power_ticks = 0;

	/* If chipset state in suspend, blink orange, 25% duty cycle,
	 * 4 sec period.
	 */
			set_color_power_led((power_ticks % 16) < 4 ?
				     LED_ORANGE : LED_OFF);
			previous_state_suspend = 1;
			return;
	}

	previous_state_suspend = 0;

	if (chipset_in_state(CHIPSET_STATE_ANY_OFF))
		set_color_power_led(LED_OFF);
	else if (chipset_in_state(CHIPSET_STATE_ON))
		set_color_power_led(LED_BLUE);

}

static void battery_led(void)
{
	struct batt_params batt;
	static unsigned battery_ticks;
	int chstate = charge_get_state();
	battery_ticks++;

	battery_get_params(&batt);
	/* If we don't control the LED, nothing to do */
	if (!led_auto_control_is_enabled(EC_LED_ID_BATTERY_LED))
		return;

	/* If Battery critical Low, blink orange, 50% duty cycle,
	 * 2 sec period.
	 */
	if (!extpower_is_present() && chipset_in_state(CHIPSET_STATE_ON) &&
	    (batt.state_of_charge <= CONFIG_BATTERY_LEVEL_CRITICAL)) {
		set_color_battery_led((battery_ticks & 0x4) ?
				      LED_ORANGE : LED_OFF);
		return;
	}

	/* If Battery Low, blink orange, 25% duty cycle, 4 sec period */
	if (!extpower_is_present() && chipset_in_state(CHIPSET_STATE_ON) &&
	    (batt.state_of_charge <= CONFIG_BATTERY_LEVEL_LOW)) {
		set_color_battery_led((battery_ticks % 16) < 4 ?
				      LED_ORANGE : LED_OFF);
		return;
	}

	/* If the system is charging, solid orange */
	if (chstate == PWR_STATE_CHARGE) {
		set_color_battery_led(LED_ORANGE);
		return;
	}

	/* If AC connected and fully charged (or close to it), solid blue */
	if (chstate == PWR_STATE_CHARGE_NEAR_FULL ||
	    chstate == PWR_STATE_IDLE) {
		set_color_battery_led(LED_BLUE);
		return;
	}

	/* Otherwise, system is off and AC not connected, LED off */
	set_color_battery_led(LED_OFF);
}

static void led_tick(void)
{
	battery_led();
	power_led();
}
DECLARE_HOOK(HOOK_TICK, led_tick, HOOK_PRIO_DEFAULT);
