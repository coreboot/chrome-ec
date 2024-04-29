/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.h"
#include "console.h"

#include <zephyr/shell/shell.h>
#include <zephyr/ztest.h>

#define TEST_PORT 0

static void console_cmd_pdc_setup(void)
{
	zassume(TEST_PORT < CONFIG_USB_PD_PORT_MAX_COUNT,
		"TEST_PORT is invalid");
}

ZTEST_SUITE(console_cmd_pdc, NULL, console_cmd_pdc_setup, NULL, NULL, NULL);

ZTEST_USER(console_cmd_pdc, test_no_args)
{
	int rv;

	rv = shell_execute_cmd(get_ec_shell(), "pdc");
	zassert_equal(rv, SHELL_CMD_HELP_PRINTED, "Expected %d, but got %d",
		      SHELL_CMD_HELP_PRINTED, rv);
}

ZTEST_USER(console_cmd_pdc, test_cable_prop)
{
	int rv;

	rv = shell_execute_cmd(get_ec_shell(), "pdc cable_prop 99");
	zassert_equal(rv, -EINVAL, "Expected %d, but got %d", -EINVAL, rv);

	rv = shell_execute_cmd(get_ec_shell(), "pdc cable_prop 0");
	zassert_equal(rv, EC_SUCCESS, "Expected %d, but got %d", EC_SUCCESS,
		      rv);
}