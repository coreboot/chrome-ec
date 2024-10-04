/*
 * Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __GSC_UTILS_BOOT_PARAM_PLATFORM_H
#define __GSC_UTILS_BOOT_PARAM_PLATFORM_H

#include "dice_types.h"

#ifdef __cplusplus
extern "C" {
#endif

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
);

/* Calculate SH256 for the provided buffer */
bool __platform_sha256(
	/* [IN] data to hash */
	const struct slice_ref_s data,
	/* [OUT] resulting digest */
	uint8_t digest[DIGEST_BYTES]
);

/* Get DICE config */
bool __platform_get_dice_config(
	/* [OUT] DICE config */
	struct dice_config_s *cfg
);

/* Generate ECDSA P-256 key using HMAC-DRBG initialized by the seed */
bool __platform_ecdsa_p256_keygen_hmac_drbg(
	/* [IN] key seed */
	const uint8_t seed[DIGEST_BYTES],
	/* [OUT] ECDSA key handle */
	const void **key
);

/* Generate ECDSA P-256 signature: 64 bytes (R | S) */
bool __platform_ecdsa_p256_sign(
	/* [IN] ECDSA key handle */
	const void *key,
	/* [IN] data to sign */
	const struct slice_ref_s data,
	/* [OUT] resulting signature */
	uint8_t signature[ECDSA_SIG_BYTES]
);

/* Get ECDSA public key X, Y */
bool __platform_ecdsa_p256_get_pub_key(
	/* [IN] ECDSA key handle */
	const void *key,
	/* [OUT] public key structure */
	struct ecdsa_public_s *pub_key
);

/* Free ECDSA key handle */
void __platform_ecdsa_p256_free(
	/* [IN] ECDSA key handle */
	const void *key
);

/* Check if APROV status allows making 'normal' boot mode decision */
bool __platform_aprov_status_allows_normal(
	/* [IN] APROV status */
	uint32_t aprov_status
);

/* Print error string to log */
void __platform_log_str(
	/* [IN] string to print */
	const char *str
);

/* memcpy */
void __platform_memcpy(void *dest, const void *src, size_t size);

/* memset */
void __platform_memset(void *dest, uint8_t fill, size_t size);

/* memcmp */
int __platform_memcmp(const void *str1, const void *str2, size_t size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __GSC_UTILS_BOOT_PARAM_PLATFORM_H */
