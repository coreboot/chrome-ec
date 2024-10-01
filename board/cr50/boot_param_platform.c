/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <boot_param_platform.h>

#include "console.h"
#include "internal.h"

#define VERBOSE_BOOT_PARAM

#ifdef VERBOSE_BOOT_PARAM
#define verbose_log(s) __platform_log_str(s)
#else
#define verbose_log(s)
#endif

#define MAX_ECDSA_KEYGEN_ATTEMPTS 16

/* Static ECDSA key seed. Only one key is allowed to be used at any time. */
static uint8_t g_key_seed[P256_NBYTES] = { 0 };
static int g_key_seed_valid = { 0 };

static inline void invalidate_g_key_seed(void)
{
	memset(g_key_seed, 0, P256_NBYTES);
	g_key_seed_valid = 0;
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
	int res = DCRYPTO_hkdf(result.data, result.size,
		salt.data, salt.size,
		ikm.data, ikm.size,
		info.data, info.size);
	return res != 0;
}

/* Calculate SHA256 for the provided buffer */
bool __platform_sha256(
	/* [IN] data to hash */
	const struct slice_ref_s data,
	/* [OUT] resulting digest */
	uint8_t digest[DIGEST_BYTES]
)
{
	/* SHA256_hw_hash produces big-endian hash */
	SHA256_hw_hash(data.data, data.size, (struct sha256_digest *)digest);

	return true;
}

/* Get DICE config */
bool __platform_get_dice_config(
	/* [OUT] DICE config */
	struct dice_config_s *cfg
)
{
	/* TODO: get all configuration variables */
	return false;
}

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
	/* TODO: get all configuration variables */
	return false;
}

/*
 * This function is used Generate ECDSA P-256 key from the DRBG output.
 *
 * We can't just call DCRYPTO_p256_key_from_bytes() on the unadjusted
 * DRBG bytes because it adds 1 to the provided seed. And we need to
 * follow an exact keygen sequence (which doesn't add 1) so that AP FW
 * can generate the same CDI key from CDI.
 *
 * We can't change DCRYPTO_p256_key_from_bytes() - it is used for EK,
 * and we can't add a new function to dcrypto - it would change the u2f
 * certified binary. So, we have to create a custom function here.
 *
 * Instead of recreating the steps of DCRYPTO_p256_key_from_bytes() locally,
 * this custom function subtracts 1 from the seed before passing it to
 * DCRYPTO_p256_key_from_bytes(). Adding a big-endian subtraction cycle
 * should result in smaller code than copy-pasting the entire logic.
 *
 * We don't need to check that the original value is not 0 because
 * DCRYPTO_p256_key_from_bytes() will fail if it sees all 0xFF.
 *
 * Having an adjust_drbg_bytes() step after hmac_drbg_generate()
 * is chosen over wrapping DCRYPTO_p256_key_from_bytes() in a
 * custom function because DCRYPTO_p256_key_from_bytes() is called
 * from multiple places: __platform_ecdsa_p256_keygen_hmac_drbg,
 * __platform_ecdsa_p256_sign, and __platform_ecdsa_p256_get_pub_key.
 * Doing the sub-1 correction in all of them would require making a local
 * copy of g_key_seed (since otherwise we'd be decrementing it on every
 * use). Having an adjust_drbg_bytes step in one place allows us to
 * avoid this extra copy.
 */
static inline void adjust_drbg_bytes(
	/* [IN/OUT] bytes from DRBG, used as a big-endian */
	uint8_t key_bytes[P256_NBYTES]
)
{
	int i;

	for (i = P256_NBYTES - 1; i >= 0; i--) {
		if (key_bytes[i] == 0) {
			key_bytes[i] = 0xFF;
		} else {
			key_bytes[i]--;
			return;
		}
	}
}


/* Generate ECDSA P-256 key using HMAC-DRBG initialized by the seed */
bool __platform_ecdsa_p256_keygen_hmac_drbg(
	/* [IN] key seed */
	const uint8_t seed[DIGEST_BYTES],
	/* [OUT] ECDSA key handle */
	const void **key
)
{
	p256_int d;
	struct drbg_ctx drbg;
	enum dcrypto_result result = DCRYPTO_FAIL;
	size_t attempt = 0;

	*key = NULL;

	hmac_drbg_init(&drbg, seed, DIGEST_BYTES,
		NULL, 0, NULL, 0,
		HMAC_DRBG_DO_NOT_AUTO_RESEED);

	g_key_seed_valid = 0;
	do {
		result = hmac_drbg_generate(&drbg,
			g_key_seed, sizeof(g_key_seed),
			NULL, 0);
		if (result != DCRYPTO_OK) {
			verbose_log("hmac_drbg_generate failed");
			drbg_exit(&drbg);
			invalidate_g_key_seed();
			return false;
		}
		/* See the description of adjust_drbg_bytes() above */
		adjust_drbg_bytes(g_key_seed);
		result = DCRYPTO_p256_key_from_bytes(NULL, NULL,
			&d, g_key_seed);
		attempt++;
	} while (result == DCRYPTO_RETRY &&
		 attempt < MAX_ECDSA_KEYGEN_ATTEMPTS);
	drbg_exit(&drbg);
	if (result != DCRYPTO_OK) {
		verbose_log("DCRYPTO_p256_key_from_bytes failed");
		invalidate_g_key_seed();
		return false;
	}

	g_key_seed_valid = 1;
	*key = g_key_seed;
	return true;
}

static bool get_ecdsa_points(
	/* [IN] ECDSA key handle */
	const void *key,
	/* [OUT] x as p256_int, can pass NULL if not required */
	p256_int *x,
	/* [OUT] y as p256_int, can pass NULL if not required */
	p256_int *y,
	/* [OUT] d as p256_int, must be non-NULL */
	p256_int *d
)
{
	enum dcrypto_result result = DCRYPTO_FAIL;

	if (key != g_key_seed || !g_key_seed_valid) {
		verbose_log("__platform_ecdsa_p256_get_pub_key: invalid key");
		return false;
	}
	result = DCRYPTO_p256_key_from_bytes(x, y, d, g_key_seed);
	if (result != DCRYPTO_OK) {
		verbose_log("DCRYPTO_p256_key_from_bytes failed");
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
	struct sha256_digest digest;
	p256_int d, h, r, s;
	struct drbg_ctx ctx;
	enum dcrypto_result result = DCRYPTO_FAIL;

	if (!get_ecdsa_points(key, NULL, NULL, &d))
		return false;

	/* SHA256_hw_hash produces big-endian hash */
	SHA256_hw_hash(data.data, data.size, &digest);
	p256_from_bin(digest.b8, &h);

	hmac_drbg_init_rfc6979(&ctx, &d, &h);
	result = dcrypto_p256_ecdsa_sign(&ctx, &d, &h, &r, &s);
	drbg_exit(&ctx);

	if (result != DCRYPTO_OK) {
		verbose_log("dcrypto_p256_ecdsa_sign failed");
		return false;
	}

	p256_to_bin(&r, signature);
	p256_to_bin(&s, signature + ECDSA_POINT_BYTES);

	return false;
}

/* Get ECDSA public key X, Y */
bool __platform_ecdsa_p256_get_pub_key(
	/* [IN] ECDSA key handle */
	const void *key,
	/* [OUT] public key structure */
	struct ecdsa_public_s *pub_key
)
{
	p256_int x, y, d;

	if (!get_ecdsa_points(key, &x, &y, &d))
		return false;

	p256_to_bin(&x, pub_key->x);
	p256_to_bin(&y, pub_key->y);

	return true;
}

/* Free ECDSA key handle */
void __platform_ecdsa_p256_free(
	/* [IN] ECDSA key handle */
	const void *key
)
{
	invalidate_g_key_seed();
}

/* Check if APROV status allows making 'normal' boot mode decision */
bool __platform_aprov_status_allows_normal(
	/* [IN] APROV status */
	uint32_t aprov_status
)
{
	/*
	 * For Cr50 ARPOV status is ignored,
	 * so this method always returns true.
	 */
	return true;
}

/* Print error string to log */
void __platform_log_str(
	/* [IN] string to print */
	const char *str
)
{
	ccprintf("%s\n", str);
	cflush();
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
