/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Power/Battery LED control for Caroline
 */

#include "charge_state.h"
#include "chipset.h"
#include "console.h"
#include "extpower.h"
#include "gpio.h"
#include "hooks.h"
#include "led_common.h"
#include "registers.h"
#include "util.h"

#define CPRINTF(format, args...) cprintf(CC_PWM, format, ## args)
#define CPRINTS(format, args...) cprints(CC_PWM, format, ## args)

#define LED_TOTAL_TICKS 16
#define LED_ON_TICKS 8

static int led_debug;

const enum ec_led_id supported_led_ids[] = {
	EC_LED_ID_POWER_LED, EC_LED_ID_BATTERY_LED};
const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

enum led_color {
	LED_OFF = 0,
	LED_RED,
	LED_GREEN,
	LED_BLUE,

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
	gpio_set_level(GPIO_POWER_LED, !(color == LED_BLUE));
	gpio_set_level(GPIO_LED_ACIN, !(color == LED_GREEN));
	gpio_set_level(GPIO_LED_CHARGE, !(color == LED_RED));
}

void led_get_brightness_range(enum ec_led_id led_id, uint8_t *brightness_range)
{
	brightness_range[EC_LED_COLOR_RED] = 1;
	brightness_range[EC_LED_COLOR_BLUE] = 1;
	brightness_range[EC_LED_COLOR_GREEN] = 1;
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
	gpio_set_level(GPIO_POWER_LED, !brightness[EC_LED_COLOR_BLUE]);
	gpio_set_level(GPIO_LED_ACIN, !brightness[EC_LED_COLOR_GREEN]);
	gpio_set_level(GPIO_LED_CHARGE, !brightness[EC_LED_COLOR_RED]);

	return EC_SUCCESS;
}

static void caroline_led_set_power_battery(void)
{
	static int power_ticks;

	if (chipset_in_state(CHIPSET_STATE_ON)) {
		set_color(LED_BLUE);
		return;
	}

	/* CHIPSET_STATE_OFF */
	switch (charge_get_state()) {
	case PWR_STATE_DISCHARGE:
		set_color(LED_OFF);
		break;
	case PWR_STATE_CHARGE:
		set_color(LED_RED);
		break;
	case PWR_STATE_ERROR:
		power_ticks++;
		set_color(((power_ticks % LED_TOTAL_TICKS)
			  < LED_ON_TICKS) ? LED_RED : LED_GREEN);
		break;
	case PWR_STATE_CHARGE_NEAR_FULL:
	case PWR_STATE_IDLE: /* External power connected in IDLE. */
		set_color(LED_GREEN);
		break;
	default:
		set_color(LED_RED);
		break;
	}
	if ((charge_get_state()) != PWR_STATE_ERROR)
		power_ticks = 0;
}

/**
 * Called by hook task every 250 ms
 */
static void led_tick(void)
{
	if (led_debug)
		return;

	if (led_auto_control_is_enabled(EC_LED_ID_POWER_LED) &&
	    led_auto_control_is_enabled(EC_LED_ID_BATTERY_LED)) {
		caroline_led_set_power_battery();
		return;
	}
}
DECLARE_HOOK(HOOK_TICK, led_tick, HOOK_PRIO_DEFAULT);
