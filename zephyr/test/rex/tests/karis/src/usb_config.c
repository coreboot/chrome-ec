/* Copyright 2023 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cros_cbi.h"
#include "driver/retimer/bb_retimer_public.h"
#include "hooks.h"
#include "usb_mux.h"
#include "usb_mux_config.h"
#include "usbc/ppc.h"
#include "usbc/tcpci.h"
#include "usbc/usb_muxes.h"
#include "usbc_config.h"

#include <zephyr/devicetree.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

FAKE_VALUE_FUNC(int, cros_cbi_get_fw_config, enum cbi_fw_config_field_id,
		uint32_t *);

FAKE_VOID_FUNC(pd_send_host_event, int);
FAKE_VOID_FUNC(pd_set_input_current_limit, int, uint32_t, uint32_t);
FAKE_VOID_FUNC(pd_power_supply_reset, int);
FAKE_VALUE_FUNC(int, pd_check_vconn_swap, int);
FAKE_VALUE_FUNC(int, pd_set_power_supply_ready, int);

int mock_cros_cbi_get_fw_config_hb(enum cbi_fw_config_field_id field_id,
				   uint32_t *value)
{
	*value = FW_USB_DB_USB4_HB;
	return 0;
}

int mock_cros_cbi_get_fw_config_no_usb_db(enum cbi_fw_config_field_id field_id,
					  uint32_t *value)
{
	*value = FW_USB_DB_NOT_CONNECTED;
	return 0;
}

int mock_cros_cbi_get_fw_config_error(enum cbi_fw_config_field_id field_id,
				      uint32_t *value)
{
	*value = FW_USB_DB_NOT_CONNECTED;
	return -1;
}

void charge_manager_update_charge(int supplier, int port,
				  const struct charge_port_info *charge)
{
}

static void usb_mux_config_before(void *fixture)
{
	ARG_UNUSED(fixture);

	RESET_FAKE(cros_cbi_get_fw_config);
}

ZTEST_USER(usb_mux_config, test_setup_usb_db_hb)
{
	cros_cbi_get_fw_config_fake.custom_fake =
		mock_cros_cbi_get_fw_config_hb;

	hook_notify(HOOK_INIT);

	zassert_equal(1, cros_cbi_get_fw_config_fake.call_count);
	zassert_equal(FW_USB_DB_USB4_HB, usb_db_type);
	zassert_equal(2, board_get_usb_pd_port_count());
	zassert_true(board_is_tbt_usb4_port(USBC_PORT_C0), NULL);
	zassert_true(board_is_tbt_usb4_port(USBC_PORT_C1), NULL);
	zassert_equal(usb_muxes[1].mux->driver, &bb_usb_retimer);
	zassert_mem_equal(&tcpc_config[1],
			  &TCPC_ALT_FROM_NODELABEL(tcpc_rt1716_port1),
			  sizeof(struct tcpc_config_t));
	zassert_mem_equal(&ppc_chips[1], &PPC_ALT_FROM_NODELABEL(ppc_syv_port1),
			  sizeof(struct ppc_config_t));
}

ZTEST_USER(usb_mux_config, test_setup_usb_db_no_usb_db)
{
	cros_cbi_get_fw_config_fake.custom_fake =
		mock_cros_cbi_get_fw_config_no_usb_db;

	hook_notify(HOOK_INIT);

	zassert_equal(1, cros_cbi_get_fw_config_fake.call_count);
	zassert_equal(0, usb_db_type);
	zassert_equal(1, board_get_usb_pd_port_count());
}

ZTEST_USER(usb_mux_config, test_setup_usb_db_error_reading_cbi)
{
	cros_cbi_get_fw_config_fake.custom_fake =
		mock_cros_cbi_get_fw_config_error;

	hook_notify(HOOK_INIT);

	zassert_equal(1, cros_cbi_get_fw_config_fake.call_count);
	zassert_equal(0, usb_db_type);
	zassert_equal(1, board_get_usb_pd_port_count());
}

ZTEST_SUITE(usb_mux_config, NULL, NULL, usb_mux_config_before, NULL, NULL);
