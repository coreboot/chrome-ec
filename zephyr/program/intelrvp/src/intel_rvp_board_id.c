/* Copyright 2022 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "intel_rvp_board_id.h"

#include <zephyr/devicetree.h>

#define DT_DRV_COMPAT intel_rvp_board_id

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "Unsupported RVP Board ID instance");

#define RVP_ID_GPIO_DT_SPEC_GET(idx, node_id, prop) \
	GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx),

#define RVP_ID_CONFIG_LIST(node_id, prop)                                \
	LISTIFY(DT_PROP_LEN(node_id, prop), RVP_ID_GPIO_DT_SPEC_GET, (), \
		node_id, prop)

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#if RVP_ID_HAS_BOM_GPIOS
const struct gpio_dt_spec bom_id_config[] = { RVP_ID_CONFIG_LIST(DT_DRV_INST(0),
								 bom_gpios) };
#endif

#if RVP_ID_HAS_FAB_GPIOS
const struct gpio_dt_spec fab_id_config[] = { RVP_ID_CONFIG_LIST(DT_DRV_INST(0),
								 fab_gpios) };
#endif

const struct gpio_dt_spec board_id_config[] = { RVP_ID_CONFIG_LIST(
	DT_DRV_INST(0), board_gpios) };
#endif /* #if DT_HAS_COMPAT_STATUS_OKAY */
