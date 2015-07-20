/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* G781 temperature sensor module for Chrome EC */

#include "common.h"
#include "console.h"
#include "g781.h"
#include "gpio.h"
#include "i2c.h"
#include "hooks.h"
#include "system.h"
#include "util.h"

static int temp_val_local;
static int temp_val_remote;
#ifdef CONFIG_TEMP_SENSOR_G781_INIT
static int temp_val_local_high_limit = 0xFF;
static int g781_reg_init = 1;
#endif
/**
 * Determine whether the sensor is powered.
 *
 * @return non-zero the g781 sensor is powered.
 */
static int has_power(void)
{
#ifdef CONFIG_TEMP_SENSOR_POWER_GPIO
	return gpio_get_level(CONFIG_TEMP_SENSOR_POWER_GPIO);
#else
	return 1;
#endif
}

static int raw_read8(const int offset, int *data_ptr)
{
	return i2c_read8(I2C_PORT_THERMAL, G781_I2C_ADDR, offset, data_ptr);
}

static int raw_write8(const int offset, int data)
{
	return i2c_write8(I2C_PORT_THERMAL, G781_I2C_ADDR, offset, data);
}

#ifdef CONFIG_TEMP_SENSOR_G781_INIT
static int status_read8(int *data)
{
	int rv;
	/* We use buf[1] here so it's aligned for DMA on STM32 */
	uint8_t buf[1];

	i2c_lock(I2C_PORT_THERMAL, 1);
	rv = i2c_xfer(I2C_PORT_THERMAL, G781_I2C_ALERT_ADDR,
				0, 0, buf, 1, I2C_XFER_SINGLE);
	i2c_lock(I2C_PORT_THERMAL, 0);

	if (!rv)
		*data = buf[0];

	return rv;
}
#endif

static int get_temp(const int offset, int *temp_ptr)
{
	int rv;
	int temp_raw = 0;

	rv = raw_read8(offset, &temp_raw);
	if (rv < 0)
		return rv;

	*temp_ptr = (int)(int8_t)temp_raw;
	return EC_SUCCESS;
}

static int set_temp(const int offset, int temp)
{
	if (temp < -127 || temp > 127)
		return EC_ERROR_INVAL;

	return raw_write8(offset, (uint8_t)temp);
}

int g781_get_val(int idx, int *temp_ptr)
{
	if (!has_power())
		return EC_ERROR_NOT_POWERED;

	switch (idx) {
	case G781_IDX_INTERNAL:
		*temp_ptr = temp_val_local;
		break;
	case G781_IDX_EXTERNAL:
		*temp_ptr = temp_val_remote;
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}

	return EC_SUCCESS;
}

static void temp_sensor_g781_init_deferred(void)
{
#ifdef CONFIG_TEMP_SENSOR_G781_INIT
	int alert_status, rv;
	if (!g781_reg_init)
		return;

	/* Make sure this init all alert status is clear */
	ccprintf("[%T Start G781 register init]\n");
	rv = status_read8(&alert_status);
	if (rv < 0)
		return;

	if (temp_val_local_high_limit == 0xFF) {
		rv = get_temp(G781_LOCAL_TEMP_HIGH_LIMIT_R,
			&temp_val_local_high_limit);
		if ((rv == EC_SUCCESS) && (temp_val_local_high_limit
				!= G781_LOCAL_TEMP_HIGH_LIMIT_VALUE)) {
			rv = set_temp(G781_LOCAL_TEMP_HIGH_LIMIT_W,
			G781_LOCAL_TEMP_HIGH_LIMIT_VALUE);
			if (rv < 0)
				return;
		}
	}
	g781_reg_init = EC_SUCCESS;

	ccprintf("[%T Enable G781 ALERT# interrupt\n");
	gpio_enable_interrupt(GPIO_FAN_ALERT_L);
#endif
}
DECLARE_DEFERRED(temp_sensor_g781_init_deferred);

static void temp_sensor_g781_init(void)
{
	hook_call_deferred(temp_sensor_g781_init_deferred, 0);
}
DECLARE_HOOK(HOOK_INIT, temp_sensor_g781_init, HOOK_PRIO_TEMP_SENSOR);

static void fan_alert_deferred(void)
{
#ifdef CONFIG_TEMP_SENSOR_G781_INIT
	ccprintf("[%T Fan alert force EC enter hibernate]\n");
	system_hibernate(0, 0);
#endif
}
DECLARE_DEFERRED(fan_alert_deferred);

void fan_alert_interrupt(enum gpio_signal signal)
{
	hook_call_deferred(fan_alert_deferred, 0);
	gpio_disable_interrupt(GPIO_FAN_ALERT_L);
}

static void temp_sensor_poll(void)
{
#ifdef CONFIG_TEMP_SENSOR_G781_INIT
	if (g781_reg_init) {
		ccprintf("[%T Retry to init G781 register]\n");
		hook_call_deferred(temp_sensor_g781_init_deferred, 0);
	}
#endif

	if (!has_power())
		return;

	get_temp(G781_TEMP_LOCAL, &temp_val_local);
	temp_val_local = C_TO_K(temp_val_local);

	get_temp(G781_TEMP_REMOTE, &temp_val_remote);
	temp_val_remote = C_TO_K(temp_val_remote);
}
DECLARE_HOOK(HOOK_SECOND, temp_sensor_poll, HOOK_PRIO_TEMP_SENSOR);

static int print_status(void)
{
	int value;
	int rv;


	rv = get_temp(G781_TEMP_LOCAL, &value);
	if (rv < 0)
		return rv;
	ccprintf("Local Temp:   %3dC\n", value);

	rv = get_temp(G781_LOCAL_TEMP_THERM_LIMIT, &value);
	if (rv < 0)
		return rv;
	ccprintf("  Therm Trip: %3dC\n", value);

	rv = get_temp(G781_LOCAL_TEMP_HIGH_LIMIT_R, &value);
	if (rv < 0)
		return rv;
	ccprintf("  High Alarm: %3dC\n", value);

	rv = get_temp(G781_LOCAL_TEMP_LOW_LIMIT_R, &value);
	if (rv < 0)
		return rv;
	ccprintf("  Low Alarm:  %3dC\n", value);

	rv = get_temp(G781_TEMP_REMOTE, &value);
	if (rv < 0)
		return rv;
	ccprintf("Remote Temp:  %3dC\n", value);

	rv = get_temp(G781_REMOTE_TEMP_THERM_LIMIT, &value);
	if (rv < 0)
		return rv;
	ccprintf("  Therm Trip: %3dC\n", value);

	rv = get_temp(G781_REMOTE_TEMP_HIGH_LIMIT_R, &value);
	if (rv < 0)
		return rv;
	ccprintf("  High Alarm: %3dC\n", value);

	rv = get_temp(G781_REMOTE_TEMP_LOW_LIMIT_R, &value);
	if (rv < 0)
		return rv;
	ccprintf("  Low Alarm:  %3dC\n", value);

	rv = raw_read8(G781_STATUS, &value);
	if (rv < 0)
		return rv;
	ccprintf("\nSTATUS: %08b\n", value);

	rv = raw_read8(G781_CONFIGURATION_R, &value);
	if (rv < 0)
		return rv;
	ccprintf("CONFIG: %08b\n", value);

	return EC_SUCCESS;
}

static int command_g781(int argc, char **argv)
{
	char *command;
	char *e;
	int data;
	int offset;
	int rv;

	if (!has_power()) {
		ccprintf("ERROR: Temp sensor not powered.\n");
		return EC_ERROR_NOT_POWERED;
	}

	/* If no args just print status */
	if (argc == 1)
		return print_status();

	if (argc < 3)
		return EC_ERROR_PARAM_COUNT;

	command = argv[1];
	offset = strtoi(argv[2], &e, 0);
	if (*e || offset < 0 || offset > 255)
		return EC_ERROR_PARAM2;

	if (!strcasecmp(command, "getbyte")) {
		rv = raw_read8(offset, &data);
		if (rv < 0)
			return rv;
		ccprintf("Byte at offset 0x%02x is %08b\n", offset, data);
		return rv;
	}

	/* Remaining commands are of the form "g781 set-command offset data" */
	if (argc != 4)
		return EC_ERROR_PARAM_COUNT;

	data = strtoi(argv[3], &e, 0);
	if (*e)
		return EC_ERROR_PARAM3;

	if (!strcasecmp(command, "settemp")) {
		ccprintf("Setting 0x%02x to %dC\n", offset, data);
		rv = set_temp(offset, data);
	} else if (!strcasecmp(command, "setbyte")) {
		ccprintf("Setting 0x%02x to 0x%02x\n", offset, data);
		rv = raw_write8(offset, data);
	} else
		return EC_ERROR_PARAM1;

	return rv;
}
DECLARE_CONSOLE_COMMAND(g781, command_g781,
	"[settemp|setbyte <offset> <value>] or [getbyte <offset>]. "
	"Temps in Celsius.",
	"Print g781 temp sensor status or set parameters.", NULL);
