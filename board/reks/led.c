/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Power/Battery LED control for Reks
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

#define BAT_LED_ON 0
#define BAT_LED_OFF 1

static int led_debug;

const enum ec_led_id supported_led_ids[] = {
	EC_LED_ID_POWER_LED, EC_LED_ID_BATTERY_LED};
const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

enum led_color {
	LED_OFF = 0,
	LED_AMBER,
	LED_GREEN,

	/* Number of colors, not a color itself */
	LED_COLOR_COUNT
};

static int bat_led_set_color(enum led_color color)
{
	switch (color) {
	case LED_OFF:
		gpio_set_level(GPIO_BAT_LED_GREEN, BAT_LED_OFF);
		gpio_set_level(GPIO_BAT_LED_AMBER, BAT_LED_OFF);
		break;
	case LED_AMBER:
		gpio_set_level(GPIO_BAT_LED_GREEN, BAT_LED_OFF);
		gpio_set_level(GPIO_BAT_LED_AMBER, BAT_LED_ON);
		break;
	case LED_GREEN:
		gpio_set_level(GPIO_BAT_LED_GREEN, BAT_LED_ON);
		gpio_set_level(GPIO_BAT_LED_AMBER, BAT_LED_OFF);
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}
	return EC_SUCCESS;
}

void led_get_brightness_range(enum ec_led_id led_id, uint8_t *brightness_range)
{
	brightness_range[EC_LED_COLOR_RED] = 1;
	brightness_range[EC_LED_COLOR_GREEN] = 1;
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
	switch (led_id) {
	case EC_LED_ID_BATTERY_LED:
		if (brightness[EC_LED_COLOR_GREEN] != 0)
			bat_led_set_color(LED_GREEN);
		else if (brightness[EC_LED_COLOR_YELLOW] != 0)
			bat_led_set_color(LED_AMBER);
		else
			bat_led_set_color(LED_OFF);
		break;
	default:
		break;
	}
	return EC_SUCCESS;
}

/**
 * Called by hook task every 250 ms
 */
static void led_tick(void)
{
	static unsigned ticks;
	int chstate = charge_get_state();

	ticks++;

	if (led_debug)
		return;

	/* If we don't control the LED, nothing to do */
	if (!led_auto_control_is_enabled(EC_LED_ID_BATTERY_LED))
		return;

	/* If charging error, blink orange, 25% duty cycle, 4 sec period */
	if (chstate == PWR_STATE_ERROR) {
		bat_led_set_color((ticks % 16) < 4 ? LED_AMBER : LED_OFF);
		return;
	}

	/* If charge-force-idle, blink green, 50% duty cycle, 2 sec period */
	if (chstate == PWR_STATE_IDLE &&
	   (charge_get_flags() & CHARGE_FLAG_FORCE_IDLE)) {
		bat_led_set_color((ticks & 0x4) ? LED_GREEN : LED_OFF);
		return;
	}

	/* If the system is charging, solid orange */
	if (chstate == PWR_STATE_CHARGE) {
		bat_led_set_color(LED_AMBER);
		return;
	}

	/* If AC connected and fully charged (or close to it), solid green */
	if (chstate == PWR_STATE_CHARGE_NEAR_FULL ||
	   chstate == PWR_STATE_IDLE) {
		bat_led_set_color(LED_GREEN);
		return;
	}

	/* Otherwise, system is off and AC not connected, LED off */
	bat_led_set_color(LED_OFF);
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
			bat_led_set_color(LED_OFF);
		} else if (!strcasecmp(argv[1], "green")) {
			bat_led_set_color(LED_GREEN);
		} else if (!strcasecmp(argv[1], "amber")) {
			bat_led_set_color(LED_AMBER);
		} else {
			/* maybe handle charger_discharge_on_ac() too? */
			return EC_ERROR_PARAM1;
		}
	}

	return EC_SUCCESS;
}
DECLARE_CONSOLE_COMMAND(ledcolor, command_led_color,
			"[debug|green|amber|off]",
			"Change LED color",
			NULL);

