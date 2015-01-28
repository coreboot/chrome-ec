/* Copyright (c) The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery LED control for mighty
 */

#include "gpio.h"
#include "hooks.h"
#include "battery.h"
#include "charge_state.h"
#include "chipset.h"
#include "led_common.h"
#include "util.h"

/*
* ----------------------- The LEDs behavior of mighty -----------------------
* Power status | the behavior of AC and Battery   |   Red  | Orange |  Green
* Power On     | Adapter in, no battery           |   on   |        |
*              | Adapter in，battery charging     |        |   on   |
*              | Adapter in，battery full charge  |        |        |   on
*              | Only battery，capacity above 10% |        |        |   on
*              | Only battery，capacity below 10% |  blink |        |
* Power Off    | Adapter in, no battery           | ---------- off ----------
*              | Adapter in，battery charging     |        |   on   |
*              | Adapter in，battery full charge  |        |        |   on
*              | Only battery，capacity above 10% | ---------- off ----------
*              | Only battery，capacity below 10% | ---------- off ----------
* Suspend      | Adapter in, no battery           |   on   |        |
*              | Adapter in，battery charging     |        |  blink |
*              | Adapter in，battery full chargee |        |        |   on
*              | Only battery，capacity above 10% |        |        |  blink
*              | Only battery，capacity below 10% |  blink |        |
*/

#ifdef CONFIG_LED_BAT_ACTIVE_LOW
#define BAT_LED_ON 0
#define BAT_LED_OFF 1
#else
#define BAT_LED_ON 1
#define BAT_LED_OFF 0
#endif

const enum ec_led_id supported_led_ids[] = {
	EC_LED_ID_BATTERY_LED,
};

const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

enum led_color {
	LED_OFF = 0,
	LED_RED,
	LED_AMBER,
	LED_GREEN,
	LED_COLOR_COUNT  /* Number of colors, not a color itself */
};

static int battery_second;

static int bat_led_set_color(enum led_color color)
{
	switch (color) {
	case LED_OFF:
		gpio_set_level(GPIO_BAT_LED_GREEN, BAT_LED_OFF);
		gpio_set_level(GPIO_BAT_LED_RED, BAT_LED_OFF);
		break;
	case LED_RED:
		gpio_set_level(GPIO_BAT_LED_GREEN, BAT_LED_OFF);
		gpio_set_level(GPIO_BAT_LED_RED, BAT_LED_ON);
		break;
	case LED_AMBER:
		gpio_set_level(GPIO_BAT_LED_GREEN, BAT_LED_ON);
		gpio_set_level(GPIO_BAT_LED_RED, BAT_LED_ON);
		break;
	case LED_GREEN:
		gpio_set_level(GPIO_BAT_LED_GREEN, BAT_LED_ON);
		gpio_set_level(GPIO_BAT_LED_RED, BAT_LED_OFF);
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
		brightness_range[EC_LED_COLOR_RED] = 1;
		brightness_range[EC_LED_COLOR_GREEN] = 1;
		break;
	default:
		/* ignore */
		break;
	}
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
	switch (led_id) {
	case EC_LED_ID_BATTERY_LED:
		gpio_set_level(GPIO_BAT_LED_RED,
			       (brightness[EC_LED_COLOR_RED] != 0) ?
					BAT_LED_ON : BAT_LED_OFF);
		gpio_set_level(GPIO_BAT_LED_GREEN,
			       (brightness[EC_LED_COLOR_GREEN] != 0) ?
					BAT_LED_ON : BAT_LED_OFF);
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}
	return EC_SUCCESS;
}

static void mighty_led_set_battery_poweron(void)
{
	switch (charge_get_state()) {
	case PWR_STATE_CHARGE:
		bat_led_set_color(LED_AMBER);
		break;
	case PWR_STATE_DISCHARGE:
		if (charge_get_percent() < 10)
			bat_led_set_color((battery_second & 0x1)
					? LED_OFF : LED_RED);
		else
			bat_led_set_color(LED_GREEN);
		break;
	case PWR_STATE_ERROR: /* Battery has been removed and AC input now. */
		bat_led_set_color(LED_RED);
		break;
	case PWR_STATE_IDLE: /* External power connected in IDLE. */
		/* Fall through. */
	case PWR_STATE_CHARGE_NEAR_FULL:
		bat_led_set_color(LED_GREEN);
		break;
	default:
		/* Other states don't alter LED behavior */
		break;
	}
}

static void mighty_led_set_battery_poweroff(void)
{
	switch (charge_get_state()) {
	case PWR_STATE_CHARGE:
		bat_led_set_color(LED_AMBER);
		break;
	case PWR_STATE_CHARGE_NEAR_FULL:
		bat_led_set_color(LED_GREEN);
		break;
	default:
		/* Close all LEDs */
		bat_led_set_color(LED_OFF);
		break;
	}
}

static void mighty_led_set_battery_suspend(void)
{
	switch (charge_get_state()) {
	case PWR_STATE_CHARGE:
		bat_led_set_color((battery_second & 0x1)
					? LED_OFF : LED_AMBER);
		break;
	case PWR_STATE_DISCHARGE:
		if (charge_get_percent() < 10)
			bat_led_set_color((battery_second & 0x1)
					? LED_OFF : LED_RED);
		else
			bat_led_set_color((battery_second & 0x1)
					? LED_OFF : LED_GREEN);
		break;
	case PWR_STATE_ERROR: /* Battery has been removed and AC input now. */
		bat_led_set_color(LED_RED);
		break;
	case PWR_STATE_IDLE: /* External power connected in IDLE. */
		/* Fall through. */
	case PWR_STATE_CHARGE_NEAR_FULL:
		bat_led_set_color(LED_GREEN);
		break;
	default:
		/* Other states don't alter LED behavior */
		break;
	}
}

static void mighty_led_set_battery(void)
{
	battery_second++;

	if (chipset_in_state(CHIPSET_STATE_ON))
		mighty_led_set_battery_poweron();
	else if (chipset_in_state(CHIPSET_STATE_ANY_OFF))
		mighty_led_set_battery_poweroff();
	else if (chipset_in_state(CHIPSET_STATE_SUSPEND))
		mighty_led_set_battery_suspend();
}

/**  * Called by hook task every 1 sec  */
static void led_second(void)
{
	if (led_auto_control_is_enabled(EC_LED_ID_BATTERY_LED))
		mighty_led_set_battery();
}
DECLARE_HOOK(HOOK_SECOND, led_second, HOOK_PRIO_DEFAULT);
