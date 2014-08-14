/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Power LED control for Kitty
 */

#include "gpio.h"
#include "hooks.h"
#include "chipset.h"
#include "led_common.h"
#include "util.h"

const enum ec_led_id supported_led_ids[] = {EC_LED_ID_POWER_LED};

const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

enum led_color {
	LED_OFF = 0,
	LED_WHITE,
	LED_COLOR_COUNT  /* Number of colors, not a color itself */
};

static int pwr_led_set_color(enum led_color color)
{
	switch (color) {
	case LED_OFF:
		gpio_set_level(GPIO_PWR_LED0, 0);
		break;
	case LED_WHITE:
		gpio_set_level(GPIO_PWR_LED0, 1);
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}
	return EC_SUCCESS;
}

void led_get_brightness_range(enum ec_led_id led_id, uint8_t *brightness_range)
{
	brightness_range[EC_LED_COLOR_WHITE] = 1;
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
	switch (led_id) {
	case EC_LED_ID_POWER_LED:
		if (brightness[EC_LED_COLOR_WHITE] != 0)
			pwr_led_set_color(LED_WHITE);
		else
			pwr_led_set_color(LED_OFF);
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}
	return EC_SUCCESS;
}

static void kitty_led_set_power(void)
{
	static int power_second;

	power_second++;

	/* PWR LED behavior:
	 * Power on: White
	 * Suspend: White in breeze mode ( 1 sec on/ 3 sec off)
	 * Power off: OFF
	 */
	if (chipset_in_state(CHIPSET_STATE_ANY_OFF))
		pwr_led_set_color(LED_OFF);
	else if (chipset_in_state(CHIPSET_STATE_ON))
		pwr_led_set_color(LED_WHITE);
	else if (chipset_in_state(CHIPSET_STATE_SUSPEND))
		pwr_led_set_color((power_second & 3) ? LED_OFF : LED_WHITE);
}

/**  * Called by hook task every 1 sec  */
static void led_second(void)
{
	if (led_auto_control_is_enabled(EC_LED_ID_POWER_LED))
		kitty_led_set_power();
}
DECLARE_HOOK(HOOK_SECOND, led_second, HOOK_PRIO_DEFAULT);

