/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "adc.h"
#include "battery.h"
#include "battery_smart.h"
#include "util.h"
#include "console.h"

/* Console output macros */
#define CPRINTS(format, args...) cprints(CC_CHARGER, format, ## args)

/* Shutdown mode parameter to write to manufacturer access register */
#define SB_SHIP_MODE_REG	0x3a
#define SB_SHUTDOWN_DATA	0xC574

/*
 * Since battery_is_present() is executed earlier than battery_get_info(),
 * which mean support_batteries[] will be used before assigning value
 * to it. Without this, A static value would initialize to 0, and system
 * would get a Sony battery, that would be wrong.
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
	SONY = 0,
	SANYO,
	UNKNOWN,
	/* Number of types, not a real type */
	BATTERY_TYPE_COUNT,
};
struct battery_device {
	char				manuf[9];
	char				device[9];
	int				design_mv;
	const struct battery_info	*battery_info;
};

/*
 * Used for the case that battery cannot be detected, such as the pre-charge
 * case. In this case, we need to provide the battery with the enough voltage
 * (usually the highest voltage among batteries, but the smallest precharge
 * current). This should be as conservative as possible.
 */
static const struct battery_info info_precharge = {
	.voltage_max    = 13050,  /* the max voltage among batteries */
	.voltage_normal = 11250,  /* min */
	.voltage_min    =  9100,

	/* Pre-charge values. */
	.precharge_current  = 256,  /* mA, the min current among batteries */

	.start_charging_min_c = 0,
	.start_charging_max_c = 50,
	.charging_min_c       = 0,
	.charging_max_c       = 60,
	.discharging_min_c    = 0,
	.discharging_max_c    = 40, /* min */
};

static const struct battery_info info_sony = {
	.voltage_max = 13050, /* mV */
	.voltage_normal = 11400,
	.voltage_min = 9100,

	.precharge_current = 256, /* mA */

	.start_charging_min_c = 0,
	.start_charging_max_c = 50,
	.charging_min_c = 0,
	.charging_max_c = 60,
	.discharging_min_c = -20,
	.discharging_max_c = 60,
};

static const struct battery_info info_sanyo = {
	.voltage_max = 13050, /* mV */
	.voltage_normal = 11250,
	.voltage_min = 9100,

	.precharge_current = 392, /* mA */

	.start_charging_min_c = 0,
	.start_charging_max_c = 50,
	.charging_min_c = 0,
	.charging_max_c = 60,
	.discharging_min_c = 0,
	.discharging_max_c = 40,
};

/* see enum battery_type */
static const struct battery_device support_batteries[BATTERY_TYPE_COUNT] = {
	{
		.manuf		= "SONYCorp",
		.device		= "AP13J4K",
		.design_mv	= 11400,
		.battery_info	= &info_sony,
	},
	{
		.manuf		= "SANYO",
		.device		= "AP13J3K",
		.design_mv	= 11400,
		.battery_info	= &info_sanyo,
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
	char manuf[9];
	char device[9];
	int design_mv;

	if (battery_manufacturer_name(manuf, sizeof(manuf))) {
		CPRINTS("Failed to get MANUF name");
		goto err_unknown;
	}

	if (battery_device_name(device, sizeof(device))) {
		CPRINTS("Failed to get DEVICE name");
		goto err_unknown;
	}

	if (battery_design_voltage((int *)&design_mv)) {
		CPRINTS("Failed to get DESIGN_VOLTAGE");
		goto err_unknown;
	}

	for (i = 0; i < BATTERY_TYPE_COUNT; ++i) {
		if ((strcasecmp(support_batteries[i].manuf, manuf) == 0) &&
		    (strcasecmp(support_batteries[i].device, device) == 0) &&
		    (support_batteries[i].design_mv == design_mv)) {
			CPRINTS("battery Manuf:%s, Device=%s, design=%u",
				manuf, device, design_mv);
			batt_inserted = i;

			return support_batteries[batt_inserted].battery_info;
		}
	}

	CPRINTS("un-recognized battery Manuf:%s, Device:%s",
		manuf, device);

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

#ifdef CONFIG_BATTERY_PRESENT_CUSTOM
/*
 * Physical detection of battery via ADC.
 *
 * Upper limit of valid voltage level (mV), when battery is attached to ADC
 * port, is across the internal thermistor with external pullup resistor.
 */
#define BATT_PRESENT_MV  1500
#define ABLE 1
#define UNABLE 0
#define SONY_DISCHARGE_FET_BIT (0x1 << 15)
#define SANYO_DISCHARGE_FET_BIT (0x1 << 14)
static int can_battery_provide_power(enum battery_type type)
{
	int batt_discharge_fet = -1;
	int rv = -1;

	/*
	 * Sony's and Sanyo's can check FETs (since status is not implemented)
	 * Make sure I2C transactions are success & the battery FET is
	 * Initialized to find out if it is a working battery and it is not in
	 * the cut-off mode.
	 *
	 * FETs are turned off after Power Shutdown time.
	 * The device will wake up when a voltage is applied to PACK.
	 * Battery FET will be active until it is initialized.
	 */

	rv = sb_read(SB_MANUFACTURER_ACCESS, &batt_discharge_fet);

	if (rv != EC_SUCCESS && type != UNKNOWN)
		return UNABLE;

	switch (type) {
	case SONY:
		if (batt_discharge_fet & SONY_DISCHARGE_FET_BIT)
			return UNABLE;
		break;
	case SANYO:
		if (!(batt_discharge_fet & SANYO_DISCHARGE_FET_BIT))
			return UNABLE;
		break;
	case UNKNOWN:
		/* give battery a chance to precharge
		   to enable I2C communication */
		break;
	default:
		/* see enum battery_type */
		return UNABLE;
	}

	return ABLE;
}

enum battery_present battery_is_present(void)
{
	enum battery_present batt_pres;

	/*
	 * if voltage is below certain level (dependent on ratio of
	 * internal thermistor and external pullup resister),
	 * battery is attached.
	 */

	batt_pres = (adc_read_channel(ADC_BATT_PRESENT) > BATT_PRESENT_MV) ?
		BP_NO : BP_YES;

	/* In cut-off mode, sony and sanyo battery can't communication
	 * via I2C, but both can get ADC value, check I2C in
	 * can_battery_provide_power().
	 */

	if (batt_pres == BP_YES &&
		can_battery_provide_power(batt_inserted) == UNABLE)
			batt_pres = BP_NO;

	return batt_pres;
}
#endif
