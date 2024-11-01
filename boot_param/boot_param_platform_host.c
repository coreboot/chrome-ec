/*
 * Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "boot_param_platform.h"

#include <openssl/core_names.h>
#include <openssl/err.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/sha.h>

#include <stdio.h>
#include <string.h>

static void print_openssl_error(const char *func_name, const char *prefix_err)
{
	unsigned long err = ERR_get_error();
	char *err_str = ERR_error_string(err, NULL);

	printf("%s: %s\n", func_name, prefix_err);
	if (err_str)
		__platform_log_str(err_str);
}

/* Perform HKDF-SHA256(ikm, salt, info) */
bool __platform_hkdf_sha256(
	/* [IN] input key material */
	const struct slice_ref_s ikm,
	/* [IN] salt */
	const struct slice_ref_s salt,
	/* [IN] info */
	const struct slice_ref_s info,
	/* [IN/OUT] .size sets length for hkdf,
	 * .data is where the digest will be placed
	 */
	const struct slice_mut_s result
)
{
	EVP_KDF *kdf;
	EVP_KDF_CTX *kctx;
	OSSL_PARAM params[5], *p = params;
	int res;

	kdf = EVP_KDF_fetch(NULL, "HKDF", NULL);
	if (kdf == NULL) {
		print_openssl_error(__func__, "Can't find HKDF");
		return false;
	}
	kctx = EVP_KDF_CTX_new(kdf);
	EVP_KDF_free(kdf);
	if (kctx == NULL) {
		print_openssl_error(__func__, "Can't create HKDF context");
		return false;
	}

	*p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST,
						SN_sha256, strlen(SN_sha256));
	*p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY,
						 (void *)ikm.data, ikm.size);
	*p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO,
						 (void *)info.data, info.size);
	*p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT,
						 (void *)salt.data, salt.size);
	*p = OSSL_PARAM_construct_end();

	res = EVP_KDF_derive(kctx, result.data, result.size, params);
	if (res <= 0)
		print_openssl_error(__func__, "Can't perform HKDF");

	EVP_KDF_CTX_free(kctx);
	return (res > 0);
}

/* Calculate SH256 for the provided buffer */
bool __platform_sha256(
	/* [IN] data to hash */
	const struct slice_ref_s data,
	/* [OUT] resulting digest */
	uint8_t digest[DIGEST_BYTES]
)
{
	return SHA256(data.data, data.size, digest) == digest;
}

struct dice_config_s g_dice_config = {
	.aprov_status = 0x20,
	.sec_ver = 1,
	.uds = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	},
	.hidden_digest = {
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	},
	.code_digest = {
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
		0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
		0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	},
	.pcr0 = {
		0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
		0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
		0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
		0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
	},
	.pcr10 = {
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
		0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
		0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
		0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	},
};

/* Get DICE config */
bool __platform_get_dice_config(
	/* [OUT] DICE config */
	struct dice_config_s *cfg
)
{
	memcpy(cfg, &g_dice_config, sizeof(struct dice_config_s));
	return true;
}

uint8_t g_early_entropy[EARLY_ENTROPY_BYTES] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
};

uint8_t g_session_key_seed[KEY_SEED_BYTES] = {
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
};

uint8_t g_auth_token_key_seed[KEY_SEED_BYTES] = {
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
};

/* Get GSC boot parameters */
bool __platform_get_gsc_boot_param(
	/* [OUT] early entropy */
	uint8_t early_entropy[EARLY_ENTROPY_BYTES],
	/* [OUT] SessionKeySeed */
	uint8_t session_key_seed[KEY_SEED_BYTES],
	/* [OUT] AuthTokenKeySeed */
	uint8_t auth_token_key_seed[KEY_SEED_BYTES]
)
{
	memcpy(early_entropy, g_early_entropy, EARLY_ENTROPY_BYTES);
	memcpy(session_key_seed, g_session_key_seed, KEY_SEED_BYTES);
	memcpy(auth_token_key_seed, g_auth_token_key_seed, KEY_SEED_BYTES);
	return true;
}

/* Generate ECDSA P-256 key using HMAC-DRBG initialized by the seed */
bool __platform_ecdsa_p256_keygen_hmac_drbg(
	/* [IN] key seed */
	const uint8_t seed[DIGEST_BYTES],
	/* [OUT] ECDSA key handle */
	const void **key
)
{
	/* NOTE: for testing we don't do the actual KDF based on HMAC_DRBG.
	 * We could use EVP_KDF-HMAC-DRBG or EVP_RAND-HMAC-DRBG, but there's
	 * no easy way to generate EC key from a seed we'd get from DRBG.
	 * Instead, we just generate a random EC key ignoring the seed.
	 */
	*key = EVP_EC_gen("P-256");

	if (*key == NULL) {
		print_openssl_error(__func__, "Can't generate EC");
		return false;
	}
	return true;
}

/* Generate ECDSA P-256 signature: 64 bytes (R | S) */
bool __platform_ecdsa_p256_sign(
	/* [IN] ECDSA key handle */
	const void *key,
	/* [IN] data to sign */
	const struct slice_ref_s data,
	/* [OUT] resulting signature */
	uint8_t signature[ECDSA_SIG_BYTES]
)
{
	const BIGNUM *r;
	const BIGNUM *s;
	EVP_PKEY_CTX *ctx;
	EVP_SIGNATURE *alg;
	uint8_t digest[DIGEST_BYTES];
	uint8_t output[128];
	int res;
	size_t siglen;
	ECDSA_SIG *sig_obj;
	const uint8_t *sig_ptr = output;

	if (!__platform_sha256(data, digest)) {
		printf("%s: SHA failed\n", __func__);
		return false;
	}

	ctx = EVP_PKEY_CTX_new((EVP_PKEY *)key, NULL /* no engine */);
	if (ctx == NULL) {
		print_openssl_error(__func__, "Can't create context");
		return false;
	}
	if (EVP_PKEY_sign_init(ctx) <= 0) {
		print_openssl_error(__func__, "Can't init sign");
		EVP_PKEY_CTX_free(ctx);
		return false;
	}

	alg = EVP_SIGNATURE_fetch(NULL, "ECDSA", NULL);
	if (alg == NULL) {
		print_openssl_error(__func__, "Can't fetch signature alg");
		EVP_PKEY_CTX_free(ctx);
		return false;
	}

	res = EVP_PKEY_sign(ctx, NULL, &siglen, digest, DIGEST_BYTES);
	if (res <= 0)
		print_openssl_error(__func__, "Can't detect signature size");

	if (siglen <= sizeof(output)) {
		res = EVP_PKEY_sign(ctx, output, &siglen, digest, DIGEST_BYTES);
		if (res <= 0)
			print_openssl_error(__func__, "Can't sign");
	} else {
		printf("%s: unexpected sigsize %zu > %zu\n",
		       __func__, siglen, sizeof(output));
		res = 0;
	}

	EVP_PKEY_CTX_free(ctx);
	EVP_SIGNATURE_free(alg);

	if (res <= 0)
		return false;

	sig_obj = d2i_ECDSA_SIG(NULL, &sig_ptr, siglen);
	if (sig_obj == NULL) {
		print_openssl_error(__func__, "Can't convert signature");
		return false;
	}

	r = ECDSA_SIG_get0_r(sig_obj);
	s = ECDSA_SIG_get0_s(sig_obj);

	if (BN_num_bytes(r) > ECDSA_POINT_BYTES) {
		printf("%s: unexpected r size %d > %u\n",
		       __func__, BN_num_bytes(r), ECDSA_POINT_BYTES);
		ECDSA_SIG_free(sig_obj);
		return false;
	}
	if (BN_num_bytes(s) > ECDSA_POINT_BYTES) {
		printf("%s: unexpected s size %d > %u\n",
		       __func__, BN_num_bytes(s), ECDSA_POINT_BYTES);
		ECDSA_SIG_free(sig_obj);
		return false;
	}
	memset(signature, 0, ECDSA_SIG_BYTES);
	BN_bn2bin(r, signature + (ECDSA_POINT_BYTES - BN_num_bytes(r)));
	BN_bn2bin(s, signature + (ECDSA_SIG_BYTES - BN_num_bytes(s)));

	ECDSA_SIG_free(sig_obj);
	return true;
}

/* Get ECDSA public key X, Y */
bool __platform_ecdsa_p256_get_pub_key(
	/* [IN] ECDSA key handle */
	const void *key,
	/* [OUT] public key structure */
	struct ecdsa_public_s *pub_key
)
{
	BIGNUM *x = NULL;
	BIGNUM *y = NULL;
	bool res = true;

	if (!EVP_PKEY_get_bn_param(key, OSSL_PKEY_PARAM_EC_PUB_X, &x)) {
		print_openssl_error(__func__, "Can't get X");
		res = false;
	}

	if (!EVP_PKEY_get_bn_param(key, OSSL_PKEY_PARAM_EC_PUB_Y, &y)) {
		print_openssl_error(__func__, "Can't get Y");
		res = false;
	}

	if (res && (BN_num_bytes(x) > ECDSA_POINT_BYTES)) {
		printf("%s: unexpected X size %d > %u\n",
		       __func__, BN_num_bytes(x), ECDSA_POINT_BYTES);
		res = false;
	}
	if (res && (BN_num_bytes(y) > ECDSA_POINT_BYTES)) {
		printf("%s: unexpected Y size %d > %u\n",
		       __func__, BN_num_bytes(y), ECDSA_POINT_BYTES);
		res = false;
	}

	if (res) {
		memset(pub_key->x, 0, ECDSA_POINT_BYTES);
		BN_bn2bin(x,
			pub_key->x + (ECDSA_POINT_BYTES - BN_num_bytes(x)));
		memset(pub_key->y, 0, ECDSA_POINT_BYTES);
		BN_bn2bin(y,
			pub_key->y + (ECDSA_POINT_BYTES - BN_num_bytes(y)));
	}

	BN_free(x);
	BN_free(y);
	return res;
}

/* Free ECDSA key handle */
void __platform_ecdsa_p256_free(
	/* [IN] ECDSA key handle */
	const void *key
)
{
	EVP_PKEY_free((EVP_PKEY *)key);
}

/* Check if APROV status allows making 'normal' boot mode decision */
bool __platform_aprov_status_allows_normal(
	/* [IN] APROV status */
	uint32_t aprov_status
)
{
	return true;
}

/* Print error string to log */
void __platform_log_str(
	/* [IN] string to print */
	const char *str
)
{
	puts(str);
}

/* memcpy */
void __platform_memcpy(void *dest, const void *src, size_t size)
{
	memcpy(dest, src, size);
}

/* memset */
void __platform_memset(void *dest, uint8_t fill, size_t size)
{
	memset(dest, fill, size);
}

/* memcmp */
int __platform_memcmp(const void *str1, const void *str2, size_t size)
{
	return memcmp(str1, str2, size);
}
