/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Provides the minimal declarations needed by pinweaver and u2f to build on
 * CHIP_HOST. While it might be preferable to simply use the original dcrypto.h,
 * That would require incorporating additional headers / dependencies such as
 * cryptoc.
 */

#ifndef __CROS_EC_DCRYPTO_HOST_H
#define __CROS_EC_DCRYPTO_HOST_H
#include <stdint.h>
#include <string.h>

/* Allow tests to return a faked result for the purpose of testing. If
 * this is not set, a combination of cryptoc and openssl are used for the
 * dcrypto implementation.
 */
#ifndef CONFIG_DCRYPTO_MOCK

/* If not using the mock struct definitions, use the ones from Cr50. */
#include "board/cr50/dcrypto/dcrypto.h"

#else  /* defined(CONFIG_DCRYPTO_MOCK) */

#include "board/cr50/dcrypto/hmacsha2.h"

#define AES256_BLOCK_CIPHER_KEY_SIZE 32

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

void SHA256_hw_init(struct sha256_ctx *ctx);

void HMAC_SHA256_hw_init(struct hmac_sha256_ctx *ctx, const void *key,
			 size_t len);
const struct sha256_digest *HMAC_SHA256_hw_final(struct hmac_sha256_ctx *ctx);

int DCRYPTO_aes_ctr(uint8_t *out, const uint8_t *key, uint32_t key_bits,
		    const uint8_t *iv, const uint8_t *in, size_t in_len);


int DCRYPTO_appkey_init(enum dcrypto_appid appid);

void DCRYPTO_appkey_finish(void);

int DCRYPTO_appkey_derive(enum dcrypto_appid appid, const uint32_t input[8],
			  uint32_t output[8]);

#include "cryptoc/p256.h"

int DCRYPTO_x509_gen_u2f_cert_name(const p256_int *d, const p256_int *pk_x,
				   const p256_int *pk_y, const p256_int *serial,
				   const char *name, uint8_t *cert,
				   const int n);

int DCRYPTO_ladder_random(void *output);

#define SHA256_DIGEST_WORDS (SHA256_DIGEST_SIZE / sizeof(uint32_t))

struct drbg_ctx {
	uint32_t k[SHA256_DIGEST_WORDS];
	uint32_t v[SHA256_DIGEST_WORDS];
	uint32_t reseed_counter;
};

int dcrypto_p256_ecdsa_sign(struct drbg_ctx *drbg, const p256_int *key,
			    const p256_int *message, p256_int *r, p256_int *s);

void hmac_drbg_init_rfc6979(struct drbg_ctx *ctx, const p256_int *key,
			    const p256_int *message);

bool fips_rand_bytes(void *buffer, size_t len);

#endif  /* CONFIG_DCRYPTO_MOCK */

#endif  /* __CROS_EC_HOST_DCRYPTO_H */
