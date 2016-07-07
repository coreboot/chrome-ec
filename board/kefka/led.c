/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Power/Battery LED control for Kefka
 */

#include "charge_state.h"
#include "chipset.h"
#include "console.h"
#include "extpower.h"
#include "gpio.h"
#include "hooks.h"
#include "led_common.h"
#include "pwm.h"
#include "registers.h"
#include "util.h"

#define CPRINTF(format, args...) cprintf(CC_PWM, format, ## args)
#define CPRINTS(format, args...) cprints(CC_PWM, format, ## args)

#define LED_BAT_LOW_LEVEL	15	/* Battery low level: 15% */
#define LED_TOTAL_TICKS 16
#define LED_ON_TICKS 4

#define LED_FORCE_IDLE_FREQ	4	/* LED on 1000ms every 2000ms */

#define BAT_LED_ON 0
#define BAT_LED_OFF 1

static int led_debug;

enum led_color {
	LED_OFF = 0,
	LED_WHITE,
	LED_AMBER,

	/* Number of colors, not a color itself */
	LED_COLOR_COUNT
};

const enum ec_led_id supported_led_ids[] = {
	EC_LED_ID_BATTERY_LED};
const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

static int kefka_led_set_color_battery(enum led_color color)
{
	switch (color) {
	case LED_OFF:
		gpio_set_level(GPIO_BAT_LED_WHITE, BAT_LED_OFF);
		gpio_set_level(GPIO_BAT_LED_AMBER, BAT_LED_OFF);
		break;
	case LED_WHITE:
		gpio_set_level(GPIO_BAT_LED_WHITE, BAT_LED_ON);
		gpio_set_level(GPIO_BAT_LED_AMBER, BAT_LED_OFF);
		break;
	case LED_AMBER:
		gpio_set_level(GPIO_BAT_LED_WHITE, BAT_LED_OFF);
		gpio_set_level(GPIO_BAT_LED_AMBER, BAT_LED_ON);
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}
	return EC_SUCCESS;
}

static int kefka_led_set_color(enum ec_led_id led_id, enum led_color color)
{
	int rv;
	led_auto_control(led_id, 0);
	switch (led_id) {
	case EC_LED_ID_BATTERY_LED:
		rv = kefka_led_set_color_battery(color);
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}
	return rv;
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
		if (brightness[EC_LED_COLOR_WHITE] != 0)
			kefka_led_set_color(led_id, LED_WHITE);
		else if (brightness[EC_LED_COLOR_AMBER] != 0)
			kefka_led_set_color(led_id, LED_AMBER);
		else
			kefka_led_set_color(led_id, LED_OFF);

	return EC_SUCCESS;
}

void led_get_brightness_range(enum ec_led_id led_id, uint8_t *brightness_range)
{

	/* Ignoring led_id as both leds support the same colors */
	brightness_range[EC_LED_COLOR_WHITE] = 1;
	brightness_range[EC_LED_COLOR_AMBER] = 1;

}

static void kefka_led_set_battery(void)
{
	static int battery_ticks;
	uint32_t chflags = charge_get_flags();

	battery_ticks++;

	switch (charge_get_state()) {
	case PWR_STATE_CHARGE:
		kefka_led_set_color_battery(LED_WHITE);
		break;
	case PWR_STATE_CHARGE_NEAR_FULL:
		kefka_led_set_color_battery(LED_OFF);
		break;
	case PWR_STATE_DISCHARGE:
		if (chipset_in_state(CHIPSET_STATE_ON) ||
		    chipset_in_state(CHIPSET_STATE_SUSPEND))
			kefka_led_set_color_battery(
			(charge_get_percent() < LED_BAT_LOW_LEVEL) ?
				LED_AMBER : LED_OFF);
		else
			kefka_led_set_color_battery(LED_OFF);
		break;
	case PWR_STATE_ERROR:
		kefka_led_set_color_battery(
			(battery_ticks % LED_TOTAL_TICKS < LED_ON_TICKS) ?
				LED_AMBER : LED_OFF);
		break;
	case PWR_STATE_IDLE:
		if (chflags & CHARGE_FLAG_FORCE_IDLE)
			kefka_led_set_color_battery(
			(battery_ticks & LED_FORCE_IDLE_FREQ) ?
				LED_WHITE : LED_OFF);
		else
			kefka_led_set_color_battery(LED_OFF);
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
		kefka_led_set_battery();
}
DECLARE_HOOK(HOOK_TICK, led_tick, HOOK_PRIO_DEFAULT);

/******************************************************************/
/* Console commands */
static int command_led_color(int argc, char **argv)
{
	if (argc > 1) {
		if (!strcasecmp(argv[1], "debug")) {
			led_debug ^= 1;
			CPRINTF("led_debug = %d\n", led_debug);
		} else if (!strcasecmp(argv[1], "off")) {
			kefka_led_set_color_battery(LED_OFF);
		} else if (!strcasecmp(argv[1], "white")) {
			kefka_led_set_color_battery(LED_WHITE);
		} else if (!strcasecmp(argv[1], "amber")) {
			kefka_led_set_color_battery(LED_AMBER);
		} else {
			/* maybe handle charger_discharge_on_ac() too? */
			return EC_ERROR_PARAM1;
		}
	}

	return EC_SUCCESS;
}
DECLARE_CONSOLE_COMMAND(ledcolor, command_led_color,
			"[debug|white|amber|off]",
			"Change LED color",
			NULL);

