/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery LED control for Clapper
 */

#include "charge_state.h"
#include "chipset.h"
#include "gpio.h"
#include "hooks.h"
#include "led_common.h"
#include "pwm.h"
#include "util.h"

const enum ec_led_id supported_led_ids[] = {
	EC_LED_ID_BATTERY_LED, EC_LED_ID_POWER_LED};
const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

enum led_color {
	LED_OFF = 0,
	LED_AMBER,
	LED_WHITE,

	/* Number of colors, not a color itself */
	LED_COLOR_COUNT
};

/* Brightness vs. color, for {amber, white} LEDs */
static const uint8_t color_brightness[LED_COLOR_COUNT][2] = {
	{0, 0},
	{100, 0},
	{0, 100},
};

/**
 * Set battery/charger LED color
 *
 * @param color		Enumerated color value
 */
static void set_color_battery(enum led_color color)
{
	pwm_set_duty(PWM_CH_BATTERY_LED_AMBER, color_brightness[color][0]);
	pwm_set_duty(PWM_CH_BATTERY_LED_WHITE, color_brightness[color][1]);
}

/**
 * Set power LED color
 *
 * @param color		Enumerated color value
 */
static void set_color_power(enum led_color color)
{
	/* Setting the color to AMBER is equivalent to OFF */
	pwm_set_duty(PWM_CH_POWER_LED_WHITE, color_brightness[color][1]);
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
		pwm_set_duty(PWM_CH_BATTERY_LED_AMBER,
			     brightness[EC_LED_COLOR_YELLOW]);
		pwm_set_duty(PWM_CH_BATTERY_LED_WHITE,
			     brightness[EC_LED_COLOR_WHITE]);
		break;
	case EC_LED_ID_POWER_LED:
		pwm_set_duty(PWM_CH_POWER_LED_WHITE,
			     brightness[EC_LED_COLOR_WHITE]);
		break;
	default:
		break;
	}
	return EC_SUCCESS;
}

static void power_led_update(void)
{
	static unsigned timer;

	/* 2 sec */
	if (++timer > 20)
		timer = 0;

	/* S5 */
	if (chipset_in_state(CHIPSET_STATE_ANY_OFF)) {
		set_color_power(LED_OFF);

	/* S3 */
	} else if (chipset_in_state(CHIPSET_STATE_SUSPEND)) {
		/* Flashing 1 sec on, 1 sec off */
		set_color_power((timer <= 10) ? LED_OFF : LED_WHITE);

	/* S0 */
	} else if (chipset_in_state(CHIPSET_STATE_ON)) {
		set_color_power(LED_WHITE);
	}
}

static void battery_led_update(void)
{
	static unsigned timer;
	unsigned battery = charge_get_percent();

	/* least common multiple : 15, 33, 51 */
	if (++timer == 2805)
		timer = 0;

	switch (charge_get_state()) {
	case PWR_STATE_CHARGE:
		/* 1% ~ 4% */
		if (battery > 0 && battery < 5) {
			/* 1000 ms on */
			if (timer%15 < 10)
				set_color_battery(LED_AMBER);
			/* 500 ms off */
			else
				set_color_battery(LED_OFF);
		}
		/* 5% ~ 19% */
		else if (battery >= 5 && battery < 20) {
			/* 3200 ms on */
			if (timer%33 < 32)
				set_color_battery(LED_AMBER);
			/* 100 ms off */
			else
				set_color_battery(LED_OFF);
		}
		/* 20% ~ 79% */
		else if (battery >= 20 && battery < 80) {
			/* 5000 ms on */
			if (timer%51 < 50)
				set_color_battery(LED_WHITE);
			/* 100 ms off */
			else
				set_color_battery(LED_OFF);
		}
		/* 80%i ~ 100% */
		else if (battery >= 80 && battery <= 100)
			set_color_battery(LED_WHITE);
		break;
	case PWR_STATE_DISCHARGE:	 /* without AC */
	case PWR_STATE_CHARGE_NEAR_FULL: /* with AC */
	case PWR_STATE_IDLE:		 /* with AC */
		/* not S0 */
		if (!chipset_in_state(CHIPSET_STATE_ON))
			set_color_battery(LED_OFF);
		/* 1% ~ 4% */
		else if (battery > 0 && battery < 5) {
			/* 1000 ms on */
			if (timer%15 < 10)
				set_color_battery(LED_AMBER);
			/* 500 ms off */
			else
				set_color_battery(LED_OFF);
		}
		/* 5% ~ 19% */
		else if (battery >= 5 && battery < 20)
			set_color_battery(LED_AMBER);
		/* 20% ~ 100% */
		else if (battery >= 20 && battery <= 100)
			set_color_battery(LED_WHITE);
		break;
	default:
		/* Other states don't alter LED behavior */
		break;
	}
}

/**
 * Called every 100 ms
 */
static void led_update(void)
{
	if (led_auto_control_is_enabled(EC_LED_ID_POWER_LED))
		power_led_update();

	if (led_auto_control_is_enabled(EC_LED_ID_BATTERY_LED))
		battery_led_update();

	hook_call_deferred(led_update, 100 * MSEC);
}
DECLARE_DEFERRED(led_update);

static void led_init(void)
{
	/* Configure GPIOs */
	gpio_config_module(MODULE_PWM_LED, 1);

	/*
	 * Enable PWMs and set to 0% duty cycle.  If they're disabled, the LM4
	 * seems to ground the pins instead of letting them float.
	 */
	pwm_enable(PWM_CH_BATTERY_LED_AMBER, 1);
	pwm_enable(PWM_CH_BATTERY_LED_WHITE, 1);
	pwm_enable(PWM_CH_POWER_LED_WHITE, 1);
	set_color_battery(LED_OFF);
	set_color_power(LED_OFF);

	hook_call_deferred(led_update, 0);
}
DECLARE_HOOK(HOOK_INIT, led_init, HOOK_PRIO_DEFAULT);
