/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery LED control for Kip
 */

#include "charge_state.h"
#include "chipset.h"
#include "extpower.h"
#include "gpio.h"
#include "hooks.h"
#include "led_common.h"
#include "util.h"

const enum ec_led_id supported_led_ids[] = {
	EC_LED_ID_BATTERY_LED, EC_LED_ID_POWER_LED};
const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

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
		gpio_set_level(GPIO_BAT_LED0, 0);
		gpio_set_level(GPIO_BAT_LED1, 0);
		break;
	case LED_WHITE:
		gpio_set_level(GPIO_BAT_LED0, 1);
		gpio_set_level(GPIO_BAT_LED1, 0);
		break;
	case LED_AMBER:
		gpio_set_level(GPIO_BAT_LED0, 0);
		gpio_set_level(GPIO_BAT_LED1, 1);
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}
	return EC_SUCCESS;
}

static int pwr_led_set_color(enum led_color color)
{
	switch (color) {
	case LED_OFF:
		gpio_set_level(GPIO_PWR_LED_L, 1);
		break;
	case LED_WHITE:
		gpio_set_level(GPIO_PWR_LED_L, 0);
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}
	return EC_SUCCESS;
}

void led_get_brightness_range(enum ec_led_id led_id, uint8_t *brightness_range)
{
	switch (led_id) {
	case EC_LED_ID_BATTERY_LED:
		brightness_range[EC_LED_COLOR_WHITE] = 100;
		brightness_range[EC_LED_COLOR_YELLOW] = 100;
		break;
	case EC_LED_ID_POWER_LED:
		brightness_range[EC_LED_COLOR_WHITE] = 100;
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
		if (brightness[EC_LED_COLOR_WHITE] != 0)
			bat_led_set_color(LED_WHITE);
		else if (brightness[EC_LED_COLOR_YELLOW] != 0)
			bat_led_set_color(LED_AMBER);
		else
			bat_led_set_color(LED_OFF);
		break;
	case EC_LED_ID_POWER_LED:
		if (brightness[EC_LED_COLOR_WHITE] != 0)
			pwr_led_set_color(LED_WHITE);
		else
			pwr_led_set_color(LED_OFF);
		break;
	default:
		break;
	}
	return EC_SUCCESS;
}

static void kip_led_set_power(void)
{
	static int power_ticks;
	static int previous_state_suspend;

	power_ticks++;

	if (chipset_in_state(CHIPSET_STATE_SUSPEND)) {
		/* Reset ticks if entering suspend so LED turns amber
		 * as soon as possible. */
		if (!previous_state_suspend)
			power_ticks = 0;

		/* Blink once every one second. */
		pwr_led_set_color((power_ticks & 0x4) ? LED_WHITE : LED_OFF);

		previous_state_suspend = 1;
		return;
	}

	previous_state_suspend = 0;

	if (chipset_in_state(CHIPSET_STATE_ANY_OFF))
		pwr_led_set_color(LED_OFF);
	else if (chipset_in_state(CHIPSET_STATE_ON))
		pwr_led_set_color(LED_WHITE);
}

static void kip_led_set_battery(void)
{
	static int battery_ticks;
	uint32_t chflags = charge_get_flags();

	battery_ticks++;

	switch (charge_get_state()) {
	case PWR_STATE_CHARGE:
		bat_led_set_color(LED_AMBER);
		break;
	case PWR_STATE_CHARGE_NEAR_FULL:
		bat_led_set_color(LED_WHITE);
		break;
	case PWR_STATE_DISCHARGE:
		/* See crosbug.com/p/22159. There's a 3% difference
		 * between the battery level seen by the kernel and what's
		 * really going on, so if they want to see 12%, we use 15%.
		 * Hard code this number here, because this only affects the
		 * LED color, not the battery charge state. */
		if (charge_get_percent() < 15)
			bat_led_set_color(
				(battery_ticks & 0x4) ? LED_WHITE : LED_OFF);
		else
			bat_led_set_color(LED_OFF);
		break;
	case PWR_STATE_ERROR:
		bat_led_set_color((battery_ticks & 0x2) ? LED_WHITE : LED_OFF);
		break;
	case PWR_STATE_IDLE:
		if (chflags & CHARGE_FLAG_FORCE_IDLE)
			bat_led_set_color(
				(battery_ticks & 0x4) ? LED_AMBER : LED_OFF);
		else
			bat_led_set_color(LED_WHITE);
		break;
	default:
		/* Other states don't alter LED behavior */
		break;
	}
}

/* Called by hook task every 250mSec */
static void led_tick(void)
{
	if (led_auto_control_is_enabled(EC_LED_ID_POWER_LED))
		kip_led_set_power();

	if (led_auto_control_is_enabled(EC_LED_ID_BATTERY_LED))
		kip_led_set_battery();
}
DECLARE_HOOK(HOOK_TICK, led_tick, HOOK_PRIO_DEFAULT);
