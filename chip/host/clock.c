/* Copyright 2013 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Mock clock driver for unit test.
 */

#include "clock.h"
#include "common.h"

int clock_get_freq(void)
{
	return 16000000;
}

test_mockable void clock_enable_module(enum module_id module, int enable)
{
}
