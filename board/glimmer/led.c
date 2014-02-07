/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery LED control for Glimmer
 */

#include "charge_state.h"
#include "chipset.h"
#include "extpower.h"
#include "gpio.h"
#include "hooks.h"
#include "led_common.h"
#include "pwm.h"
#include "util.h"

const enum ec_led_id supported_led_ids[] = {EC_LED_ID_POWER_LED};
const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);
static int ac_in_blink_times;

enum led_color {
	LED_OFF = 0,
	LED_RED,

	/* Number of colors, not a color itself */
	LED_COLOR_COUNT
};

/**
 * Set LED color
 *
 * @param color		Enumerated color value
 */
static void set_color(enum led_color color)
{
	pwm_set_duty(PWM_CH_LED_RED, color ? 100 : 0);
}

void led_get_brightness_range(enum ec_led_id led_id, uint8_t *brightness_range)
{
	brightness_range[EC_LED_COLOR_RED] = 100;
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
	pwm_set_duty(PWM_CH_LED_RED, brightness[EC_LED_COLOR_RED]);
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
	set_color(LED_OFF);
}
DECLARE_HOOK(HOOK_INIT, led_init, HOOK_PRIO_DEFAULT);

static void ac_in_blink(void)
{
	if (extpower_is_present())
		ac_in_blink_times = 6;
}
DECLARE_HOOK(HOOK_AC_CHANGE, ac_in_blink, HOOK_PRIO_DEFAULT);

/**
 * Called by hook task every 250 ms
 */
static void led_tick(void)
{
	/* If we don't control the LED, nothing to do */
	if (!led_auto_control_is_enabled(EC_LED_ID_POWER_LED))
		return;

	if (ac_in_blink_times > 0) {
		set_color((ac_in_blink_times & 1) ?
			  LED_OFF : LED_RED);

		ac_in_blink_times--;
		return;
	}

	/* If suspended, breathe the led */
	if (chipset_in_state(CHIPSET_STATE_SUSPEND)) {
		set_color(LED_OFF);
	/* TODO keep led off now, will to be done in next CL */
		return;
	}

	/* If powered on, the led is on */
	if (chipset_in_state(CHIPSET_STATE_ON)) {
		set_color(LED_RED);
		return;
	}

	/* Otherwise, system is off then LED off */
	set_color(LED_OFF);
}
DECLARE_HOOK(HOOK_TICK, led_tick, HOOK_PRIO_DEFAULT);
