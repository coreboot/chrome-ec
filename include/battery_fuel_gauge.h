/* Copyright 2018 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery fuel gauge parameters
 */

#ifndef __CROS_EC_BATTERY_FUEL_GAUGE_H
#define __CROS_EC_BATTERY_FUEL_GAUGE_H

#include "battery.h"
#include "common.h"
#include "ec_commands.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Represent a battery config embedded in FW.
 */
struct batt_conf_embed {
	char *manuf_name;
	char *device_name;
	struct board_batt_params config;
};

/* Forward declare board specific data used by common code */
extern const struct batt_conf_embed board_battery_info[];
extern const enum battery_type DEFAULT_BATTERY_TYPE;

/**
 * Return the board-specific default battery type.
 *
 * @return a value of `enum battery_type`.
 */
__override_proto int board_get_default_battery_type(void);

/**
 * Detect a battery model.
 *
 * When no matching is found, it'll use the default config.
 *
 * Concurrency isn't considered because this routine shall be called only from:
 *
 * - HOOK_INIT (before charge_task starts) AND
 * - charge_task (when a battery becomes newly present),
 *
 * which are serialized.
 */
void init_battery_type(void);

/**
 * Return pointer to active battery config.
 */
const struct batt_conf_embed *get_batt_conf(void);

/**
 * Return 1 if CFET is disabled, 0 if enabled. -1 if an error was encountered.
 * If the CFET mask is not defined, it will return 0.
 */
int battery_is_charge_fet_disabled(void);

/**
 * Send the fuel gauge sleep command through SMBus.
 *
 * @return	0 if successful, non-zero if error occurred
 */
enum ec_error_list battery_sleep_fuel_gauge(void);

/**
 * Report the absolute difference between the highest and lowest cell voltage in
 * the battery pack, in millivolts.  On error or unimplemented, returns '0'.
 */
__override_proto int
board_battery_imbalance_mv(const struct board_batt_params *info);

#ifdef __cplusplus
}
#endif

#endif /* __CROS_EC_BATTERY_FUEL_GAUGE_H */
