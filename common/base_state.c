/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "base_state.h"
#include "console.h"

static int command_setbasestate(int argc, char **argv)
{
	if (argc != 2)
		return EC_ERROR_PARAM_COUNT;
	if (argv[1][0] == 'a')
		base_force_state(1);
	else if (argv[1][0] == 'd')
		base_force_state(0);
	else if (argv[1][0] == 'r')
		base_force_state(2);
	else
		return EC_ERROR_PARAM1;

	return EC_SUCCESS;

}
DECLARE_CONSOLE_COMMAND(basestate, command_setbasestate,
	"[attach | detach | reset]",
	"Manually force base state to attached, detached or reset.");
