/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Standard Battery LED and Power LED control
 * This assumes a red/green battery led and a single power led.
 */

#include "gpio.h"
#include "hooks.h"
#include "battery.h"
#include "charge_state.h"
#include "chipset.h"
#include "led_common.h"
#include "util.h"
#include "lid_switch.h"

#ifdef CONFIG_LED_BAT_ACTIVE_LOW
#define BAT_LED_ON 0
#define BAT_LED_OFF 1
#else
#define BAT_LED_ON 1
#define BAT_LED_OFF 0
#endif

#ifdef CONFIG_LED_POWER_ACTIVE_LOW
#define POWER_LED_ON 0
#define POWER_LED_OFF 1
#else
#define POWER_LED_ON 1
#define POWER_LED_OFF 0
#endif

const enum ec_led_id supported_led_ids[] = {
#ifdef CONFIG_LED_POLICY_STD_BATTERY
	EC_LED_ID_BATTERY_LED,
#endif
#ifdef CONFIG_LED_POLICY_STD_POWER
	EC_LED_ID_POWER_LED
#endif
};

const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

enum led_color {
	LED_OFF = 0,
	LED_RED,
	LED_AMBER,
	LED_GREEN,
	LED_WHITE,
	LED_COLOR_COUNT  /* Number of colors, not a color itself */
};

#ifndef CONFIG_LED_POWER_COLOR
#define CONFIG_LED_POWER_COLOR WHITE
#endif

#define EC_LED_COLOR_POWER CONCAT2(EC_LED_COLOR_,CONFIG_LED_POWER_COLOR)
#define POWER_LED_COLOR CONCAT2(LED_,CONFIG_LED_POWER_COLOR)

#ifdef CONFIG_LED_POLICY_STD_BATTERY
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
#endif

#ifdef CONFIG_LED_POLICY_STD_POWER
static int pwr_led_set_color(enum led_color color)
{
	switch (color) {
	case LED_OFF:
		gpio_set_level(GPIO_POWER_LED, POWER_LED_OFF);
		break;
	case POWER_LED_COLOR:
#ifdef CONFIG_LED_POWER_IS_EXTERNAL
		gpio_set_level(GPIO_POWER_LED, POWER_LED_ON);
#else
		gpio_set_level(GPIO_POWER_LED,
			       lid_is_open() ? POWER_LED_ON : POWER_LED_OFF);
#endif
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}
	return EC_SUCCESS;
}
#endif

void led_get_brightness_range(enum ec_led_id led_id, uint8_t *brightness_range)
{
	switch (led_id) {
#ifdef CONFIG_LED_POLICY_STD_BATTERY
	case EC_LED_ID_BATTERY_LED:
		brightness_range[EC_LED_COLOR_RED] = 1;
		brightness_range[EC_LED_COLOR_GREEN] = 1;
		break;
#endif
#ifdef CONFIG_LED_POLICY_STD_POWER
	case EC_LED_ID_POWER_LED:
		brightness_range[EC_LED_COLOR_POWER] = 1;
		break;
#endif
	default:
		/* ignore */
		break;
	}
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
	switch (led_id) {
#ifdef CONFIG_LED_POLICY_STD_BATTERY
	case EC_LED_ID_BATTERY_LED:
		gpio_set_level(GPIO_BAT_LED_RED,
			       (brightness[EC_LED_COLOR_RED] != 0) ?
					BAT_LED_ON : BAT_LED_OFF);
		gpio_set_level(GPIO_BAT_LED_GREEN,
			       (brightness[EC_LED_COLOR_GREEN] != 0) ?
					BAT_LED_ON : BAT_LED_OFF);
		break;
#endif
#ifdef CONFIG_LED_POLICY_STD_POWER
	case EC_LED_ID_POWER_LED:
		gpio_set_level(GPIO_POWER_LED,
			       (brightness[EC_LED_COLOR_POWER] != 0) ?
					POWER_LED_ON : POWER_LED_OFF);
		break;
#endif
	default:
		return EC_ERROR_UNKNOWN;
	}
	return EC_SUCCESS;
}

#ifdef CONFIG_LED_POLICY_STD_POWER
static void std_led_set_power(void)
{
	static int power_second;

	power_second++;

	if (chipset_in_state(CHIPSET_STATE_ANY_OFF))
		pwr_led_set_color(LED_OFF);
	else if (chipset_in_state(CHIPSET_STATE_ON))
		pwr_led_set_color(POWER_LED_COLOR);
	else if (chipset_in_state(CHIPSET_STATE_SUSPEND))
		pwr_led_set_color((power_second & 3) ? LED_OFF : POWER_LED_COLOR);
}
#endif

#ifdef CONFIG_LED_POLICY_STD_BATTERY
static void std_led_set_battery(void)
{
	static int battery_second;
	uint32_t chflags = charge_get_flags();

	battery_second++;

	/* BAT LED behavior:
	 * Same as the chromeos spec
	 * Green/Amber for CHARGE_FLAG_FORCE_IDLE
	 */
	switch (charge_get_state()) {
	case PWR_STATE_CHARGE:
		bat_led_set_color(LED_AMBER);
		break;
	case PWR_STATE_DISCHARGE:
		if (charge_get_percent() < 3)
			bat_led_set_color((battery_second & 1)
					? LED_OFF : LED_AMBER);
		else if (charge_get_percent() < 10)
			bat_led_set_color((battery_second & 3)
					? LED_OFF : LED_AMBER);
		else
			bat_led_set_color(LED_OFF);
		break;
	case PWR_STATE_ERROR:
		bat_led_set_color((battery_second & 1) ? LED_OFF : LED_RED);
		break;
	case PWR_STATE_CHARGE_NEAR_FULL:
		bat_led_set_color(LED_GREEN);
		break;
	case PWR_STATE_IDLE: /* External power connected in IDLE. */
		if (chflags & CHARGE_FLAG_FORCE_IDLE)
			bat_led_set_color(
				(battery_second & 0x2) ? LED_GREEN : LED_AMBER);
		else
			bat_led_set_color(LED_GREEN);
		break;
	default:
		/* Other states don't alter LED behavior */
		break;
	}
}
#endif

/**  * Called by hook task every 1 sec  */
static void led_second(void)
{
#ifdef CONFIG_LED_POLICY_STD_POWER
	if (led_auto_control_is_enabled(EC_LED_ID_POWER_LED))
		std_led_set_power();
#endif
#ifdef CONFIG_LED_POLICY_STD_BATTERY
	if (led_auto_control_is_enabled(EC_LED_ID_BATTERY_LED))
		std_led_set_battery();
#endif
}
DECLARE_HOOK(HOOK_SECOND, led_second, HOOK_PRIO_DEFAULT);

