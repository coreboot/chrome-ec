/* Copyright 2015 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Crypto wrapper library for the g chip.
 */
#ifndef __EC_BOARD_CR50_DCRYPTO_DCRYPTO_H
#define __EC_BOARD_CR50_DCRYPTO_DCRYPTO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "common.h"

/**
 * Result codes for crypto operations, targeting
 * high Hamming distance from each other.
 */
enum dcrypto_result {
	DCRYPTO_OK = 0xAA33AAFF, /* Success. */
	DCRYPTO_FAIL = 0x55665501, /* Failure. */
	DCRYPTO_RETRY = 0xA5775A33,
	DCRYPTO_RESEED_NEEDED = 0x36AA6355,
};

enum cipher_mode {
	CIPHER_MODE_ECB = 0, /* NIST SP 800-38A */
	CIPHER_MODE_CTR = 1, /* NIST SP 800-38A */
	CIPHER_MODE_CBC = 2, /* NIST SP 800-38A */
	CIPHER_MODE_GCM = 3 /* NIST SP 800-38D */
};

enum encrypt_mode { DECRYPT_MODE = 0, ENCRYPT_MODE = 1 };

enum hashing_mode {
	HASH_SHA1 = 0x0004, /* = TPM_ALG_SHA1 */
	HASH_SHA256 = 0x000B, /* = TPM_ALG_SHA256 */
	HASH_SHA384 = 0x000C, /* = TPM_ALG_SHA384 */
	HASH_SHA512 = 0x000D, /* = TPM_ALG_SHA512 */
	HASH_NULL = 0x0010 /* = TPM_ALG_NULL, Only supported for PKCS#1 */
};

/**
 * SHA1/SHA2, HMAC API
 */
#define SHA1_DIGEST_SIZE 20
#define SHA_DIGEST_SIZE	 SHA1_DIGEST_SIZE

#define SHA224_DIGEST_SIZE  28
#define SHA256_DIGEST_SIZE  32
#define SHA1_BLOCK_SIZE	    64
#define SHA1_BLOCK_WORDS    (SHA1_BLOCK_SIZE / sizeof(uint32_t))
#define SHA1_BLOCK_DWORDS   (SHA1_BLOCK_SIZE / sizeof(uint64_t))
#define SHA1_DIGEST_WORDS   (SHA1_DIGEST_SIZE / sizeof(uint32_t))
#define SHA224_BLOCK_SIZE   64
#define SHA224_BLOCK_WORDS  (SHA224_BLOCK_SIZE / sizeof(uint32_t))
#define SHA224_BLOCK_DWORDS (SHA224_BLOCK_SIZE / sizeof(uint64_t))
#define SHA224_DIGEST_WORDS (SHA224_DIGEST_SIZE / sizeof(uint32_t))
#define SHA256_BLOCK_SIZE   64
#define SHA256_BLOCK_WORDS  (SHA256_BLOCK_SIZE / sizeof(uint32_t))
#define SHA256_BLOCK_DWORDS (SHA256_BLOCK_SIZE / sizeof(uint64_t))
#define SHA256_DIGEST_WORDS (SHA256_DIGEST_SIZE / sizeof(uint32_t))

#define SHA_DIGEST_WORDS    (SHA_DIGEST_SIZE / sizeof(uint32_t))
#define SHA256_DIGEST_WORDS (SHA256_DIGEST_SIZE / sizeof(uint32_t))

#ifdef CONFIG_UPTO_SHA512
#define SHA_DIGEST_MAX_BYTES SHA512_DIGEST_SIZE
#else
#define SHA_DIGEST_MAX_BYTES SHA256_DIGEST_SIZE
#endif

/**
 * Hash contexts. Each context starts with pointer to vtable containing
 * functions to perform implementation specific operations.
 * It is designed to support both software and hardware implementations.
 * Contexts for different digest types can overlap, but vtable stores
 * actual size of context which enables stack-efficient implementation of
 * HMAC - say HMAC SHA2-256 shouldn't reserve space as for HMAC SHA2-512.
 */
union hash_ctx; /* forward declaration of generic hash context type */
union hmac_ctx; /* forward declaration of generic HMAC context type */

union sha_digests; /* forward declaration of generic digest type */

/* Combined HASH & HMAC vtable to support SW & HW implementations. */
struct hash_vtable {
	/* SHA init function, used primarily by SW HMAC implementation. */
	void (*const init)(union hash_ctx *const);
	/* Update function for SHA & HMAC, assuming it's the same. */
	void (*const update)(union hash_ctx *const, const void *, size_t);
	/* SHA final function, digest specific. */
	const union sha_digests *(*const final)(union hash_ctx *const);

	/* HW HMAC support may require special ending. */
	const union sha_digests *(*const hmac_final)(union hmac_ctx *const);

	/* Digest size of in bytes. */
	size_t digest_size;

	/* Digest block size in bytes. */
	size_t block_size;

	/* Offset of first byte after context, used for HMAC */
	size_t context_size;
};

struct sha256_digest {
	union {
		uint8_t b8[SHA256_DIGEST_SIZE];
		uint32_t b32[SHA256_DIGEST_WORDS];
	};
};
BUILD_ASSERT(sizeof(struct sha256_digest) == SHA256_DIGEST_SIZE);

struct sha224_digest {
	union {
		uint8_t b8[SHA224_DIGEST_SIZE];
		uint32_t b32[SHA224_DIGEST_WORDS];
	};
};
BUILD_ASSERT(sizeof(struct sha224_digest) == SHA224_DIGEST_SIZE);

struct sha1_digest {
	union {
		uint8_t b8[SHA1_DIGEST_SIZE];
		uint32_t b32[SHA1_DIGEST_WORDS];
	};
};
BUILD_ASSERT(sizeof(struct sha1_digest) == SHA1_DIGEST_SIZE);


/* SHA256 specific type to allocate just enough memory. */
struct sha256_ctx {
	const struct hash_vtable *f; /* metadata & vtable */
	size_t count; /* number of bytes processed */
	uint32_t state[SHA256_DIGEST_WORDS]; /* up to SHA2-256 */
	union {
		uint8_t b8[SHA256_BLOCK_SIZE];
		uint32_t b32[SHA256_BLOCK_WORDS];
		uint64_t b64[SHA256_BLOCK_DWORDS];
		struct sha256_digest digest;
	};
};

#define sha224_ctx sha256_ctx

struct sha1_ctx {
	const struct hash_vtable *f; /* metadata & vtable. */
	size_t count; /* number of bytes processed. */
	uint32_t state[SHA1_DIGEST_WORDS];
	union {
		uint8_t b8[SHA1_BLOCK_SIZE];
		uint32_t b32[SHA1_BLOCK_WORDS];
		uint64_t b64[SHA1_BLOCK_DWORDS];
		struct sha1_digest digest;
	};
};

#ifdef CONFIG_UPTO_SHA512
#define SHA384_DIGEST_SIZE 48
#define SHA512_DIGEST_SIZE 64

#define SHA384_BLOCK_SIZE 128
#define SHA512_BLOCK_SIZE 128

#define SHA384_BLOCK_WORDS   (SHA384_BLOCK_SIZE / sizeof(uint32_t))
#define SHA384_BLOCK_DWORDS  (SHA384_BLOCK_SIZE / sizeof(uint64_t))
#define SHA384_DIGEST_WORDS  (SHA384_DIGEST_SIZE / sizeof(uint32_t))
#define SHA384_DIGEST_DWORDS (SHA384_DIGEST_SIZE / sizeof(uint64_t))

#define SHA512_BLOCK_WORDS   (SHA512_BLOCK_SIZE / sizeof(uint32_t))
#define SHA512_BLOCK_DWORDS  (SHA512_BLOCK_SIZE / sizeof(uint64_t))
#define SHA512_DIGEST_WORDS  (SHA512_DIGEST_SIZE / sizeof(uint32_t))
#define SHA512_DIGEST_DWORDS (SHA512_DIGEST_SIZE / sizeof(uint64_t))

struct sha384_digest {
	union {
		uint8_t b8[SHA384_DIGEST_SIZE];
		uint32_t b32[SHA384_DIGEST_WORDS];
	};
};
BUILD_ASSERT(sizeof(struct sha384_digest) == SHA384_DIGEST_SIZE);

struct sha512_digest {
	union {
		uint8_t b8[SHA512_DIGEST_SIZE];
		uint32_t b32[SHA512_DIGEST_WORDS];
	};
};
BUILD_ASSERT(sizeof(struct sha512_digest) == SHA512_DIGEST_SIZE);

struct sha512_ctx {
	const struct hash_vtable *f; /* metadata & vtable. */
	size_t count; /* number of bytes processed. */
	uint64_t state[SHA512_DIGEST_DWORDS]; /* up to SHA2-512. */
	union {
		uint8_t b8[SHA512_BLOCK_SIZE];
		uint32_t b32[SHA512_BLOCK_WORDS];
		uint64_t b64[SHA512_BLOCK_DWORDS];
		struct sha512_digest digest;
	};
};

#define sha384_ctx sha512_ctx
#endif

/**
 * Generic hash type, allocating memory for any supported hash context
 * Each context should have header at known location.
 */
union hash_ctx {
	const struct hash_vtable *f; /* common metadata & vtable */
	struct sha1_ctx sha1;
	struct sha256_ctx sha256;
	struct sha224_ctx sha224;
#ifdef CONFIG_UPTO_SHA512
	struct sha384_ctx sha384;
	struct sha512_ctx sha512;
#endif
};

union sha_digests {
	struct sha1_digest sha1;
	struct sha224_digest sha224;
	struct sha256_digest sha256;
#ifdef CONFIG_UPTO_SHA512
	struct sha384_digest sha384;
	struct sha512_digest sha512;
#endif
	/* Convenience accessor to bytes. */
	uint8_t b8[SHA256_DIGEST_SIZE];
};

/* Header should be at constant offset to safely cast types to smaller size */
BUILD_ASSERT(offsetof(union hash_ctx, f) == offsetof(struct sha1_ctx, f));
BUILD_ASSERT(offsetof(union hash_ctx, f) == offsetof(struct sha256_ctx, f));
BUILD_ASSERT(offsetof(union hash_ctx, f) == offsetof(struct sha224_ctx, f));

#ifdef CONFIG_UPTO_SHA512
BUILD_ASSERT(offsetof(union hash_ctx, f) == offsetof(struct sha384_ctx, f));
BUILD_ASSERT(offsetof(union hash_ctx, f) == offsetof(struct sha512_ctx, f));
#endif

struct hmac_sha1_ctx {
	struct sha1_ctx hash;
	uint32_t opad[SHA1_BLOCK_WORDS];
};

struct hmac_sha224_ctx {
	struct sha224_ctx hash;
	uint32_t opad[SHA224_BLOCK_WORDS];
};

struct hmac_sha256_ctx {
	struct sha256_ctx hash;
	uint32_t opad[SHA256_BLOCK_WORDS];
};

#ifdef CONFIG_UPTO_SHA512
struct hmac_sha384_ctx {
	struct sha384_ctx hash;
	uint32_t opad[SHA384_BLOCK_WORDS];
};

struct hmac_sha512_ctx {
	struct sha512_ctx hash;
	uint32_t opad[SHA512_BLOCK_WORDS];
};
#endif

/**
 * HMAC context reserving memory for any supported hash type.
 * It's SHA context following storage for ipad/opad
 */
union hmac_ctx {
	const struct hash_vtable *f; /* common metadata & vtable */
	union hash_ctx hash; /* access as hash */
	/* hmac contexts */
	struct hmac_sha1_ctx hmac_sha1;
	struct hmac_sha256_ctx hmac_sha256;
	struct hmac_sha224_ctx hmac_sha224;
#ifdef CONFIG_UPTO_SHA512
	struct hmac_sha384_ctx hmac_sha384;
	struct hmac_sha512_ctx hmac_sha512;
#endif
};

/* Header should be at constant offset to safely cast types to smaller size */
BUILD_ASSERT(offsetof(union hmac_ctx, f) == offsetof(struct sha1_ctx, f));
BUILD_ASSERT(offsetof(union hmac_ctx, f) == offsetof(struct sha256_ctx, f));
BUILD_ASSERT(offsetof(union hmac_ctx, f) == offsetof(struct sha224_ctx, f));

#ifdef CONFIG_UPTO_SHA512
BUILD_ASSERT(offsetof(union hmac_ctx, f) == offsetof(struct sha384_ctx, f));
BUILD_ASSERT(offsetof(union hmac_ctx, f) == offsetof(struct sha512_ctx, f));
#endif

/**
 * Initialize software version of hash computation which explicitly allows
 * context switching / serialization.
 *
 * @param ctx storage for context
 * @param mode hashing algorithm
 *
 * @return DCRYPTO_OK if successful, DCRYPTO_FAIL otherwise.
 */
enum dcrypto_result DCRYPTO_sw_hash_init(
	union hash_ctx *ctx, enum hashing_mode mode) __warn_unused_result;

/**
 * Initialize hardware-accelerated or software version of hash computation,
 * preferring hardware version when available.
 *
 * @param ctx storage for context
 * @param mode hashing algorithm
 *
 * @return DCRYPTO_OK if successful, DCRYPTO_FAIL otherwise.
 */
enum dcrypto_result DCRYPTO_hw_hash_init(
	union hash_ctx *ctx, enum hashing_mode mode) __warn_unused_result;

/**
 * Return hash size for specified hash algorithm
 * @param mode hash algorithm
 * @return non-zero if algorithm is supported, 0 otherwise
 */
size_t DCRYPTO_hash_size(enum hashing_mode mode);

/**
 * Initialize software version of hash computation which explicitly allows
 * context switching / serialization.
 *
 * @param ctx storage for context
 * @param key HMAC key
 * @param len length of HMAC key
 * @param mode hashing algorithm
 *
 * @return DCRYPTO_OK if successful, DCRYPTO_FAIL otherwise.
 */
enum dcrypto_result DCRYPTO_sw_hmac_init(union hmac_ctx *ctx, const void *key,
					 size_t len, enum hashing_mode mode)
	__warn_unused_result;

/**
 * Initialize hardware-accelerated or software version of HMAC computation,
 * preferring hardware version when available.
 *
 * @param ctx storage for context
 * @param key HMAC key
 * @param len length of HMAC key
 * @param mode hashing algorithm
 *
 * @return DCRYPTO_OK if successful, DCRYPTO_FAIL otherwise.
 */
enum dcrypto_result DCRYPTO_hw_hmac_init(union hmac_ctx *ctx, const void *key,
					 size_t len, enum hashing_mode mode)
	__warn_unused_result;

/**
 * Compute SHA256 using preferably hardware implementation.
 * API maintained compatible with RO code.
 *
 * @param data input data
 * @param n length of data
 * @param digest destination
 * @return NULL if failure, digest if successful
 */
const uint8_t *DCRYPTO_SHA256_hash(const void *data, size_t n, uint8_t *digest);

/**
 * Compute SHA256 using preferably hardware implementation.
 * API maintained compatible with RO code.
 *
 * @param data input data
 * @param n length of data
 * @param digest destination
 * @return NULL if failure, digest if successful
 */
const uint8_t *DCRYPTO_SHA1_hash(const void *data, size_t n, uint8_t *digest);

/**
 * Convenience wrappers with type checks.
 */
#ifndef CONFIG_DCRYPTO_MOCK

/**
 * Add data to message, call configured transform function when block
 * is full.
 * @param ctx digest context (can be one of union subtypes).
 * @param data input data
 * @param len length of data
 */
__always_inline void HASH_update(union hash_ctx *const ctx, const void *data,
				 size_t len)
{
	ctx->f->update(ctx, data, len);
}
/**
 * Finalize hash computation by adding padding, message length.
 * Returns pointer to the computed digest stored inside the provided context.
 *
 * @param ctx digest context (can be one of union subtypes).
 *
 * @return pointer to computed digest inside ctx.
 */
__always_inline const union sha_digests *HASH_final(union hash_ctx *const ctx)
{
	return ctx->f->final(ctx);
}

/**
 * Add data to message, call configured transform function when block
 * is full.
 * @param ctx SHA256 digest context
 * @param data input data
 * @param len length of data
 */
__always_inline void SHA256_update(struct sha256_ctx *const ctx,
				   const void *data, size_t len)
{
	ctx->f->update((union hash_ctx *)ctx, data, len);
}

/**
 * Finalize hash computation by adding padding, message length.
 * Returns pointer to the computed digest stored inside the provided context.
 *
 * @param ctx SHA256 digest context.
 *
 * @return pointer to computed digest inside ctx.
 */
__always_inline const struct sha256_digest *SHA256_final(
	struct sha256_ctx *const ctx)
{
	return &ctx->f->final((union hash_ctx *)ctx)->sha256;
}
__always_inline void HMAC_SHA256_update(struct hmac_sha256_ctx *const ctx,
					const void *data, size_t len)
{
	ctx->hash.f->update((union hash_ctx *)&ctx->hash, data, len);
}

__always_inline const struct sha256_digest *HMAC_SHA256_final(
	struct hmac_sha256_ctx *ctx)
{
	return &ctx->hash.f->hmac_final((union hmac_ctx *)ctx)->sha256;
}
__always_inline void SHA1_update(struct sha1_ctx *const ctx, const void *data,
				 size_t len)
{
	ctx->f->update((union hash_ctx *)ctx, data, len);
}
__always_inline const struct sha1_digest *SHA1_final(struct sha1_ctx *const ctx)
{
	return &ctx->f->final((union hash_ctx *)ctx)->sha1;
}

/**
 * Initialize SHA2-256 computation
 *
 * @param ctx SHA256 context

 * @return DCRYPTO_OK if successful
 */
__always_inline __warn_unused_result enum dcrypto_result DCRYPTO_hw_sha256_init(
	struct sha256_ctx *ctx)
{
	return DCRYPTO_hw_hash_init((union hash_ctx *)ctx, HASH_SHA256);
}

/**
 * Initialize HMAC SHA2-256 computation
 *
 * @param ctx HMAC SHA256 context
 * @param key HMAC key
 * @param len length of key
 * @return DCRYPTO_OK if successful
 */
__always_inline __warn_unused_result enum dcrypto_result
DCRYPTO_hw_hmac_sha256_init(struct hmac_sha256_ctx *ctx, const void *key,
			    size_t len)
{
	return DCRYPTO_hw_hmac_init((union hmac_ctx *)ctx, key, len,
				    HASH_SHA256);
}

#else
/* To enable DCRYPTO mocks just provide prototypes. */
void HASH_update(union hash_ctx *const ctx, const void *data, size_t len);
const union sha_digests *HASH_final(union hash_ctx *const ctx);
void SHA256_update(struct sha256_ctx *const ctx, const void *data, size_t len);
const struct sha256_digest *SHA256_final(struct sha256_ctx *const ctx);
void HMAC_SHA256_update(struct hmac_sha256_ctx *const ctx, const void *data,
			size_t len);
const struct sha256_digest *HMAC_SHA256_final(struct hmac_sha256_ctx *ctx);
void SHA1_update(struct sha1_ctx *const ctx, const void *data, size_t len);
const struct sha1_digest *SHA1_final(struct sha1_ctx *const ctx);
enum dcrypto_result DCRYPTO_hw_sha256_init(struct sha256_ctx *ctx);
enum dcrypto_result DCRYPTO_hw_hmac_sha256_init(struct hmac_sha256_ctx *ctx,
						const void *key, size_t len);
#endif

/**
 * Returns digest size for configured hash.
 */
__always_inline size_t HASH_size(union hash_ctx *const ctx)
{
	return ctx->f->digest_size;
}

/**
 * Return block size for configured hash.
 */
__always_inline size_t HASH_block_size(union hash_ctx *const ctx)
{
	return ctx->f->block_size;
}

/* HMAC_update() is same as HASH_update(). */
__always_inline void HMAC_update(union hmac_ctx *const ctx, const void *data,
				 size_t len)
{
	ctx->f->update(&ctx->hash, data, len);
}

__always_inline size_t HMAC_size(union hmac_ctx *const ctx)
{
	return ctx->f->digest_size;
}

__always_inline const union sha_digests *HMAC_final(union hmac_ctx *const ctx)
{
	return ctx->f->hmac_final(ctx);
}

__always_inline void HMAC_SHA1_update(struct hmac_sha1_ctx *const ctx,
				      const void *data, size_t len)
{
	ctx->hash.f->update((union hash_ctx *)&ctx->hash, data, len);
}

__always_inline const struct sha1_digest *HMAC_SHA1_final(
	struct hmac_sha1_ctx *const ctx)
{
	return &ctx->hash.f->hmac_final((union hmac_ctx *)ctx)->sha1;
}

#ifdef CONFIG_UPTO_SHA512
__always_inline void SHA384_update(struct sha384_ctx *const ctx,
				   const void *data, size_t len)
{
	ctx->f->update((union hash_ctx *)ctx, data, len);
}

__always_inline const struct sha384_digest *SHA384_final(
	struct sha384_ctx *const ctx)
{
	return &ctx->f->final((union hash_ctx *)ctx)->sha384;
}

__always_inline void SHA512_update(struct sha512_ctx *const ctx,
				   const void *data, size_t len)
{
	ctx->f->update((union hash_ctx *)ctx, data, len);
}

__always_inline const struct sha512_digest *SHA512_final(
	struct sha512_ctx *const ctx)
{
	return &ctx->f->final((union hash_ctx *)ctx)->sha512;
}

__always_inline void HMAC_SHA384_update(struct hmac_sha384_ctx *ctx,
					const void *data, size_t len)
{
	ctx->hash.f->update((union hash_ctx *)&ctx->hash, data, len);
}
__always_inline const struct sha384_digest *HMAC_SHA384_final(
	struct hmac_sha384_ctx *ctx)
{
	return &ctx->hash.f->hmac_final((union hmac_ctx *)ctx)->sha384;
}

__always_inline void HMAC_SHA512_update(struct hmac_sha512_ctx *ctx,
					const void *data, size_t len)
{
	ctx->hash.f->update((union hash_ctx *)&ctx->hash, data, len);
}
__always_inline const struct sha512_digest *HMAC_SHA512_final(
	struct hmac_sha512_ctx *ctx)
{
	return &ctx->hash.f->hmac_final((union hmac_ctx *)ctx)->sha512;
}
#endif

/*
 * AES implementation, based on a hardware AES block.
 * FIPS Publication 197, The Advanced Encryption Standard (AES)
 */
#define AES256_BLOCK_CIPHER_KEY_SIZE 32

/**
 * Initialize AES hardware into specified mode
 *
 * @param key AES key
 * @param key_len key length in bits 128/192/256
 * @param iv initialization vector for CTR/CBC mode
 * @param c_mode cipher mode ECB/CTR/CBC
 * @param e_mode encryption mode (encrypt/decrypt)
 * @return DCRYPTO_OK if successful
 */
enum dcrypto_result DCRYPTO_aes_init(const uint8_t *key, size_t key_len,
				     const uint8_t *iv, enum cipher_mode c_mode,
				     enum encrypt_mode e_mode);

enum dcrypto_result DCRYPTO_aes_block(const uint8_t *in, uint8_t *out);

/* AES-CTR-128/192/256
 * NIST Special Publication 800-38A
 */
enum dcrypto_result DCRYPTO_aes_ctr(uint8_t *out, const uint8_t *key,
				    uint32_t key_bits, const uint8_t *iv,
				    const uint8_t *in, size_t in_len);

void DCRYPTO_aes_write_iv(const uint8_t *iv);
void DCRYPTO_aes_read_iv(uint8_t *iv);


/*
 * BIGNUM utility methods.
 */

/*
 * Use this structure to avoid alignment problems with input and output
 * pointers.
 */
struct access_helper {
	uint32_t udata;
} __packed;


struct LITE_BIGNUM {
	uint32_t dmax;              /* Size of d, in 32-bit words. */
	struct access_helper *d;  /* Word array, little endian format ... */
};


void DCRYPTO_bn_wrap(struct LITE_BIGNUM *b, void *buf, size_t len);

/**
 * Return number of bits in big number.
 * @param b pointer to big number
 * @return length in bits
 */
inline size_t bn_bits(const struct LITE_BIGNUM *b)
{
	return b->dmax * sizeof(*b->d) * 8;
}

/**
 * Return number of bytes in big number.
 * @param b pointer to big number
 * @return length in bits
 */
inline size_t bn_size(const struct LITE_BIGNUM *b)
{
	return b->dmax * sizeof(*b->d);
}

/*
 *  RSA.
 */

/* Largest supported key size for signing / encryption: 2048-bits.
 * Verification is a special case and supports 4096-bits (signing /
 * decryption could also support 4k-RSA, but is disabled since support
 * is not required, and enabling support would result in increased
 * stack usage for all key sizes.)
 */
#define RSA_BYTES_2K 256
#define RSA_BYTES_4K 512
#define RSA_WORDS_2K (RSA_BYTES_2K / sizeof(uint32_t))
#define RSA_WORDS_4K (RSA_BYTES_4K / sizeof(uint32_t))
#ifndef RSA_MAX_BYTES
#define RSA_MAX_BYTES RSA_BYTES_2K
#endif
#define RSA_MAX_WORDS (RSA_MAX_BYTES / sizeof(uint32_t))
#define RSA_F4	      65537

struct RSA {
	uint32_t e;
	struct LITE_BIGNUM N;
	struct LITE_BIGNUM d;
};

enum padding_mode {
	PADDING_MODE_PKCS1 = 0,
	PADDING_MODE_OAEP = 1,
	PADDING_MODE_PSS = 2,
	/* USE OF NULL PADDING IS NOT RECOMMENDED.
	 * SUPPORT EXISTS AS A REQUIREMENT FOR TPM2 OPERATION. */
	PADDING_MODE_NULL = 3
};

/* RSA support, FIPS PUB 186-4 *
 * Calculate r = m ^ e mod N
 */
enum dcrypto_result DCRYPTO_rsa_encrypt(struct RSA *rsa, uint8_t *out,
					size_t *out_len, const uint8_t *in,
					size_t in_len,
					enum padding_mode padding,
					enum hashing_mode hashing,
					const char *label);

/* Calculate r = m ^ d mod N
 * return DCRYPTO_OK if success
 */
enum dcrypto_result DCRYPTO_rsa_decrypt(struct RSA *rsa, uint8_t *out,
					size_t *out_len, const uint8_t *in,
					const size_t in_len,
					enum padding_mode padding,
					enum hashing_mode hashing,
					const char *label);

/* Calculate r = m ^ d mod N
 * return DCRYPTO_OK if success
 */
enum dcrypto_result DCRYPTO_rsa_sign(struct RSA *rsa, uint8_t *out,
				     size_t *out_len, const uint8_t *in,
				     const size_t in_len,
				     enum padding_mode padding,
				     enum hashing_mode hashing);

/* Calculate r = m ^ e mod N
 * return DCRYPTO_OK if success
 */
enum dcrypto_result DCRYPTO_rsa_verify(const struct RSA *rsa,
				       const uint8_t *digest, size_t digest_len,
				       const uint8_t *sig, const size_t sig_len,
				       enum padding_mode padding,
				       enum hashing_mode hashing);

/* Calculate n = p * q, d = e ^ -1 mod phi. */
enum dcrypto_result DCRYPTO_rsa_key_compute(struct LITE_BIGNUM *N,
					    struct LITE_BIGNUM *d,
					    struct LITE_BIGNUM *p,
					    struct LITE_BIGNUM *q, uint32_t e);

/*
 *  EC.
 */

/*
 * Accelerated p256. FIPS PUB 186-4
 */
#define P256_BITSPERDIGIT 32
#define P256_NDIGITS	  8
#define P256_NBYTES	  32

typedef uint32_t p256_digit;
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

/* Clear a p256_int to zero. */
void p256_clear(p256_int *a);

/* Check p256 is odd. */
int p256_is_odd(const p256_int *a);

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
 * Check if point is on NIST P-256 curve
 *
 * @param x point coordinate
 * @param y point coordinate
 *
 * @return DCRYPTO_OK if (x,y) is a valid point, DCRYPTO_FAIL otherwise
 */
enum dcrypto_result DCRYPTO_p256_is_valid_point(
	const p256_int *x, const p256_int *y) __warn_unused_result;

/**
 * Base point multiplications (compute public key from private).
 * Sets {out_x,out_y} = nG, where n is < the order of the group.
 *
 * @param out_x output public key component x
 * @param out_y output public key component y
 * @param n private key
 *
 * @return DCRYPTO_OK if successful
 */
enum dcrypto_result DCRYPTO_p256_base_point_mul(
	p256_int *out_x, p256_int *out_y,
	const p256_int *n) __warn_unused_result;

/**
 * DCRYPTO_p256_point_mul sets {out_x,out_y} = n*{in_x,in_y}, where n is <
 * the order of the group. Prior to computation check than {in_x,in_y} is
 * on NIST P-256 curve. Used to implement ECDH.
 *
 * @param out_x output shared coordinate x
 * @param out_y output shared coordinate y
 * @param n private key
 * @param in_x input public point x
 * @param in_y input public point y
 *
 * @return DCRYPTO_OK if success
 */
enum dcrypto_result DCRYPTO_p256_point_mul(
	p256_int *out_x, p256_int *out_y, const p256_int *n,
	const p256_int *in_x, const p256_int *in_y) __warn_unused_result;

/**
 * Key selection based on FIPS-186-4, section B.4.2 (Key Pair
 * Generation by Testing Candidates).
 *
 * @param x output public key component x
 * @param y output public key component y
 * @param d output private key
 * @param bytes 32 byte random seed
 *
 * d = p256_from_bytes(bytes) + 1
 *
 * If x or y is NULL, the public key part is not computed.
 *
 * @return DCRYPTO_OK on success, DCRYPTO_RETRY if d is out of range, try
 * with another seed bytes and DCRYPTO_FAIL for any other error.
 */
enum dcrypto_result DCRYPTO_p256_key_from_bytes(
	p256_int *x, p256_int *y, p256_int *d,
	const uint8_t bytes[P256_NBYTES]) __warn_unused_result;

/**
 * Verify NIST P-256 signature.
 *
 * @param key_x public key coordinate x
 * @param key_y public key coordinate x
 * @param message message digest to verify
 * @param r signature component r
 * @param s signature component s
 *
 * @return DCRYPTO_OK if success
 */
enum dcrypto_result DCRYPTO_p256_ecdsa_verify(
	const p256_int *key_x, const p256_int *key_y, const p256_int *message,
	const p256_int *r, const p256_int *s) __warn_unused_result;

/**
 * NIST ECDSA P-256 Sign.
 *
 * @param key private key
 * @param message message digest (in p256_int form)
 * @param r output signature component r
 * @param s output signature component s
 *
 * @return DCRYPTO_OK if success.
 */
enum dcrypto_result DCRYPTO_p256_ecdsa_sign(const p256_int *key,
					    const p256_int *message,
					    p256_int *r,
					    p256_int *s) __warn_unused_result;

/************************************************************/

/*
 *  BN.
 */

/* Apply Miller-Rabin test for prime candidate p.
 * Returns DCRYPTO_OK if test passed, DCRYPTO_FAIL otherwise
 */
enum dcrypto_result DCRYPTO_bn_generate_prime(struct LITE_BIGNUM *p);

/* Compute c = a * b. */
void DCRYPTO_bn_mul(struct LITE_BIGNUM *c, const struct LITE_BIGNUM *a,
		    const struct LITE_BIGNUM *b);

/* Compute (quotient, remainder) = input / divisor. */
int DCRYPTO_bn_div(struct LITE_BIGNUM *quotient, struct LITE_BIGNUM *remainder,
		   const struct LITE_BIGNUM *input,
		   const struct LITE_BIGNUM *divisor);

/*
 * ASN.1 DER
 */
size_t DCRYPTO_asn1_sigp(uint8_t *buf, const p256_int *r, const p256_int *s);
size_t DCRYPTO_asn1_pubp(uint8_t *buf, const p256_int *x, const p256_int *y);

/*
 *  X509.
 */
/* DCRYPTO_x509_verify verifies that the provided X509 certificate was issued
 * by the specified certificate authority.
 *
 * cert is a pointer to a DER encoded X509 certificate, as specified
 * in https://tools.ietf.org/html/rfc5280#section-4.1.  In ASN.1
 * notation, the certificate has the following structure:
 *
 *   Certificate  ::=  SEQUENCE  {
 *        tbsCertificate       TBSCertificate,
 *        signatureAlgorithm   AlgorithmIdentifier,
 *        signatureValue       BIT STRING  }
 *
 *   TBSCertificate  ::=  SEQUENCE  { }
 *   AlgorithmIdentifier  ::=  SEQUENCE  { }
 *
 * where signatureValue = SIGN(HASH(tbsCertificate)), with SIGN and
 * HASH specified by signatureAlgorithm.
 * Accepts only certs with OID: sha256WithRSAEncryption:
 * 30 0d 06 09 2a 86 48 86 f7 0d 01 01 0b 05 00
 */
enum dcrypto_result DCRYPTO_x509_verify(const uint8_t *cert, size_t len,
			const struct RSA *ca_pub_key);

/* Generate U2F Certificate and sign it
 * Use ECDSA with NIST P-256 curve, and SHA2-256 digest
 * @param d: private key to use
 * @param pk_x, pk_y: public key
 * @param serial: serial number for certificate
 * @param name: certificate issuer and subject
 * @param cert: output buffer for certificate
 * @param n: max size of cert
 *
 * @returns size of certificate or 0 if failure
 */
size_t DCRYPTO_x509_gen_u2f_cert_name(const p256_int *d, const p256_int *pk_x,
				      const p256_int *pk_y,
				      const p256_int *serial, const char *name,
				      uint8_t *cert, const size_t n);

/* Generate U2F Certificate with DCRYPTO_x509_gen_u2f_cert_name
 * Providing certificate issuer as BOARD or U2F
 * @param d: private key to use
 * @param pk_x, pk_y: public key
 * @param serial: serial number for certificate
 * @param name: certificate issuer and subject
 * @param cert: output buffer for certificate
 * @param n: max size of cert
 *
 * @returns size of certificate or 0 if failure
 */
size_t DCRYPTO_x509_gen_u2f_cert(const p256_int *d, const p256_int *pk_x,
				 const p256_int *pk_y, const p256_int *serial,
				 uint8_t *cert, const size_t n);

/*
 * Memory related functions.
 */
enum dcrypto_result DCRYPTO_equals(const void *a, const void *b, size_t len);

/*
 * Key-ladder and application key related functions.
 */
enum dcrypto_appid {
	RESERVED = 0,
	NVMEM = 1,
	U2F_ATTEST = 2,
	U2F_ORIGIN = 3,
	U2F_WRAP = 4,
	PERSO_AUTH = 5,
	PINWEAVER = 6,
	/* This enum value should not exceed 7. */
};

/* Retrieve Firmware Root Key from hardware key ladder. */
int DCRYPTO_ladder_compute_frk2(size_t major_fw_version, uint8_t *frk2);

/* Revoke access to hardware key ladder. */
void DCRYPTO_ladder_revoke(void);

/* Preload application specific secret into key ladder register. */
int DCRYPTO_appkey_init(enum dcrypto_appid id);

/* Clean-up secret loaded from key ladder. */
void DCRYPTO_appkey_finish(void);

/* Compute application-specific, hardware-bound constant. */
int DCRYPTO_appkey_derive(enum dcrypto_appid appid, const uint32_t input[8],
			  uint32_t output[8]);

/*
 * Encrypt/decrypt a flat blob.
 *
 * Encrypt or decrypt the input buffer, and write the correspondingly
 * ciphered output to out.  The number of bytes produced is equal to
 * the number of input bytes.  Note that the input and output pointers
 * MUST be word-aligned.
 *
 * This API is expected to be applied to a single contiguous region.

 * WARNING: A given salt/"in" pair MUST be unique, i.e. re-using a
 * salt with a logically different input buffer is catastrophic.  An
 * example of a suitable salt is one that is derived from "in", e.g. a
 * digest of the input data.
 *
 * @param appid the application-id of the calling context.
 * @param salt pointer to a unique value to be associated with this blob,
 *	       used for derivation of the proper IV, the size of the value
 *	       is as defined by DCRYPTO_CIPHER_SALT_SIZE above.
 * @param out Destination pointer where to write plaintext / ciphertext.
 * @param in  Source pointer where to read ciphertext / plaintext.
 * @param len Number of bytes to read from in / write to out.
 * @return non-zero on success, and zero otherwise.
 */
int DCRYPTO_app_cipher(enum dcrypto_appid appid, const void *salt, void *out,
		       const void *in, size_t len);

/*
 * Query whether Key Ladder is enabled.
 *
 * @return 1 if Key Ladder is enabled, and 0 otherwise.
 */
int DCRYPTO_ladder_is_enabled(void);

/**
 * Random number generation functions.
 */

/**
 * Initialize the true random number generator (TRNG) in FIPS-compliant
 * way:
 * 1. Set 1-bit alphabet
 * 2. Set maximum possible range for internal ring-oscillator
 * 3. Disable any other post-processing beyond #2
 **/
void fips_init_trng(void);

/**
 * Return true if fips_trng_rand() result contains valid random from TRNG.
 * @param rand value from fips_trng_rand32() or read_rand()
 *
 * @return true if rand contains valid random
 */
inline bool rand_valid(uint64_t rand)
{
	return (rand >> 32) != 0;
}

/**
 * Fill buffer with FIPS health checked randoms directly from TRNG.
 *
 * @param buffer buffer to fill
 * @param len size of buffer in bytes
 * @return true if successful
 * @return false if TRNG failed, values didn't pass health test
 *         or module crypto failed
 */
bool fips_trng_bytes(void *buffer, size_t len)
	__attribute__((warn_unused_result));

/**
 * Fill buffer with random bytes from FIPS-compliant HMAC_DRBG_SHA256,
 * instantiated during system start-up and reseeded as needed.
 *
 * @param buffer buffer to fill
 * @param len size of buffer in bytes
 * @return true if successful
 * @return false if any errors occurred or module crypto failed
 */
bool fips_rand_bytes(void *buffer, size_t len)
	__attribute__((warn_unused_result));


/**
 * Utility functions.
 */

/**
 * An implementation of memset that ought not to be optimized away;
 * useful for scrubbing security sensitive buffers.
 *
 * @param d destination buffer
 * @param c 8-bit value to fill buffer
 * @param n size of buffer in bytes
 * @return d
 */
void *always_memset(void *d, int c, size_t n);

/**
 * FIPS module digest for reporting.
 */
extern const struct sha256_digest fips_integrity;

#ifdef __cplusplus
}
#endif

#endif /* ! __EC_BOARD_CR50_DCRYPTO_DCRYPTO_H */
