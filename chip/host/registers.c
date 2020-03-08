/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Registers for test
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "registers.h"
#include "util.h"

static struct faux_register_array {
	const char * const name;
	unsigned int var;
} mock_registers_[] = {
	{
		.name = GNAME(PMU, PWRDN_SCRATCH20),
	},

	/* Define registers as needed. */
};

void *get_reg_addr(const char * const reg_name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mock_registers_); i++)
		if (!strcmp(mock_registers_[i].name, reg_name))
			return &mock_registers_[i].var;

	fprintf(stderr, "Unknown register is accessed: %s\n", reg_name);
	exit(1);
}
