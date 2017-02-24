/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery.h"
#include "battery_smart.h"
#include "bd9995x.h"
#include "charge_state.h"
#include "console.h"
#include "ec_commands.h"
#include "gpio.h"
#include "util.h"

/* Console output macros */
#define CPRINTS(format, args...) cprints(CC_CHARGER, format, ## args)

/* array size of block reading */
#define CUSTOM_DATA_SIZE	9

/* Shutdown mode parameter to write to manufacturer access register */
#define SB_SHIP_MODE_REG	0x3a
#define SB_SHUTDOWN_DATA	0xC574

/*
 * Since battery_is_present() is executed earlier than battery_get_info(),
 * which mean support_batteries[] will be used before assigning value
 * to it. Without this, A static value would initialize to 0, and system
 * would get SMP battery, that would be wrong.
 * If put unknown firstly and treat unknown battery as UNABLE to provide
 * power, we will never pre-charge a I2C failed battery. Treat it as ABLE,
 * system reboots because battery could not provide power.
 *
 * So INIT state is used for preventing an initial wrong state, and never
 * use support_batteries[INIT] in any other place.
 *
 * enum battery_type must be the same order as support_batteries[]
 */
enum battery_type {
	INIT = -1,	/* only use this as default static value */
	SMP = 0,
	LG,
	UNKNOWN,
	/* Number of types, not a real type */
	BATTERY_TYPE_COUNT,
};

struct battery_device {
	char				manuf[CUSTOM_DATA_SIZE];
	char				device[CUSTOM_DATA_SIZE];
	int				design_mv;
	const struct battery_info	*battery_info;
};

/*
 * Used for the case that battery cannot be detected, such as the pre-charge
 * case. In this case, we need to provide the unknown battery with the enough
 * voltage (usually the highest voltage among batteries, but the smallest
 * precharge current). This should be as conservative as possible.
 */
static const struct battery_info info_precharge = {
	.voltage_max    = 8700,  /* the max voltage among batteries */
	.voltage_normal = 7600,  /* min */
	.voltage_min    =  6100,

	/* Pre-charge values. */
	.precharge_current  = 180,  /* mA, the min current among batteries */

	.start_charging_min_c = 0,
	.start_charging_max_c = 50,
	.charging_min_c       = 0,
	.charging_max_c       = 50,
	.discharging_min_c    = -20,
	.discharging_max_c    = 75, /* min */
};

static const struct battery_info info_SMP = {
	.voltage_max = 8700, /* mV */
	.voltage_normal = 7640,
	.voltage_min = 6100,

	.precharge_current = 180, /* mA */

	.start_charging_min_c = 0,
	.start_charging_max_c = 60,
	.charging_min_c = 0,
	.charging_max_c = 60,
	.discharging_min_c = -20,
	.discharging_max_c = 75,
};

static const struct battery_info info_LG = {
	.voltage_max = 8600, /* mV */
	.voltage_normal = 7600,
	.voltage_min = 6100,

	.precharge_current = 256, /* mA */

	.start_charging_min_c = 0,
	.start_charging_max_c = 50,
	.charging_min_c = 0,
	.charging_max_c = 50,
	.discharging_min_c = -20,
	.discharging_max_c = 75,
};

/* see enum battery_type */
static const struct battery_device support_batteries[BATTERY_TYPE_COUNT] = {
	{
		.manuf		= "SMP-SDI3",
		.device		= "AC16B7K",
		.design_mv	= 7640,
		.battery_info	= &info_SMP,
	},
	{
		.manuf		= "LGC",
		.device		= "Empoli",
		.design_mv	= 7600,
		.battery_info	= &info_LG,
	},
	{
		.manuf		= "Unknown",
		.battery_info	= &info_precharge,
	},
};

static enum battery_type batt_inserted = INIT;

const struct battery_info *battery_get_info(void)
{
	int i;
	char manuf[CUSTOM_DATA_SIZE];
	char device[CUSTOM_DATA_SIZE];
	int design_mv;
	static int print = 1;

	if ((batt_inserted != INIT) && (batt_inserted != UNKNOWN))
		return support_batteries[batt_inserted].battery_info;

	if (battery_manufacturer_name(manuf, sizeof(manuf)))
		goto err_unknown;
	if (print == 1) {
		CPRINTS("Battery=%s\n", manuf);
		print = 0;
	}

	if (battery_device_name(device, sizeof(device)))
		goto err_unknown;

	if (battery_design_voltage(&design_mv))
		goto err_unknown;

	for (i = 0; i < BATTERY_TYPE_COUNT; ++i) {
		if ((strcasecmp(support_batteries[i].manuf, manuf) == 0) &&
		    (strcasecmp(support_batteries[i].device, device) == 0) &&
		    (support_batteries[i].design_mv == design_mv)) {
			batt_inserted = i;

			return support_batteries[batt_inserted].battery_info;
		}
	}

	CPRINTS("un-recognized battery Manuf:%s, Device:%s", manuf, device);

err_unknown:
	batt_inserted = UNKNOWN;
	return support_batteries[batt_inserted].battery_info;
}

int board_cut_off_battery(void)
{
	int rv;

	/* Ship mode command must be sent twice to take effect */
	rv = sb_write(SB_SHIP_MODE_REG, SB_SHUTDOWN_DATA);

	if (rv != EC_SUCCESS)
		return rv;

	return sb_write(SB_SHIP_MODE_REG, SB_SHUTDOWN_DATA);
}
/*
 * 1. Physical detection of battery via GPIO.
 * 2. Check DFET is on/off by reading battery custom register
 *
 *    SMP  : SB_MANUFACTURER_ACCESS.[13] : Discharge Disabled
 *	   : 0b - Allowed to discharge
 *	   : 1b - Not allowed to discharge
 */
#define SMP_DISCHARGE_DISABLE_FET_BIT	(0x01 << 13)
enum battery_present battery_is_present(void)
{
	int batt_discharge_fet = -1;
	enum battery_present batt_pres = BP_NOT_SURE;

	if (battery_is_cut_off())
		return BP_NO;

	batt_pres = gpio_get_level(GPIO_EC_BATT_PRES_L) ? BP_NO : BP_YES;

	if (batt_pres == BP_YES) {
		if (sb_read(SB_MANUFACTURER_ACCESS, &batt_discharge_fet))
			batt_pres = BP_NOT_SURE;
		else if ((batt_discharge_fet & SMP_DISCHARGE_DISABLE_FET_BIT)
			  != 0)
			batt_pres = BP_NOT_SURE;
	}

	return batt_pres;
}

int board_battery_initialized(void)
{
	return (battery_is_present() == BP_YES);
}

int charger_profile_override(struct charge_state_data *curr)
{
	if (curr->batt.flags & BATT_FLAG_WANT_CHARGE)
		charger_discharge_on_ac(0);

	return EC_SUCCESS;
}

enum ec_status charger_profile_override_get_param(uint32_t param,
						  uint32_t *value)
{
	return EC_RES_INVALID_PARAM;
}

enum ec_status charger_profile_override_set_param(uint32_t param,
						  uint32_t value)
{
	return EC_RES_INVALID_PARAM;
}
