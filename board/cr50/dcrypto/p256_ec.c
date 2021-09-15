/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "dcrypto.h"
#include "fips.h"
#include "fips_rand.h"

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

enum dcrypto_result DCRYPTO_p256_is_valid_point(const p256_int *x,
						const p256_int *y)
{
	return dcrypto_p256_is_valid_point(x, y);
}

/* DCRYPTO_p256_point_mul sets {out_x,out_y} = n*{in_x,in_y}, where n is <
 * the order of the group. */
int DCRYPTO_p256_point_mul(p256_int *out_x, p256_int *out_y, const p256_int *n,
			   const p256_int *in_x, const p256_int *in_y)
{
	if (p256_is_zero(n) != 0 ||
	    (dcrypto_p256_is_valid_point(in_x, in_y) != DCRYPTO_OK)) {
		p256_clear(out_x);
		p256_clear(out_y);
		return 0;
	}
	return dcrypto_p256_point_mul(n, in_x, in_y, out_x, out_y);
}


int dcrypto_p256_fips_sign_internal(struct drbg_ctx *drbg, const p256_int *key,
				  const p256_int *message, p256_int *r,
				  p256_int *s)
{
	int result;
	p256_int k;

	/* Pick uniform 0 < k < R */
	result = fips_p256_hmac_drbg_generate(drbg, &k) - HMAC_DRBG_SUCCESS;

	result |= dcrypto_p256_ecdsa_sign_raw(&k, key, message, r, s) - 1;

	/* Wipe temp k */
	p256_clear(&k);

	return result == 0;
}

int DCRYPTO_p256_key_pwct(struct drbg_ctx *drbg, const p256_int *d,
			  const p256_int *x, const p256_int *y)
{
	p256_int message, r, s;
	int result;

	if (p256_is_zero(d))
		return 0;

	/* set some pseudo-random message. */
	p256_fast_random(&message);

	if (dcrypto_p256_fips_sign_internal(drbg, d, &message, &r, &s) == 0)
		return 0;

#ifdef CRYPTO_TEST_SETUP
	if (fips_break_cmd == FIPS_BREAK_ECDSA_PWCT)
		message.a[0] = ~message.a[0];
#endif
	result = dcrypto_p256_ecdsa_verify(x, y, &message, &r, &s);
	return result;
}

/**
 * Key selection based on FIPS-186-4, section B.4.2 (Key Pair
 * Generation by Testing Candidates).
 */
int DCRYPTO_p256_key_from_bytes(p256_int *x, p256_int *y, p256_int *d,
				const uint8_t key_bytes[P256_NBYTES])
{
	p256_int key;
	p256_int tx, ty;

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

	/* We need public key anyway for pair-wise consistency test. */
	if (!x)
		x = &tx;
	if (!y)
		y = &ty;

	/* Compute public key (x,y) = d * G */
	if (dcrypto_p256_base_point_mul(d, x, y) == 0)
		return 0;

	return DCRYPTO_p256_key_pwct(&fips_drbg, d, x, y);
}
