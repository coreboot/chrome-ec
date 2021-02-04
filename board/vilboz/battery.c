/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery_fuel_gauge.h"
#include "common.h"
#include "util.h"

/*
 * Battery info for all Zork battery types. Note that the fields
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
const struct board_batt_params board_battery_info[] = {
	/* SMP L19M3PG1 */
	[BATTERY_SMP] = {
		.fuel_gauge = {
			.manuf_name = "SMP",
			.device_name = "L19M3PG1",
			.ship_mode = {
				.reg_addr = 0x34,
				.reg_data = { 0x0000, 0x1000 },
			},
			.fet = {
				.reg_addr = 0x34,
				.reg_mask = 0x0100,
				.disconnect_val = 0x0100,
			}
		},
		.batt_info = {
			.voltage_max		= 13200, /* mV */
			.voltage_normal		= 11520, /* mV */
			.voltage_min		= 9000,  /* mV */
			.precharge_current	= 200,	 /* mA */
			.start_charging_min_c	= 0,
			.start_charging_max_c	= 60,
			.charging_min_c		= 0,
			.charging_max_c		= 50,
			.discharging_min_c	= -20,
			.discharging_max_c	= 73,
		},
	},

	/* SMP L20M3PG1 57W
	 * Gauge IC: TI BQ40Z696A
	 */
	[BATTERY_SMP_1] = {
		.fuel_gauge = {
			.manuf_name = "SMP",
			.device_name = "L20M3PG1",
			.ship_mode = {
				.reg_addr = 0x00,
				.reg_data = { 0x0010, 0x0010 },
			},
			.fet = {
				.mfgacc_support = 1,
				.reg_addr = 0x0000,
				.reg_mask = 0x6000,
				.disconnect_val = 0x6000,
			}
		},
		.batt_info = {
			.voltage_max		= 13200, /* mV */
			.voltage_normal		= 11520, /* mV */
			.voltage_min		= 9000,  /* mV */
			.precharge_current	= 247,	 /* mA */
			.start_charging_min_c	= 0,
			.start_charging_max_c	= 50,
			.charging_min_c		= 0,
			.charging_max_c		= 60,
			.discharging_min_c	= -20,
			.discharging_max_c	= 70,
		},
	},

	/* SMP L20M3PG0 47W
	 * Gauge IC: TI BQ40Z696A
	 */
	[BATTERY_SMP_2] = {
		.fuel_gauge = {
			.manuf_name = "SMP",
			.device_name = "L20M3PG0",
			.ship_mode = {
				.reg_addr = 0x00,
				.reg_data = { 0x0010, 0x0010 },
			},
			.fet = {
				.mfgacc_support = 1,
				.reg_addr = 0x0000,
				.reg_mask = 0x6000,
				.disconnect_val = 0x6000,
			}
		},
		.batt_info = {
			.voltage_max		= 13200, /* mV */
			.voltage_normal		= 11520, /* mV */
			.voltage_min		= 9000,  /* mV */
			.precharge_current	= 256,	 /* mA */
			.start_charging_min_c	= 0,
			.start_charging_max_c	= 50,
			.charging_min_c		= 0,
			.charging_max_c		= 60,
			.discharging_min_c	= -20,
			.discharging_max_c	= 70,
		},
	},

	/* SMP L20M3PG3 47W
	 * Gauge IC: Renesas RAJ240047
	 */
	[BATTERY_SMP_3] = {
		.fuel_gauge = {
			.manuf_name = "SMP",
			.device_name = "L20M3PG3",
			.ship_mode = {
				.reg_addr = 0x34,
				.reg_data = { 0x0000, 0x1000 },
			},
			.fet = {
				.reg_addr = 0x0,
				.reg_mask = 0x0010,
				.disconnect_val = 0x0,
			},
		},
		.batt_info = {
			.voltage_max            = 13200, /* mV */
			.voltage_normal         = 11520, /* mV */
			.voltage_min            = 9000,  /* mV */
			.precharge_current      = 256,   /* mA */
			.start_charging_min_c   = 0,
			.start_charging_max_c   = 50,
			.charging_min_c         = 0,
			.charging_max_c         = 60,
			.discharging_min_c      = -20,
			.discharging_max_c      = 70,
		},
	},

	/* LGC  L19L3PG1 */
	[BATTERY_LGC] = {
		.fuel_gauge = {
			.manuf_name = "LGC",
			.device_name = "L19L3PG1",
			.ship_mode = {
				.reg_addr = 0x34,
				.reg_data = { 0x0000, 0x1000 },
			},
			.fet = {
				.reg_addr = 0x34,
				.reg_mask = 0x0100,
				.disconnect_val = 0x0100,
			}
		},
		.batt_info = {
			.voltage_max		= 13200, /* mV */
			.voltage_normal		= 11550, /* mV */
			.voltage_min		= 9000,  /* mV */
			.precharge_current	= 200,	 /* mA */
			.start_charging_min_c	= 0,
			.start_charging_max_c	= 60,
			.charging_min_c		= 0,
			.charging_max_c		= 50,
			.discharging_min_c	= -20,
			.discharging_max_c	= 73,
		},
	},

	/* LGC L20L3PG1 57W
	 * Gauge IC: Renesas
	 */
	[BATTERY_LGC_1] = {
		.fuel_gauge = {
			.manuf_name = "LGC",
			.device_name = "L20L3PG1",
			.ship_mode = {
				.reg_addr = 0x34,
				.reg_data = { 0x0000, 0x1000 },
			},
			.fet = {
				.reg_addr = 0x0,
				.reg_mask = 0x0010,
				.disconnect_val = 0x0,
			},
		},
		.batt_info = {
			.voltage_max		= 13200, /* mV */
			.voltage_normal		= 11580, /* mV */
			.voltage_min		= 9000,  /* mV */
			.precharge_current	= 256,	 /* mA */
			.start_charging_min_c	= 0,
			.start_charging_max_c	= 50,
			.charging_min_c		= 0,
			.charging_max_c		= 60,
			.discharging_min_c	= -20,
			.discharging_max_c	= 70,
		},
	},

	/* LGC L20L3PG0 47W
	 * Gauge IC: Renesas
	 */
	[BATTERY_LGC_2] = {
		.fuel_gauge = {
			.manuf_name = "LGC",
			.device_name = "L20L3PG0",
			.ship_mode = {
				.reg_addr = 0x34,
				.reg_data = { 0x0000, 0x1000 },
			},
			.fet = {
				.reg_addr = 0x0,
				.reg_mask = 0x0010,
				.disconnect_val = 0x0,
			},
		},
		.batt_info = {
			.voltage_max		= 13200, /* mV */
			.voltage_normal		= 11580, /* mV */
			.voltage_min		= 9000,  /* mV */
			.precharge_current	= 256,	 /* mA */
			.start_charging_min_c	= 0,
			.start_charging_max_c	= 50,
			.charging_min_c		= 0,
			.charging_max_c		= 60,
			.discharging_min_c	= -20,
			.discharging_max_c	= 70,
		},
	},

	/* Celxpert  L19C3PG1 */
	[BATTERY_CEL] = {
		.fuel_gauge = {
			.manuf_name = "Celxpert",
			.device_name = "L19C3PG1",
			.ship_mode = {
				.reg_addr = 0x34,
				.reg_data = { 0x0000, 0x1000 },
			},
			.fet = {
				.reg_addr = 0x34,
				.reg_mask = 0x0100,
				.disconnect_val = 0x0100,
			}
		},
		.batt_info = {
			.voltage_max		= 13200, /* mV */
			.voltage_normal		= 11520, /* mV */
			.voltage_min		= 9000,  /* mV */
			.precharge_current	= 200,	 /* mA */
			.start_charging_min_c	= 0,
			.start_charging_max_c	= 60,
			.charging_min_c		= 0,
			.charging_max_c		= 50,
			.discharging_min_c	= -20,
			.discharging_max_c	= 70,
		},
	},

	/* Celxpert L20C3PG0 57W
	 * Gauge IC: TI
	 */
	[BATTERY_CEL_1] = {
		.fuel_gauge = {
			.manuf_name = "Celxpert",
			.device_name = "L20C3PG0",
			.ship_mode = {
				.reg_addr = 0x00,
				.reg_data = { 0x0010, 0x0010 },
			},
			.fet = {
				.mfgacc_support = 1,
				.reg_addr = 0x0000,
				.reg_mask = 0x6000,
				.disconnect_val = 0x6000,
			}
		},
		.batt_info = {
			.voltage_max		= 13200, /* mV */
			.voltage_normal		= 11520, /* mV */
			.voltage_min		= 9000,  /* mV */
			.precharge_current	= 200,	 /* mA */
			.start_charging_min_c	= 0,
			.start_charging_max_c	= 50,
			.charging_min_c		= 0,
			.charging_max_c		= 60,
			.discharging_min_c	= -20,
			.discharging_max_c	= 70,
		},
	},

	/* SUNWODA L20D3PG1 57W
	 * Gauge IC: TI
	 */
	[BATTERY_SUNWODA] = {
		.fuel_gauge = {
			.manuf_name = "Sunwoda",
			.device_name = "L20D3PG1",
			.ship_mode = {
				.reg_addr = 0x00,
				.reg_data = { 0x0010, 0x0010 },
			},
			.fet = {
				.mfgacc_support = 1,
				.reg_addr = 0x0000,
				.reg_mask = 0x6000,
				.disconnect_val = 0x6000,
			}
		},
		.batt_info = {
			.voltage_max		= 13200, /* mV */
			.voltage_normal		= 11520, /* mV */
			.voltage_min		= 9000,  /* mV */
			.precharge_current	= 250,	 /* mA */
			.start_charging_min_c	= 0,
			.start_charging_max_c	= 50,
			.charging_min_c		= 0,
			.charging_max_c		= 60,
			.discharging_min_c	= -20,
			.discharging_max_c	= 70,
		},
	},

	/* SUNWODA L20D3PG0 47W
	 * Gauge IC: TI
	 */
	[BATTERY_SUNWODA_1] = {
		.fuel_gauge = {
			.manuf_name = "Sunwoda",
			.device_name = "L20D3PG0",
			.ship_mode = {
				.reg_addr = 0x00,
				.reg_data = { 0x0010, 0x0010 },
			},
			.fet = {
				.mfgacc_support = 1,
				.reg_addr = 0x0000,
				.reg_mask = 0x6000,
				.disconnect_val = 0x6000,
			}
		},
		.batt_info = {
			.voltage_max		= 13200, /* mV */
			.voltage_normal		= 11520, /* mV */
			.voltage_min		= 9000,  /* mV */
			.precharge_current	= 205,	 /* mA */
			.start_charging_min_c	= 0,
			.start_charging_max_c	= 50,
			.charging_min_c		= 0,
			.charging_max_c		= 60,
			.discharging_min_c	= -20,
			.discharging_max_c	= 70,
		},
	},
};
BUILD_ASSERT(ARRAY_SIZE(board_battery_info) == BATTERY_TYPE_COUNT);

const enum battery_type DEFAULT_BATTERY_TYPE = BATTERY_SMP;
