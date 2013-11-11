/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Power and battery LED control for Wolf.
 */

#include "battery.h"
#include "charge_state.h"
#include "chipset.h"
#include "gpio.h"
#include "hooks.h"

enum led_color {
	LED_OFF = 0,
	LED_WHITE,
	LED_AMBER,
	LED_COLOR_COUNT  /* Number of colors, not a color itself */
};

static int bat_led_set_color(enum led_color color)
{
	switch (color) {
	case LED_OFF:
		gpio_set_level(GPIO_BAT_LED0_L, 1);
		gpio_set_level(GPIO_BAT_LED1_L, 1);
		break;
	case LED_WHITE:
		gpio_set_level(GPIO_BAT_LED0_L, 0);
		gpio_set_level(GPIO_BAT_LED1_L, 1);
		break;
	case LED_AMBER:
		gpio_set_level(GPIO_BAT_LED0_L, 1);
		gpio_set_level(GPIO_BAT_LED1_L, 0);
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}
	return EC_SUCCESS;
}

/* Called by hook task every 250mSec */
static void led_tick(void)
{
	static int ticks;
	uint32_t chflags = charge_get_flags();

	ticks++;

	switch (charge_get_state()) {
	case PWR_STATE_CHARGE:
		bat_led_set_color(LED_WHITE);
		break;
	case PWR_STATE_CHARGE_NEAR_FULL:
		bat_led_set_color(LED_OFF);
		break;
	case PWR_STATE_DISCHARGE:
		if (chipset_in_state(CHIPSET_STATE_ON) ||
		    chipset_in_state(CHIPSET_STATE_SUSPEND))
			bat_led_set_color(
			 (charge_get_percent() < BATTERY_LEVEL_LOW) ?
			  LED_AMBER : LED_OFF);
		else
			bat_led_set_color(LED_OFF);
		break;
	case PWR_STATE_ERROR:
		bat_led_set_color((ticks & 0x2) ? LED_AMBER : LED_OFF);
		break;
	case PWR_STATE_IDLE:
		if (chflags & CHARGE_FLAG_FORCE_IDLE)
			bat_led_set_color((ticks & 0x4) ? LED_WHITE : LED_OFF);
		else
			bat_led_set_color(LED_OFF);
		break;
	default:
		/* Other states don't alter LED behavior */
		break;
	}
}
DECLARE_HOOK(HOOK_TICK, led_tick, HOOK_PRIO_DEFAULT);

