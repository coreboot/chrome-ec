/* Copyright 2021 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "internal.h"

size_t DCRYPTO_hash_size(enum hashing_mode mode)
{
	if (!fips_crypto_allowed())
		return 0;

	switch (mode) {
	case HASH_SHA1:
		return SHA1_DIGEST_SIZE;
	case HASH_SHA256:
		return SHA256_DIGEST_SIZE;
#ifdef CONFIG_UPTO_SHA512
	case HASH_SHA384:
		return SHA384_DIGEST_SIZE;
	case HASH_SHA512:
		return SHA512_DIGEST_SIZE;
#endif
	default:
		return 0;
	}
	return 0;
}

enum dcrypto_result DCRYPTO_sw_hash_init(union hash_ctx *ctx,
					 enum hashing_mode mode)
{
	if (!fips_crypto_allowed())
		return DCRYPTO_FAIL;

	switch (mode) {
	case HASH_SHA1:
		SHA1_sw_init(&ctx->sha1);
		break;
	case HASH_SHA256:
		SHA256_sw_init(&ctx->sha256);
		break;
#ifdef CONFIG_UPTO_SHA512
	case HASH_SHA384:
		SHA384_sw_init(&ctx->sha384);
		break;
	case HASH_SHA512:
		SHA512_sw_init(&ctx->sha512);
		break;
#endif
	default:
		return DCRYPTO_FAIL;
	}
	return DCRYPTO_OK;
}

enum dcrypto_result DCRYPTO_hw_hash_init(union hash_ctx *ctx,
					 enum hashing_mode mode)
{
	if (!fips_crypto_allowed())
		return DCRYPTO_FAIL;

	switch (mode) {
	case HASH_SHA1:
		SHA1_hw_init(&ctx->sha1);
		break;
	case HASH_SHA256:
		SHA256_hw_init(&ctx->sha256);
		break;
#ifdef CONFIG_UPTO_SHA512
	case HASH_SHA384:
		SHA384_hw_init(&ctx->sha384);
		break;
	case HASH_SHA512:
		SHA512_hw_init(&ctx->sha512);
		break;
#endif
	default:
		return DCRYPTO_FAIL;
	}
	return DCRYPTO_OK;
}

enum dcrypto_result DCRYPTO_sw_hmac_init(union hmac_ctx *ctx, const void *key,
					 size_t len, enum hashing_mode mode)
{
	if (!fips_crypto_allowed())
		return DCRYPTO_FAIL;

	switch (mode) {
	case HASH_SHA1:
		SHA1_sw_init(&ctx->hmac_sha1.hash);
		break;
	case HASH_SHA256:
		SHA256_sw_init(&ctx->hmac_sha256.hash);
		break;
#ifdef CONFIG_UPTO_SHA512
	case HASH_SHA384:
		SHA384_sw_init(&ctx->hmac_sha384.hash);
		break;
	case HASH_SHA512:
		SHA512_sw_init(&ctx->hmac_sha512.hash);
		break;
#endif
	default:
		return DCRYPTO_FAIL;
	}
	HMAC_sw_init(ctx, key, len);
	return DCRYPTO_OK;
}

enum dcrypto_result DCRYPTO_hw_hmac_init(union hmac_ctx *ctx, const void *key,
					 size_t len, enum hashing_mode mode)
{
	if (!fips_crypto_allowed())
		return DCRYPTO_FAIL;

	switch (mode) {
	case HASH_SHA1:
		SHA1_hw_init(&ctx->hmac_sha1.hash);
		HMAC_sw_init(ctx, key, len);
		break;
	case HASH_SHA256:
		HMAC_SHA256_hw_init(&ctx->hmac_sha256, key, len);
		break;
#ifdef CONFIG_UPTO_SHA512
	case HASH_SHA384:
		SHA384_hw_init(&ctx->hmac_sha384.hash);
		HMAC_sw_init(ctx, key, len);
		break;
	case HASH_SHA512:
		SHA512_hw_init(&ctx->hmac_sha512.hash);
		HMAC_sw_init(ctx, key, len);
		break;
#endif
	default:
		return DCRYPTO_FAIL;
	}
	return DCRYPTO_OK;
}

const uint8_t *DCRYPTO_SHA1_hash(const void *data, size_t n, uint8_t *digest)
{
	if (!fips_crypto_allowed())
		return NULL;

	if (is_not_aligned(digest)) {
		struct sha1_digest d;

		SHA1_hw_hash(data, n, &d);
		memcpy(digest, d.b8, sizeof(d));
	} else
		SHA1_hw_hash(data, n, (struct sha1_digest *)digest);
	return digest;
}

const uint8_t *DCRYPTO_SHA256_hash(const void *data, size_t n, uint8_t *digest)
{
	if (!fips_crypto_allowed())
		return NULL;

	if (is_not_aligned(digest)) {
		struct sha256_digest d;

		SHA256_hw_hash(data, n, &d);
		memcpy(digest, d.b8, sizeof(d));
	} else
		SHA256_hw_hash(data, n, (struct sha256_digest *)digest);
	return digest;
}
