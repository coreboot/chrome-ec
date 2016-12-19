/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery LED and Power LED control for Hana Board.
 */

#include "battery.h"
#include "charge_state.h"
#include "chipset.h"
#include "extpower.h"
#include "gpio.h"
#include "hooks.h"
#include "host_command.h"
#include "led_common.h"
#include "util.h"
#include "system.h"

#define CRITICAL_LOW_BATTERY_PERMILLAGE 86		/* 5*(95-4)/100+4   to meet UI display 5% */
#define FULL_BATTERY_PERMILLAGE 923		/* 97*(95-4)/100+4   to meet UI display 97% */

const enum ec_led_id supported_led_ids[] = {
	EC_LED_ID_BATTERY_LED,
	EC_LED_ID_POWER_LED
};

const int supported_led_ids_count = ARRAY_SIZE(supported_led_ids);

enum led_color {
	BAT_LED_GREEN = 0,
	BAT_LED_RED,
	PWR_LED_WHITE,
	LED_COLOR_COUNT		/* Number of colors, not a color itself */
};

static int bat_led_set(enum led_color color, int on)
{
	switch (color) {
	case BAT_LED_GREEN:
		gpio_set_level(GPIO_BAT_LED0, on); /* BAT_LED_GREEN */
		break;
	case BAT_LED_RED:
		gpio_set_level(GPIO_BAT_LED1, on); /* BAT_LED_RED */
		break;
	case PWR_LED_WHITE:
		gpio_set_level(GPIO_PWR_LED0, on); /* PWR_LED_WHITE */
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
		brightness_range[EC_LED_COLOR_GREEN] = 1;
		brightness_range[EC_LED_COLOR_RED] = 1;
		brightness_range[EC_LED_COLOR_AMBER] = 1;
		break;
	case EC_LED_ID_POWER_LED:
		brightness_range[EC_LED_COLOR_WHITE] = 1;
		break;
	default:
		break;
	}
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
	if (EC_LED_ID_BATTERY_LED == led_id) {
		if (brightness[EC_LED_COLOR_GREEN] != 0) {
			bat_led_set(BAT_LED_GREEN, 1);
			bat_led_set(BAT_LED_RED, 0);
		} else if (brightness[EC_LED_COLOR_RED] != 0) {
			bat_led_set(BAT_LED_GREEN, 0);
			bat_led_set(BAT_LED_RED, 1);
		} else if (brightness[EC_LED_COLOR_AMBER] != 0) {
			bat_led_set(BAT_LED_GREEN, 1);
			bat_led_set(BAT_LED_RED, 1);
		} else {
			bat_led_set(BAT_LED_GREEN, 0);
			bat_led_set(BAT_LED_RED, 0);
		}
		return EC_SUCCESS;
	} else if (EC_LED_ID_POWER_LED == led_id) {
		if (brightness[EC_LED_COLOR_WHITE] != 0) {
			bat_led_set(PWR_LED_WHITE, 1);
		} else {
			bat_led_set(PWR_LED_WHITE, 0);
		}
		return EC_SUCCESS;
	} else {
		return EC_ERROR_UNKNOWN;
	}
}

static unsigned blink_500ms;

/*
 * Power Led Control
 * S0: White led always on
 * S5: White led always off
 * S3: off for AC out, 500ms off/3s on for AC in
 */
static void hana_led_set_power(void)
{
	if (chipset_in_state(CHIPSET_STATE_ANY_OFF)) {
		bat_led_set(PWR_LED_WHITE, 0);
	} else if (chipset_in_state(CHIPSET_STATE_ON)) {
		bat_led_set(PWR_LED_WHITE, 1);
	} else if (chipset_in_state(CHIPSET_STATE_SUSPEND)) {
		if (extpower_is_present()) {
			bat_led_set(PWR_LED_WHITE,
				(blink_500ms % 7) ? 1 : 0);
		} else {
			bat_led_set(PWR_LED_WHITE, 0);
		}
	}
}

/*
 * Battery Led Control
 * Discharge: OFF
 * Charge:
 * 	Battery_Capacity 0%~4%		Red
 * 	Battery_Capacity 5%~96%		Orange (Red+Green)
 * 	Battery_Capacity 97%~100%	Green
 * Battery error: Red blink(1:1)
 * Factory force idle: Green 2 sec, Red 2 sec
 */
static void hana_led_set_battery(void)
{
	uint32_t charge_flags = charge_get_flags();
	int remaining_capacity;
	int full_charge_capacity;
	int permillage;

	/* Make the percentage approximate to UI shown */
	remaining_capacity = *(int *)host_get_memmap(EC_MEMMAP_BATT_CAP);
	full_charge_capacity = *(int *)host_get_memmap(EC_MEMMAP_BATT_LFCC);
	permillage = !full_charge_capacity ? 0 :
		(1000 * remaining_capacity) / full_charge_capacity;

	switch (charge_get_state()) {
	case PWR_STATE_CHARGE:
	case PWR_STATE_CHARGE_NEAR_FULL:
		if (permillage < CRITICAL_LOW_BATTERY_PERMILLAGE) {
			bat_led_set(BAT_LED_GREEN, 0);
			bat_led_set(BAT_LED_RED, 1);
		} else if (permillage < FULL_BATTERY_PERMILLAGE) {
			bat_led_set(BAT_LED_GREEN, 1);
			bat_led_set(BAT_LED_RED, 1);
		} else {
			bat_led_set(BAT_LED_GREEN, 1);
			bat_led_set(BAT_LED_RED, 0);
		}
		break;
	case PWR_STATE_DISCHARGE:
		bat_led_set(BAT_LED_GREEN, 0);
		bat_led_set(BAT_LED_RED, 0);
		break;
	case PWR_STATE_ERROR:
		bat_led_set(BAT_LED_GREEN, 0);
		bat_led_set(BAT_LED_RED, (blink_500ms & 2) ? 0 : 1);
		break;
	case PWR_STATE_IDLE: /* Ext. power connected in IDLE. */
		if (charge_flags & CHARGE_FLAG_FORCE_IDLE) {
			bat_led_set(BAT_LED_GREEN, (blink_500ms & 4) ? 0 : 1);
			bat_led_set(BAT_LED_RED, (blink_500ms & 4) ? 1 : 0);
		} else {
			bat_led_set(BAT_LED_GREEN, 1);
			bat_led_set(BAT_LED_RED, 0);
		}
		break;
	default:
		/* Other states don't alter LED behavior */
		break;
	}
}

/**
 * Called by hook task every 500 milliseconds
 */
static void led_500ms(void)
{
	blink_500ms++;

	if (led_auto_control_is_enabled(EC_LED_ID_POWER_LED))
		hana_led_set_power();
	if (led_auto_control_is_enabled(EC_LED_ID_BATTERY_LED))
		hana_led_set_battery();
}
DECLARE_HOOK(HOOK_TICK, led_500ms, HOOK_PRIO_DEFAULT);
