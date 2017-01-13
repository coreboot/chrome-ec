/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* NCT7717 temperature sensor module for Chrome EC */

#include "console.h"
#include "i2c.h"
#include "nct7717.h"
#include "util.h"

static int raw_read8(const int offset, int *data_ptr)
{
	return i2c_read8(I2C_PORT_THERMAL, NCT7717_I2C_ADDR, offset, data_ptr);
}

int nct7717_get_val(int idx, int *temp_ptr)
{
	int rv;

	if (idx != 0)
		return EC_ERROR_INVAL;

	rv = raw_read8(NCT7717_TEMP_LOCAL, temp_ptr);
	if (!rv)
		*temp_ptr = C_TO_K(*temp_ptr);

	return rv;
}

#ifdef CONFIG_CMD_TEMP_SENSOR
static const char *conversion_rate[] =
{
	"0.0625", "0.125", "0.25", "0.5", "1", "2", "4", "8", "16"
};

static int raw_write8(const int offset, int data)
{
	return i2c_write8(I2C_PORT_THERMAL, NCT7717_I2C_ADDR, offset, data);
}

static int print_status(void)
{
	int v, i;

	if (!raw_read8(NCT7717_TEMP_LOCAL, &v))
		ccprintf("local temp: %dC\n", v);

	if (!raw_read8(NCT7717_LT_HIGH_ALERT_TEMP_W, &v))
		ccprintf("alert temp: %dC\n", v);

	if (!raw_read8(NCT7717_ALERT_STATUS, &v)) {
		ccprintf("status: STS_LTHA: %s\n",
			v & NCT7717_ALERT_STATUS_STS_LTHA ? "YES" : "NO");
		ccprintf("status: ADC_BUSY: %s\n",
			v & NCT7717_ALERT_STATUS_ADC_BUSY ? "YES" : "NO");
	}

	if (!raw_read8(NCT7717_CONFIGURATION_R, &v)) {
		ccprintf("configuration: ALERT# function: %s\n",
			v & NCT7717_CONFIGURATION_ALERT_MSK ? "EN" : "DIS");
		ccprintf("configuration: STOP Monitor: %s\n",
			v & NCT7717_CONFIGURATION_STOP_MNT ? "YES" : "NO");
		ccprintf("configuration: Fault Queue function: %s\n",
			v & NCT7717_CONFIGURATION_EN_FAULTQ ? "EN" : "DIS");
	}

	if (!raw_read8(NCT7717_CONVERSION_RATE_R, &v))
		ccprintf("conversion rate: %d(%sHz)\n", v, conversion_rate[v]);

	ccprintf("customer data log register: ");
	for (i = 0; i < 3; i++)
		if (!raw_read8(NCT7717_CUSTOMER_DATA_LOG_REG_1, &v))
			ccprintf("%d ", v);
	ccprintf("\n");

	if (!raw_read8(NCT7717_ALERT_MODE, &v)) {
		ccprintf("alert mode: %s\n", v & 0x01 ? "Comparator mode" :
			"Interrupt or SMBus alert mode");
	}

	if (!raw_read8(NCT7717_CHIP_ID, &v))
		ccprintf("chip id: %x\n");

	if (!raw_read8(NCT7717_VENDOR_ID, &v))
		ccprintf("vendor id: %x\n");

	if (!raw_read8(NCT7717_DEVICE_ID, &v))
		ccprintf("device id: %x\n");

	return EC_SUCCESS;
}

static int command_nct7717(int argc, char **argv)
{
	char *command;
	char *e;
	int data;
	int offset;
	int rv;

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

	/* Remaining commands are of the form "set-command offset data" */
	if (argc != 4)
		return EC_ERROR_PARAM_COUNT;

	data = strtoi(argv[3], &e, 0);
	if (*e)
		return EC_ERROR_PARAM3;

	if (!strcasecmp(command, "setbyte")) {
		ccprintf("Setting 0x%02x to 0x%02x\n", offset, data);
		rv = raw_write8(offset, data);
	} else
		return EC_ERROR_PARAM1;

	return rv;
}
DECLARE_CONSOLE_COMMAND(nct7717, command_nct7717,
	"[setbyte <offset> <value>] or [getbyte <offset>]",
	"Print nct7717 temp sensor status or set parameters.");
#endif
