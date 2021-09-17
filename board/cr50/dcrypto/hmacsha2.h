/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#pragma once
#include "common.h"

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

#define SHA_DIGEST_WORDS   (SHA_DIGEST_SIZE / sizeof(uint32_t))
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
 * Reset hash context with the same hash function as configured.
 * Will crash if previously not configured! Used for HMAC.
 */
static inline void HASH_reinit(union hash_ctx *const ctx)
{
	ctx->f->init(ctx);
}

#ifndef CONFIG_DCRYPTO_MOCK
/**
 * Add data to message, call configured transform function when block
 * is full.
 */
static inline void HASH_update(union hash_ctx *const ctx, const void *data,
			       size_t len)
{
	ctx->f->update(ctx, data, len);
}
#else
void HASH_update(union hash_ctx *const ctx, const void *data, size_t len);
#endif

static inline void SHA1_update(struct sha1_ctx *const ctx, const void *data,
			       size_t len)
{
	ctx->f->update((union hash_ctx *)ctx, data, len);
}

static inline void SHA256_update(struct sha256_ctx *const ctx, const void *data,
				 size_t len)
{
	ctx->f->update((union hash_ctx *)ctx, data, len);
}

/**
 * Finalize hash computation by adding padding, message length.
 * Returns pointer to computed digest stored inside provided context.
 */
#ifndef CONFIG_DCRYPTO_MOCK
static inline const union sha_digests *HASH_final(union hash_ctx *const ctx)
{
	return ctx->f->final(ctx);
}
#else
const union sha_digests *HASH_final(union hash_ctx *const ctx);
#endif

static inline const struct sha1_digest *SHA1_final(struct sha1_ctx *const ctx)
{
	return &ctx->f->final((union hash_ctx *)ctx)->sha1;
}

static inline const struct sha256_digest *SHA256_final(
				struct sha256_ctx *const ctx)
{
	return &ctx->f->final((union hash_ctx *)ctx)->sha256;
}

/**
 * Returns digest size for configured hash.
 */
static inline size_t HASH_size(union hash_ctx *const ctx)
{
	return ctx->f->digest_size;
}

/**
 * Return block size for configured hash.
 */
static inline size_t HASH_block_size(union hash_ctx *const ctx)
{
	return ctx->f->block_size;
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

/* HMAC update is same as SHA update. */
static inline void HMAC_update(union hmac_ctx *const ctx, const void *data,
			       size_t len)
{
	ctx->f->update(&ctx->hash, data, len);
}

static inline size_t HMAC_size(union hmac_ctx *const ctx)
{
	return ctx->f->digest_size;
}

static inline const union sha_digests *HMAC_final(union hmac_ctx *const ctx)
{
	return ctx->f->hmac_final(ctx);
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

static inline void HMAC_SHA1_update(struct hmac_sha1_ctx *const ctx,
				    const void *data, size_t len)
{
	ctx->hash.f->update((union hash_ctx *)&ctx->hash, data, len);
}

static inline const struct sha1_digest *
HMAC_SHA1_final(struct hmac_sha1_ctx *const ctx)
{
	return &ctx->hash.f->hmac_final((union hmac_ctx *)ctx)->sha1;
}

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

static inline void HMAC_SHA256_update(struct hmac_sha256_ctx *const ctx,
				      const void *data, size_t len)
{
	ctx->hash.f->update((union hash_ctx *)&ctx->hash, data, len);
}

static inline const struct sha256_digest *
HMAC_SHA256_final(struct hmac_sha256_ctx *ctx)
{
	return &ctx->hash.f->hmac_final((union hmac_ctx *)ctx)->sha256;
}

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

static inline void SHA384_update(struct sha384_ctx *const ctx, const void *data,
				 size_t len)
{
	ctx->f->update((union hash_ctx *)ctx, data, len);
}

static inline const struct sha384_digest *SHA384_final(
				struct sha384_ctx *const ctx)
{
	return &ctx->f->final((union hash_ctx *)ctx)->sha384;
}

static inline void SHA512_update(struct sha512_ctx *const ctx, const void *data,
				 size_t len)
{
	ctx->f->update((union hash_ctx *)ctx, data, len);
}

static inline const struct sha512_digest *SHA512_final(
				struct sha512_ctx *const ctx)
{
	return &ctx->f->final((union hash_ctx *)ctx)->sha512;
}

/**
 * HMAC SHA2-384 initialization.
 */
static inline void HMAC_SHA384_sw_init(struct hmac_sha384_ctx *ctx,
				       const void *key, size_t len)
{
	SHA384_sw_init(&ctx->hash);
	HMAC_sw_init((union hmac_ctx *)ctx, key, len);
}

static inline void HMAC_SHA384_update(struct hmac_sha384_ctx *ctx,
				      const void *data, size_t len)
{
	ctx->hash.f->update((union hash_ctx *)&ctx->hash, data, len);
}
static inline const struct sha384_digest *
HMAC_SHA384_final(struct hmac_sha384_ctx *ctx)
{
	return &ctx->hash.f->hmac_final((union hmac_ctx *)ctx)->sha384;
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
static inline void HMAC_SHA512_update(struct hmac_sha512_ctx *ctx,
				      const void *data, size_t len)
{
	ctx->hash.f->update((union hash_ctx *)&ctx->hash, data, len);
}
static inline const struct sha512_digest *
HMAC_SHA512_final(struct hmac_sha512_ctx *ctx)
{
	return &ctx->hash.f->hmac_final((union hmac_ctx *)ctx)->sha512;
}
#endif
