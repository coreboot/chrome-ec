/* Copyright 2019 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stddef.h>

#include "chipset.h"
#include "system.h"
#include "host_command.h"
#include "util.h"

static int host_command_get_uptime_info(struct host_cmd_handler_args *args)
{
	/*
	 * In the current implementation, not all terms are preserved across a
	 * sysjump.  Future implementations may preserve additional information.
	 *
	 * time_since_ec_boot_ms:   preserved, but wraps at ~50 days
	 * ec_reset_flags:          preserved, with 'sysjump' added
	 * ap_resets_since_ec_boot: Not preserved
	 * recent_ap_reset[*]:      Not preserved
	 */
	struct ec_response_uptime_info *r = args->response;
	timestamp_t now = get_time();
	uint32_t now_ms = (uint32_t)(now.val / MSEC);

	r->time_since_ec_boot_ms = now_ms;
	r->ec_reset_flags = system_get_reset_flags();

	memset(r->recent_ap_reset, 0, sizeof(r->recent_ap_reset));
	/* Skip setting AP reset stats */

	args->response_size = sizeof(*r);
	return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_GET_UPTIME_INFO,
	host_command_get_uptime_info,
	EC_VER_MASK(0));
