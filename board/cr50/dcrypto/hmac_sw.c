/* Copyright 2021 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "dcrypto.h"
#include "internal.h"

#include <stdint.h>

/**
 * Generic software HMAC support for any type of hash.
 */

/* Calculate location of storage for ipad/opad in generic way */
static inline uint32_t *HMAC_opad(union hmac_ctx *const ctx)
{
	return (uint32_t *)((uint8_t *)ctx + ctx->f->context_size);
}
/**
 * Initialize HMAC for pre-configured hash function.
 * This is generic function which can initialize HMAC with any supported
 * hash function.
 */
void HMAC_sw_init(union hmac_ctx *const ctx, const void *key, size_t len)
{
	uint32_t *const opad = HMAC_opad(ctx);
	const size_t block_size = HASH_block_size(&ctx->hash);
	size_t i;
	/* inner padding with zeros */
	memset(opad, 0, block_size);
	/**
	 * HMAC (K, m) = H( (K' ⊕ opad) || H ((K' ⊕ ipad) || m) )
	 * K' = H(K) if K is longer than block size for H, or
	 *    = K padded with zeroes otherwise
	 */
	if (len > block_size) {
		/* ctx already contains proper vtable for hash functions */
		/* But we need to reinit it after use. */
		HASH_update(&ctx->hash, key, len);
		memcpy(opad, HASH_final(&ctx->hash), HASH_size(&ctx->hash));
		HASH_reinit(&ctx->hash);
	} else {
		memcpy(opad, key, len);
	}
	/* inner pad is K ⊕ 0x36, computed at word level */
	for (i = 0; i < block_size / sizeof(opad[0]); ++i)
		opad[i] ^= 0x36363636;

	HASH_update(&ctx->hash, opad, block_size); /* hash ipad */
	/* compute outer padding from the inner. */
	for (i = 0; i < block_size / sizeof(opad[0]); ++i)
		opad[i] ^= (0x36363636 ^ 0x5c5c5c5c);
}

const union sha_digests *HMAC_sw_final(union hmac_ctx *const ctx)
{
	uint32_t *const opad = HMAC_opad(ctx);
	uint32_t *digest; /* storage for intermediate digest */
	const size_t block_size = HASH_block_size(&ctx->hash);
	const size_t hash_size = HASH_size(&ctx->hash);
	/* allocate storage dynamically, just enough for particular hash. */
	digest = alloca(hash_size);
	memcpy(digest, HASH_final(&ctx->hash), hash_size);
	HASH_reinit(&ctx->hash);
	HASH_update(&ctx->hash, opad, block_size);
	HASH_update(&ctx->hash, digest, hash_size);
	memset(opad, 0, block_size); /* wipe key */
	return HASH_final(&ctx->hash);
}
