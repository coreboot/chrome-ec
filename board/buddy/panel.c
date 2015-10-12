/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* EC for Buddy panel control */

#include "gpio.h"
#include "hooks.h"
#include "i2c.h"
#include "panel.h"
#include "task.h"
#include "timer.h"
#include "util.h"


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
		return PANEL_LGC_SLE;
	case 0x01:
		return PANEL_BOE;
	case 0x04:
		return PANEL_LGC_SLK;
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

	/* Unidentified panel id must not be configured */
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

static int i2c_write_block(int port, int addr, uint8_t *data, int size)
{
	int rv;

	i2c_lock(port, 1);
	/* Extend timeout setting to avoid timeout error */
	i2c_set_timeout(port, 700 * MSEC);
	rv = i2c_xfer(port, addr, data, size, NULL, 0, I2C_XFER_SINGLE);
	i2c_set_timeout(port, 0);
	i2c_lock(port, 0);

	return rv;
}

static int i2c_read_block(int port, int addr, int cmd, uint8_t *buf, int size)
{
	int rv;
	uint8_t reg;
	reg = cmd;

	i2c_lock(port, 1);
	rv = i2c_xfer(port, addr, &reg, 1, buf, size, I2C_XFER_SINGLE);
	i2c_lock(port, 0);

	return rv;
}

static int flash_transmitter_fw(void)
{
	int crc = 0;
	int crc_done = 0;
	int rv = 0;
	int retry = 0;

	/* EC program SRAM start */
	i2c_write8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0x6F, 0x80);

	/* Write FW into transmitter SRAM */
	i2c_write8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0x5D, 0x00);
	i2c_write8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0x5E, 0x00);
	i2c_write_block(I2C_PORT_TRANSM, I2C_ADDR_TRANSM,
		(uint8_t *)transmitter_fw, sizeof(transmitter_fw));

	/* Setup HW CRC module, verify range from 0x0000 to 0x17FE for 6K FW */
	i2c_write8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0x70, 0x00);
	i2c_write8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0x71, 0x00);
	i2c_write8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0x72, 0x17);
	i2c_write8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0x73, 0xFE);

	/* HW CRC start */
	i2c_write8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0x74, 0x01);

	/* Check HW CRC ready */
	do {
		retry++;
		i2c_read8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0x74, &crc_done);
	} while ((retry < 3) && (crc_done & 0x02) == 0);

	/* Compare HW CRC value with FW CRC (last byte of transmitter FW) */
	if (crc_done & 0x02) {
		rv = i2c_read8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0x75, &crc);

		if (!rv && (crc == transmitter_fw[sizeof(transmitter_fw)-1]))
			return EC_SUCCESS;

	}

	return EC_ERROR_UNKNOWN;
}

static int flash_edid(void)
{
	int i;
	uint8_t edid_tmp[128];
	int panel_id = get_panel_id();

	/* Setup */
	i2c_write8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0x61, 0x02);

	/* Write EDID into transmitter SRAM */
	i2c_write8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0x5D, 0x81);
	i2c_write8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0x5E, 0x00);
	i2c_write_block(I2C_PORT_TRANSM, I2C_ADDR_TRANSM,
		(uint8_t *)edid[panel_id], sizeof(edid_tmp) + 1);

	/* Read EDID from transmitter to verify */
	i2c_write8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0x5D, 0x81);
	i2c_write8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0x5E, 0x00);
	i2c_read_block(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0x5C,
		edid_tmp, sizeof(edid_tmp));

	/* Verify EDID */
	for (i = 0; i < sizeof(edid_tmp); i++) {
		if (edid_tmp[i] != edid[panel_id][i+1])
			return EC_ERROR_UNKNOWN;
	}

	return EC_SUCCESS;
}

static void transmitter_reset(void)
{
	/* Reset and re-run transmitter */
	i2c_write8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0xFD, 0x05);
	i2c_write8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0xFC, 0x83);
	i2c_write8(I2C_PORT_TRANSM, I2C_ADDR_TRANSM, 0x6F, 0x40);
}

void panel_task(void)
{
	int rv;
	int retry;

	while (1) {
		/* Wait for task wake event */
		task_wait_event(-1);

		/* Delay 50ms to wait transmitter ready */
		msleep(50);

		retry = 0;

		do {
			retry++;
			rv = flash_transmitter_fw();
		} while ((retry < 3) && rv);

		if (rv)
			return;

		retry = 0;

		do {
			retry++;
			rv = flash_edid();
		} while ((retry < 3) && rv);

		if (rv)
			return;

		transmitter_reset();
	}
}

static void transmitter_power_on(void)
{
	task_wake(TASK_ID_PANEL);
}
DECLARE_HOOK(HOOK_CHIPSET_STARTUP, transmitter_power_on, HOOK_PRIO_DEFAULT);
