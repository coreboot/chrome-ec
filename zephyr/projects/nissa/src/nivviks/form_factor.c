/* Copyright 2022 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <devicetree.h>
#include <init.h>
#include <logging/log.h>

#include "accelgyro.h"
#include "cros_cbi.h"
#include "hooks.h"
#include "motionsense_sensors.h"

#include "nissa_common.h"

LOG_MODULE_DECLARE(nissa, CONFIG_NISSA_LOG_LEVEL);

/*
 * Mainboard orientation support.
 */

#define ALT_MAT		SENSOR_ROT_STD_REF_NAME(DT_NODELABEL(base_rot_inverted))
#define BASE_SENSOR	SENSOR_ID(DT_NODELABEL(base_accel))

static int form_factor_init(const struct device *unused)
{
	int ret;
	uint32_t val;
	/*
	 * If the firmware config indicates
	 * an inverted form factor, use the alternative
	 * rotation matrix.
	 */
	ret = cros_cbi_get_fw_config(FW_BASE_INVERSION, &val);
	if (ret != 0) {
		LOG_ERR("Error retrieving CBI FW_CONFIG field %d",
			FW_BASE_INVERSION);
		return 0;
	}
	if (val == FW_BASE_INVERTED) {
		LOG_INF("Switching to inverted base");
		motion_sensors[BASE_SENSOR].rot_standard_ref = &ALT_MAT;
	}
	return 0;
}

SYS_INIT(form_factor_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
