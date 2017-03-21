/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery.h"
#include "battery_smart.h"
#include "console.h"
#include "util.h"

/* Console output macros */
#define CPRINTS(format, args...) cprints(CC_CHARGER, format, ## args)

enum battery_type {
	INIT = -1,
	SIMPLO = 0,
	SUNWODA,
	BATTERY_TYPE_COUNT,
	DEFAULT_BATTERY_TYPE = SIMPLO,
};

static enum battery_type board_battery_type = INIT;

struct ship_mode_info {
	int ship_mode_reg;
	int ship_mode_data[2];
};

struct battery_device {
	const char			*manuf;
	const char			*device;
	int				design_mv;
	const struct battery_info	*battery_info;
	const struct ship_mode_info	*ship_mode_info;
};

static const struct battery_info info_simplo = {
	.voltage_max = 12600,
	.voltage_normal = 11100,
	.voltage_min = 9100,
	/* Pre-charge values. */
	.precharge_current = 256, /* mA */

	.start_charging_min_c = 0,
	.start_charging_max_c = 50,
	.charging_min_c = 0,
	.charging_max_c = 60,
	.discharging_min_c = 0,
	.discharging_max_c = 60,
};

static const struct ship_mode_info ship_mode_info_simplo = {
	.ship_mode_reg = 0x34,
	.ship_mode_data = { 0x0000, 0x1000 },
};

static const struct battery_info info_sunwoda = {
	.voltage_max = 13050,
	.voltage_normal = 11400,
	.voltage_min = 9000,
	/* Pre-charge values. */
	.precharge_current = 300, /* mA */

	.start_charging_min_c = 0,
	.start_charging_max_c = 45,
	.charging_min_c = 0,
	.charging_max_c = 60,
	.discharging_min_c = 0,
	.discharging_max_c = 60,
};

static const struct ship_mode_info ship_mode_info_sunwoda = {
	.ship_mode_reg = 0x00,
	.ship_mode_data = { 0x0010, 0x0010 },
};

static const struct battery_device support_batteries[BATTERY_TYPE_COUNT] = {
	[SIMPLO] = {
		.manuf		= "SMP",
		.device		= "L15M3PB1",
		.design_mv	= 11100,
		.battery_info	= &info_simplo,
		.ship_mode_info	= &ship_mode_info_simplo,
	},
	[SUNWODA] = {
		.manuf		= "sunwoda",
		.device		= "BBLD485595",
		.design_mv	= 11400,
		.battery_info	= &info_sunwoda,
		.ship_mode_info	= &ship_mode_info_sunwoda,
	},
};

static inline const struct battery_device *get_battery_device(void)
{
	int i;
	char manuf[32], device[32];
	int design_mv;

	if (board_battery_type != INIT)
		return &support_batteries[board_battery_type];

	if (battery_manufacturer_name(manuf, sizeof(manuf)))
		goto err_unknown;

	if (battery_device_name(device, sizeof(device)))
		goto err_unknown;

	if (battery_design_voltage((int *)&design_mv))
		goto err_unknown;

	for (i = 0; i < BATTERY_TYPE_COUNT; ++i) {
		if ((strcasecmp(support_batteries[i].manuf, manuf) == 0) &&
		    (strcasecmp(support_batteries[i].device, device) == 0) &&
		    (support_batteries[i].design_mv == design_mv)) {
			board_battery_type = i;

			return &support_batteries[board_battery_type];
		}
	}

	CPRINTS("un-recognized battery Manuf:%s, Device:%s",
		manuf, device);

err_unknown:
	CPRINTS("Use default battery profile");
	board_battery_type = DEFAULT_BATTERY_TYPE;

	return &support_batteries[board_battery_type];
}

const struct battery_info *battery_get_info(void)
{
	return get_battery_device()->battery_info;
}

int board_cut_off_battery(void)
{
	int rv;
	const struct ship_mode_info *ship_mode_inf =
				get_battery_device()->ship_mode_info;

	/* Ship mode command must be sent twice to take effect */
	rv = sb_write(ship_mode_inf->ship_mode_reg,
			ship_mode_inf->ship_mode_data[0]);

	if (rv != EC_SUCCESS)
		return rv;

	return sb_write(ship_mode_inf->ship_mode_reg,
			ship_mode_inf->ship_mode_data[1]);
}
