/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* TMP431/TMP432 temperature sensor module for Chrome EC */

#include "common.h"
#include "console.h"
#include "tmp43x.h"
#include "gpio.h"
#include "i2c.h"
#include "hooks.h"
#include "util.h"

#ifdef CONFIG_TEMP_SENSOR_TMP431
static int tmp431_val_local;
static int tmp431_val_remote1;
#endif

#ifdef CONFIG_TEMP_SENSOR_TMP432
static int tmp432_val_local;
static int tmp432_val_remote1;
static int tmp432_val_remote2;
#endif

/**
 * Determine whether the sensor is powered.
 *	This assumed a single power GPIO is shared between TMP431 and TMP432
 *	if both are configured to be enabled.
 *
 * @return non-zero the sensor is powered.
 */
static int has_power(void)
{
#ifdef CONFIG_TEMP_SENSOR_POWER_GPIO
	return gpio_get_level(CONFIG_TEMP_SENSOR_POWER_GPIO);
#else
	return 1;
#endif
}

static int raw_read8(const int addr, const int offset, int *data_ptr)
{
	return i2c_read8(I2C_PORT_THERMAL, addr, offset, data_ptr);
}

static int raw_write8(const int addr, const int offset, int data)
{
	return i2c_write8(I2C_PORT_THERMAL, addr, offset, data);
}

static int get_temp(const int addr, const int offset, int *temp_ptr)
{
	int rv;
	int temp_raw = 0;

	rv = raw_read8(addr, offset, &temp_raw);
	if (rv != EC_SUCCESS)
		return rv;

	*temp_ptr = (int)(int8_t)temp_raw;
	return EC_SUCCESS;
}

static int tmp43x_set_temp(const int addr, const int offset, int temp)
{
	if (temp < -127 || temp > 127)
		return EC_ERROR_INVAL;

	return raw_write8(addr, offset, (uint8_t)temp);
}

int tmp43x_get_val(int idx, int *temp_ptr)
{
	if (!has_power())
		return EC_ERROR_NOT_POWERED;

	switch (idx) {
#ifdef CONFIG_TEMP_SENSOR_TMP432
	case TMP432_IDX_LOCAL:
		*temp_ptr = tmp432_val_local;
		break;
	case TMP432_IDX_REMOTE1:
		*temp_ptr = tmp432_val_remote1;
		break;
	case TMP432_IDX_REMOTE2:
		*temp_ptr = tmp432_val_remote2;
		break;
#endif
#ifdef CONFIG_TEMP_SENSOR_TMP431
	case TMP431_IDX_LOCAL:
		*temp_ptr = tmp431_val_local;
		break;
	case TMP431_IDX_REMOTE1:
		*temp_ptr = tmp431_val_remote1;
		break;
#endif
	default:
		return EC_ERROR_UNKNOWN;
	}

	return EC_SUCCESS;
}

static void temp_sensor_poll(void)
{
	int temp_c;

	if (!has_power())
		return;

#ifdef CONFIG_TEMP_SENSOR_TMP432
	if (get_temp(TMP432_I2C_ADDR, TMP43X_LOCAL, &temp_c) == EC_SUCCESS)
		tmp432_val_local = C_TO_K(temp_c);

	if (get_temp(TMP432_I2C_ADDR, TMP43X_REMOTE1, &temp_c) == EC_SUCCESS)
		tmp432_val_remote1 = C_TO_K(temp_c);

	if (get_temp(TMP432_I2C_ADDR, TMP432_REMOTE2, &temp_c) == EC_SUCCESS)
		tmp432_val_remote2 = C_TO_K(temp_c);
#endif
#ifdef CONFIG_TEMP_SENSOR_TMP431
	if (get_temp(TMP431_I2C_ADDR, TMP43X_LOCAL, &temp_c) == EC_SUCCESS)
		tmp431_val_local = C_TO_K(temp_c);

	if (get_temp(TMP432_I2C_ADDR, TMP43X_REMOTE1, &temp_c) == EC_SUCCESS)
		tmp431_val_remote1 = C_TO_K(temp_c);
#endif
}
DECLARE_HOOK(HOOK_SECOND, temp_sensor_poll, HOOK_PRIO_TEMP_SENSOR);

static void print_temps(
		const char *name,
		const int i2c_addr,
		const int temp_reg,
		const int therm_limit_reg,
		const int high_limit_reg,
		const int low_limit_reg)
{
	int value;

	ccprintf("%s:\n", name);

	if (get_temp(i2c_addr, temp_reg, &value) == EC_SUCCESS)
		ccprintf("  Temp       %3dC\n", value);

	if (get_temp(i2c_addr, therm_limit_reg, &value) == EC_SUCCESS)
		ccprintf("  Therm Trip %3dC\n", value);

	if (get_temp(i2c_addr, high_limit_reg, &value) == EC_SUCCESS)
		ccprintf("  High Alarm %3dC\n", value);

	if (get_temp(i2c_addr, low_limit_reg, &value) == EC_SUCCESS)
		ccprintf("  Low Alarm  %3dC\n", value);
}

static int print_status(int addr)
{
	int value;
	int offset;

	print_temps("Local", addr,
		    TMP43X_LOCAL,
		    TMP43X_LOCAL_THERM_LIMIT,
		    TMP43X_LOCAL_HIGH_LIMIT_R,
		    TMP43X_LOCAL_LOW_LIMIT_R);

	print_temps("Remote1", addr,
		    TMP43X_REMOTE1,
		    TMP43X_REMOTE1_THERM_LIMIT,
		    TMP43X_REMOTE1_HIGH_LIMIT_R,
		    TMP43X_REMOTE1_LOW_LIMIT_R);

	if (addr == TMP432_I2C_ADDR)
		print_temps("Remote2", addr,
			    TMP432_REMOTE2,
			    TMP432_REMOTE2_THERM_LIMIT,
			    TMP432_REMOTE2_HIGH_LIMIT_R,
			    TMP432_REMOTE2_LOW_LIMIT_R);

	ccprintf("\n");

	if (raw_read8(addr, TMP43X_STATUS, &value) == EC_SUCCESS)
		ccprintf("STATUS:  %08b\n", value);

	if (raw_read8(addr, TMP43X_CONFIGURATION1_R, &value) == EC_SUCCESS)
		ccprintf("CONFIG1: %08b\n", value);

	offset = TMP431_CONFIGURATION2_R;
	if (addr == TMP432_I2C_ADDR)
		offset = TMP432_CONFIGURATION2_R;

	if (raw_read8(addr, offset, &value) == EC_SUCCESS)
		ccprintf("CONFIG2: %08b\n", value);

	return EC_SUCCESS;
}

static int command_tmp43x(int argc, char **argv, int addr)
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
		return print_status(addr);

	if (argc < 3)
		return EC_ERROR_PARAM_COUNT;

	command = argv[1];
	offset = strtoi(argv[2], &e, 0);
	if (*e || offset < 0 || offset > 255)
		return EC_ERROR_PARAM2;

	if (!strcasecmp(command, "getbyte")) {
		rv = raw_read8(addr, offset, &data);
		if (rv < 0)
			return rv;
		ccprintf("Byte at offset 0x%02x is %08b\n", offset, data);
		return rv;
	}

	/* Remaining commands are "tmp432 set-command offset data" */
	if (argc != 4)
		return EC_ERROR_PARAM_COUNT;

	data = strtoi(argv[3], &e, 0);
	if (*e)
		return EC_ERROR_PARAM3;

	if (!strcasecmp(command, "settemp")) {
		ccprintf("Setting 0x%02x to %dC\n", offset, data);
		rv = tmp43x_set_temp(addr, offset, data);
	} else if (!strcasecmp(command, "setbyte")) {
		ccprintf("Setting 0x%02x to 0x%02x\n", offset, data);
		rv = raw_write8(addr, offset, data);
	} else
		return EC_ERROR_PARAM1;

	return rv;
}

#ifdef CONFIG_TEMP_SENSOR_TMP431
int command_tmp431(int argc, char **argv)
{
	return command_tmp43x(argc, argv, TMP431_I2C_ADDR);
}
DECLARE_CONSOLE_COMMAND(tmp431, command_tmp431,
	"[settemp|setbyte <offset> <value>] or [getbyte <offset>]. "
	"Temps in Celsius.",
	"Print tmp431 temp sensor status or set parameters.", NULL);
#endif

#ifdef CONFIG_TEMP_SENSOR_TMP432
int command_tmp432(int argc, char **argv)
{
	return command_tmp43x(argc, argv, TMP432_I2C_ADDR);
}
DECLARE_CONSOLE_COMMAND(tmp432, command_tmp432,
	"[settemp|setbyte <offset> <value>] or [getbyte <offset>]. "
	"Temps in Celsius.",
	"Print tmp432 temp sensor status or set parameters.", NULL);
#endif
