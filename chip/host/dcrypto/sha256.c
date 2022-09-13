/* Copyright 2018 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "internal.h"

void SHA1_hw_init(struct sha1_ctx *ctx)
{
	SHA1_sw_init(ctx);
}

const struct sha1_digest *SHA1_hw_hash(const void *data, size_t n,
					   struct sha1_digest *digest)
{
	return SHA1_sw_hash(data, n, digest);
}

void SHA256_hw_init(struct sha256_ctx *ctx)
{
	SHA256_sw_init(ctx);
}

const struct sha256_digest *SHA256_hw_hash(const void *data, size_t n,
					   struct sha256_digest *digest)
{
	SHA256_sw_hash(data, n, digest);
	return digest;
}

void HMAC_SHA256_hw_init(struct hmac_sha256_ctx *ctx, const void *key,
			      size_t len)
{
	SHA256_hw_init(&ctx->hash);
	HMAC_sw_init((union hmac_ctx *)ctx, key, len);
}

const struct sha256_digest *HMAC_SHA256_hw_final(struct hmac_sha256_ctx *ctx)
{
	return HMAC_SHA256_final(ctx);
}
