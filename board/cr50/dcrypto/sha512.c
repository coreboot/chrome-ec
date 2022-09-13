/* Copyright 2016 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "dcrypto.h"
#include "endian.h"
#include "internal.h"

static void SHA512_transform(struct sha512_ctx *ctx)
{
	static const uint64_t K[80] = {
		0x428A2F98D728AE22ll, 0x7137449123EF65CDll,
		0xB5C0FBCFEC4D3B2Fll, 0xE9B5DBA58189DBBCll,
		0x3956C25BF348B538ll, 0x59F111F1B605D019ll,
		0x923F82A4AF194F9Bll, 0xAB1C5ED5DA6D8118ll,
		0xD807AA98A3030242ll, 0x12835B0145706FBEll,
		0x243185BE4EE4B28Cll, 0x550C7DC3D5FFB4E2ll,
		0x72BE5D74F27B896Fll, 0x80DEB1FE3B1696B1ll,
		0x9BDC06A725C71235ll, 0xC19BF174CF692694ll,
		0xE49B69C19EF14AD2ll, 0xEFBE4786384F25E3ll,
		0x0FC19DC68B8CD5B5ll, 0x240CA1CC77AC9C65ll,
		0x2DE92C6F592B0275ll, 0x4A7484AA6EA6E483ll,
		0x5CB0A9DCBD41FBD4ll, 0x76F988DA831153B5ll,
		0x983E5152EE66DFABll, 0xA831C66D2DB43210ll,
		0xB00327C898FB213Fll, 0xBF597FC7BEEF0EE4ll,
		0xC6E00BF33DA88FC2ll, 0xD5A79147930AA725ll,
		0x06CA6351E003826Fll, 0x142929670A0E6E70ll,
		0x27B70A8546D22FFCll, 0x2E1B21385C26C926ll,
		0x4D2C6DFC5AC42AEDll, 0x53380D139D95B3DFll,
		0x650A73548BAF63DEll, 0x766A0ABB3C77B2A8ll,
		0x81C2C92E47EDAEE6ll, 0x92722C851482353Bll,
		0xA2BFE8A14CF10364ll, 0xA81A664BBC423001ll,
		0xC24B8B70D0F89791ll, 0xC76C51A30654BE30ll,
		0xD192E819D6EF5218ll, 0xD69906245565A910ll,
		0xF40E35855771202All, 0x106AA07032BBD1B8ll,
		0x19A4C116B8D2D0C8ll, 0x1E376C085141AB53ll,
		0x2748774CDF8EEB99ll, 0x34B0BCB5E19B48A8ll,
		0x391C0CB3C5C95A63ll, 0x4ED8AA4AE3418ACBll,
		0x5B9CCA4F7763E373ll, 0x682E6FF3D6B2B8A3ll,
		0x748F82EE5DEFB2FCll, 0x78A5636F43172F60ll,
		0x84C87814A1F0AB72ll, 0x8CC702081A6439ECll,
		0x90BEFFFA23631E28ll, 0xA4506CEBDE82BDE9ll,
		0xBEF9A3F7B2C67915ll, 0xC67178F2E372532Bll,
		0xCA273ECEEA26619Cll, 0xD186B8C721C0C207ll,
		0xEADA7DD6CDE0EB1Ell, 0xF57D4F7FEE6ED178ll,
		0x06F067AA72176FBAll, 0x0A637DC5A2C898A6ll,
		0x113F9804BEF90DAEll, 0x1B710B35131C471Bll,
		0x28DB77F523047D84ll, 0x32CAAB7B40C72493ll,
		0x3C9EBE0A15C9BEBCll, 0x431D67C49C100D4Cll,
		0x4CC5D4BECB3E42B6ll, 0x597F299CFC657E2All,
		0x5FCB6FAB3AD6FAECll, 0x6C44198C4A475817ll
	};
	uint64_t W[80];
	uint64_t A, B, C, D, E, F, G, H;
	size_t t;

	for (t = 0; t < 16; t++)
		W[t] = be64toh(ctx->b64[t]);

	for (; t < 80; t++) {
		uint64_t Wt2 = W[t - 2];
		uint64_t Wt7 = W[t - 7];
		uint64_t Wt15 = W[t - 15];
		uint64_t Wt16 = W[t - 16];
		uint64_t s0 = ror64(Wt15, 1) ^ ror64(Wt15, 8) ^ (Wt15 >> 7);
		uint64_t s1 = ror64(Wt2, 19) ^ ror64(Wt2, 61) ^ (Wt2 >> 6);

		W[t] = s1 + Wt7 + s0 + Wt16;
	}
	A = ctx->state[0];
	B = ctx->state[1];
	C = ctx->state[2];
	D = ctx->state[3];
	E = ctx->state[4];
	F = ctx->state[5];
	G = ctx->state[6];
	H = ctx->state[7];
	for (t = 0; t < 80; t++) {
		uint64_t s0 = ror64(A, 28) ^ ror64(A, 34) ^ ror64(A, 39);
		uint64_t maj = (A & B) ^ (A & C) ^ (B & C);
		uint64_t t2 = s0 + maj;
		uint64_t s1 = ror64(E, 14) ^ ror64(E, 18) ^ ror64(E, 41);
		uint64_t ch = (E & F) ^ ((~E) & G);
		uint64_t t1 = H + s1 + ch + W[t] + K[t];

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
 * void SHA512_init_as_hash(HASH_CTX *const ctx) {SHA512_init(&ctx.sha512);}
 * but save some space for embedded uses.
 */
BUILD_ASSERT(sizeof(union hash_ctx) >= sizeof(struct sha512_ctx));
BUILD_ASSERT(sizeof(union hash_ctx) >= sizeof(struct sha384_ctx));

static void SHA512_init_as_hash(union hash_ctx *const ctx)
	__alias(SHA512_sw_init);
static void SHA512_update_as_hash(union hash_ctx *const ctx, const void *data,
				  size_t len) __alias(SHA512_sw_update);
static const union sha_digests *SHA512_final_as_hash(union hash_ctx *const ctx)
	__alias(SHA512_sw_final);
static void SHA384_init_as_hash(union hash_ctx *const ctx)
	__alias(SHA384_sw_init);

void SHA512_sw_init(struct sha512_ctx *const ctx)
{
	static const struct hash_vtable sha512_vtable = {
		SHA512_init_as_hash,	  SHA512_update_as_hash,
		SHA512_final_as_hash,	  HMAC_sw_final,
		SHA512_DIGEST_SIZE,	  SHA512_BLOCK_SIZE,
		sizeof(struct sha512_ctx)
	};
	static const uint64_t sha512_init[SHA512_DIGEST_DWORDS] = {
		0x6a09e667f3bcc908ll, 0xbb67ae8584caa73bll,
		0x3c6ef372fe94f82bll, 0xa54ff53a5f1d36f1ll,
		0x510e527fade682d1ll, 0x9b05688c2b3e6c1fll,
		0x1f83d9abfb41bd6bll, 0x5be0cd19137e2179ll
	};

	memcpy(ctx->state, sha512_init, sizeof(ctx->state));
	ctx->f = &sha512_vtable;
	ctx->count = 0;
}

/* SHA2-384 and SHA2-512 use same internal context. */
BUILD_ASSERT(sizeof(struct sha384_ctx) == sizeof(struct sha512_ctx));

void SHA384_sw_update(struct sha384_ctx *const ctx, const void *data,
		      size_t len) __alias(SHA512_sw_update);

void SHA512_sw_update(struct sha512_ctx *const ctx, const void *data,
		      size_t len)
{
	size_t i = ctx->count & (SHA512_BLOCK_SIZE - 1);
	const uint8_t *p = (const uint8_t *)data;

	ctx->count += len;
	while (len--) {
		ctx->b8[i++] = *p++;
		if (i == SHA512_BLOCK_SIZE) {
			SHA512_transform(ctx);
			i = 0;
		}
	}
}

const struct sha384_digest *SHA384_sw_final(struct sha384_ctx *const ctx)
	__alias(SHA512_sw_final);

const struct sha512_digest *SHA512_sw_final(struct sha512_ctx *const ctx)
{
	uint64_t cnt = (uint64_t)ctx->count * CHAR_BIT;
	size_t i = ctx->count & (SHA512_BLOCK_SIZE - 1);

	/**
	 * Append the bit '1' to the message which would be 0x80 if message
	 * length is a multiple of 8 bits.
	 */
	ctx->b8[i++] = 0x80;
	/**
	 * Append 0 ≤ k < 1024 bits '0', such that the resulting message
	 * length in bits is congruent to −64 ≡ 896 (mod 1024).
	 */
	if (i > (SHA512_BLOCK_SIZE - 2 * sizeof(cnt))) {
		/* Current block won't fit length, so move to next. */
		while (i < SHA512_BLOCK_SIZE)
			ctx->b8[i++] = 0;
		SHA512_transform(ctx);
		i = 0;
	}
	/* pad rest of zeros, including 8 bytes of 128-bit counter */
	while (i < (SHA512_BLOCK_SIZE - sizeof(cnt)))
		ctx->b8[i++] = 0;
	/* place big-endian 64-bit bit counter at the end of block */
	ctx->b64[SHA512_BLOCK_DWORDS - 1] = htobe64(cnt);
	SHA512_transform(ctx);
	for (i = 0; i < 8; i++)
		ctx->b64[i] = htobe64(ctx->state[i]);
	return &ctx->digest;
}

const struct sha512_digest *SHA512_sw_hash(const void *data, size_t len,
			      struct sha512_digest *digest)
{
	struct sha512_ctx ctx;

	SHA512_sw_init(&ctx);
	SHA512_sw_update(&ctx, data, len);
	memcpy(digest->b8, SHA512_sw_final(&ctx)->b8, SHA512_DIGEST_SIZE);
	return digest;
}

void SHA384_sw_init(struct sha384_ctx *ctx)
{
	/* SHA2-384 differs from SHA2-512 only in initialization. */
	static const struct hash_vtable sha384_vtable = {
		SHA384_init_as_hash,	  SHA512_update_as_hash,
		SHA512_final_as_hash,	  HMAC_sw_final,
		SHA384_DIGEST_SIZE,	  SHA384_BLOCK_SIZE,
		sizeof(struct sha384_ctx)
	};
	static const uint64_t sha384_init[SHA512_DIGEST_DWORDS] = {
		0xcbbb9d5dc1059ed8ll, 0x629a292a367cd507ll,
		0x9159015a3070dd17ll, 0x152fecd8f70e5939ll,
		0x67332667ffc00b31ll, 0x8eb44a8768581511ll,
		0xdb0c2e0d64f98fa7ll, 0x47b5481dbefa4fa4ll
	};

	memcpy(ctx->state, sha384_init, sizeof(ctx->state));
	ctx->count = 0;
	ctx->f = &sha384_vtable;
}

const struct sha384_digest *SHA384_sw_hash(const void *data, size_t len,
			      struct sha384_digest *digest)
{
	struct sha384_ctx ctx;

	SHA384_sw_init(&ctx);
	SHA384_sw_update(&ctx, data, len);
	memcpy(digest->b8, SHA384_sw_final(&ctx)->b8, SHA384_DIGEST_SIZE);
	return digest;
}

#if defined(CONFIG_SHA512_HW_EQ_SW) || !defined(CONFIG_DCRYPTO_SHA512)
/**
 * We don't support HW-accelerated SHA384/SHA512 yet, so alias it to software.
 */
const struct sha512_digest *SHA512_hw_hash(const void *data, size_t len,
					   struct sha512_digest *digest)
	__alias(SHA512_sw_hash);


void SHA512_hw_init(struct sha512_ctx *const ctx) __alias(SHA512_sw_init);

void SHA512_hw_update(struct sha512_ctx *const ctx, const void *data,
		      size_t len) __alias(SHA512_sw_update);

const struct sha512_digest *SHA512_hw_final(struct sha512_ctx *const ctx)
	__alias(SHA512_sw_final);


const struct sha384_digest *SHA384_hw_hash(const void *data, size_t len,
					   struct sha384_digest *digest)
	__alias(SHA384_sw_hash);

void SHA384_hw_init(struct sha384_ctx *const ctx) __alias(SHA384_sw_init);

void SHA384_hw_update(struct sha384_ctx *const ctx, const void *data,
		      size_t len) __alias(SHA384_sw_update);

const struct sha384_digest *SHA384_hw_final(struct sha384_ctx *const ctx)
	__alias(SHA384_sw_final);

#endif
