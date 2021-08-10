/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "dcrypto.h"

#include <stdint.h>

/* p256_base_point_mul sets {out_x,out_y} = nG, where n is < the
 * order of the group. */
int DCRYPTO_p256_base_point_mul(p256_int *out_x, p256_int *out_y,
				const p256_int *n)
{
	if (p256_is_zero(n) != 0) {
		p256_clear(out_x);
		p256_clear(out_y);
		return 0;
	}

	return dcrypto_p256_base_point_mul(n, out_x, out_y);
}

/* DCRYPTO_p256_point_mul sets {out_x,out_y} = n*{in_x,in_y}, where n is <
 * the order of the group. */
int DCRYPTO_p256_point_mul(p256_int *out_x, p256_int *out_y,
			const p256_int *n, const p256_int *in_x,
			const p256_int *in_y)
{
	if (p256_is_zero(n) != 0) {
		p256_clear(out_x);
		p256_clear(out_y);
		return 0;
	}

	return dcrypto_p256_point_mul(n, in_x, in_y, out_x, out_y);
}

/**
 * Key selection based on FIPS-186-4, section B.4.2 (Key Pair
 * Generation by Testing Candidates).
 */
int DCRYPTO_p256_key_from_bytes(p256_int *x, p256_int *y, p256_int *d,
				const uint8_t key_bytes[P256_NBYTES])
{
	p256_int key;

	p256_from_bin(key_bytes, &key);

	/**
	 * We need key to be in the range  0 < key < SECP256r1 - 1.
	 * To achieve that, first check key < SECP256r1 - 2, and
	 * then add 1 to key. Since key is unsigned number this will
	 * bring key in proper range.
	 */
	if (p256_lt_blinded(&key, &SECP256r1_nMin2) >= 0)
		return 0;
	p256_add_d(&key, 1, d);
	always_memset(&key, 0, sizeof(key));
	if (x == NULL || y == NULL)
		return 1;
	return dcrypto_p256_base_point_mul(d, x, y);
}
