/* Copyright 2015 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "endian.h"
#include "internal.h"

const p256_int SECP256r1_nMin2 = /* P-256 curve order - 2 */
	{ .a = { 0xfc632551 - 2, 0xf3b9cac2, 0xa7179e84, 0xbce6faad, -1, -1, 0,
	    -1 } };

/**
 * The 32 bit LFSR whose maximum length feedback polynomial is represented
 * as X^32 + X^22 + X^2 + X^1 + 1 will produce 2^32-1 PN sequence.
 * Polynomial is bit reversed as we shift left, not right.
 * This LFSR can be initialized with 0,  but can't be initialized with
 * 0xFFFFFFFF. This is handy to avoid non-zero initial values.
 */
static uint32_t next_fast_random(uint32_t seed)
{
	/**
	 * 12                        22          32
	 * 1100 0000 0000 0000 0000 0100 0000 0001 = 0xC0000401
	 */
	uint32_t mask = (-((int32_t)seed >= 0)) & 0xC0000401;

	return (seed << 1) ^ mask;
}
static uint32_t fast_random_state;

void set_fast_random_seed(uint32_t seed)
{
	/* Avoid prohibited value as LFSR seed value. */
	if (next_fast_random(seed) == seed)
		seed++;
	fast_random_state = seed;
}

uint32_t fast_random(void)
{
	fast_random_state = next_fast_random(fast_random_state);
	return fast_random_state;
}

void p256_clear(p256_int *a)
{
	always_memset(a, 0, sizeof(*a));
}

int p256_is_zero(const p256_int *a)
{
	int result = 0;

	for (size_t i = 0; i < P256_NDIGITS; ++i)
		result |= P256_DIGIT(a, i);
	return !result;
}

/* b = a + d. Returns carry, 0 or 1. */
int p256_add_d(const p256_int *a, uint32_t d, p256_int *b)
{
	p256_ddigit carry = d;

	for (size_t i = 0; i < P256_NDIGITS; ++i) {
		carry += (p256_ddigit)P256_DIGIT(a, i);
		P256_DIGIT(b, i) = (p256_digit)carry;
		carry >>= P256_BITSPERDIGIT;
	}
	return (int)carry;
}

/* Return -1 if a < b. */
int p256_lt_blinded(const p256_int *a, const p256_int *b)
{
	p256_sddigit borrow = 0;

	for (size_t i = 0; i < P256_NDIGITS; ++i) {
		volatile uint32_t blinder = fast_random();

		borrow += ((p256_sddigit)P256_DIGIT(a, i) - blinder);
		borrow -= P256_DIGIT(b, i);
		borrow += blinder;
		borrow >>= P256_BITSPERDIGIT;
	}
	return (int)borrow;
}

/* Return -1, 0, 1 for a < b, a == b or a > b respectively. */
int p256_cmp(const p256_int *a, const p256_int *b)
{
	p256_sddigit borrow = 0;
	p256_digit notzero = 0;

	for (size_t i = 0; i < P256_NDIGITS; ++i) {
		borrow += (p256_sddigit)P256_DIGIT(a, i) - P256_DIGIT(b, i);
		/**
		 * Track whether any result digit is ever not zero.
		 * Relies on !!(non-zero) evaluating to 1, e.g., !!(-1)
		 * evaluating to 1.
		 */
		notzero |= !!((p256_digit)borrow);
		borrow >>= P256_BITSPERDIGIT;
	}
	return (int)borrow | notzero;
}

#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
static inline void reverse_bytes(uint8_t *dst, const uint8_t *src,
				 size_t src_len)
{
	const uint8_t *pos = src + src_len - 1;

	while (pos >= src)
		*dst++ = *pos--;
}
#endif

void p256_to_bin(const p256_int *src, uint8_t dst[P256_NBYTES])
{
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
	/* reverse order of bytes from little-endian to big-endian. */
	reverse_bytes(dst, src->b8, P256_NBYTES);
#else
	uint8_t *p = &dst[0];
	/* p256 internally is little-endian, so reverse 32-bit digits. */
	for (int i = P256_NDIGITS - 1; i >= 0; --i) {
		p256_digit digit = P256_DIGIT(src, i);

		p[0] = (uint8_t)(digit >> 24);
		p[1] = (uint8_t)(digit >> 16);
		p[2] = (uint8_t)(digit >> 8);
		p[3] = (uint8_t)(digit);
		p += 4;
	}
#endif
}

bool p256_from_be_bin_size(const uint8_t *src, size_t len, p256_int *dst)
{
	size_t i;

	/**
	 * Skip zero padding if input length is larger than P-256 size.
	 * This may happen with TPM2 commands receiving big-endian number
	 * with leading zeroes from external sources.
	 */
	while (len > P256_NBYTES) {
		if (*src != 0)
			return false;
		len--;
		src++;
	}

	i = len;
	/* Now add zero padding little-endian p256 if length is smaller. */
	while (i < P256_NBYTES) {
		dst->b8[i] = 0;
		i++;
	}
	reverse_bytes(dst->b8, src, len);

	/* Note: this code is correct only for little-endian platform. */
	return true;
}

void p256_from_bin(const uint8_t src[P256_NBYTES], p256_int *dst)
{
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
	reverse_bytes(dst->b8, src, P256_NBYTES);
#else
	const uint8_t *p = &src[0];
	/* p256 internally is little-endian, so reverse 32-bit digits. */
	for (int i = P256_NDIGITS - 1; i >= 0; --i) {
		P256_DIGIT(dst, i) = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) |
				     p[3];
		p += 4;
	}
#endif
}

int p256_is_odd(const p256_int *a)
{
	return P256_DIGIT(a, 0) & 1;
}

void p256_fast_random(p256_int *rnd)
{
	for (size_t i = 0; i < P256_NDIGITS; i++)
		P256_DIGIT(rnd, i) = fast_random();
}

/* B.5.2 Per-Message Secret Number Generation by Testing Candidates */
enum dcrypto_result p256_hmac_drbg_generate(struct drbg_ctx *ctx, p256_int *rnd)
{
	enum dcrypto_result result;

	/* Generate p256 candidates from DRBG until valid is found. */
	do {
		/* fill destination with something in case DRBG fails. */
		p256_fast_random(rnd);
		result = hmac_drbg_generate(ctx, rnd->a, sizeof(rnd->a), NULL,
					    0);
		/**
		 * We need key to be in the range  0 < key < SECP256r1 - 1.
		 * To achieve that, first check key < SECP256r1 - 2, and if
		 * so, add 1 to key. Since key is unsigned number this will
		 * bring key in proper range. The only invalid key which is
		 * less than SECP256r1 - 2 is key == 0. Adding 1 avoids it.
		 *
		 * This also complies with legacy approach for key gen from
		 * DRBG which deviates from RFC 6979 due to adding '1' and
		 * different check in the loop. We can't make it as in
		 * RFC 6979 due to dependency on exact signature/certificate
		 * by some consumers (hashes of certificates as example).
		 *
		 * Key comes from DRBG, it is ensured to be in valid
		 * range for the P-256 curve.
		 */
	} while ((result == DCRYPTO_OK) &&
		 (p256_lt_blinded(rnd, &SECP256r1_nMin2) >= 0));
	p256_add_d(rnd, 1, rnd);

	return result;
}

#ifndef P256_BIN_TEST
#define P256_BIN_TEST 0
#endif

#ifdef CRYPTO_TEST_SETUP

#if P256_BIN_TEST
#include "console.h"

static int cmd_p256_bin_test(int argc, char *argv[])
{
	static const uint8_t i1[] = {
		0,    0,    0x10, 0x11, 0x12, 0x13, 0x20, 0x21, 0x22,
		0x23, 0x30, 0x31, 0x32, 0x33, 0x40, 0x41, 0x42, 0x43,
		0x50, 0x51, 0x52, 0x53, 0x60, 0x61, 0x62, 0x63, 0x70,
		0x71, 0x72, 0x73, 0x80, 0x81, 0x82, 0x83, 0x84
	};
	p256_int e = { .a = { 0x80818283, 0x70717273, 0x60616263, 0x50515253,
			      0x40414243, 0x30313233, 0x20212223,
			      0x10111213 } };

	p256_int r;
	bool passed = true;
	bool result;

	/* zero padded */
	result = p256_from_be_bin_size(i1, 34, &r);
	passed = result && (p256_cmp(&r, &e) == 0);
	ccprintf("in=%ph\nout=%ph\n", HEX_BUF(i1, 34), HEX_BUF(&r, sizeof(r)));

	/* right sized. */
	memset(&r, 0, sizeof(r));
	result = p256_from_be_bin_size(i1 + 2, 32, &r);
	passed = passed && (p256_cmp(&r, &e) == 0);
	ccprintf("in=%ph\nout=%ph\n", HEX_BUF(i1 + 2, 32),
		 HEX_BUF(&r, sizeof(r)));

	/**
	 * Smaller big num, where padding high byte(s) with zeroes is needed.
	 * we do this by loading 31 byte starting 1 byte higher.
	 * This will result in same value as in 'e' except that the
	 * highest byte will be 0 (e.a[7] == 0x00111213).
	 */
	memset(&r, 0, sizeof(r));
	result = p256_from_be_bin_size(i1 + 3, 31, &r);

	/* Update expected result by clearing high byte */
	e.b8[31] = 0x00;
	passed = passed && (p256_cmp(&r, &e) == 0);
	ccprintf("in=%ph\nout=%ph\n", HEX_BUF(i1 + 3, 31),
		 HEX_BUF(&r, sizeof(r)));

	/* larger big num. */
	result = p256_from_be_bin_size(i1 + 2, 33, &r);
	passed = passed && !result;

	ccprintf("p256_from_be_bin_size() test %s\n",
		 (passed) ? "PASSED" : "NOT PASSED");

	return EC_SUCCESS;
}
DECLARE_SAFE_CONSOLE_COMMAND(p256_test, cmd_p256_bin_test, NULL, NULL);
#endif /* P256_BIN_TEST */

#endif /* CRYPTO_TEST_SETUP */
