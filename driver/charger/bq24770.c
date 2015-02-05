/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * TI BQ24770 battery charger driver.
 */

#include "battery_smart.h"
#include "bq24770.h"
#include "charger.h"
#include "console.h"
#include "common.h"
#include "util.h"

/* Sense resistor configurations and macros */
#define DEFAULT_SENSE_RESISTOR 10
#define R_SNS CONFIG_CHARGER_SENSE_RESISTOR
#define R_AC  (CONFIG_CHARGER_SENSE_RESISTOR_AC)
#define REG_TO_CURRENT(REG, RS) ((REG) * DEFAULT_SENSE_RESISTOR / (RS))
#define CURRENT_TO_REG(CUR, RS) ((CUR) * (RS) / DEFAULT_SENSE_RESISTOR)

/* Console output macros */
#define CPUTS(outstr) cputs(CC_CHARGER, outstr)
#define CPRINTS(format, args...) cprints(CC_CHARGER, format, ## args)


/* Charger parameters */
static const struct charger_info bq24770_charger_info = {
	.name         = "bq24770",
	.voltage_max  = CHARGE_V_MAX,
	.voltage_min  = CHARGE_V_MIN,
	.voltage_step = CHARGE_V_STEP,
	.current_max  = REG_TO_CURRENT(CHARGE_I_MAX, R_SNS),
	.current_min  = REG_TO_CURRENT(CHARGE_I_MIN, R_SNS),
	.current_step = REG_TO_CURRENT(CHARGE_I_STEP, R_SNS),
	.input_current_max  = REG_TO_CURRENT(INPUT_I_MAX, R_AC),
	.input_current_min  = REG_TO_CURRENT(INPUT_I_MIN, R_AC),
	.input_current_step = REG_TO_CURRENT(INPUT_I_STEP, R_AC),
};

/* bq24770 specific interfaces */

int charger_set_input_current(int input_current)
{
	return sbc_write(BQ24770_INPUT_CURRENT,
			 CURRENT_TO_REG(input_current, R_AC));
}

int charger_get_input_current(int *input_current)
{
	int rv;
	int reg;

	rv = sbc_read(BQ24770_INPUT_CURRENT, &reg);
	if (rv)
		return rv;

	*input_current = REG_TO_CURRENT(reg, R_AC);

	return EC_SUCCESS;
}

int charger_manufacturer_id(int *id)
{
	return sbc_read(BQ24770_MANUFACTURER_ID, id);
}

int charger_device_id(int *id)
{
	return sbc_read(BQ24770_DEVICE_ADDRESS, id);
}

int charger_get_option(int *option)
{
	return sbc_read(BQ24770_CHARGE_OPTION0, option);
}

int charger_set_option(int option)
{
	return sbc_write(BQ24770_CHARGE_OPTION0, option);
}

/* Charger interfaces */

const struct charger_info *charger_get_info(void)
{
	return &bq24770_charger_info;
}

int charger_get_status(int *status)
{
	int rv;
	int option;

	rv = charger_get_option(&option);
	if (rv)
		return rv;

	/* Default status */
	*status = CHARGER_LEVEL_2;

	if (option & OPTION0_CHARGE_INHIBIT)
		*status |= CHARGER_CHARGE_INHIBITED;

	return EC_SUCCESS;
}

int charger_set_mode(int mode)
{

	int rv;
	int option;

	rv = charger_get_option(&option);
	if (rv)
		return rv;

	if (mode & CHARGE_FLAG_INHIBIT_CHARGE) {
		option |= OPTION0_CHARGE_INHIBIT;
		sbc_write(BQ24770_CHARGE_OPTION1, 0x0211);
	} else
		option &= ~OPTION0_CHARGE_INHIBIT;

	return charger_set_option(option);
}

int charger_get_current(int *current)
{
	int rv;
	int reg;

	rv = sbc_read(BQ24770_CHARGE_CURRENT, &reg);
	if (rv)
		return rv;

	*current = REG_TO_CURRENT(reg, R_SNS);
	return EC_SUCCESS;
}

int charger_set_current(int current)
{
	current = charger_closest_current(current);

	return sbc_write(BQ24770_CHARGE_CURRENT,
			CURRENT_TO_REG(current, R_SNS));
}

int charger_get_voltage(int *voltage)
{
	return sbc_read(BQ24770_MAX_CHARGE_VOLTAGE, voltage);
}

int charger_set_voltage(int voltage)
{
	return sbc_write(BQ24770_MAX_CHARGE_VOLTAGE, voltage);
}

/* Charging power state initialization */
int charger_post_init(void)
{
	int rv;
	int value;

	rv = sbc_write(BQ24770_CHARGE_OPTION2, 0);	/* disable ILIM */
	charger_set_mode(CHARGE_FLAG_INHIBIT_CHARGE);	/* disable charge */
	rv = sbc_write(BQ24770_CHARGE_OPTION1, 0x0211); /* enalbe auto wakeup */

	sbc_read(BQ24770_CHARGE_OPTION0, &value);
	ccprintf("\n0x%x = 0x%04x\n", BQ24770_CHARGE_OPTION0, value);

	sbc_read(BQ24770_CHARGE_OPTION1, &value);
	ccprintf("0x%x = 0x%04x\n", BQ24770_CHARGE_OPTION1, value);

	sbc_read(BQ24770_CHARGE_OPTION2, &value);
	ccprintf("0x%x = 0x%04x\n---------\n", BQ24770_CHARGE_OPTION2, value);

	return rv;

#if 0
#ifdef CONFIG_CHARGER_ILIM_PIN_DISABLED
	int rv;
	int option;
#endif

#ifdef CONFIG_CHARGER_ILIM_PIN_DISABLED
	/* Read the external ILIM pin enabled flag. */
	rv = sbc_read(BQ24770_CHARGE_OPTION2, &option);
	if (rv)
		return rv;

	/* Set ILIM pin disabled if it is currently enabled. */
	if (option & OPTION2_EN_EXTILIM) {
		option &= ~OPTION2_EN_EXTILIM;
		rv = sbc_write(BQ24770_CHARGE_OPTION2, option);
	}

	return rv;
#else
	return EC_SUCCESS;
#endif
#endif

}

int charger_discharge_on_ac(int enable)
{
	int rv;
	int option;

	rv = charger_get_option(&option);
	if (rv)
		return rv;

	if (enable)
		rv = charger_set_option(option | OPTION0_LEARN_ENABLE);
	else
		rv = charger_set_option(option & ~OPTION0_LEARN_ENABLE);

	return rv;
}
