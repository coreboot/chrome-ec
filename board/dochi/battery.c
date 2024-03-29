/* Copyright 2023 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery_fuel_gauge.h"
#include "cbi.h"
#include "common.h"
#include "compile_time_macros.h"
#include "gpio.h"
/*
 * Battery info for all Dochi battery types. Note that the fields
 * start_charging_min/max and charging_min/max are not used for the charger.
 * The effective temperature limits are given by discharging_min/max_c.
 *
 * Fuel Gauge (FG) parameters which are used for determining if the battery
 * is connected, the appropriate ship mode (battery cutoff) command, and the
 * charge/discharge FETs status.
 *
 * Ship mode (battery cutoff) requires 2 writes to the appropriate smart battery
 * register. For some batteries, the charge/discharge FET bits are set when
 * charging/discharging is active, in other types, these bits set mean that
 * charging/discharging is disabled. Therefore, in addition to the mask for
 * these bits, a disconnect value must be specified. Note that for TI fuel
 * gauge, the charge/discharge FET status is found in Operation Status (0x54),
 * but a read of Manufacturer Access (0x00) will return the lower 16 bits of
 * Operation status which contains the FET status bits.
 *
 * The assumption for battery types supported is that the charge/discharge FET
 * status can be read with a sb_read() command and therefore, only the register
 * address, mask, and disconnect value need to be provided.
 */
const struct batt_conf_embed board_battery_info[] = {
	/* POW-TECH GQA05 Battery Information */
	[BATTERY_POWER_TECH] = {
			/* BQ40Z50 Fuel Gauge */
		.manuf_name = "POW-TECH",
		.device_name = "BATGQA05L22",
		.config = {
			.fuel_gauge = {
				.ship_mode = {
					.reg_addr = 0x00,
					.reg_data = { 0x0010, 0x0010 },
				},
				.fet = {
					.reg_addr = 0x00,
					.reg_mask = 0x2000, /* XDSG */
					.disconnect_val = 0x2000,
				},
				.flags = FUEL_GAUGE_FLAG_MFGACC,
			},
			.batt_info = {
				.voltage_max = TARGET_WITH_MARGIN(13050, 5),
				.voltage_normal		= 11400, /* mV */
				.voltage_min		= 9000, /* mV */
				.precharge_current	= 280,	/* mA */
				.start_charging_min_c	= 0,
				.start_charging_max_c	= 45,
				.charging_min_c		= 0,
				.charging_max_c		= 45,
				.discharging_min_c	= -10,
				.discharging_max_c	= 60,
			},
		},
	},
	/* LGC L17L3PB0 Battery Information */
	/*
	 * Battery info provided by ODM on b/143477210, comment #11
	 */
	[BATTERY_LGC011] = {
		.manuf_name = "LGC",
		.config = {
			.fuel_gauge = {
				.ship_mode = {
					.reg_addr = 0x00,
					.reg_data = { 0x0010, 0x0010 },
				},
				.fet = {
					.reg_addr = 0x0,
					.reg_mask = 0x6000,
					.disconnect_val = 0x6000,
				},
			},
			.batt_info = {
				.voltage_max = TARGET_WITH_MARGIN(13200, 5),
				.voltage_normal		= 11550, /* mV */
				.voltage_min		= 9000, /* mV */
				.precharge_current	= 256,	/* mA */
				.start_charging_min_c	= 0,
				.start_charging_max_c	= 45,
				.charging_min_c		= 0,
				.charging_max_c		= 60,
				.discharging_min_c	= 0,
				.discharging_max_c	= 75,
			},
		},
	},
	/* COSMX AP20CBL Battery Information (new firmware ver) */
	[BATTERY_COSMX_AP20CBL_004] = {
		.manuf_name = "COSMX KT0030B004",
		.device_name = "AP20CBL",
		.config = {
			.fuel_gauge = {
				.ship_mode = {
					.reg_addr = 0x3A,
					.reg_data = { 0xC574, 0xC574 },
				},
				.fet = {
					.reg_addr = 0x0,
					.reg_mask = 0x2000,
					.disconnect_val = 0x2000,
					.cfet_mask = 0x4000,
					.cfet_off_val = 0x4000,
				},
				.flags = FUEL_GAUGE_FLAG_MFGACC,
			},
			.batt_info = {
				.voltage_max            = 13200,
				.voltage_normal         = 11550,
				.voltage_min            = 9000,
				.precharge_current      = 256,
				.start_charging_min_c   = 0,
				.start_charging_max_c   = 50,
				.charging_min_c         = 0,
				.charging_max_c         = 60,
				.discharging_min_c      = -20,
				.discharging_max_c      = 75,
			},
		},
	},
	/* SIMPLO AP23A7L Battery Information */
	[BATTERY_AP23A7L] = {
		.manuf_name = "SMP KT00307012",
		.device_name = "AP23A7L",
		.config = {
			.fuel_gauge = {
				.ship_mode = {
					.reg_addr = 0x3A,
					.reg_data = { 0xC574, 0xC574 },
				},
				.fet = {
					.reg_addr = 0x0,
					.reg_mask = 0x8000,
					.disconnect_val = 0x0000,
					.cfet_mask = 0x4000,
					.cfet_off_val = 0x0000,
				},
			},
			.batt_info = {
				.voltage_max		= 13200,
				.voltage_normal		= 11250, /* mV */
				.voltage_min		= 9000, /* mV */
				.precharge_current	= 256,	/* mA */
				.start_charging_min_c	= 0,
				.start_charging_max_c	= 45,
				.charging_min_c		= 0,
				.charging_max_c		= 60,
				.discharging_min_c	= -20,
				.discharging_max_c	= 60,
			},
		},
	},
	/* LGES AP23A8L Battery Information */
	[BATTERY_AP23A8L] = {
		.manuf_name = "LGES KT0030G025",
		.device_name = "AP23A8L",
		.config = {
			.fuel_gauge = {
				.ship_mode = {
					.reg_addr = 0x3A,
					.reg_data = { 0xC574, 0xC574 },
				},
				.fet = {
					.reg_addr = 0x0,
					.reg_mask = 0x8000,
					.disconnect_val = 0x0000,
					.cfet_mask = 0x4000,
					.cfet_off_val = 0x0000,
				},
			},
			.batt_info = {
				.voltage_max		= 13200,
				.voltage_normal		= 11280, /* mV */
				.voltage_min		= 9000, /* mV */
				.precharge_current	= 256,	/* mA */
				.start_charging_min_c	= 0,
				.start_charging_max_c	= 50,
				.charging_min_c		= 0,
				.charging_max_c		= 60,
				.discharging_min_c	= -20,
				.discharging_max_c	= 75,
			},
		},
	},
};
BUILD_ASSERT(ARRAY_SIZE(board_battery_info) == BATTERY_TYPE_COUNT);

const enum battery_type DEFAULT_BATTERY_TYPE = BATTERY_COSMX_AP20CBL_004;

enum battery_present battery_hw_present(void)
{
	/* The GPIO is low when the battery is physically present */
	return gpio_get_level(GPIO_EC_BATT_PRES_ODL) ? BP_NO : BP_YES;
}
