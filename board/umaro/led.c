/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Power/Battery LED control for Umaro
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

/*
 *  * ----------------- The LEDs behavior of umaro -----------------
 *  * Power status | the behavior of AC and Battery    |   red amber green
 *  * Power On     | Adapter in, no battery            |   red
 *  *              | Adapter in，battery charging      |   amber
 *  *              | Adapter in，battery full charge   |   green
 *  *              | Only battery，capacity above 5%   |   green
 *  *              | Only battery，capacity below 5%   |   amber blink
 *  * Power Off    | Adapter in, no battery            |   red
 *  *              | Adapter in，battery charging      |   amber
 *  *              | Adapter in，battery full charge   |   green
 *  *              | Only battery，capacity above 5%   |   off
 *  *              | Only battery，capacity below 5%   |   off
 *  * Suspend      | Adapter in, no battery            |   red
 *  *              | Adapter in，battery charging      |   amber
 *  *              | Adapter in，battery full chargee  |   green
 *  *              | Only battery，capacity above 5%   |   amber blink
 *  *              | Only battery，capacity below 5%   |   amber blink
 *  */


#define LED_TOTAL_TICKS 16
#define LED_ON_TICKS 4

const enum ec_led_id supported_led_ids[] = {
	 EC_LED_ID_BATTERY_LED};
const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

enum led_color {
	LED_OFF = 0,
	LED_RED,
	LED_AMBER,
	LED_GREEN,

	/* Number of colors, not a color itself */
	LED_COLOR_COUNT
};

/**
 * Set LED color
 *
 * @param color		Enumerated color value
 */
static int set_color(enum led_color color)
{
	switch (color) {
	case LED_OFF:
		gpio_set_level(GPIO_LED_RED, 1);
		gpio_set_level(GPIO_LED_AMBER, 1);
		gpio_set_level(GPIO_LED_GREEN, 1);
		break;
	case LED_RED:
		gpio_set_level(GPIO_LED_RED, 0);
		gpio_set_level(GPIO_LED_AMBER, 1);
		gpio_set_level(GPIO_LED_GREEN, 1);
		break;
	case LED_AMBER:
		gpio_set_level(GPIO_LED_RED, 1);
		gpio_set_level(GPIO_LED_AMBER, 0);
		gpio_set_level(GPIO_LED_GREEN, 1);
		break;
	case LED_GREEN:
		gpio_set_level(GPIO_LED_RED, 1);
		gpio_set_level(GPIO_LED_AMBER, 1);
		gpio_set_level(GPIO_LED_GREEN, 0);
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}
	return EC_SUCCESS;
}

void led_get_brightness_range(enum ec_led_id led_id, uint8_t *brightness_range)
{
	brightness_range[EC_LED_COLOR_RED] = 1;
	brightness_range[EC_LED_COLOR_YELLOW] = 1;
	brightness_range[EC_LED_COLOR_GREEN] = 1;
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
	switch (led_id) {
	case EC_LED_ID_BATTERY_LED:
		if (brightness[EC_LED_COLOR_RED] != 0)
			set_color(LED_RED);
		else if (brightness[EC_LED_COLOR_YELLOW] != 0)
			set_color(LED_AMBER);
		else if (brightness[EC_LED_COLOR_GREEN] != 0)
			set_color(LED_GREEN);
		else
			set_color(LED_OFF);
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}
	return EC_SUCCESS;
}

static void umaro_led_set_power(void)
{
	static int power_ticks;
	static int previous_state_suspend;

	power_ticks++;

	if (chipset_in_state(CHIPSET_STATE_SUSPEND)) {
		/* Reset ticks if entering suspend so LED turns amber
		 * as soon as possible. */
		if (!previous_state_suspend)
			power_ticks = 0;

		/* Blink once every four seconds. */
		set_color(
			(power_ticks % LED_TOTAL_TICKS) < LED_ON_TICKS ?
			LED_AMBER : LED_OFF);

		previous_state_suspend = 1;
		return;
	}

	previous_state_suspend = 0;

	if (chipset_in_state(CHIPSET_STATE_ANY_OFF))
		set_color(LED_OFF);
	else if (chipset_in_state(CHIPSET_STATE_ON)) {
			if (charge_get_percent() < 5)
				set_color(
					(power_ticks % LED_TOTAL_TICKS) < LED_ON_TICKS ?
					LED_AMBER : LED_OFF);
			else
				set_color(LED_GREEN);
	}
}

static void umaro_led_set_battery(void)
{
	static int battery_ticks;

	battery_ticks++;

	switch (charge_get_state()) {
	case PWR_STATE_CHARGE:
		set_color(LED_AMBER);
		break;
	case PWR_STATE_ERROR:
		set_color(LED_RED);
		break;
	case PWR_STATE_CHARGE_NEAR_FULL:
	case PWR_STATE_IDLE: /* External power connected in IDLE. */
		set_color(LED_GREEN);
		break;
	default:
		/* Other states don't alter LED behavior */
		break;
	}
}

static void led_init(void)
{
	set_color(LED_OFF);
}
DECLARE_HOOK(HOOK_INIT, led_init, HOOK_PRIO_DEFAULT);

/**
 * Called by hook task every 250 ms
 */
static void led_tick(void)
{
	if (led_auto_control_is_enabled(EC_LED_ID_BATTERY_LED)) {
		if (extpower_is_present()) {
			umaro_led_set_battery();
			return;
		} else {
			umaro_led_set_power();
			return;
		}
	}
}
DECLARE_HOOK(HOOK_TICK, led_tick, HOOK_PRIO_DEFAULT);

/******************************************************************/
/* Console commands */
static int command_led_color(int argc, char **argv)
{
	if (argc > 1) {
		if (!strcasecmp(argv[1], "off")) {
			set_color(LED_OFF);
		} else if (!strcasecmp(argv[1], "red")) {
			set_color(LED_RED);
		} else if (!strcasecmp(argv[1], "green")) {
			set_color(LED_GREEN);
		} else if (!strcasecmp(argv[1], "amber")) {
			set_color(LED_AMBER);
		} else {
			/* maybe handle charger_discharge_on_ac() too? */
			return EC_ERROR_PARAM1;
		}
	}
	return EC_SUCCESS;
}
DECLARE_CONSOLE_COMMAND(ledcolor, command_led_color,
			"[red|green|amber|off]",
			"Change LED color",
			NULL);

