/* Copyright 2019 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdbool.h>

#include "common.h"
#include "ec_commands.h"
#include "host_command.h"
#include "test_util.h"
#include "timer.h"
#include "util.h"

static bool get_ap_reset_stats_should_succeed = true;

/* Mocks */

timestamp_t get_time(void)
{
	timestamp_t fake_time = { .val = 42 * MSEC };
	return fake_time;
}

/* Tests */

test_static int test_host_uptime_info_command_success(void)
{
	int rv;
	struct ec_response_uptime_info resp = { 0 };

	rv = test_send_host_command(EC_CMD_GET_UPTIME_INFO, 0, NULL, 0, &resp,
				    sizeof(resp));

	TEST_ASSERT(rv == EC_RES_SUCCESS);
	TEST_ASSERT(resp.time_since_ec_boot_ms == 42);

	return EC_RES_SUCCESS;
}

void run_test(void)
{
	test_reset();

	RUN_TEST(test_host_uptime_info_command_success);

	test_print_result();
}
