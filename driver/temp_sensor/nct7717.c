/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* NCT7717 temperature sensor module for Chrome EC */

#include "common.h"
#include "console.h"
#include "nct7717.h"
#include "gpio.h"
#include "i2c.h"
#include "hooks.h"
#include "util.h"

static int temp_val_local;

/**
 * Determine whether the sensor is powered.
 *
 * @return non-zero the nct7717 sensor is powered.
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
	return i2c_read8(I2C_PORT_THERMAL, NCT7717_I2C_ADDR, offset, data_ptr);
}

static int raw_write8(const int offset, int data)
{
	return i2c_write8(I2C_PORT_THERMAL, NCT7717_I2C_ADDR, offset, data);
}

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

int nct7717_get_val(int idx, int *temp_ptr)
{
	if (!has_power())
		return EC_ERROR_NOT_POWERED;

	switch (idx) {
	case NCT7717_IDX_INTERNAL:
		*temp_ptr = temp_val_local;
		break;
	default:
		return EC_ERROR_UNKNOWN;
	}

	return EC_SUCCESS;
}

static void temp_sensor_poll(void)
{
	if (!has_power())
		return;

	get_temp(NCT7717_TEMP_LOCAL, &temp_val_local);
	temp_val_local = C_TO_K(temp_val_local);

}
DECLARE_HOOK(HOOK_SECOND, temp_sensor_poll, HOOK_PRIO_TEMP_SENSOR);

static int print_status(void)
{
	int value;
	int rv;

	rv = get_temp(NCT7717_TEMP_LOCAL, &value);
	if (rv < 0)
		return rv;
	ccprintf("Local Temp:   %3dC\n", value);

	rv = get_temp(NCT7717_LOCAL_TEMP_HIGH_LIMIT_R, &value);
	if (rv < 0)
		return rv;
	ccprintf("  High Alarm: %3dC\n", value);

	rv = raw_read8(NCT7717_STATUS, &value);
	if (rv < 0)
		return rv;
	ccprintf("\nSTATUS: %08b\n", value);

	rv = raw_read8(NCT7717_CONFIGURATION_R, &value);
	if (rv < 0)
		return rv;
	ccprintf("CONFIG: %08b\n", value);

	return EC_SUCCESS;
}

static int command_nct7717(int argc, char **argv)
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

	/* Remaining commands are the form "nct7717 set-command offset data" */
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
DECLARE_CONSOLE_COMMAND(nct7717, command_nct7717,
	"[settemp|setbyte <offset> <value>] or [getbyte <offset>]. "
	"Temps in Celsius.",
	"Print nct7717 temp sensor status or set parameters.", NULL);
