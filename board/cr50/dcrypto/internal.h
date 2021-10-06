/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef __EC_CHIP_G_DCRYPTO_INTERNAL_H
#define __EC_CHIP_G_DCRYPTO_INTERNAL_H

#include <stddef.h>
#include <string.h>

#include "common.h"
#include "dcrypto.h"
#include "fips.h"
#include "fips_rand.h"
#include "util.h"

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

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif


#ifndef SECTION_IS_RO
int dcrypto_grab_sha_hw(void);
void dcrypto_release_sha_hw(void);
#endif

/* Load data into KEYMGR SHA FIFO. */
void dcrypto_sha_fifo_load(const void *data, size_t n);

/*
 * SHA implementation.  This abstraction is backed by either a
 * software or hardware implementation.
 *
 * There could be only a single hardware SHA context in progress. The init
 * functions will try using the HW context, if available, unless 'sw_required'
 * is TRUE, in which case there will be no attempt to use the hardware for
 * this particular hashing session.
 */

/**
 * Reset hash context with the same hash function as configured.
 * Will crash if previously not configured! Used for HMAC.
 */
static inline void HASH_reinit(union hash_ctx *const ctx)
{
	ctx->f->init(ctx);
}

/* Software implementations of hash functions. */
void SHA1_sw_init(struct sha1_ctx *const ctx);
void SHA1_sw_update(struct sha1_ctx *const ctx, const void *data, size_t len);
const struct sha1_digest *SHA1_sw_final(struct sha1_ctx *const ctx);
const struct sha1_digest *SHA1_sw_hash(const void *data, size_t len,
				       struct sha1_digest *digest);
void SHA256_sw_init(struct sha256_ctx *const ctx);
void SHA256_sw_update(struct sha256_ctx *const ctx, const void *data,
		      size_t len);
const struct sha256_digest *SHA256_sw_final(struct sha256_ctx *const ctx);
const struct sha256_digest *SHA256_sw_hash(const void *data, size_t len,
					   struct sha256_digest *digest);
void SHA224_sw_init(struct sha224_ctx *const ctx);
void SHA224_sw_update(struct sha224_ctx *const ctx, const void *data,
		      size_t len);
const struct sha224_digest *SHA224_sw_final(struct sha224_ctx *const ctx);
const struct sha224_digest *SHA224_sw_hash(const void *data, size_t len,
					   struct sha224_digest *digest);


/**
 * Initialize HMAC for pre-configured hash.
 * This is generic function which can initialize HMAC with any supported
 * hash function.
 */
void HMAC_sw_init(union hmac_ctx *const ctx, const void *key, size_t len);
const union sha_digests *HMAC_sw_final(union hmac_ctx *const ctx);

/**
 * HMAC SHA2-224 initialization.
 */
static inline void HMAC_SHA224_sw_init(struct hmac_sha224_ctx *const ctx,
				       const void *key, size_t len)
{
	SHA224_sw_init(&ctx->hash);
	HMAC_sw_init((union hmac_ctx *)ctx, key, len);
}

static inline void HMAC_SHA224_update(struct hmac_sha224_ctx *const ctx,
				      const void *data, size_t len)
{
	ctx->hash.f->update((union hash_ctx *)&ctx->hash, data, len);
}

static inline const struct sha224_digest *
HMAC_SHA224_final(struct hmac_sha224_ctx *const ctx)
{
	return &ctx->hash.f->hmac_final((union hmac_ctx *)ctx)->sha224;
}

/**
 * HMAC SHA2-256 initialization.
 */
static inline void HMAC_SHA256_sw_init(struct hmac_sha256_ctx *const ctx,
				       const void *key, size_t len)
{
	SHA256_sw_init(&ctx->hash);
	HMAC_sw_init((union hmac_ctx *)ctx, key, len);
}


/**
 * HMAC SHA1 initialization.
 */
static inline void HMAC_SHA1_sw_init(struct hmac_sha1_ctx *const ctx,
				     const void *key, size_t len)
{
	SHA1_sw_init(&ctx->hash);
	HMAC_sw_init((union hmac_ctx *)ctx, key, len);
}

void SHA1_hw_init(struct sha1_ctx *ctx);
void SHA256_hw_init(struct sha256_ctx *ctx);
const struct sha1_digest *SHA1_hw_hash(const void *data, size_t len,
				       struct sha1_digest *digest);
const struct sha256_digest *SHA256_hw_hash(const void *data, size_t len,
					   struct sha256_digest *digest);

#ifdef CONFIG_UPTO_SHA512
void SHA384_sw_init(struct sha384_ctx *const ctx);
void SHA384_sw_update(struct sha384_ctx *const ctx, const void *data,
		      size_t len);
const struct sha384_digest *SHA384_sw_final(struct sha384_ctx *const ctx);
const struct sha384_digest *SHA384_sw_hash(const void *data, size_t len,
					   struct sha384_digest *digest);
void SHA512_sw_init(struct sha512_ctx *const ctx);
void SHA512_sw_update(struct sha512_ctx *const ctx, const void *data,
		      size_t len);
const struct sha512_digest *SHA512_sw_final(struct sha512_ctx *ctx);
const struct sha512_digest *SHA512_sw_hash(const void *data, size_t len,
					   struct sha512_digest *digest);

void SHA384_hw_init(struct sha384_ctx *ctx);
void SHA512_hw_init(struct sha512_ctx *ctx);
const struct sha384_digest *SHA384_hw_hash(const void *data, size_t len,
					   struct sha384_digest *digest);

const struct sha512_digest *SHA512_hw_hash(const void *data, size_t len,
					   struct sha512_digest *digest);


/**
 * HMAC SHA2-384 initialization.
 */
static inline void HMAC_SHA384_sw_init(struct hmac_sha384_ctx *ctx,
				       const void *key, size_t len)
{
	SHA384_sw_init(&ctx->hash);
	HMAC_sw_init((union hmac_ctx *)ctx, key, len);
}
/**
 * HMAC SHA2-512 initialization.
 */
static inline void HMAC_SHA512_sw_init(struct hmac_sha512_ctx *ctx,
				       const void *key, size_t len)
{
	SHA512_sw_init(&ctx->hash);
	HMAC_sw_init((union hmac_ctx *)ctx, key, len);
}
#endif

/*
 *  HMAC. FIPS 198-1
 */
void HMAC_SHA256_hw_init(struct hmac_sha256_ctx *ctx, const void *key,
			      size_t len);
/* DCRYPTO HMAC-SHA256 final */
const struct sha256_digest *HMAC_SHA256_hw_final(struct hmac_sha256_ctx *ctx);

/*
 * Hardware AES.
 */
enum dcrypto_result dcrypto_aes_init(const uint8_t *key, size_t key_len,
				     const uint8_t *iv, enum cipher_mode c_mode,
				     enum encrypt_mode e_mode);
/*
 * BIGNUM.
 */
#define LITE_BN_BITS2        32
#define LITE_BN_BYTES        4


#define BN_DIGIT(b, i) ((b)->d[(i)].udata)

void bn_init(struct LITE_BIGNUM *bn, void *buf, size_t len);
#define bn_words(b) ((b)->dmax)

int bn_eq(const struct LITE_BIGNUM *a, const struct LITE_BIGNUM *b);
int bn_check_topbit(const struct LITE_BIGNUM *N);
enum dcrypto_result bn_modexp(struct LITE_BIGNUM *output,
			      const struct LITE_BIGNUM *input,
			      const struct LITE_BIGNUM *exp,
			      const struct LITE_BIGNUM *N);
enum dcrypto_result bn_modexp_word(struct LITE_BIGNUM *output,
				   const struct LITE_BIGNUM *input,
				   uint32_t pubexp,
				   const struct LITE_BIGNUM *N);
enum dcrypto_result bn_modexp_blinded(struct LITE_BIGNUM *output,
				      const struct LITE_BIGNUM *input,
				      const struct LITE_BIGNUM *exp,
				      const struct LITE_BIGNUM *N,
				      uint32_t pubexp);
uint32_t bn_add(struct LITE_BIGNUM *c, const struct LITE_BIGNUM *a);
int32_t bn_sub(struct LITE_BIGNUM *c, const struct LITE_BIGNUM *a);
enum dcrypto_result bn_modinv_vartime(struct LITE_BIGNUM *r,
				      const struct LITE_BIGNUM *e,
				      const struct LITE_BIGNUM *MOD);
int bn_is_bit_set(const struct LITE_BIGNUM *a, size_t n);

/*
 * Accelerated bn.
 */
enum dcrypto_result dcrypto_modexp(struct LITE_BIGNUM *output,
				   const struct LITE_BIGNUM *input,
				   const struct LITE_BIGNUM *exp,
				   const struct LITE_BIGNUM *N);
enum dcrypto_result dcrypto_modexp_word(struct LITE_BIGNUM *output,
					const struct LITE_BIGNUM *input,
					uint32_t pubexp,
					const struct LITE_BIGNUM *N);
enum dcrypto_result dcrypto_modexp_blinded(struct LITE_BIGNUM *output,
					   const struct LITE_BIGNUM *input,
					   const struct LITE_BIGNUM *exp,
					   const struct LITE_BIGNUM *N,
					   uint32_t pubexp);

/**
 * NIST SP 800-90A HMAC_DRBG_SHA2-256.
 */
struct drbg_ctx {
	uint32_t k[SHA256_DIGEST_WORDS];
	uint32_t v[SHA256_DIGEST_WORDS];
	uint32_t reseed_counter;
	uint32_t reseed_threshold;
	enum dcrypto_result magic_cookie;
};
/* According to NIST SP 800-90A rev 1 B.2
 * Maximum number of bits per request = 7500 bit, ~937 bytes
 */
#define HMAC_DRBG_MAX_OUTPUT_SIZE 937U

#define HMAC_DRBG_DO_NOT_AUTO_RESEED 0xFFFFFFFF

/* Check that DRBG is properly initialized. */
static inline bool hmac_drbg_ctx_valid(const struct drbg_ctx *drbg)
{
	return drbg->magic_cookie == DCRYPTO_OK;
}

/* Standard initialization. */
void hmac_drbg_init(struct drbg_ctx *ctx, const void *entropy,
		    size_t entropy_len, const void *nonce, size_t nonce_len,
		    const void *perso, size_t perso_len,
		    uint32_t reseed_threshold);

void hmac_drbg_reseed(struct drbg_ctx *ctx, const void *entropy,
		      size_t entropy_len, const void *additional_input,
		      size_t additional_input_len);

enum dcrypto_result hmac_drbg_generate(struct drbg_ctx *ctx, void *out,
				       size_t out_len, const void *input,
				       size_t input_len) __warn_unused_result;

void drbg_exit(struct drbg_ctx *ctx);

/**
 * TRNG service functions
 */

/**
 * Returns random number with indication wherever reading is valid. This is
 * different from rand() which doesn't provide any indication.
 * High 32-bits set to zero in case of error; otherwise value >> 32 == 1
 * Use of uint64_t vs. struct results in more efficient code.
 */
uint64_t read_rand(void);

/**
 * FIPS-compliant TRNG startup.
 * The entropy source's startup tests shall run the continuous health tests
 * over at least 4096 consecutive samples.
 * Note: This function can throw FIPS_FATAL_TRNG error
 *
 * To hide latency of reading TRNG data, this test is executed in 2 stages
 * @param stage is 0 or 1, choosing the stage. On each stage 2048
 * samples are processed. Assuming that some other tasks can be executed
 * between stages, when TRNG FIFO if filled with samples.
 *
 * Some number of samples will be available in entropy_fifo
 */
bool fips_trng_startup(int stage);

/**
 * Check that Cr50-wide HMAC_DRBG seeded according NIST SP 800-90A
 * recomendation is properly initialized and can be used.
 * Includes fips_crypto_allowed() check.
 * Initialize DRBG if it was not yet initialized.
 */
bool fips_drbg_init(void);

/* FIPS DRBG initialized at boot time/first use. */
extern struct drbg_ctx fips_drbg;

/* Initialize for use as RFC6979 DRBG. */
void hmac_drbg_init_rfc6979(struct drbg_ctx *ctx, const p256_int *key,
			    const p256_int *message);

/* Generate a p256 number between 1 < k < |p256| using provided DRBG. */
enum dcrypto_result p256_hmac_drbg_generate(struct drbg_ctx *ctx,
					    p256_int *k_out);

/* Set seed for fast random number generator using LFSR. */
void set_fast_random_seed(uint32_t seed);

/* Generate week pseudorandom using LFSR for blinding purposes. */
uint32_t fast_random(void);


typedef int32_t p256_sdigit;
typedef uint64_t p256_ddigit;
typedef int64_t p256_sddigit;

#define P256_DIGITS(x)	 ((x)->a)
#define P256_DIGIT(x, y) ((x)->a[y])

extern const p256_int SECP256r1_nMin2;

/* Check p256 is a zero. */
int p256_is_zero(const p256_int *a);

/* c := a + (single digit)b, returns carry 1 on carry. */
int p256_add_d(const p256_int *a, p256_digit b, p256_int *c);

/* Returns -1, 0 or 1. */
int p256_cmp(const p256_int *a, const p256_int *b);

/* Return -1 if a < b. */
int p256_lt_blinded(const p256_int *a, const p256_int *b);

/**
 * Raw sign with provided nonce (k). Used internally and for testing.
 *
 * @param k valid random per-message nonce for ECDSA sign
 * @param key valid private key for ECDSA sign
 * @param message message to sign encoded as p-256 int
 * @param r generated signature
 * @param s generated signature
 *
 * @return DCRYPTO_OK if successful
 */
enum dcrypto_result dcrypto_p256_ecdsa_sign_raw(
	const p256_int *k, const p256_int *key, const p256_int *message,
	p256_int *r, p256_int *s) __warn_unused_result;

/**
 * ECDSA P-256 Sign using provide DRBG as source for per-message random
 * nonces. Used primarily for deterministic signatures per RFC 6979.
 *
 * @param drbg initialized DRBG
 * @param key valid private key for ECDSA sign
 * @param message message to sign encoded as p-256 int
 * @param r generated signature
 * @param s  generated signature
 *
 * @return DCRYPTO_OK if successful
 */
enum dcrypto_result dcrypto_p256_ecdsa_sign(struct drbg_ctx *drbg,
					    const p256_int *key,
					    const p256_int *message,
					    p256_int *r,
					    p256_int *s) __warn_unused_result;

/**
 * Compute k*G (base point multiplication). Used to compute public key
 * from private scalar.
 *
 * @param k private key
 * @param x output component x
 * @param y output component y
 *
 * @return DCRYPTO_OK if successful
 */
enum dcrypto_result dcrypto_p256_base_point_mul(
	const p256_int *k, p256_int *x, p256_int *y) __warn_unused_result;

/**
 * Compute multiplication of input point (in_x, in_y) by scalar k.
 * Used to compute shared point in ECDH.
 *
 * @param k private key
 * @param in_x input public component x
 * @param in_y input public component y
 * @param x output shared component x
 * @param y output shared component y
 *
 * @return DCRYPTO_OK if successful
 */
enum dcrypto_result dcrypto_p256_point_mul(const p256_int *k,
					   const p256_int *in_x,
					   const p256_int *in_y, p256_int *x,
					   p256_int *y) __warn_unused_result;

/**
 * Verify ECDSA NIST P-256 signature.
 *
 * @param key_x public key component x
 * @param key_y public key component y
 * @param message message digest converted to p256_int
 * @param r signature component r
 * @param s signature component s
 *
 * @return DCRYPTO_OK if signature is valid
 */
enum dcrypto_result dcrypto_p256_ecdsa_verify(
	const p256_int *key_x, const p256_int *key_y, const p256_int *message,
	const p256_int *r, const p256_int *s) __warn_unused_result;

/**
 * Verify that provided point is on NIST P-256 curve.
 *
 * @param x public key component x
 * @param y public key component y
 *
 * @return DCRYPTO_OK if point is on curve
 */
enum dcrypto_result dcrypto_p256_is_valid_point(
	const p256_int *x, const p256_int *y) __warn_unused_result;

/**
 * Pair-wise consistency test for private and public key.
 *
 * @param drbg - DRBG to use for nonce generation
 * @param d - private key (scalar)
 * @param x - public key part
 * @param y - public key part
 *
 * @return DCRYPTO_OK on success
 */
enum dcrypto_result dcrypto_p256_key_pwct(
	struct drbg_ctx *drbg, const p256_int *d, const p256_int *x,
	const p256_int *y) __warn_unused_result;

/* Wipe content of rnd with pseudo-random values. */
void p256_fast_random(p256_int *rnd);

/**
 * Sign using provided DRBG. Reseed DRBG with entropy from verified TRNG if
 * needed.
 *
 * @param drbg DRBG to use
 * @param key P-256 private key
 * @param message - Message to sign as P-256 (in little-endian)
 * @param r - Generated signature
 * @param s - Generated signature
 * @return DCRYPTO_OK if success
 */
enum dcrypto_result dcrypto_p256_fips_sign_internal(
	struct drbg_ctx *drbg, const p256_int *key, const p256_int *message,
	p256_int *r, p256_int *s) __warn_unused_result;


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

/* Functions to convert between uint32_t and uint64_t */
static inline uint32_t lo32(uint64_t v)
{
	return (uint32_t)v;
}
static inline uint32_t hi32(uint64_t v)
{
	return (uint32_t)(v >> 32);
}
static inline uint64_t make64(uint32_t hi, uint32_t lo)
{
	return (((uint64_t)hi) << 32) | lo;
}

static inline uint32_t lo16(uint32_t v)
{
	return (uint32_t)(v)&0xffff;
}

static inline uint32_t hi16(uint32_t v)
{
	return (uint32_t)(v >> 16);
}

static inline int count_leading_zeros(uint32_t x)
{
	/* __builtin_clz(0) is undefined, so explicitly return bit size. */
	return (x) ? __builtin_clz(x) : 32;
}

static inline int count_trailing_zeros(uint32_t x)
{
	/* __builtin_ctz(0) is undefined, so explicitly return bit size. */
	return (x) ? __builtin_ctz(x) : 32;
}

/* stack based allocation */
#ifndef alloca
#define alloca __builtin_alloca
#endif

/* Define machine word alignment mask. */
#define WORD_MASK (sizeof(uintptr_t) - 1)

/* return true if pointer is not word aligned. */
static inline bool is_not_aligned(const void *ptr)
{
	return (uintptr_t)ptr & WORD_MASK;
}

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
