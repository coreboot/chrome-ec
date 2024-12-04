/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * This file tests various Attached.Snk scenarios.
 */

#include "emul/emul_pdc.h"
#include "usbc/pdc_power_mgmt.h"

#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

LOG_MODULE_REGISTER(pdc_attached_snk);

#define RTS5453P_NODE DT_NODELABEL(pdc_emul1)
static const struct emul *emul = EMUL_DT_GET(RTS5453P_NODE);
#define TEST_PORT 0

ZTEST_SUITE(pdc_attached_snk, NULL, NULL, NULL, NULL, NULL);

/* Verify the DUT sinks the correct amount of current, when
 * the partner sends new source caps.
 */

ZTEST_USER(pdc_attached_snk, test_new_pd_sink_contract)
{
	union connector_status_t in = { 0 };
	union conn_status_change_bits_t in_conn_status_change_bits;
	bool sink_path_en;

	const uint32_t pdos[] = {
		PDO_FIXED(5000, 3000, PDO_FIXED_DUAL_ROLE),
		PDO_FIXED(15000, 3000, PDO_FIXED_DUAL_ROLE),
	};

	/* Connect a sourcing port partner */
	emul_pdc_configure_snk(emul, &in);
	emul_pdc_set_pdos(emul, SOURCE_PDO, PDO_OFFSET_0, 1, PARTNER_PDO, pdos);
	emul_pdc_connect_partner(emul, &in);

	/* Ensure we are connected */
	pdc_power_mgmt_wait_for_sync(TEST_PORT, -1);

	/* Simulate the port partner changing its PDOs. The sink path is
	 * disabled during this step */
	emul_pdc_set_pdos(emul, SOURCE_PDO, PDO_OFFSET_0, ARRAY_SIZE(pdos),
			  PARTNER_PDO, pdos);
	in_conn_status_change_bits.battery_charging_status = 1;
	in.raw_conn_status_change_bits = in_conn_status_change_bits.raw_value;
	emul_pdc_connect_partner(emul, &in);

	/* Pause to allow pdc_power_mgmt to process interrupt and re-settle */
	pdc_power_mgmt_wait_for_sync(TEST_PORT, -1);

	/* Check that the sink path is on again */
	zassert_ok(emul_pdc_get_sink_path(emul, &sink_path_en));
	zassert_true(sink_path_en);
}
