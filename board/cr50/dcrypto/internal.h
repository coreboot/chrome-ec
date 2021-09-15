/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef __EC_CHIP_G_DCRYPTO_INTERNAL_H
#define __EC_CHIP_G_DCRYPTO_INTERNAL_H

#include <stddef.h>
#include <string.h>

#include "common.h"
#include "crypto_common.h"
#include "util.h"

#include "hmacsha2.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SHA.
 */
#define CTRL_CTR_BIG_ENDIAN (__BYTE_ORDER__  == __ORDER_BIG_ENDIAN__)
#define CTRL_ENABLE         1
#define CTRL_ENCRYPT        1
#define CTRL_NO_SOFT_RESET  0

#define SHA_DIGEST_WORDS   (SHA_DIGEST_SIZE / sizeof(uint32_t))
#define SHA256_DIGEST_WORDS (SHA256_DIGEST_SIZE / sizeof(uint32_t))

#ifdef CONFIG_UPTO_SHA512
#define SHA_DIGEST_MAX_BYTES SHA512_DIGEST_SIZE
#else
#define SHA_DIGEST_MAX_BYTES SHA256_DIGEST_SIZE
#endif

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

enum sha_mode {
	SHA1_MODE = 0,
	SHA256_MODE = 1
};

/*
 * Use this structure to avoid alignment problems with input and output
 * pointers.
 */
struct access_helper {
	uint32_t udata;
} __packed;

#ifndef SECTION_IS_RO
int dcrypto_grab_sha_hw(void);
void dcrypto_release_sha_hw(void);
#endif

/*
 * BIGNUM.
 */
#define LITE_BN_BITS2        32
#define LITE_BN_BYTES        4

struct LITE_BIGNUM {
	uint32_t dmax;              /* Size of d, in 32-bit words. */
	struct access_helper *d;  /* Word array, little endian format ... */
};

#define BN_DIGIT(b, i) ((b)->d[(i)].udata)

void bn_init(struct LITE_BIGNUM *bn, void *buf, size_t len);
#define bn_size(b) ((b)->dmax * LITE_BN_BYTES)
#define bn_words(b) ((b)->dmax)
#define bn_bits(b) ((b)->dmax * LITE_BN_BITS2)
int bn_eq(const struct LITE_BIGNUM *a, const struct LITE_BIGNUM *b);
int bn_check_topbit(const struct LITE_BIGNUM *N);
int bn_modexp(struct LITE_BIGNUM *output,
			const struct LITE_BIGNUM *input,
			const struct LITE_BIGNUM *exp,
			const struct LITE_BIGNUM *N);
int bn_modexp_word(struct LITE_BIGNUM *output,
			const struct LITE_BIGNUM *input,
			uint32_t pubexp,
			const struct LITE_BIGNUM *N);
int bn_modexp_blinded(struct LITE_BIGNUM *output,
			const struct LITE_BIGNUM *input,
			const struct LITE_BIGNUM *exp,
			const struct LITE_BIGNUM *N,
			uint32_t pubexp);
uint32_t bn_add(struct LITE_BIGNUM *c,
		const struct LITE_BIGNUM *a);
uint32_t bn_sub(struct LITE_BIGNUM *c,
		const struct LITE_BIGNUM *a);
int bn_modinv_vartime(struct LITE_BIGNUM *r,
			const struct LITE_BIGNUM *e,
			const struct LITE_BIGNUM *MOD);
int bn_is_bit_set(const struct LITE_BIGNUM *a, int n);

/*
 * Accelerated bn.
 */
int dcrypto_modexp(struct LITE_BIGNUM *output,
			const struct LITE_BIGNUM *input,
			const struct LITE_BIGNUM *exp,
			const struct LITE_BIGNUM *N);
int dcrypto_modexp_word(struct LITE_BIGNUM *output,
			const struct LITE_BIGNUM *input,
			uint32_t pubexp,
			const struct LITE_BIGNUM *N);
int dcrypto_modexp_blinded(struct LITE_BIGNUM *output,
			const struct LITE_BIGNUM *input,
			const struct LITE_BIGNUM *exp,
			const struct LITE_BIGNUM *N,
			uint32_t pubexp);

struct drbg_ctx {
	uint32_t k[SHA256_DIGEST_WORDS];
	uint32_t v[SHA256_DIGEST_WORDS];
	uint32_t reseed_counter;
};

/*
 * NIST SP 800-90A HMAC DRBG.
 */
enum hmac_result {
	HMAC_DRBG_SUCCESS = 0,
	HMAC_DRBG_INVALID_PARAM = 1,
	HMAC_DRBG_RESEED_REQUIRED = 2
};

/* Standard initialization. */
void hmac_drbg_init(struct drbg_ctx *ctx,
		    const void *p0, size_t p0_len,
		    const void *p1, size_t p1_len,
		    const void *p2, size_t p2_len);
/* Initialize with at least nbits of random entropy. */
void hmac_drbg_init_rand(struct drbg_ctx *ctx, size_t nbits);
void hmac_drbg_reseed(struct drbg_ctx *ctx,
		      const void *p0, size_t p0_len,
		      const void *p1, size_t p1_len,
		      const void *p2, size_t p2_len);
enum hmac_result hmac_drbg_generate(struct drbg_ctx *ctx, void *out,
				    size_t out_len, const void *input,
				    size_t input_len);
void drbg_exit(struct drbg_ctx *ctx);

/* Set seed for fast random number generator using LFSR. */
void set_fast_random_seed(uint32_t seed);

/* Generate week pseudorandom using LFSR for blinding purposes. */
uint32_t fast_random(void);

/*
 * Accelerated p256. FIPS PUB 186-4
 */
#define P256_BITSPERDIGIT 32
#define P256_NDIGITS	  8
#define P256_NBYTES	  32

typedef uint32_t p256_digit;
typedef int32_t p256_sdigit;
typedef uint64_t p256_ddigit;
typedef int64_t p256_sddigit;

#define P256_DIGITS(x)	 ((x)->a)
#define P256_DIGIT(x, y) ((x)->a[y])

/**
 * P-256 integers internally represented as little-endian 32-bit integer
 * digits in platform-specific format. On little-endian platform this would
 * be regular 256-bit little-endian unsigned integer. On big-endian platform
 * it would big-endian 32-bit digits in little-endian order.
 *
 * Defining p256_int as struct to leverage struct assignment.
 */
typedef struct p256_int {
	union {
		p256_digit a[P256_NDIGITS];
		uint8_t b8[P256_NBYTES];
	};
} p256_int;

extern const p256_int SECP256r1_nMin2;

/* Clear a p256_int to zero. */
void p256_clear(p256_int *a);

/* Check p256 is a zero. */
int p256_is_zero(const p256_int *a);

/* Check p256 is odd. */
int p256_is_odd(const p256_int *a);

/* c := a + (single digit)b, returns carry 1 on carry. */
int p256_add_d(const p256_int *a, p256_digit b, p256_int *c);

/* Returns -1, 0 or 1. */
int p256_cmp(const p256_int *a, const p256_int *b);

/* Return -1 if a < b. */
int p256_lt_blinded(const p256_int *a, const p256_int *b);

/* Outputs big-endian binary form. No leading zero skips. */
void p256_to_bin(const p256_int *src, uint8_t dst[P256_NBYTES]);

/**
 * Reads from big-endian binary form, thus pre-pad with leading
 * zeros if short. Input length is assumed P256_NBYTES bytes.
 */
void p256_from_bin(const uint8_t src[P256_NBYTES], p256_int *dst);

/**
 * Reads from big-endian binary form of given size, add padding with
 * zeros if short. Check that leading digits beyond P256_NBYTES are zeroes.
 *
 * @return true if provided big-endian fits into p256.
 */
bool p256_from_be_bin_size(const uint8_t *src, size_t len, p256_int *dst);

/**
 * Raw sign with provided nonce (k). Used internally and for testing.
 *
 * @param k - valid nonce for ECDSA sign
 * @param key - valid private key for ECDSA sign
 * @param message - message to sign encoded as p-256 int
 * @param r - generated signature
 * @param s  - generated signature
 * @return !0 if success
 */
int dcrypto_p256_ecdsa_sign_raw(const p256_int *k, const p256_int *key,
				const p256_int *message, p256_int *r,
				p256_int *s);

int dcrypto_p256_ecdsa_sign(struct drbg_ctx *drbg, const p256_int *key,
			    const p256_int *message, p256_int *r, p256_int *s)
	__attribute__((warn_unused_result));
int dcrypto_p256_base_point_mul(const p256_int *k, p256_int *x, p256_int *y)
	__attribute__((warn_unused_result));
int dcrypto_p256_point_mul(const p256_int *k,
		const p256_int *in_x, const p256_int *in_y,
		p256_int *x, p256_int *y)
	__attribute__((warn_unused_result));
int dcrypto_p256_ecdsa_verify(const p256_int *key_x, const p256_int *key_y,
		const p256_int *message, const p256_int *r,
		const p256_int *s)
	__attribute__((warn_unused_result));
enum dcrypto_result dcrypto_p256_is_valid_point(const p256_int *x,
						const p256_int *y)
	__attribute__((warn_unused_result));

/* Wipe content of rnd with pseudo-random values. */
void p256_fast_random(p256_int *rnd);

/* Generate a p256 number between 1 < k < |p256| using provided DRBG. */
enum hmac_result p256_hmac_drbg_generate(struct drbg_ctx *ctx, p256_int *k_out);

/**
 * Sign using provided DRBG. Reseed DRBG with entropy from verified TRNG if
 * needed.
 *
 * @param drbg DRBG to use
 * @param key P-256 private key
 * @param message - Message to sign as P-256 (in little-endian)
 * @param r - Generated signature
 * @param s - Generated signature
 * @return int
 */
int dcrypto_p256_fips_sign_internal(struct drbg_ctx *drbg, const p256_int *key,
				    const p256_int *message, p256_int *r,
				    p256_int *s);

/* Initialize for use as RFC6979 DRBG. */
void hmac_drbg_init_rfc6979(struct drbg_ctx *ctx,
			    const p256_int *key,
			    const p256_int *message);

/*
 * Accelerator runtime.
 *
 * Note dcrypto_init_and_lock grabs a mutex and dcrypto_unlock releases it.
 * Do not use dcrypto_call, dcrypto_imem_load or dcrypto_dmem_load w/o holding
 * the mutex.
 */
void dcrypto_init_and_lock(void);
void dcrypto_unlock(void);
uint32_t dcrypto_call(uint32_t adr) __attribute__((warn_unused_result));
void dcrypto_imem_load(size_t offset, const uint32_t *opcodes,
		       size_t n_opcodes);
/*
 * Returns 0 iff no difference was observed between existing and new content.
 */
uint32_t dcrypto_dmem_load(size_t offset, const void *words, size_t n_words);

/**
 * An implementation of memset that ought not to be optimized away;
 * useful for scrubbing security sensitive buffers.
 */
void *always_memset(void *s, int c, size_t n);

#ifndef __alias
#define __alias(func) __attribute__((alias(#func)))
#endif

/**
 * Macro to control optimization at function level. Typically used in
 * very tight and critical loops to override -Os and get a better code.
 * Only supported by gcc, so ignore it for clang.
 */
#ifndef __optimize
#ifndef __clang__
#define __optimize(a) __attribute__((optimize(a)))
#else
#define __optimize(a)
#endif
#endif

/* rotate 32-bit value right */
static inline uint32_t ror(uint32_t value, int bits)
{
	/* return __builtin_rotateright32(value, bits); */
	return (value >> bits) | (value << (32 - bits));
}

/* rotate 64-bit value right */
static inline uint64_t ror64(uint64_t value, int bits)
{
	/* return __builtin_rotateright64(value, bits); */
	return (value >> bits) | (value << (64 - bits));
}

/* rotate 32-bit value left */
static inline uint32_t rol(uint32_t value, int bits)
{
	/* return __builtin_rotateleft32(value, bits); */
	return (value << bits) | (value >> (32 - bits));
}

/* rotate 64-bit value left */
static inline uint64_t rol64(uint64_t value, int bits)
{
	/* return __builtin_rotateleft64(value, bits); */
	return (value << bits) | (value >> (64 - bits));
}

/* stack based allocation */
#ifndef alloca
#define alloca __builtin_alloca
#endif

/* Define machine word alignment mask. */
#define WORD_MASK (sizeof(uintptr_t) - 1)

/**
 * @brief Launders the machine register sized value `val`.
 *
 * It returns a value identical to the input, while introducing an optimization
 * barrier that prevents the compiler from learning new information about the
 * original value, based on operations on the laundered value. For example
 * with code:
 *  temp = b - a;
 *  c = (cond)? temp + a : a;
 * compiler will reduce code to:
 *  c = (cond)? b : a;
 * Which may not be desirable.
 * making temp = value_barrier(b-a) prevents compiler from such optimization.
 *
 * Another case is for double checks against fault injection:
 * if (cond == 0) { if (value_barrier(cond) != 0 ) panic(); }
 * Without value_barrier the nested check will be removed.
 *
 * @param val A machine register size integer to launder.
 * @return An integer which will happen to have the same value as `val` at
 *         runtime.
 */
static inline uintptr_t value_barrier(uintptr_t val)
{
	/**
	 * The +r constraint tells the compiler that this is an "inout"
	 * parameter: it means that not only does the black box depend on `val`,
	 * but it also mutates it in an unspecified way. This ensures that the
	 * laundering operation occurs in the right place in the dependency
	 * ordering.
	 */
	asm("" : "+r"(val));
	return val;
}
static inline void *value_barrier_ptr(void *val)
{
	/**
	 * The +r constraint tells the compiler that this is an "inout"
	 * parameter: it means that not only does the black box depend on `val`,
	 * but it also mutates it in an unspecified way. This ensures that the
	 * laundering operation occurs in the right place in the dependency
	 * ordering.
	 */
	asm("" : "+r"(val));
	return val;
}

/**
 * Hardened select of word without branches.
 *
 * @param test the value to test
 * @param a the value returned if 'test' is not 0
 * @param b the value returned if 'test' is zero
 *
 * @return b if test==0 and a otherwise;
 *
 * This function is mostly useful when test == 0 is a good result and
 * hardening is required to prevent easy generation of 'b'.
 *
 * Example:
 *   hardened_select_if_zero(test, CRYPTO_FAIL, CRYPTO_OK)
 */
static inline __attribute__((always_inline))
uintptr_t hardened_select_if_zero(uintptr_t test, uintptr_t a, uintptr_t b)
{
	uintptr_t bma = value_barrier(b - a);

	/* For test == 0, does nothing, otherwise damage bma. */
	bma &= value_barrier(~test);
	return (test == 0) ? bma + a : a;
}

/* Convenience wrapper to return DCRYPTO_OK iff val == 0. */
static inline enum dcrypto_result dcrypto_ok_if_zero(uintptr_t val)
{
	return (enum dcrypto_result)hardened_select_if_zero(val, DCRYPTO_FAIL,
							    DCRYPTO_OK);
}

/*
 * Key ladder.
 */
#ifndef __cplusplus
enum dcrypto_appid;      /* Forward declaration. */

int dcrypto_ladder_compute_usr(enum dcrypto_appid id,
			const uint32_t usr_salt[8]);
int dcrypto_ladder_derive(enum dcrypto_appid appid, const uint32_t salt[8],
			  const uint32_t input[8], uint32_t output[8]);
#endif

#ifdef __cplusplus
}
#endif

#endif  /* ! __EC_CHIP_G_DCRYPTO_INTERNAL_H */
