/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "../common/rollback_private.h"
#include "common.h"
#include "console.h"
#include "flash.h"
#include "mpu.h"
#include "otp_key.h"
#include "rollback.h"
#include "string.h"
#include "system.h"
#include "util.h"

#include <zephyr/ztest.h>

ZTEST_SUITE(rollback_entropy, NULL, NULL, NULL, NULL, NULL);

static const uint32_t VALID_ROLLBACK_COOKIE = 0x0b112233;

static const uint8_t FAKE_ENTROPY[] = { 0xff, 0xff, 0xff, 0xff };

/*
 * Generated by concatenating 32-bytes (256-bits) of zeros with the 4 bytes
 * of FAKE_ENTROPY and computing SHA256 sum:
 *
 * echo -n -e '\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'\
 * '\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'\
 * '\xFF\xFF\xFF\xFF'  | sha256sum
 *
 * 890ed82cf09f22243bdc4252e4d79c8a9810c1391f455dce37a7b732eb0a0e4f
 */
#define EXPECTED_SECRET                                                     \
	0x89, 0x0e, 0xd8, 0x2c, 0xf0, 0x9f, 0x22, 0x24, 0x3b, 0xdc, 0x42,   \
		0x52, 0xe4, 0xd7, 0x9c, 0x8a, 0x98, 0x10, 0xc1, 0x39, 0x1f, \
		0x45, 0x5d, 0xce, 0x37, 0xa7, 0xb7, 0x32, 0xeb, 0x0a, 0x0e, \
		0x4f
__maybe_unused static const uint8_t _EXPECTED_SECRET[] = { EXPECTED_SECRET };
BUILD_ASSERT(sizeof(_EXPECTED_SECRET) == CONFIG_ROLLBACK_SECRET_SIZE);

/*
 * Generated by concatenating 32-bytes (256-bits) of EXPECTED_SECRET with the 4
 * bytes of FAKE_ENTROPY and computing SHA256 sum:
 *
 * echo -n -e '\x89\x0e\xd8\x2c\xf0\x9f\x22\x24\x3b\xdc\x42\x52\xe4\xd7\x9c'\
 * '\x8a\x98\x10\xc1\x39\x1f\x45\x5d\xce\x37\xa7\xb7\x32\xeb\x0a\x0e\x4f\xFF'\
 * '\FF\xFF' | sha256sum
 *
 * b5d2c08b1f9109ac5c67de15486f0ac267ef9501bd9f646f4ea80085cb08284c
 */
#define EXPECTED_SECRET2                                                    \
	0xb5, 0xd2, 0xc0, 0x8b, 0x1f, 0x91, 0x09, 0xac, 0x5c, 0x67, 0xde,   \
		0x15, 0x48, 0x6f, 0x0a, 0xc2, 0x67, 0xef, 0x95, 0x01, 0xbd, \
		0x9f, 0x64, 0x6f, 0x4e, 0xa8, 0x00, 0x85, 0xcb, 0x08, 0x28, \
		0x4c
__maybe_unused static const uint8_t _EXPECTED_SECRET2[] = { EXPECTED_SECRET2 };
BUILD_ASSERT(sizeof(_EXPECTED_SECRET2) == CONFIG_ROLLBACK_SECRET_SIZE);

static void check_equal(const struct rollback_data *actual,
			const struct rollback_data *expected)
{
	int rv = memcmp(actual->secret, expected->secret,
			sizeof(actual->secret));

	zassert_equal(rv, 0);
	zassert_equal(actual->rollback_min_version,
		      expected->rollback_min_version);
	zassert_equal(actual->id, expected->id);
	zassert_equal(actual->cookie, expected->cookie);
}

ZTEST(rollback_entropy, test_add_entropy)
{
	int rv;
	struct rollback_data rb_data;
	uint8_t otp_key_buffer[OTP_KEY_SIZE_BYTES] = { 0 };

	const struct rollback_data expected_empty = {
		.id = 0,
		.rollback_min_version = 0,
		.secret = { 0 },
		.cookie = VALID_ROLLBACK_COOKIE
	};

	const struct rollback_data expected_secret = {
		.id = 1,
		.rollback_min_version = 0,
		.secret = { EXPECTED_SECRET },
		.cookie = VALID_ROLLBACK_COOKIE
	};

	const struct rollback_data expected_secret2 = {
		.id = 2,
		.rollback_min_version = 0,
		.secret = { EXPECTED_SECRET2 },
		.cookie = VALID_ROLLBACK_COOKIE
	};

	if (system_get_image_copy() != EC_IMAGE_RO) {
		ccprintf("This test is only works when running RO\n");
		zassert_unreachable();
	}

	if (IS_ENABLED(CONFIG_OTP_KEY)) {
		/* Power on OTP memory. */
		otp_key_init();

		/* Check OTP key is unset initially. */
		rv = otp_key_read(otp_key_buffer);
		zassert_equal(rv, EC_SUCCESS);
		zassert_equal(bytes_are_trivial(otp_key_buffer,
						OTP_KEY_SIZE_BYTES),
			      true);

		/* Power off OTP memory. */
		otp_key_exit();
	}

	/* As opposite to CrosEC, both rollback regions are always initialized
	 * in Zephyr, not only the first one. rollback_initial_data structure
	 * defines the initial values. It is initialized as a part of the
	 * firmware image, see binman node for more details.
	 */
	for (int i = 0; i <= 1; i++) {
		rv = read_rollback(i, &rb_data);
		zassert_equal(rv, EC_SUCCESS);
		check_equal(&rb_data, &expected_empty);
	}

	/*
	 * Add entropy. The result should end up being written to the unused
	 * region (region 1).
	 */
	if (IS_ENABLED(SECTION_IS_RO)) {
		rv = rollback_add_entropy(FAKE_ENTROPY, sizeof(FAKE_ENTROPY));
		zassert_equal(rv, EC_SUCCESS);
	}

	/* Validate that region 1 has been updated correctly. */
	rv = read_rollback(1, &rb_data);
	zassert_equal(rv, EC_SUCCESS);
	check_equal(&rb_data, &expected_secret);

	/* Validate that region 0 has not changed. */
	rv = read_rollback(0, &rb_data);
	zassert_equal(rv, EC_SUCCESS);
	check_equal(&rb_data, &expected_empty);

	/*
	 * Add more entropy. The result should now end up being written to
	 * region 0.
	 */
	if (IS_ENABLED(SECTION_IS_RO)) {
		rv = rollback_add_entropy(FAKE_ENTROPY, sizeof(FAKE_ENTROPY));
		zassert_equal(rv, EC_SUCCESS);
	}

	/* Check region 0. */
	rv = read_rollback(0, &rb_data);
	zassert_equal(rv, EC_SUCCESS);
	check_equal(&rb_data, &expected_secret2);

	/* Check region 1 has not changed. */
	rv = read_rollback(1, &rb_data);
	zassert_equal(rv, EC_SUCCESS);
	check_equal(&rb_data, &expected_secret);

	if (IS_ENABLED(CONFIG_OTP_KEY)) {
		/* Power on OTP memory. */
		otp_key_init();

		/* Check OTP key has been written. */
		rv = otp_key_read(otp_key_buffer);
		zassert_equal(rv, EC_SUCCESS);
		zassert_equal(bytes_are_trivial(otp_key_buffer,
						OTP_KEY_SIZE_BYTES),
			      false);

		/* Power off OTP memory. */
		otp_key_exit();
	}
}
