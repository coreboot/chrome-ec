/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Power/Battery LED control for wizpig
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
#include "system.h"
#include "util.h"

/*
 * ----------------------- The LEDs behavior of wizpig ------------------------
 * Power status | the behavior of AC and Battery   |   red orange green purple
 * Power On     | Adapter in, no battery           |   red
 *              | Adapter in，battery charging      |   orange
 *              | Adapter in，battery full charge   |   green
 *              | Only battery，capacity above 10%  |   purple
 *              | Only battery，capacity below 10%  |   red blink
 * Power Off    | Adapter in, no battery           |   red
 *              | Adapter in，battery charging      |   orange
 *              | Adapter in，battery full charge   |   green
 *              | Only battery，capacity above 10%  |   off
 *              | Only battery，capacity below 10%  |   off
 * Suspend      | Adapter in, no battery           |   red
 *              | Adapter in，battery charging      |   orange blink
 *              | Adapter in，battery full chargee  |   green
 *              | Only battery，capacity above 10%  |   purple blink
 *              | Only battery，capacity below 10%  |   red blink
 */


#define CPRINTF(format, args...) cprintf(CC_PWM, format, ## args)
#define CPRINTS(format, args...) cprints(CC_PWM, format, ## args)

#define LED_TOTAL_TICKS 16
#define LED_ON_TICKS 4

static int led_debug;
static int battery_second;

const enum ec_led_id supported_led_ids[] = {
	EC_LED_ID_BATTERY_LED};
const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

enum led_color {
	LED_OFF = 0,
	LED_RED,
	LED_ORANGE,
	LED_GREEN,
	LED_BLUE,
	LED_PURPLE,
	LED_YELLOW,
	/* Number of colors, not a color itself */
	LED_COLOR_COUNT
};

/* Brightness vs. color, in the order of off, red, amber, and green */
static const uint8_t color_brightness[LED_COLOR_COUNT][3] = {
			/* {Red, Blue, Green}, */
	[LED_OFF]    =  {  0,   0,   0},
	[LED_RED]    =  {100,   0,   0},
	[LED_ORANGE] =  { 70,   0,  30},
	[LED_GREEN]  =  {  0,   0, 100},
	[LED_BLUE]   =  {  0,   100, 0},
	[LED_PURPLE] =  {  50,  50, 0 },
	[LED_YELLOW] =  { 50,   00, 50},

};

/**
 * Set LED color
 *
 * @param color		Enumerated color value
 */
static void bat_led_set_color(enum led_color color)
{
	pwm_set_duty(PWM_CH_LED_RED, color_brightness[color][0]);
	pwm_set_duty(PWM_CH_LED_BLUE, color_brightness[color][1]);
	pwm_set_duty(PWM_CH_LED_GREEN, color_brightness[color][2]);
}

void led_get_brightness_range(enum ec_led_id led_id, uint8_t *brightness_range)
{
	brightness_range[EC_LED_COLOR_RED] = 100;
	brightness_range[EC_LED_COLOR_BLUE] = 100;
	brightness_range[EC_LED_COLOR_GREEN] = 100;
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
	pwm_set_duty(PWM_CH_LED_RED, brightness[EC_LED_COLOR_RED]);
	pwm_set_duty(PWM_CH_LED_BLUE, brightness[EC_LED_COLOR_BLUE]);
	pwm_set_duty(PWM_CH_LED_GREEN, brightness[EC_LED_COLOR_GREEN]);
	return EC_SUCCESS;
}

static void wizpig_led_set_battery_poweron(void)
{
	switch (charge_get_state()) {
	case PWR_STATE_CHARGE:
		bat_led_set_color(LED_ORANGE);
		break;
	case PWR_STATE_DISCHARGE:
		if (charge_get_percent() < 10)
			bat_led_set_color((battery_second & 0x1)
					? LED_OFF : LED_RED);
		else
			bat_led_set_color(LED_PURPLE);
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

static void wizpig_led_set_battery_poweroff(void)
{
	switch (charge_get_state()) {
	case PWR_STATE_CHARGE:
		bat_led_set_color(LED_ORANGE);
		break;
	case PWR_STATE_CHARGE_NEAR_FULL:
		bat_led_set_color(LED_GREEN);
		break;
	case PWR_STATE_ERROR: /* Battery has been removed and AC input now. */
		bat_led_set_color(LED_RED);
		break;
	default:
		/* Close all LEDs */
		bat_led_set_color(LED_OFF);
		break;
	}
}

static void wizpig_led_set_battery_suspend(void)
{
	switch (charge_get_state()) {
	case PWR_STATE_CHARGE:
		bat_led_set_color((battery_second & 0x1)
					? LED_OFF : LED_ORANGE);
		break;
	case PWR_STATE_DISCHARGE:
		if (charge_get_percent() < 10)
			bat_led_set_color((battery_second & 0x1)
					? LED_OFF : LED_RED);
		else
			bat_led_set_color((battery_second & 0x1)
					? LED_OFF : LED_PURPLE);
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


static void wizpig_led_set_battery(void)
{
	battery_second++;

	if (chipset_in_state(CHIPSET_STATE_ON))
		wizpig_led_set_battery_poweron();
	else if (chipset_in_state(CHIPSET_STATE_ANY_OFF))
		wizpig_led_set_battery_poweroff();
	else if (chipset_in_state(CHIPSET_STATE_SUSPEND))
		wizpig_led_set_battery_suspend();
}

static void led_init(void)
{
	/* Configure GPIOs */
	gpio_config_module(MODULE_PWM_LED, 1);

	/*
	 * Enable PWMs and set to 0% duty cycle.  If they're disabled,
	 * seems to ground the pins instead of letting them float.
	 */
	pwm_enable(PWM_CH_LED_RED, 1);
	pwm_enable(PWM_CH_LED_GREEN, 1);
	pwm_enable(PWM_CH_LED_BLUE, 1);

	bat_led_set_color(LED_OFF);
}
DECLARE_HOOK(HOOK_INIT, led_init, HOOK_PRIO_DEFAULT);

/**  * Called by hook task every 1 sec  */
static void led_second(void)
{
	if (led_debug)
		return;
	if (led_auto_control_is_enabled(EC_LED_ID_BATTERY_LED))
		wizpig_led_set_battery();
}
DECLARE_HOOK(HOOK_SECOND, led_second, HOOK_PRIO_DEFAULT);


static void dump_pwm_channels(void)
{
	int ch;
	for (ch = 0; ch < 4; ch++) {
		CPRINTF("channel = %d\n", ch);
		CPRINTF("0x%04X 0x%04X 0x%04X\n",
			MEC1322_PWM_CFG(ch),
			MEC1322_PWM_ON(ch),
			MEC1322_PWM_OFF(ch));
	}
}
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
		} else if (!strcasecmp(argv[1], "red")) {
			bat_led_set_color(LED_RED);
		} else if (!strcasecmp(argv[1], "green")) {
			bat_led_set_color(LED_GREEN);
		} else if (!strcasecmp(argv[1], "orange")) {
			bat_led_set_color(LED_ORANGE);
		} else if (!strcasecmp(argv[1], "blue")) {
			bat_led_set_color(LED_BLUE);
		} else if (!strcasecmp(argv[1], "purple")) {
			bat_led_set_color(LED_PURPLE);
		} else if (!strcasecmp(argv[1], "yellow")) {
			bat_led_set_color(LED_YELLOW);
		} else {
			/* maybe handle charger_discharge_on_ac() too? */
			return EC_ERROR_PARAM1;
		}
	}

	if (led_debug == 1)
		dump_pwm_channels();
	return EC_SUCCESS;
}
DECLARE_CONSOLE_COMMAND(ledcolor, command_led_color,
			"[debug|red|green|orange|blue|purple|yellow|off]",
			"Change LED color",
			NULL);

