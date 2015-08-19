/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* EC for Buddy panel control */

#include "gpio.h"
#include "hooks.h"
#include "i2c.h"
#include "timer.h"
#include "util.h"

#define I2C_PORT_CONVERT 0
#define I2C_ADDR_CONVERT 0x62
#define CMD_NUM 6


enum panel_id_list {
	PANEL_LGC = 0,
	PANEL_BOE,
	PANEL_COUNT,
	PANEL_UNKNOWN,
	PANEL_NONE,
};

struct converter_cfg {
	uint8_t reg;
	uint8_t val;
};

static const struct converter_cfg converter_cfg_vals[PANEL_COUNT][CMD_NUM] = {
	{
		{0x00, 0xB1},
		{0x01, 0x43},
		{0x02, 0x73},
		{0x03, 0x00},
		{0x04, 0x00},
		{0x05, 0xC7},
	},
	{
		{0x00, 0xB1},
		{0x01, 0x43},
		{0x02, 0x7B},
		{0x03, 0x00},
		{0x04, 0x00},
		{0x05, 0xB7},
	},
};

static int get_panel_id(void)
{
	int pin_status = 0;

	if (gpio_get_level(GPIO_PANEL_ID1))
		pin_status |= 0x01;
	if (gpio_get_level(GPIO_PANEL_ID2))
		pin_status |= 0x02;
	if (gpio_get_level(GPIO_PANEL_ID3))
		pin_status |= 0x04;

	switch (pin_status) {
	case 0x00:
		return PANEL_LGC;
	case 0x01:
		return PANEL_BOE;
	case 0x07:
		return PANEL_NONE;
	default:
		return PANEL_UNKNOWN;
	}
}

static void panel_converter_setting(void)
{
	int i;
	int panel_id = get_panel_id();

	/* Unidentified panel id must not be configured*/
	ASSERT(panel_id != PANEL_UNKNOWN);

	if (panel_id == PANEL_NONE)
		return;

	if (gpio_get_level(CONFIG_BACKLIGHT_REQ_GPIO)) {
		for (i = 0; i < CMD_NUM; i++) {
			i2c_write8(I2C_PORT_CONVERT, I2C_ADDR_CONVERT,
				converter_cfg_vals[panel_id][i].reg,
				converter_cfg_vals[panel_id][i].val);
		}
	}
}
DECLARE_DEFERRED(panel_converter_setting);

static void update_backlight(void)
{
#ifdef CONFIG_BACKLIGHT_REQ_GPIO
	if (gpio_get_level(CONFIG_BACKLIGHT_REQ_GPIO)) {
		gpio_set_level(GPIO_ENABLE_BACKLIGHT, 1);
		hook_call_deferred(&panel_converter_setting, 200 * MSEC);
	} else {
		gpio_set_level(GPIO_ENABLE_BACKLIGHT, 0);
	}
#endif
}

void backlight_interrupt(enum gpio_signal signal)
{
	update_backlight();
}

static void convert_init(void)
{
	update_backlight();

#ifdef CONFIG_BACKLIGHT_REQ_GPIO
	gpio_enable_interrupt(CONFIG_BACKLIGHT_REQ_GPIO);
#endif
}
DECLARE_HOOK(HOOK_INIT, convert_init, HOOK_PRIO_DEFAULT);
