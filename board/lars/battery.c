/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery.h"
#include "battery_smart.h"
#include "util.h"
#include "console.h"

/* Console output macros */
#define CPRINTS(format, args...) cprints(CC_CHARGER, format, ## args)

/* Shutdown mode parameter to write to manufacturer access register */
#define SB_SHIP_MODE_REG	0x3a
#define SB_SHUTDOWN_DATA	0xC574

static struct battery_info *battery_info;

struct battery_device {
	char			manuf[9];
	char			device[9];
	int			design_mv;
	struct battery_info	*battery_info;
};

/*
 * Used for the case that battery cannot be detected, such as the pre-charge
 * case. In this case, we need to provide the battery with the enough voltage
 * (usually the highest voltage among batteries, but the smallest precharge
 * current). This should be as conservative as possible.
 */
static struct battery_info info_precharge = {
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

static struct battery_info info_sony = {
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

static struct battery_info info_sanyo = {
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

static struct battery_device support_batteries[] = {
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
};

const struct battery_info *battery_get_info(void)
{
	int i;
	char manuf[9];
	char device[9];
	int design_mv;

	if (battery_manufacturer_name(manuf, sizeof(manuf))) {
		CPRINTS("Failed to get MANUF name");
		return &info_precharge;
	}

	if (battery_device_name(device, sizeof(device))) {
		CPRINTS("Failed to get DEVICE name");
		return &info_precharge;
	}

	if (battery_design_voltage((int *)&design_mv)) {
		CPRINTS("Failed to get DESIGN_VOLTAGE");
		return &info_precharge;
	}

	for (i = 0; i < ARRAY_SIZE(support_batteries); ++i) {
		if ((strcasecmp(support_batteries[i].manuf, manuf) == 0) &&
		    (strcasecmp(support_batteries[i].device, device) == 0) &&
		    (support_batteries[i].design_mv == design_mv)) {
			CPRINTS("battery Manuf:%s, Device=%s, design=%u",
				manuf, device, design_mv);
			battery_info = support_batteries[i].battery_info;
			return battery_info;
		}
	}

	CPRINTS("un-recognized battery Manuf:%s, Device:%s",
		manuf, device);

	return &info_precharge;
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
