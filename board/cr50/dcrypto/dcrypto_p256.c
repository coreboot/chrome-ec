/* Copyright 2016 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "dcrypto.h"
#include "internal.h"
#include "registers.h"
#include "fips_rand.h"

/* Firmware blob for crypto accelerator */
#include "dcrypto_p256.inc"

/*
 * This struct is "calling convention" for passing parameters into the
 * code block above for ecc operations.  Writes to this struct should be done
 * via the cp1w() and cp8w() functions to guarantee that word writes are used,
 * as the dcrypto peripheral does not support byte writes.
 * Parameters start at &DMEM[0].
 */
struct DMEM_ecc {
	uint32_t pK;
	uint32_t pRnd;
	uint32_t pMsg;
	uint32_t pR;
	uint32_t pS;
	uint32_t pX;
	uint32_t pY;
	uint32_t pD;
	p256_int __internal_use[16];
	p256_int k;
	p256_int rnd;
	p256_int msg;
	p256_int r;
	p256_int s;
	p256_int x;
	p256_int y;
	p256_int d;
};

#define DMEM_CELL_SIZE 32
#define DMEM_OFFSET(p) (offsetof(struct DMEM_ecc, p))
#define DMEM_INDEX(p) (DMEM_OFFSET(p) / DMEM_CELL_SIZE)

/*
 * Read-only pointer to read-only DMEM_ecc struct, use cp*w()
 * functions for writes.
 */
static const volatile struct DMEM_ecc *dmem_ecc =
	(const volatile struct DMEM_ecc *)GREG32_ADDR(CRYPTO, DMEM_DUMMY);

/*
 * Writes one word to DMEM, at the address derived from the base
 * offset and number of words. These parameters can be used for example
 * by specifying the offset of a p256_int, and the index of a word within
 * that p256_int.
 */
static void cp1w(size_t base_offset, int word, const uint32_t src)
{
	/* Destination address, always 32-bit aligned. */
	volatile uint32_t *dst =
		REG32_ADDR((uint8_t *)GREG32_ADDR(CRYPTO, DMEM_DUMMY) +
			   base_offset + (word * sizeof(uint32_t)));

	*dst = src;
}

/*
 * Copies the contents of the src p256_int to the specified offset in DMEM.
 * The src argument does not need to be aligned.
 */
static void cp8w(size_t offset, const volatile p256_int *src)
{
	int i;

	/*
	 * If p256_int is packed (as it is on cr50), the compiler
	 * cannot assume src will be aligned, and so performs
	 * byte reads into a register before calling cp1w (which
	 * is typically inlined).
	 *
	 * Note that the dcrypto peripheral supports byte reads,
	 * so it is safe to specify a pointer based on dmem_ecc
	 * as the src argument.
	 */
	for (i = 0; i < P256_NDIGITS; i++)
		cp1w(offset, i, P256_DIGIT(src, i));
}

static void cp8w_blinded(size_t offset, const volatile p256_int *src,
			 const p256_int *blinder)
{
	int i;

	/*
	 * If p256_int is packed (as it is on cr50), the compiler
	 * cannot assume src will be aligned, and so performs
	 * byte reads into a register before calling cp1w (which
	 * is typically inlined).
	 *
	 * Note that the dcrypto peripheral supports byte reads,
	 * so it is safe to specify a pointer based on dmem_ecc
	 * as the src argument.
	 */
	for (i = 0; i < P256_NDIGITS; i++)
		cp1w(offset, i, P256_DIGIT(src, i) ^ P256_DIGIT(blinder, i));
}

/* Convenience macros for above copy functions. */
#define CP1W(a, b, c) cp1w(DMEM_OFFSET(a), b, c)
#define CP8W(a, b) cp8w(DMEM_OFFSET(a), b)
#define CP8WB(a, b, c) cp8w_blinded(DMEM_OFFSET(a), b, c)

static void dcrypto_ecc_init(void)
{
	dcrypto_imem_load(0, IMEM_dcrypto_p256, ARRAY_SIZE(IMEM_dcrypto_p256));

	CP1W(pK, 0, DMEM_INDEX(k));
	CP1W(pRnd, 0, DMEM_INDEX(rnd));
	CP1W(pMsg, 0, DMEM_INDEX(msg));
	CP1W(pR, 0, DMEM_INDEX(r));
	CP1W(pS, 0, DMEM_INDEX(s));
	CP1W(pX, 0, DMEM_INDEX(x));
	CP1W(pY, 0, DMEM_INDEX(y));
	CP1W(pD, 0, DMEM_INDEX(d));

	/* (over)write first words to ensure pairwise mismatch. */
	CP1W(k, 0, 1);
	CP1W(rnd, 0, 2);
	CP1W(msg, 0, 3);
	CP1W(r, 0, 4);
	CP1W(s, 0, 5);
	CP1W(x, 0, 6);
	CP1W(y, 0, 7);
	CP1W(d, 0, 8);
}

enum dcrypto_result dcrypto_p256_ecdsa_sign(struct drbg_ctx *drbg,
					    const p256_int *key,
					    const p256_int *message,
					    p256_int *r, p256_int *s)
{
	enum dcrypto_result ret1, ret2;
	p256_int nonce;

	do {
		/* Pick uniform 0 < k < R */
		ret1 = p256_hmac_drbg_generate(drbg, &nonce);
		/* Don't even try with broken nonce. */
		if (ret1 != DCRYPTO_OK)
			return ret1;

		ret2 = dcrypto_p256_ecdsa_sign_raw(&nonce, key, message, r, s);
		ret1 |= ret2;
	} while (ret2 == DCRYPTO_RETRY);

	/* Wipe temp nonce */
	p256_clear(&nonce);

	return dcrypto_ok_if_zero(ret1 - DCRYPTO_OK);
}

enum dcrypto_result dcrypto_p256_ecdsa_sign_raw(const p256_int *nonce,
						const p256_int *key,
						const p256_int *message,
						p256_int *r, p256_int *s)
{
	int result;
	p256_int rnd;
	bool s_zero, r_zero;

	dcrypto_init_and_lock();
	dcrypto_ecc_init();
	result = dcrypto_call(CF_p256init_adr);

	/* Pick a blinder. */
	p256_fast_random(&rnd);
	CP8W(rnd, &rnd);

	CP8WB(k, nonce, &rnd);

	CP8W(msg, message);
	CP8WB(d, key, &rnd);

	result |= dcrypto_call(CF_p256sign_adr);

	if (result == 0) {
		*r = dmem_ecc->r;
		*s = dmem_ecc->s;
	} else {
		p256_clear(r);
		p256_clear(s);
	}
	s_zero = p256_is_zero(s);
	r_zero = p256_is_zero(r);

	/* Wipe d,k */
	CP8W(d, &rnd);
	CP8W(k, &rnd);
	dcrypto_unlock();

	/**
	 * Very unlikely case where combination of private key, nonce and
	 * message results in zero r or s.
	 * r = nonce * G and take its x-coordinate: r = R.x mod n
	 * s = nonce^{-1} * (message + r * key) mod n
	 */
	if ((s_zero || r_zero) && !result)
		return DCRYPTO_RETRY;

	return dcrypto_ok_if_zero(result);
}

enum dcrypto_result dcrypto_p256_base_point_mul(const p256_int *k, p256_int *x,
						p256_int *y)
{
	int result;
	p256_int rnd;

	dcrypto_init_and_lock();
	dcrypto_ecc_init();
	result = dcrypto_call(CF_p256init_adr);

	/* Pick a blinder. */
	p256_fast_random(&rnd);
	CP8W(rnd, &rnd);

	CP8WB(d, k, &rnd);

	result |= dcrypto_call(CF_p256scalarbasemult_adr);

	if (result == 0) {
		*x = dmem_ecc->x;
		*y = dmem_ecc->y;
	} else {
		p256_clear(x);
		p256_clear(y);
	}
	/* Wipe d */
	CP8W(d, &dmem_ecc->rnd);

	dcrypto_unlock();
	return dcrypto_ok_if_zero(result);
}

enum dcrypto_result dcrypto_p256_point_mul(const p256_int *k,
					   const p256_int *in_x,
					   const p256_int *in_y, p256_int *x,
					   p256_int *y)
{
	int result;
	p256_int rnd;

	dcrypto_init_and_lock();
	dcrypto_ecc_init();
	result = dcrypto_call(CF_p256init_adr);

	/* Generate blinder. */
	p256_fast_random(&rnd);
	CP8W(rnd, &rnd);

	CP8WB(k, k, &rnd);
	CP8W(x, in_x);
	CP8W(y, in_y);

	result |= dcrypto_call(CF_p256scalarmult_adr);

	if (result == 0) {
		*x = dmem_ecc->x;
		*y = dmem_ecc->y;
	} else {
		p256_clear(x);
		p256_clear(y);
	}

	/* Wipe k,x,y */
	CP8W(k, &rnd);
	CP8W(x, &rnd);
	CP8W(y, &rnd);

	dcrypto_unlock();
	return dcrypto_ok_if_zero(result);
}

enum dcrypto_result dcrypto_p256_ecdsa_verify(const p256_int *key_x,
					      const p256_int *key_y,
					      const p256_int *message,
					      const p256_int *r,
					      const p256_int *s)
{
	int i, result;

	dcrypto_init_and_lock();
	dcrypto_ecc_init();
	result = dcrypto_call(CF_p256init_adr);

	CP8W(msg, message);
	CP8W(r, r);
	CP8W(s, s);
	CP8W(x, key_x);
	CP8W(y, key_y);

	result |= dcrypto_call(CF_p256verify_adr);

	for (i = 0; i < 8; ++i)
		result |= (dmem_ecc->rnd.a[i] ^ r->a[i]);

	dcrypto_unlock();
	return dcrypto_ok_if_zero(result);
}

enum dcrypto_result dcrypto_p256_is_valid_point(const p256_int *x,
						const p256_int *y)
{
	int i, result;

	dcrypto_init_and_lock();
	dcrypto_ecc_init();
	result = dcrypto_call(CF_p256init_adr);

	CP8W(x, x);
	CP8W(y, y);

	result |= dcrypto_call(CF_p256isoncurve_adr);

	for (i = 0; i < 8; ++i)
		result |= (dmem_ecc->r.a[i] ^ dmem_ecc->s.a[i]);

	dcrypto_unlock();
	return dcrypto_ok_if_zero(result);
}
