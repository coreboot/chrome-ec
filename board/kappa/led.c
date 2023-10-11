/* Copyright 2020 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Power and battery LED control for Kappa
 */

#include "charge_state.h"
#include "extpower.h"
#include "hooks.h"
#include "ioexpander.h"
#include "led_common.h"

#define BAT_LED_ON 0
#define BAT_LED_OFF 1

const enum ec_led_id supported_led_ids[] = { EC_LED_ID_BATTERY_LED };

const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

enum led_color {
	LED_OFF = 0,
	LED_AMBER,
	LED_WHITE,
	LED_COLOR_COUNT /* Number of colors, not a color itself */
};

static int led_set_color_battery(enum led_color color)
{
	switch (color) {
	case LED_OFF:
		ioex_set_level(IOEX_BAT_LED_WHITE_L, BAT_LED_OFF);
		ioex_set_level(IOEX_BAT_LED_AMBER_L, BAT_LED_OFF);
		break;
	case LED_WHITE:
		ioex_set_level(IOEX_BAT_LED_WHITE_L, BAT_LED_ON);
		ioex_set_level(IOEX_BAT_LED_AMBER_L, BAT_LED_OFF);
		break;
	case LED_AMBER:
		ioex_set_level(IOEX_BAT_LED_WHITE_L, BAT_LED_OFF);
		ioex_set_level(IOEX_BAT_LED_AMBER_L, BAT_LED_ON);
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}
	return EC_SUCCESS;
}

void led_get_brightness_range(enum ec_led_id led_id, uint8_t *brightness_range)
{
	brightness_range[EC_LED_COLOR_WHITE] = 1;
	brightness_range[EC_LED_COLOR_AMBER] = 1;
}

static int led_set_color(enum ec_led_id led_id, enum led_color color)
{
	int rv;

	switch (led_id) {
	case EC_LED_ID_BATTERY_LED:
		rv = led_set_color_battery(color);
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}
	return rv;
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
	if (brightness[EC_LED_COLOR_WHITE] != 0)
		led_set_color(led_id, LED_WHITE);
	else if (brightness[EC_LED_COLOR_AMBER] != 0)
		led_set_color(led_id, LED_AMBER);
	else
		led_set_color(led_id, LED_OFF);

	return EC_SUCCESS;
}

static void led_set_battery(void)
{
	static int battery_ticks;
	static int power_ticks;

	battery_ticks++;

	/* override battery led for system suspend */
	if (chipset_in_state(CHIPSET_STATE_SUSPEND | CHIPSET_STATE_STANDBY) &&
	    led_pwr_get_state() != LED_PWRS_CHARGE) {
		led_set_color_battery(power_ticks++ & 0x2 ? LED_WHITE :
							    LED_OFF);
		return;
	}

	power_ticks = 0;

	switch (led_pwr_get_state()) {
	case LED_PWRS_CHARGE:
		led_set_color_battery(LED_AMBER);
		break;
	case LED_PWRS_DISCHARGE_FULL:
		if (extpower_is_present()) {
			led_set_color_battery(LED_WHITE);
			break;
		}
		__fallthrough;
	case LED_PWRS_DISCHARGE:
		/*
		 * Blink white light (1 sec on, 1 sec off)
		 * when battery capacity is less than 10%
		 */
		if (charge_get_percent() < 10)
			led_set_color_battery(
				(battery_ticks & 0x2) ? LED_WHITE : LED_OFF);
		else
			led_set_color_battery(LED_OFF);
		break;
	case LED_PWRS_ERROR:
		led_set_color_battery((battery_ticks % 0x2) ? LED_WHITE :
							      LED_OFF);
		break;
	case LED_PWRS_CHARGE_NEAR_FULL:
		led_set_color_battery(LED_WHITE);
		break;
	case LED_PWRS_IDLE: /* External power connected in IDLE */
		led_set_color_battery(LED_WHITE);
		break;
	case LED_PWRS_FORCED_IDLE:
		led_set_color_battery((battery_ticks & 0x2) ? LED_AMBER :
							      LED_OFF);
		break;
	default:
		/* Other states don't alter LED behavior */
		break;
	}
}

/* Called by hook task every TICK */
static void led_tick(void)
{
	if (led_auto_control_is_enabled(EC_LED_ID_BATTERY_LED))
		led_set_battery();
}
DECLARE_HOOK(HOOK_TICK, led_tick, HOOK_PRIO_DEFAULT);
