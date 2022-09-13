/* Copyright 2015 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "dcrypto.h"
#include "endian.h"
#include "internal.h"
#include "registers.h"
#include "util.h"

static void SHA256_transform(struct sha256_ctx *const ctx)
{
	static const uint32_t K[64] = {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
		0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
		0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
		0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
		0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
		0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
		0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
		0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
		0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
		0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
		0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
		0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
	};
	uint32_t W[64];
	uint32_t A, B, C, D, E, F, G, H;
	size_t t;

	for (t = 0; t < 16; ++t)
		W[t] = be32toh(ctx->b32[t]);
	for (; t < 64; t++) {
		uint32_t s0 = ror(W[t - 15], 7) ^ ror(W[t - 15], 18) ^
			      (W[t - 15] >> 3);
		uint32_t s1 = ror(W[t - 2], 17) ^ ror(W[t - 2], 19) ^
			      (W[t - 2] >> 10);

		W[t] = W[t - 16] + s0 + W[t - 7] + s1;
	}
	A = ctx->state[0];
	B = ctx->state[1];
	C = ctx->state[2];
	D = ctx->state[3];
	E = ctx->state[4];
	F = ctx->state[5];
	G = ctx->state[6];
	H = ctx->state[7];
	for (t = 0; t < 64; t++) {
		uint32_t s0 = ror(A, 2) ^ ror(A, 13) ^ ror(A, 22);
		uint32_t maj = (A & B) ^ (A & C) ^ (B & C);
		uint32_t t2 = s0 + maj;
		uint32_t s1 = ror(E, 6) ^ ror(E, 11) ^ ror(E, 25);
		uint32_t ch = (E & F) ^ ((~E) & G);
		uint32_t t1 = H + s1 + ch + K[t] + W[t];

		H = G;
		G = F;
		F = E;
		E = D + t1;
		D = C;
		C = B;
		B = A;
		A = t1 + t2;
	}
	ctx->state[0] += A;
	ctx->state[1] += B;
	ctx->state[2] += C;
	ctx->state[3] += D;
	ctx->state[4] += E;
	ctx->state[5] += F;
	ctx->state[6] += G;
	ctx->state[7] += H;
}
/**
 * Define aliases taking union type as parameter. This is safe
 * as union type has header in same place and is not less than original type.
 * Equal to:
 * void SHA256_init_as_hash(HASH_CTX *const ctx) {SHA256_init(&ctx.sha256);}
 * but save some space for embedded uses.
 */
BUILD_ASSERT(sizeof(union hash_ctx) >= sizeof(struct sha256_ctx));
BUILD_ASSERT(sizeof(union hash_ctx) >= sizeof(struct sha224_ctx));

static void SHA256_init_as_hash(union hash_ctx *const ctx)
	__alias(SHA256_sw_init);
static void SHA256_update_as_hash(union hash_ctx *const ctx, const void *data,
				  size_t len) __alias(SHA256_sw_update);
static const union sha_digests *SHA256_final_as_hash(union hash_ctx *const ctx)
	__alias(SHA256_sw_final);
static void SHA224_init_as_hash(union hash_ctx *const ctx)
	__alias(SHA224_sw_init);

void SHA256_sw_init(struct sha256_ctx *const ctx)
{
	static const struct hash_vtable sha256_vtable = {
		SHA256_init_as_hash,	  SHA256_update_as_hash,
		SHA256_final_as_hash,	  HMAC_sw_final,
		SHA256_DIGEST_SIZE,	  SHA256_BLOCK_SIZE,
		sizeof(struct sha256_ctx)
	};
	static const uint32_t sha256_init[SHA256_DIGEST_WORDS] = {
		0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
		0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
	};

	ctx->f = &sha256_vtable;
	memcpy(ctx->state, sha256_init, sizeof(ctx->state));
	ctx->count = 0;
}

/* SHA2-224 and SHA2-256 use same internal context. */
BUILD_ASSERT(sizeof(struct sha224_ctx) == sizeof(struct sha256_ctx));
void SHA224_sw_update(struct sha224_ctx *ctx, const void *data, size_t len)
	__alias(SHA256_sw_update);

void SHA256_sw_update(struct sha256_ctx *ctx, const void *data, size_t len)
{
	size_t i = ctx->count & (SHA256_BLOCK_SIZE - 1);
	const uint8_t *p = (const uint8_t *)data;

	ctx->count += len;
	while (len--) {
		ctx->b8[i++] = *p++;
		if (i == SHA256_BLOCK_SIZE) {
			SHA256_transform(ctx);
			i = 0;
		}
	}
}
const struct sha224_digest *SHA224_sw_final(struct sha224_ctx *const ctx)
	__alias(SHA256_sw_final);
const struct sha256_digest *SHA256_sw_final(struct sha256_ctx *const ctx)
{
	uint64_t cnt = (uint64_t)ctx->count * CHAR_BIT;
	size_t i = ctx->count & (SHA256_BLOCK_SIZE - 1);

	/**
	 * append the bit '1' to the message which would be 0x80 if message
	 * length is a multiple of 8 bits.
	 */
	ctx->b8[i++] = 0x80;
	/**
	 * Append 0 ≤ k < 512 bits '0', such that the resulting message length
	 * in bits is congruent to −64 ≡ 448 (mod 512).
	 */
	if (i > (SHA256_BLOCK_SIZE - sizeof(cnt))) {
		/* Current block won't fit length, so move to next. */
		while (i < SHA256_BLOCK_SIZE)
			ctx->b8[i++] = 0;
		SHA256_transform(ctx);
		i = 0;
	}
	/* Pad rest of zeros. */
	while (i < (SHA256_BLOCK_SIZE - sizeof(cnt)))
		ctx->b8[i++] = 0;

	/* Place big-endian 64-bit bit counter at the end of block. */
	ctx->b64[SHA256_BLOCK_DWORDS - 1] = htobe64(cnt);
	SHA256_transform(ctx);
	for (i = 0; i < 8; i++)
		ctx->b32[i] = htobe32(ctx->state[i]);
	return &ctx->digest;
}

void SHA224_sw_init(struct sha224_ctx *const ctx)
{
	/* SHA2-224 differs from SHA2-256 only in initialization. */
	static const struct hash_vtable sha224_vtable = {
		SHA224_init_as_hash,	  SHA256_update_as_hash,
		SHA256_final_as_hash,	  HMAC_sw_final,
		SHA224_DIGEST_SIZE,	  SHA224_BLOCK_SIZE,
		sizeof(struct sha224_ctx)
	};
	static const uint32_t sha224_init[SHA256_DIGEST_WORDS] = {
		0xc1059ed8, 0x367cd507, 0x3070dd17, 0xf70e5939,
		0xffc00b31, 0x68581511, 0x64f98fa7, 0xbefa4fa4
	};

	ctx->f = &sha224_vtable;
	memcpy(ctx->state, sha224_init, sizeof(ctx->state));
	ctx->count = 0;
}

/* One shot hash computation. */
const struct sha224_digest *SHA224_sw_hash(const void *data, size_t len,
					   struct sha224_digest *digest)
{
	struct sha224_ctx ctx;

	SHA224_sw_init(&ctx);
	SHA224_sw_update(&ctx, data, len);
	memcpy(digest->b8, SHA224_sw_final(&ctx), SHA224_DIGEST_SIZE);
	return digest;
}
/* One shot hash computation */
const struct sha256_digest *SHA256_sw_hash(const void *data, size_t len,
					   struct sha256_digest *digest)
{
	struct sha256_ctx ctx;

	SHA256_sw_init(&ctx);
	SHA256_sw_update(&ctx, data, len);
	memcpy(digest->b8, SHA256_sw_final(&ctx)->b8, SHA256_DIGEST_SIZE);
	return digest;
}
