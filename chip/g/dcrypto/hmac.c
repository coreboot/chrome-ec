/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "internal.h"
#include "dcrypto.h"

#include <stdint.h>

#include "cryptoc/sha256.h"
#include "cryptoc/util.h"

/* TODO(sukhomlinov): add support for hardware hmac. */
static void hmac_sha256_init(LITE_HMAC_CTX *ctx, const void *key,
			     unsigned int len)
{
	unsigned int i;

	BUILD_ASSERT(sizeof(ctx->opad) >= SHA256_BLOCK_SIZE);

	memset(&ctx->opad[0], 0, SHA256_BLOCK_SIZE);

	if (len > sizeof(ctx->opad)) {
		DCRYPTO_SHA256_init(&ctx->hash, 0);
		HASH_update(&ctx->hash, key, len);
		memcpy(&ctx->opad[0], HASH_final(&ctx->hash),
		       HASH_size(&ctx->hash));
	} else {
		memcpy(&ctx->opad[0], key, len);
	}

	for (i = 0; i < SHA256_BLOCK_SIZE; ++i)
		ctx->opad[i] ^= 0x36;

	DCRYPTO_SHA256_init(&ctx->hash, 0);
	/* hash ipad */
	HASH_update(&ctx->hash, ctx->opad, SHA256_BLOCK_SIZE);

	for (i = 0; i < SHA256_BLOCK_SIZE; ++i)
		ctx->opad[i] ^= (0x36 ^ 0x5c);
}

void DCRYPTO_HMAC_SHA256_init(LITE_HMAC_CTX *ctx, const void *key,
			      unsigned int len)
{
	hmac_sha256_init(ctx, key, len);
}

const uint8_t *DCRYPTO_HMAC_final(LITE_HMAC_CTX *ctx)
{
	uint8_t digest[SHA256_DIGEST_SIZE]; /* up to SHA256 */

	memcpy(digest, HASH_final(&ctx->hash),
	       (HASH_size(&ctx->hash) <= sizeof(digest) ?
			HASH_size(&ctx->hash) :
			sizeof(digest)));
	DCRYPTO_SHA256_init(&ctx->hash, 0);
	HASH_update(&ctx->hash, ctx->opad, SHA256_BLOCK_SIZE);
	HASH_update(&ctx->hash, digest, HASH_size(&ctx->hash));
	always_memset(&ctx->opad[0], 0, SHA256_BLOCK_SIZE); /* wipe key */
	return HASH_final(&ctx->hash);
}
