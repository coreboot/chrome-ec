/* Copyright 2015 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "internal.h"

enum dcrypto_result DCRYPTO_p256_ecdsa_verify(const p256_int *key_x,
					      const p256_int *key_y,
					      const p256_int *message,
					      const p256_int *r,
					      const p256_int *s)
{
	if (!fips_crypto_allowed())
		return DCRYPTO_FAIL;

	return dcrypto_p256_ecdsa_verify(key_x, key_y, message, r, s);
}

/* return codes match dcrypto_p256_ecdsa_sign */
enum dcrypto_result DCRYPTO_p256_ecdsa_sign(const p256_int *key,
					    const p256_int *message,
					    p256_int *r, p256_int *s)
{
	if (!fips_drbg_init()) /* Also check for fips_crypto_allowed(). */
		return DCRYPTO_FAIL;

	return dcrypto_p256_fips_sign_internal(&fips_drbg, key, message, r, s);
}

/* p256_base_point_mul sets {out_x,out_y} = nG, where n is < the
 * order of the group. */
enum dcrypto_result DCRYPTO_p256_base_point_mul(p256_int *out_x,
		p256_int *out_y, const p256_int *n)
{
	if (!fips_crypto_allowed() || p256_is_zero(n)) {
		p256_clear(out_x);
		p256_clear(out_y);
		return DCRYPTO_FAIL;
	}

	return dcrypto_p256_base_point_mul(n, out_x, out_y);
}

enum dcrypto_result DCRYPTO_p256_is_valid_point(const p256_int *x,
						const p256_int *y)
{
	if (!fips_crypto_allowed())
		return DCRYPTO_FAIL;
	return dcrypto_p256_is_valid_point(x, y);
}

/* DCRYPTO_p256_point_mul sets {out_x,out_y} = n*{in_x,in_y}, where n is <
 * the order of the group. */
enum dcrypto_result DCRYPTO_p256_point_mul(p256_int *out_x, p256_int *out_y,
		const p256_int *n, const p256_int *in_x, const p256_int *in_y)
{
	if (!fips_crypto_allowed() || p256_is_zero(n) ||
	    (dcrypto_p256_is_valid_point(in_x, in_y) != DCRYPTO_OK)) {
		p256_clear(out_x);
		p256_clear(out_y);
		return DCRYPTO_FAIL;
	}
	return dcrypto_p256_point_mul(n, in_x, in_y, out_x, out_y);
}

/**
 * This function serves as workaround for gcc 11.2 crash.
 */
static enum dcrypto_result fips_p256_hmac_drbg_generate(struct drbg_ctx *ctx,
							p256_int *rnd)
{
	return p256_hmac_drbg_generate(ctx, rnd);
}

enum dcrypto_result dcrypto_p256_fips_sign_internal(struct drbg_ctx *drbg,
						    const p256_int *key,
						    const p256_int *message,
						    p256_int *r, p256_int *s)
{
	enum dcrypto_result result;
	p256_int k;

	/* Pick uniform 0 < k < R */
	result = fips_p256_hmac_drbg_generate(drbg, &k);

	result |= dcrypto_p256_ecdsa_sign_raw(&k, key, message, r, s);

	/* Wipe temp k */
	p256_clear(&k);

	return dcrypto_ok_if_zero(result - DCRYPTO_OK);
}

enum dcrypto_result dcrypto_p256_key_pwct(struct drbg_ctx *drbg,
					  const p256_int *d, const p256_int *x,
					  const p256_int *y)
{
	p256_int message, r, s;
	enum dcrypto_result result;
#ifdef CRYPTO_TEST_SETUP
	p256_int d_altered;
#endif

	if (p256_is_zero(d))
		return DCRYPTO_FAIL;

	/* set some pseudo-random message. */
	p256_fast_random(&message);

#ifdef CRYPTO_TEST_SETUP
	if (fips_break_cmd == FIPS_BREAK_ECDSA_PWCT) {
		/* Modify key used for signing. */
		d_altered = *d;
		d_altered.a[1] ^= 1;
		d = &d_altered;
	}
#endif

	result = dcrypto_p256_fips_sign_internal(drbg, d, &message, &r, &s);
	if (result != DCRYPTO_OK) {
		fips_set_status(FIPS_FATAL_ECDSA_PWCT);
		return result;
	}

	result = dcrypto_p256_ecdsa_verify(x, y, &message, &r, &s);
	if (result != DCRYPTO_OK) {
		fips_set_status(FIPS_FATAL_ECDSA_PWCT);
		return result;
	}

	return result;
}

/**
 * Key selection based on FIPS-186-4, section B.4.2 (Key Pair
 * Generation by Testing Candidates).
 */
enum dcrypto_result DCRYPTO_p256_key_from_bytes(p256_int *x,
	p256_int *y, p256_int *d, const uint8_t key_bytes[P256_NBYTES])
{
	p256_int key;
	p256_int tx, ty;

	if (!fips_crypto_allowed())
		return DCRYPTO_FAIL;

	p256_from_bin(key_bytes, &key);

	/**
	 * We need key to be in the range  0 < key < SECP256r1 - 1.
	 * To achieve that, first check key < SECP256r1 - 2, and
	 * then add 1 to key. Since key is unsigned number this will
	 * bring key in proper range.
	 */
	if (p256_lt_blinded(&key, &SECP256r1_nMin2) >= 0)
		return DCRYPTO_RETRY;
	p256_add_d(&key, 1, d);
	always_memset(&key, 0, sizeof(key));

	/* We need public key anyway for pair-wise consistency test. */
	if (!x)
		x = &tx;
	if (!y)
		y = &ty;

	/* Compute public key (x,y) = d * G */
	if (dcrypto_p256_base_point_mul(d, x, y) != DCRYPTO_OK)
		return DCRYPTO_FAIL;

	return dcrypto_p256_key_pwct(&fips_drbg, d, x, y);
}
