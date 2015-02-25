/* Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery LED control for Banjo
 */

#include "charge_state.h"
#include "chipset.h"
#include "gpio.h"
#include "hooks.h"
#include "host_command.h"
#include "led_common.h"
#include "pwm.h"
#include "util.h"
#include "extpower.h"

#define CRITICAL_LOW_BATTERY_PERMILLAGE 71
#define LOW_BATTERY_PERMILLAGE 137
#define FULL_BATTERY_PERMILLAGE 937

#define LED_TOTAL_4SECS_TICKS 16
#define LED_TOTAL_2SECS_TICKS 8
#define LED_ON_1SEC_TICKS 4
#define LED_ON_2SECS_TICKS 8

const enum ec_led_id supported_led_ids[] = {
	EC_LED_ID_BATTERY_LED , EC_LED_ID_POWER_LED};
const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

enum led_color {
	LED_OFF = 0,
	LED_ORANGE,
	LED_RED,
	LED_YELLOW,
	LED_BLUE,

	/* Number of colors, not a color itself */
	LED_COLOR_COUNT
};

/* Brightness vs. color, for {red, green} LEDs */
static const uint8_t color_brightness[LED_COLOR_COUNT][2] = {
	{0, 0},
	{100, 0},
	{30, 45},
	{20, 60},
	{0, 100},
};

/**
 * Set LED color
 *
 * @param color		Enumerated color value
 */
static void set_color_battery_led(enum led_color color)
{
	pwm_set_duty(PWM_CH_LED_ORANGE, color_brightness[color][0]);
	pwm_set_duty(PWM_CH_LED_BLUE, color_brightness[color][1]);
}

static void set_color_power_led(enum led_color color)
{
	pwm_set_duty(PWM_CH_LED_ORANGE_POWER_LED, color_brightness[color][0]);
	pwm_set_duty(PWM_CH_LED_BLUE_POWER_LED, color_brightness[color][1]);
}

void led_get_brightness_range(enum ec_led_id led_id, uint8_t *brightness_range)
{
	brightness_range[EC_LED_COLOR_YELLOW] = 100;
	brightness_range[EC_LED_COLOR_BLUE] = 100;
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
	if (led_id == EC_LED_ID_POWER_LED) {
		pwm_set_duty(PWM_CH_LED_BLUE_POWER_LED,
			     brightness[EC_LED_COLOR_BLUE]);
		pwm_set_duty(PWM_CH_LED_ORANGE_POWER_LED,
			     brightness[EC_LED_COLOR_YELLOW]);
	} else {
		pwm_set_duty(PWM_CH_LED_BLUE,
			     brightness[EC_LED_COLOR_BLUE]);
		pwm_set_duty(PWM_CH_LED_ORANGE,
			     brightness[EC_LED_COLOR_YELLOW]);
	}
	return EC_SUCCESS;
}

static void led_init(void)

{
	/* Configure GPIOs */
	gpio_config_module(MODULE_PWM_LED, 1);

	/*
	 * Enable PWMs and set to 0% duty cycle.  If they're disabled, the LM4
	 * seems to ground the pins instead of letting them float.
	 */
	pwm_enable(PWM_CH_LED_ORANGE, 1);
	pwm_enable(PWM_CH_LED_BLUE, 1);
	pwm_enable(PWM_CH_LED_ORANGE_POWER_LED, 1);
	pwm_enable(PWM_CH_LED_BLUE_POWER_LED, 1);
	set_color_battery_led(LED_OFF);
	set_color_power_led(LED_OFF);
}
DECLARE_HOOK(HOOK_INIT, led_init, HOOK_PRIO_DEFAULT);

/**
 * Called by hook task every 250 ms
 */
static void power_led(void)
{
	static unsigned int power_ticks;
	static int previous_state_suspend;

	power_ticks++;

	/* If we don't control the LED, nothing to do */
	if (!led_auto_control_is_enabled(EC_LED_ID_POWER_LED))
		return;

	if (chipset_in_state(CHIPSET_STATE_SUSPEND)) {
		if (!previous_state_suspend)
			power_ticks = 0;

		/* Blink once every four seconds. */
		set_color_power_led((power_ticks % LED_TOTAL_4SECS_TICKS <
				    LED_ON_1SEC_TICKS) ? LED_ORANGE : LED_OFF);

		previous_state_suspend = 1;
		return;
	}

	previous_state_suspend = 0;

	if (chipset_in_state(CHIPSET_STATE_ANY_OFF))
		set_color_power_led(LED_OFF);
	else if (chipset_in_state(CHIPSET_STATE_ON))
		set_color_power_led(LED_BLUE);

}

static void battery_led(void)
{
	static unsigned int battery_ticks;
	uint32_t chflags = charge_get_flags();
	int remaining_capacity;
	int full_charge_capacity;
	int permillage;

	battery_ticks++;

	/* If we don't control the LED, nothing to do */
	if (!led_auto_control_is_enabled(EC_LED_ID_BATTERY_LED))
		return;

	remaining_capacity = *(int *)host_get_memmap(EC_MEMMAP_BATT_CAP);
	full_charge_capacity = *(int *)host_get_memmap(EC_MEMMAP_BATT_LFCC);
	permillage = !full_charge_capacity ? 0 :
		(1000 * remaining_capacity) / full_charge_capacity;

	switch (charge_get_state()) {
	case PWR_STATE_CHARGE:
		/* Make the percentage approximate to UI shown */
		set_color_battery_led(permillage <
			FULL_BATTERY_PERMILLAGE ? LED_ORANGE : LED_BLUE);
		break;
	case PWR_STATE_CHARGE_NEAR_FULL:
		set_color_battery_led(LED_BLUE);
		break;
	case PWR_STATE_DISCHARGE:
		/* Less than 3%, blink one second every two seconds */
		if (!chipset_in_state(CHIPSET_STATE_ANY_OFF) &&
		    permillage <= CRITICAL_LOW_BATTERY_PERMILLAGE)
			set_color_battery_led(
				(battery_ticks % LED_TOTAL_2SECS_TICKS <
				 LED_ON_1SEC_TICKS) ? LED_ORANGE : LED_OFF);
		/* Less than 10%, blink one second every four seconds */
		else if (!chipset_in_state(CHIPSET_STATE_ANY_OFF) &&
			 permillage <= LOW_BATTERY_PERMILLAGE)
			set_color_battery_led(
				(battery_ticks % LED_TOTAL_4SECS_TICKS <
				 LED_ON_1SEC_TICKS) ? LED_ORANGE : LED_OFF);
		else
			set_color_battery_led(LED_OFF);
		break;
	case PWR_STATE_ERROR:
		set_color_battery_led(
			(battery_ticks % LED_TOTAL_2SECS_TICKS <
			 LED_ON_1SEC_TICKS) ? LED_ORANGE : LED_OFF);
		break;
	case PWR_STATE_IDLE: /* External power connected in IDLE. */
		if (chflags & CHARGE_FLAG_FORCE_IDLE)
			set_color_battery_led(
				(battery_ticks % LED_TOTAL_4SECS_TICKS <
				 LED_ON_2SECS_TICKS) ? LED_BLUE : LED_ORANGE);
		else
			set_color_battery_led(LED_BLUE);
		break;
	default:
		/* Other states don't alter LED behavior */
		break;
	}
}

static void led_tick(void)
{
	battery_led();
	power_led();
}
DECLARE_HOOK(HOOK_TICK, led_tick, HOOK_PRIO_DEFAULT);
