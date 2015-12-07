/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery.h"
#include "battery_smart.h"
#include "console.h"

#define CPRINTS(format, args...) cprints(CC_CHARGER, format, ## args)

/* Shutdown mode parameter to write to manufacturer access register */
#define SB_SHUTDOWN_DATA	0x0010

enum BatterySeries {
	UNKNOWN,
	SERIES_2,
	SERIES_3,
};

static enum BatterySeries series;

static const struct battery_info info_prechg = {
	.voltage_max = 8700,/* mV */
	.voltage_normal = 8400,
	.voltage_min = 6000,
	.precharge_current = 256,/* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 55,
	.charging_min_c = 0,
	.charging_max_c = 55,
	.discharging_min_c = 0,
	.discharging_max_c = 60,
};

static const struct battery_info info_2s = {
	.voltage_max = 8700,/* mV */
	.voltage_normal = 7600,
	.voltage_min = 6000,
	.precharge_current = 256,/* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 55,
	.charging_min_c = 0,
	.charging_max_c = 55,
	.discharging_min_c = 0,
	.discharging_max_c = 60,
};

static const struct battery_info info_3s = {
	.voltage_max = 13050,/* mV */
	.voltage_normal = 11400,
	.voltage_min = 9000,
	.precharge_current = 256,/* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 55,
	.charging_min_c = 0,
	.charging_max_c = 55,
	.discharging_min_c = 0,
	.discharging_max_c = 60,
};

const struct battery_info *battery_get_info(void)
{
	const struct battery_info *ret = &info_prechg;

	if (series == UNKNOWN) {
		char manuf[11];

		if (!battery_manufacturer_name(manuf, sizeof(manuf))) {
			if (manuf[9] == 'A') {
				CPRINTS("2s battery");
				series = SERIES_2;
			} else if (manuf[9] == 'B') {
				CPRINTS("3s battery");
				series = SERIES_3;
			} else {
				CPRINTS("unknown battery");
			}
		} else {
			CPRINTS("read manufacture name error");
		}
	}

	switch (series) {
	case UNKNOWN:
		ret = &info_prechg;
		break;
	case SERIES_2:
		ret = &info_2s;
		break;
	case SERIES_3:
		ret = &info_3s;
		break;
	}

	return ret;
}

int board_cut_off_battery(void)
{
	int rv;

	/* Ship mode command must be sent twice to take effect */
	rv = sb_write(SB_MANUFACTURER_ACCESS, SB_SHUTDOWN_DATA);

	if (rv != EC_SUCCESS)
		return rv;

	return sb_write(SB_MANUFACTURER_ACCESS, SB_SHUTDOWN_DATA);
}
