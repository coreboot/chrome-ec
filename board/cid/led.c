/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Power and battery LED control for cid.
 */

#include "battery.h"
#include "charge_state.h"
#include "chipset.h"
#include "ec_commands.h"
#include "gpio.h"
#include "hooks.h"
#include "host_command.h"
#include "led_common.h"
#include "util.h"

#define LED_ERROR_FREQ		2	/* LED on 500ms every 1000ms */
#define LED_FORCE_IDLE_FREQ	4	/* LED on 1000ms every 2000ms */
#define LED_BATTERY_LOW_LEVEL	15	/* Battery low level: 15% */

enum led_color {
	LED_OFF = 0,
	LED_WHITE,
	LED_AMBER,
	LED_COLOR_COUNT  /* Number of colors, not a color itself */
};

const enum ec_led_id supported_led_ids[] = {
	EC_LED_ID_BATTERY_LED};

const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

static int cid_led_set_gpio(enum led_color color,
			     enum gpio_signal gpio_led_white_l,
			     enum gpio_signal gpio_led_amber_l)
{
	switch (color) {
	case LED_OFF:
		gpio_set_level(gpio_led_white_l, 1);
		gpio_set_level(gpio_led_amber_l, 1);
		break;
	case LED_WHITE:
		gpio_set_level(gpio_led_white_l, 0);
		gpio_set_level(gpio_led_amber_l, 1);
		break;
	case LED_AMBER:
		gpio_set_level(gpio_led_white_l, 1);
		gpio_set_level(gpio_led_amber_l, 0);
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}
	return EC_SUCCESS;
}

static int cid_led_set_color_battery(enum led_color color)
{
	return cid_led_set_gpio(color, GPIO_BAT_LED0_L, GPIO_BAT_LED1_L);
}

static int cid_led_set_color(enum ec_led_id led_id, enum led_color color)
{
	int rv;

	led_auto_control(led_id, 0);
	switch (led_id) {
	case EC_LED_ID_BATTERY_LED:
		rv = cid_led_set_color_battery(color);
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}
	return rv;
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
	if (brightness[EC_LED_COLOR_WHITE] != 0)
		cid_led_set_color(led_id, LED_WHITE);
	else if (brightness[EC_LED_COLOR_YELLOW] != 0)
		cid_led_set_color(led_id, LED_AMBER);
	else
		cid_led_set_color(led_id, LED_OFF);

	return EC_SUCCESS;
}

void led_get_brightness_range(enum ec_led_id led_id, uint8_t *brightness_range)
{
	/* Ignoring led_id as both leds support the same colors */
	brightness_range[EC_LED_COLOR_WHITE] = 1;
	brightness_range[EC_LED_COLOR_YELLOW] = 1;
}

static void cid_led_set_battery(void)
{
	static int battery_ticks;
	uint32_t chflags = charge_get_flags();

	battery_ticks++;

	switch (charge_get_state()) {
	case PWR_STATE_CHARGE:
		cid_led_set_color_battery(LED_WHITE);
		break;
	case PWR_STATE_CHARGE_NEAR_FULL:
		cid_led_set_color_battery(LED_OFF);
		break;
	case PWR_STATE_DISCHARGE:
		if (chipset_in_state(CHIPSET_STATE_ON) ||
		    chipset_in_state(CHIPSET_STATE_SUSPEND))
			cid_led_set_color_battery(
			(charge_get_percent() < LED_BATTERY_LOW_LEVEL) ?
			LED_AMBER : LED_OFF);
		else
			cid_led_set_color_battery(LED_OFF);
		break;
	case PWR_STATE_ERROR:
		cid_led_set_color_battery(
			(battery_ticks & LED_ERROR_FREQ) ? LED_AMBER : LED_OFF);
		break;
	case PWR_STATE_IDLE: /* External power connected in IDLE. */
		if (chflags & CHARGE_FLAG_FORCE_IDLE)
			cid_led_set_color_battery(
			(battery_ticks & LED_FORCE_IDLE_FREQ) ?
				LED_WHITE : LED_OFF);
		else
			cid_led_set_color_battery(LED_OFF);
		break;
	default:
		/* Other states don't alter LED behavior */
		break;
	}
}

/* Called by hook task every 250mSec */
static void led_tick(void)
{
	if (led_auto_control_is_enabled(EC_LED_ID_BATTERY_LED))
		cid_led_set_battery();
}
DECLARE_HOOK(HOOK_TICK, led_tick, HOOK_PRIO_DEFAULT);
