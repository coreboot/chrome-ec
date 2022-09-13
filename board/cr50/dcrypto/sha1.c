/* Copyright 2021 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "dcrypto.h"
#include "endian.h"
#include "internal.h"

static void SHA1_transform(struct sha1_ctx *const ctx)
{
	uint32_t W[80];
	uint32_t A, B, C, D, E;
	size_t t;
	static const uint32_t K[4] = { 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC,
				       0xCA62C1D6 };

	for (t = 0; t < 16; ++t)
		W[t] = be32toh(ctx->b32[t]);
	for (; t < 80; t++)
		W[t] = rol(W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16], 1);

	A = ctx->state[0];
	B = ctx->state[1];
	C = ctx->state[2];
	D = ctx->state[3];
	E = ctx->state[4];
	for (t = 0; t < 80; t++) {
		uint32_t tmp = rol(A, 5) + E + W[t];

		if (t < 20)
			tmp += (D ^ (B & (C ^ D))) + K[0];
		else if (t < 40)
			tmp += (B ^ C ^ D) + K[1];
		else if (t < 60)
			tmp += ((B & C) | (D & (B | C))) + K[2];
		else
			tmp += (B ^ C ^ D) + K[3];
		E = D;
		D = C;
		C = rol(B, 30);
		B = A;
		A = tmp;
	}
	ctx->state[0] += A;
	ctx->state[1] += B;
	ctx->state[2] += C;
	ctx->state[3] += D;
	ctx->state[4] += E;
}
/**
 * Define aliases taking union type as parameter. This is safe
 * as union type has header in same place and is not less than original type.
 * Equal to:
 * void SHA1_init_as_hash(hash_ctx_t *const ctx) {SHA1_init(&ctx.sha1);}
 * but save some space for embedded uses.
 */
BUILD_ASSERT(sizeof(union hash_ctx) >= sizeof(struct sha1_ctx));
static void SHA1_init_as_hash(union hash_ctx *const ctx) __alias(SHA1_sw_init);
static void SHA1_update_as_hash(union hash_ctx *const ctx, const void *data,
				size_t len) __alias(SHA1_sw_update);
static const union sha_digests *SHA1_final_as_hash(union hash_ctx *const ctx)
	__alias(SHA1_sw_final);

void SHA1_sw_init(struct sha1_ctx *const ctx)
{
	static const struct hash_vtable sha1_vtab = {
		SHA1_init_as_hash,	SHA1_update_as_hash, SHA1_final_as_hash,
		HMAC_sw_final,		SHA1_DIGEST_SIZE,    SHA1_BLOCK_SIZE,
		sizeof(struct sha1_ctx)
	};
	static const uint32_t sha1_init[SHA1_DIGEST_WORDS] = {
		0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0
	};

	ctx->f = &sha1_vtab;
	memcpy(ctx->state, sha1_init, sizeof(ctx->state));
	ctx->count = 0;
}

void SHA1_sw_update(struct sha1_ctx *const ctx, const void *data, size_t len)
{
	size_t i = ctx->count & (SHA1_BLOCK_SIZE - 1);
	const uint8_t *p = (const uint8_t *)data;

	ctx->count += len;
	while (len--) {
		ctx->b8[i++] = *p++;
		if (i == SHA1_BLOCK_SIZE) {
			SHA1_transform(ctx);
			i = 0;
		}
	}
}

const struct sha1_digest *SHA1_sw_final(struct sha1_ctx *const ctx)
{
	uint64_t cnt = (uint64_t)ctx->count * CHAR_BIT;
	size_t i = ctx->count & (SHA1_BLOCK_SIZE - 1);

	/**
	 * append the bit '1' to the message which would be 0x80 if message
	 * length is a multiple of 8 bits.
	 */
	ctx->b8[i++] = 0x80;
	/**
	 * append 0 ≤ k < 512 bits '0', such that the resulting message length
	 * in bits is congruent to −64 ≡ 448 (mod 512)
	 */
	if (i > (SHA1_BLOCK_SIZE - sizeof(cnt))) {
		/* current block won't fit length, so move to next */
		while (i < SHA1_BLOCK_SIZE)
			ctx->b8[i++] = 0;
		SHA1_transform(ctx);
		i = 0;
	}
	/* pad rest of zeros */
	while (i < (SHA1_BLOCK_SIZE - sizeof(cnt)))
		ctx->b8[i++] = 0;
	/* place big-endian 64-bit bit counter at the end of block */
	ctx->b64[SHA1_BLOCK_DWORDS - 1] = htobe64(cnt);
	SHA1_transform(ctx);
	for (i = 0; i < 5; i++)
		ctx->b32[i] = htobe32(ctx->state[i]);
	return &ctx->digest;
}

/* One shot SHA1 calculation */
const struct sha1_digest *SHA1_sw_hash(const void *data, size_t len,
				       struct sha1_digest *digest)
{
	struct sha1_ctx ctx;

	SHA1_sw_init(&ctx);
	SHA1_sw_update(&ctx, data, len);
	memcpy(digest->b8, SHA1_sw_final(&ctx)->b8, SHA1_DIGEST_SIZE);
	return digest;
}
