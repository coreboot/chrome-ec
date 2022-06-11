/* Copyright 2022 The Chromium OS Authors.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Test Base-64 encoding.
 */

#include <stdio.h>
#include "base64.h"
#include "common.h"
#include "test_util.h"
#include "util.h"

static char results[100];
static size_t index;

static void printer(char c)
{
	if (index < sizeof(results))
		results[index++] = c;
}

static int test_encode(void)
{
	const char *data = "\x00\x01\x02\x80\xf0\ffabcdefghij "
			   "random text to wrap at 64 characters at most";
	const struct {
		size_t len; /* Length of input. */
		const char *encoding; /* Expected encoding. */
	} cases[] = {
		{ 1, "AA==\n" },
		{ 2, "AAE=\n" },
		{ 3, "AAEC\n" },
		{ 4, "AAECgA==\n" },
		{ 60, "AAECgPAMZmFiY2RlZmdoaWogcmFuZG9tIHRleHQg"
		      "dG8gd3JhcCBhdCA2NCBjaGFy\nYWN0ZXJzIGF0IG1v\n" },
	};
	size_t i;

	for (i = 0; i < ARRAY_SIZE(cases); i++) {
		index = 0;
		base64_encode_to_console(data, cases[i].len, printer);
		TEST_ASSERT(index == strlen(cases[i].encoding));
		TEST_ASSERT(memcmp(cases[i].encoding, results, index) == 0);
	}

	return EC_SUCCESS;
}

void run_test(void)
{
	test_reset();

	RUN_TEST(test_encode);

	test_print_result();
}
