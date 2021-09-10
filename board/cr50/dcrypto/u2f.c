/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#if defined(CRYPTO_TEST_SETUP) || defined(CR50_DEV)
#include "console.h"
#endif

#include "dcrypto.h"
#include "fips_rand.h"

#include "u2f_cmds.h"
#include "u2f_impl.h"
#include "util.h"

enum ec_error_list u2f_generate_hmac_key(struct u2f_state *state)
{
	/* HMAC key for key handle. */
	if (!fips_rand_bytes(state->hmac_key, sizeof(state->hmac_key)))
		return EC_ERROR_HW_INTERNAL;
	return EC_SUCCESS;
}

enum ec_error_list u2f_generate_drbg_entropy(struct u2f_state *state)
{
	state->drbg_entropy_size = 0;
	/* Get U2F entropy from health-checked TRNG. */
	if (!fips_trng_bytes(state->drbg_entropy, sizeof(state->drbg_entropy)))
		return EC_ERROR_HW_INTERNAL;
	state->drbg_entropy_size = sizeof(state->drbg_entropy);
	return EC_SUCCESS;
}

enum ec_error_list u2f_generate_g2f_secret(struct u2f_state *state)
{
	/* G2F specific path. */
	if (!fips_rand_bytes(state->salt, sizeof(state->salt)))
		return EC_ERROR_HW_INTERNAL;
	return EC_SUCCESS;
}

/* Compute Key handle's HMAC. */
static void u2f_origin_user_mac(const struct u2f_state *state,
				const uint8_t *user, const uint8_t *origin,
				const uint8_t *origin_seed, uint8_t kh_version,
				uint8_t *kh_hmac)
{
	struct hmac_sha256_ctx ctx;

	/* HMAC(u2f_hmac_key, origin || user || origin seed || version) */

	HMAC_SHA256_hw_init(&ctx, state->hmac_key, SHA256_DIGEST_SIZE);
	HMAC_SHA256_update(&ctx, origin, U2F_APPID_SIZE);
	HMAC_SHA256_update(&ctx, user, U2F_USER_SECRET_SIZE);
	HMAC_SHA256_update(&ctx, origin_seed, U2F_ORIGIN_SEED_SIZE);
	if (kh_version != 0)
		HMAC_SHA256_update(&ctx, &kh_version, sizeof(kh_version));
#ifdef CR50_DEV_U2F_VERBOSE
	ccprintf("origin %ph\n", HEX_BUF(origin, U2F_APPID_SIZE));
	ccprintf("user %ph\n", HEX_BUF(user, U2F_USER_SECRET_SIZE));
	ccprintf("origin_seed %ph\n",
		 HEX_BUF(origin_seed, U2F_ORIGIN_SEED_SIZE));
	cflush();
#endif
	memcpy(kh_hmac, HMAC_SHA256_final(&ctx), SHA256_DIGEST_SIZE);
#ifdef CR50_DEV_U2F_VERBOSE
	ccprintf("kh_hmac %ph\n", HEX_BUF(kh_hmac, SHA256_DIGEST_SIZE));
	cflush();
#endif
}

static void u2f_authorization_mac(const struct u2f_state *state,
				  const union u2f_key_handle_variant *kh,
				  uint8_t kh_version,
				  const uint8_t *auth_time_secret_hash,
				  uint8_t *kh_auth_mac)
{
	struct hmac_sha256_ctx ctx;
	const uint8_t *auth_salt = NULL;
	const void *kh_header = NULL;
	size_t kh_header_size = 0;

	if (kh_version == 0) {
		memset(kh_auth_mac, 0xff, SHA256_DIGEST_SIZE);
		return;
	}
	/* At some point we may have v2 key handle, so prepare for it. */
	if (kh_version == 1) {
		auth_salt = kh->v1.authorization_salt;
		kh_header = &kh->v1;
		kh_header_size = U2F_V1_KH_HEADER_SIZE;
	}

	/**
	 * HMAC(u2f_hmac_key, auth_salt || key_handle_header
	 *                              || authTimeSecret)
	 */
	HMAC_SHA256_hw_init(&ctx, state->hmac_key, SHA256_DIGEST_SIZE);
	HMAC_SHA256_update(&ctx, auth_salt, U2F_AUTHORIZATION_SALT_SIZE);
	HMAC_SHA256_update(&ctx, kh_header, kh_header_size);

	HMAC_SHA256_update(&ctx, auth_time_secret_hash,
			   U2F_AUTH_TIME_SECRET_SIZE);

	memcpy(kh_auth_mac, HMAC_SHA256_final(&ctx), SHA256_DIGEST_SIZE);
}

static int app_hw_device_id(enum dcrypto_appid appid, const uint32_t input[8],
			    uint32_t output[8])
{
	struct APPKEY_CTX ctx;
	int result;

	/**
	 * Setup USR-based application key. This loads (if not already done)
	 * application-specific DeviceID.
	 * Internally it computes:
	 * HMAC(hw_device_id, SHA256(name[appid])), but we don't care about
	 * process.
	 * Important property:
	 *          For same appid it will load same value.
	 */
	if (!DCRYPTO_appkey_init(appid, &ctx))
		return 0;

	/**
	 * Compute HMAC(HMAC(hw_device_id, SHA256(name[appid])), input)
	 * It is not used as a key though, and treated as personalization
	 * string for DRBG.
	 */
	result = DCRYPTO_appkey_derive(appid, input, output);

	DCRYPTO_appkey_finish(&ctx);
	return result;
}

/**
 * Generate an origin and user-specific ECDSA key pair from the specified
 * key handle.
 *
 * If pk_x and pk_y are NULL, public key generation will be skipped.
 *
 * @param state U2F state parameters
 * @param kh key handle
 * @param kh_version key handle version (0 - legacy, 1 - versioned)
 * @param d pointer to ECDSA private key
 * @param pk_x pointer to public key point
 * @param pk_y pointer to public key point
 *
 * @return EC_SUCCESS if a valid key pair was created.
 */
static enum ec_error_list u2f_origin_user_key_pair(
	const struct u2f_state *state, const union u2f_key_handle_variant *kh,
	uint8_t kh_version, p256_int *d, p256_int *pk_x, p256_int *pk_y)
{
	uint32_t dev_salt[P256_NDIGITS];
	uint8_t key_seed[P256_NBYTES];

	struct drbg_ctx drbg;
	size_t key_handle_size = 0;
	uint8_t *key_handle = NULL;

	if (kh_version == 0) {
		key_handle_size = sizeof(struct u2f_key_handle_v0);
		key_handle = (uint8_t *)&kh->v0;
	} else if ((kh_version == 1) && (kh->v1.version == kh_version)) {
		key_handle_size = U2F_V1_KH_HEADER_SIZE;
		key_handle = (uint8_t *)&kh->v1;
	} else {
		return EC_ERROR_INVAL;
	}

	if (!app_hw_device_id(U2F_ORIGIN, state->hmac_key, dev_salt))
		return EC_ERROR_UNKNOWN;

	/* Check that U2F state is valid. */
	if (state->drbg_entropy_size != 64 && state->drbg_entropy_size != 32)
		return EC_ERROR_HW_INTERNAL;

	if (state->drbg_entropy_size == 32) {
		/**
		 * Legacy path, seeding DRBG not as NIST SP 800-90A requires.
		 */
		hmac_drbg_init(&drbg, state->drbg_entropy,
			       state->drbg_entropy_size, dev_salt, P256_NBYTES,
			       NULL, 0);
		hmac_drbg_generate(&drbg, key_seed, sizeof(key_seed),
				   key_handle, key_handle_size);
	} else {
		/**
		 * FIPS-compliant path.
		 *
		 * Seed DRBG with:
		 * 512 bit of entropy from TRNG (stored outside module
		 * boundary).
		 * nonce = key_handle - contains fresh, unique 256-bit random
		 * personalization strint - empty
		 */
		hmac_drbg_init(&drbg, state->drbg_entropy,
			       state->drbg_entropy_size, key_handle,
			       key_handle_size, NULL, 0);

		/**
		 * Additional data = Device_ID (constant coming from HW).
		 */
		hmac_drbg_generate(&drbg, key_seed, sizeof(key_seed), dev_salt,
				   P256_NBYTES);
	}
	if (!DCRYPTO_p256_key_from_bytes(pk_x, pk_y, d, key_seed))
		return EC_ERROR_TRY_AGAIN;

#ifdef CR50_DEV_U2F_VERBOSE
	ccprintf("user private key %ph\n", HEX_BUF(d, sizeof(*d)));
	cflush();
	if (pk_x)
		ccprintf("user public x %ph\n", HEX_BUF(pk_x, sizeof(*pk_x)));
	if (pk_y)
		ccprintf("user public y %ph\n", HEX_BUF(pk_y, sizeof(*pk_y)));
	cflush();
#endif

	return EC_SUCCESS;
}

enum ec_error_list u2f_generate(const struct u2f_state *state,
				const uint8_t *user, const uint8_t *origin,
				const uint8_t *authTimeSecretHash,
				union u2f_key_handle_variant *kh,
				uint8_t kh_version, struct u2f_ec_point *pubKey)
{
	uint8_t *kh_hmac, *kh_origin_seed;
	int generate_key_pair_rc;
	/* Generated public keys associated with key handle. */
	p256_int opk_x, opk_y;

	/* Compute constants for request key handler version. */
	if (kh_version == 0) {
		kh_hmac = kh->v0.hmac;
		kh_origin_seed = kh->v0.origin_seed;
	} else if (kh_version == 1) {
		kh_hmac = kh->v1.kh_hmac;
		kh_origin_seed = kh->v1.origin_seed;
		/**
		 * This may overwrite input parameters if shared
		 * request/response buffer is used by caller.
		 */
		kh->v1.version = kh_version;
	} else
		return EC_ERROR_INVAL;

	/* Generate key handle candidates and origin-specific key pair. */
	do {
		p256_int od;
		/* Generate random origin seed for key handle candidate. */
		if (!fips_rand_bytes(kh_origin_seed, U2F_ORIGIN_SEED_SIZE))
			return EC_ERROR_HW_INTERNAL;

		u2f_origin_user_mac(state, user, origin, kh_origin_seed,
				    kh_version, kh_hmac);

		/**
		 * Try to generate key pair using key handle. This may fail if
		 * key handle results in private key which is out of allowed
		 * range. If this is the case, repeat with another origin seed.
		 */
		generate_key_pair_rc = u2f_origin_user_key_pair(
			state, kh, kh_version, &od, &opk_x, &opk_y);

		p256_clear(&od);
	} while (generate_key_pair_rc == EC_ERROR_TRY_AGAIN);

	if (generate_key_pair_rc != EC_SUCCESS)
		return generate_key_pair_rc;

	if (kh_version == 1) {
		if (!fips_rand_bytes(kh->v1.authorization_salt,
				     U2F_AUTHORIZATION_SALT_SIZE))
			return EC_ERROR_HW_INTERNAL;

		u2f_authorization_mac(state, kh, kh_version, authTimeSecretHash,
				      kh->v1.authorization_hmac);
	}

	pubKey->pointFormat = U2F_POINT_UNCOMPRESSED;
	p256_to_bin(&opk_x, pubKey->x); /* endianness */
	p256_to_bin(&opk_y, pubKey->y); /* endianness */

	return EC_SUCCESS;
}

enum ec_error_list u2f_authorize_keyhandle(
	const struct u2f_state *state, const union u2f_key_handle_variant *kh,
	uint8_t kh_version, const uint8_t *user, const uint8_t *origin,
	const uint8_t *authTimeSecretHash)
{
	/* Re-created key handle. */
	uint8_t recreated_hmac[SHA256_DIGEST_SIZE];
	const uint8_t *origin_seed, *kh_hmac;
	int result = 0;

	/*
	 * Re-create the key handle and compare against that which
	 * was provided. This allows us to verify that the key handle
	 * is owned by this combination of device, current user and origin.
	 */
	if (kh_version == 0) {
		origin_seed = kh->v0.origin_seed;
		kh_hmac = kh->v0.hmac;
	} else {
		origin_seed = kh->v1.origin_seed;
		kh_hmac = kh->v1.kh_hmac;
	}
	/* First, check inner part. */
	u2f_origin_user_mac(state, user, origin, origin_seed, kh_version,
			    recreated_hmac);

	/**
	 * DCRYPTO_equals return 1 if success, by subtracting 1 we make it
	 * zero, and other results - zero or non-zero will be detected.
	 */
	result |= DCRYPTO_equals(&recreated_hmac, kh_hmac,
				 sizeof(recreated_hmac)) - DCRYPTO_OK;

	always_memset(recreated_hmac, 0, sizeof(recreated_hmac));

	if ((kh_version != 0) && (authTimeSecretHash != NULL)) {
		u2f_authorization_mac(state, kh, kh_version, authTimeSecretHash,
				      recreated_hmac);
		result |= DCRYPTO_equals(&recreated_hmac,
					 kh->v1.authorization_hmac,
					 sizeof(recreated_hmac)) - DCRYPTO_OK;
		always_memset(recreated_hmac, 0, sizeof(recreated_hmac));
	}

	return (result == 0) ? EC_SUCCESS : EC_ERROR_ACCESS_DENIED;
}

static enum ec_error_list
u2f_attest_keyhandle_pubkey(const struct u2f_state *state,
			    const union u2f_key_handle_variant *key_handle,
			    uint8_t kh_version, const uint8_t *user,
			    const uint8_t *origin,
			    const uint8_t *authTimeSecretHash,
			    const struct u2f_ec_point *public_key)
{
	struct u2f_ec_point kh_pubkey;
	p256_int od, opk_x, opk_y;
	enum ec_error_list result;

	/* Check this is a correct key handle for provided user/origin. */
	result = u2f_authorize_keyhandle(state, key_handle, kh_version, user,
					 origin, authTimeSecretHash);

	if (result != EC_SUCCESS)
		return result;

	/* Recreate public key from key handle. */
	result = u2f_origin_user_key_pair(state, key_handle, kh_version, &od,
					  &opk_x, &opk_y);
	if (result != EC_SUCCESS)
		return result;

	p256_clear(&od);
	/* Reconstruct the public key. */
	p256_to_bin(&opk_x, kh_pubkey.x);
	p256_to_bin(&opk_y, kh_pubkey.y);
	kh_pubkey.pointFormat = U2F_POINT_UNCOMPRESSED;

#ifdef CR50_DEV_U2F_VERBOSE
	ccprintf("recreated key %ph\n", HEX_BUF(&kh_pubkey, sizeof(kh_pubkey)));
	ccprintf("provided key %ph\n", HEX_BUF(public_key, sizeof(kh_pubkey)));
#endif
	return (DCRYPTO_equals(&kh_pubkey, public_key,
			       sizeof(struct u2f_ec_point)) == DCRYPTO_OK) ?
		       EC_SUCCESS :
			     EC_ERROR_ACCESS_DENIED;
}

enum ec_error_list u2f_sign(const struct u2f_state *state,
			    const union u2f_key_handle_variant *kh,
			    uint8_t kh_version, const uint8_t *user,
			    const uint8_t *origin,
			    const uint8_t *authTimeSecretHash,
			    const uint8_t *hash, struct u2f_signature *sig)
{
	/* Origin private key. */
	p256_int origin_d;

	/* Hash, and corresponding signature. */
	p256_int h, r, s;

	struct drbg_ctx ctx;
	enum ec_error_list result;

	result = u2f_authorize_keyhandle(state, kh, kh_version, user, origin,
					 authTimeSecretHash);

	if (result != EC_SUCCESS)
		return result;

	/* Re-create origin-specific key. */
	result = u2f_origin_user_key_pair(state, kh, kh_version, &origin_d,
					  NULL, NULL);
	if (result != EC_SUCCESS)
		return result;

	/* Prepare hash to sign. */
	p256_from_bin(hash, &h);

	/* Now, we processed input parameters, so clean-up output. */
	memset(sig, 0, sizeof(*sig));

	/* Sign. */
	hmac_drbg_init_rfc6979(&ctx, &origin_d, &h);
	result = (dcrypto_p256_ecdsa_sign(&ctx, &origin_d, &h, &r, &s) != 0) ?
			 EC_SUCCESS :
			       EC_ERROR_HW_INTERNAL;

	p256_clear(&origin_d);

	p256_to_bin(&r, sig->sig_r);
	p256_to_bin(&s, sig->sig_s);

	return result;
}

/**
 * Generate a hardware derived ECDSA key pair for individual attestation.
 *
 * @param state U2F state parameters
 * @param d pointer to ECDSA private key
 * @param pk_x pointer to public key point
 * @param pk_y pointer to public key point
 *
 * @return true if a valid key pair was created.
 */
static bool g2f_individual_key_pair(const struct u2f_state *state, p256_int *d,
				    p256_int *pk_x, p256_int *pk_y)
{
	uint32_t buf[SHA256_DIGEST_WORDS];

	/* Incorporate HIK & diversification constant. */
	if (!app_hw_device_id(U2F_ATTEST, state->salt, buf))
		return false;

	/* Check that U2F state is valid. */
	if (state->drbg_entropy_size != 64 && state->drbg_entropy_size != 32)
		return false;

	if (state->drbg_entropy_size != 64) {
		/* Generate unbiased private key (non-FIPS path). */
		while (!DCRYPTO_p256_key_from_bytes(pk_x, pk_y, d,
						    (uint8_t *)buf)) {
			struct sha256_ctx sha;

			SHA256_hw_init(&sha);
			SHA256_update(&sha, buf, sizeof(buf));
			memcpy(buf, SHA256_final(&sha), sizeof(buf));
		}
	} else {
		struct drbg_ctx drbg;
		uint8_t key_candidate[P256_NBYTES];
		/**
		 * Entropy = 512 of entropy from TRNG
		 * Nonce = 256-bit random
		 * Personalization string = []
		 */
		hmac_drbg_init(&drbg, state->drbg_entropy,
			       state->drbg_entropy_size, state->salt,
			       sizeof(state->salt), NULL, 0);

		do {
			/**
			 * Additional data = constant coming from HW.
			 */
			hmac_drbg_generate(&drbg, key_candidate,
					   sizeof(key_candidate), buf,
					   sizeof(buf));
		} while (!DCRYPTO_p256_key_from_bytes(pk_x, pk_y, d,
						      key_candidate));
	}

	return true;
}

#define G2F_CERT_NAME "CrO2"

size_t g2f_attestation_cert_serial(const struct u2f_state *state,
				   const p256_int *serial, uint8_t *buf)
{
	p256_int d, pk_x, pk_y;

	if (g2f_individual_key_pair(state, &d, &pk_x, &pk_y))
		return 0;

	/* Note that max length is not currently respected here. */
	return DCRYPTO_x509_gen_u2f_cert_name(&d, &pk_x, &pk_y, serial,
					      G2F_CERT_NAME, buf,
					      G2F_ATTESTATION_CERT_MAX_LEN);
}

enum ec_error_list u2f_attest(const struct u2f_state *state,
			      const union u2f_key_handle_variant *kh,
			      uint8_t kh_version, const uint8_t *user,
			      const uint8_t *origin,
			      const uint8_t *authTimeSecretHash,
			      const struct u2f_ec_point *public_key,
			      const uint8_t *data, size_t data_size,
			      struct u2f_signature *sig)
{
	struct sha256_ctx h_ctx;
	struct drbg_ctx dr_ctx;

	/* Data hash, and corresponding signature. */
	p256_int h, r, s;

	/* Attestation key. */
	p256_int d, pk_x, pk_y;

	enum ec_error_list result;

	result = u2f_attest_keyhandle_pubkey(state, kh, kh_version, user,
					     origin, authTimeSecretHash,
					     public_key);

	if (result != EC_SUCCESS)
		return result;

	/* Derive G2F Attestation Key. */
	if (!g2f_individual_key_pair(state, &d, &pk_x, &pk_y)) {
#ifdef CR50_DEV
		ccprintf("G2F Attestation key generation failed\n");
#endif
		return EC_ERROR_HW_INTERNAL;
	}

	/* Message signature. */
	SHA256_hw_init(&h_ctx);
	SHA256_update(&h_ctx, data, data_size);
	p256_from_bin(SHA256_final(&h_ctx)->b8, &h);

	/* Now, we processed input parameters, so clean-up output. */
	memset(sig, 0, sizeof(*sig));

	/* Sign over the response w/ the attestation key. */
	hmac_drbg_init_rfc6979(&dr_ctx, &d, &h);

	result = (dcrypto_p256_ecdsa_sign(&dr_ctx, &d, &h, &r, &s) != 0) ?
			 EC_SUCCESS :
			       EC_ERROR_HW_INTERNAL;
	p256_clear(&d);

	p256_to_bin(&r, sig->sig_r);
	p256_to_bin(&s, sig->sig_s);

	return result;
}

#ifndef CRYPTO_TEST_CMD_U2F_TEST
#define CRYPTO_TEST_CMD_U2F_TEST 0
#endif

#if defined(CRYPTO_TEST_SETUP) && CRYPTO_TEST_CMD_U2F_TEST

static const char *expect_bool(enum ec_error_list value,
			       enum ec_error_list expect)
{
	if (value == expect)
		return "PASSED";
	return "NOT PASSED";
}

static int cmd_u2f_test(int argc, char **argv)
{
	static struct u2f_state state;
	static union u2f_key_handle_variant kh;
	static const uint8_t origin[32] = { 0xff, 0xfe, 0xfd, 8, 8, 8, 8, 8,
					    8,	  8,	8,    8, 8, 8, 8, 8,
					    8,	  8,	8,    8, 8, 8, 8, 8,
					    8,	  8,	8,    8, 8, 8, 8, 8 };
	static const uint8_t user[32] = { 0x88, 0x8e, 0x8d, 7, 7, 7, 7, 7,
					  7,	7,    7,    7, 7, 7, 7, 7,
					  7,	7,    7,    7, 7, 7, 7, 7,
					  7,	7,    7,    7, 7, 7, 7, 7 };
	static const uint8_t authTime[32] = { 0x99, 0x91, 2, 3, 4, 5, 5, 5, 5,
					      5,    5,	  5, 5, 5, 5, 5, 5, 5,
					      5,    5,	  5, 5, 5, 5, 5, 5, 5 };
	static struct u2f_ec_point pubKey;
	static struct u2f_signature sig;

	ccprintf("u2f_generate_hmac_key - %s\n",
		 expect_bool(u2f_generate_hmac_key(&state), EC_SUCCESS));

	ccprintf("u2f_generate_g2f_secret - %s\n",
		 expect_bool(u2f_generate_g2f_secret(&state), EC_SUCCESS));

	ccprintf("u2f_generate_drbg_entropy - %s\n",
		 expect_bool(u2f_generate_drbg_entropy(&state), EC_SUCCESS));

	/* Version 0 key handle. */
	ccprintf("u2f_generate - %s\n",
		 expect_bool(u2f_generate(&state, user, origin, authTime, &kh,
					  0, &pubKey),
			     EC_SUCCESS));
	ccprintf("kh: %ph\n", HEX_BUF(&kh, sizeof(kh.v0)));
	ccprintf("pubKey: %ph\n", HEX_BUF(&pubKey, sizeof(pubKey)));

	ccprintf("u2f_authorize_keyhandle - %s\n",
		 expect_bool(u2f_authorize_keyhandle(&state, &kh, 0, user,
						     origin, authTime),
			     EC_SUCCESS));

	kh.v0.origin_seed[0] ^= 0x10;
	ccprintf("u2f_authorize_keyhandle - %s\n",
		 expect_bool(u2f_authorize_keyhandle(&state, &kh, 0, user,
						     origin, authTime),
			     EC_ERROR_ACCESS_DENIED));

	kh.v0.origin_seed[0] ^= 0x10;
	ccprintf("u2f_sign - %s\n",
		 expect_bool(u2f_sign(&state, &kh, 0, user, origin, authTime,
				      authTime, &sig),
			     EC_SUCCESS));
	ccprintf("sig: %ph\n", HEX_BUF(&sig, sizeof(sig)));

	ccprintf("u2f_attest - %s\n",
		 expect_bool(u2f_attest(&state, &kh, 0, user, origin, authTime,
					&pubKey, authTime, sizeof(authTime),
					&sig),
			     EC_SUCCESS));
	ccprintf("sig: %ph\n", HEX_BUF(&sig, sizeof(sig)));

	/* Should fail with incorrect key handle. */
	kh.v0.origin_seed[0] ^= 0x10;
	ccprintf("u2f_sign - %s\n",
		 expect_bool(u2f_sign(&state, &kh, 0, user, origin, authTime,
				      authTime, &sig),
			     EC_ERROR_ACCESS_DENIED));
	ccprintf("sig: %ph\n", HEX_BUF(&sig, sizeof(sig)));

	/* Version 1 key handle. */
	ccprintf("\nVersion 1 tests\n");
	ccprintf("u2f_generate - %s\n",
		 expect_bool(u2f_generate(&state, user, origin, authTime, &kh,
					  1, &pubKey),
			     EC_SUCCESS));
	ccprintf("kh: %ph\n", HEX_BUF(&kh, sizeof(kh.v1)));
	ccprintf("pubKey: %ph\n", HEX_BUF(&pubKey, sizeof(pubKey)));

	ccprintf("u2f_authorize_keyhandle - %s\n",
		 expect_bool(u2f_authorize_keyhandle(&state, &kh, 1, user,
						     origin, authTime),
			     EC_SUCCESS));

	kh.v1.authorization_salt[0] ^= 0x10;
	ccprintf("u2f_authorize_keyhandle - %s\n",
		 expect_bool(u2f_authorize_keyhandle(&state, &kh, 1, user,
						     origin, authTime),
			     EC_ERROR_ACCESS_DENIED));

	kh.v1.authorization_salt[0] ^= 0x10;
	ccprintf("u2f_sign - %s\n",
		 expect_bool(u2f_sign(&state, &kh, 1, user, origin, authTime,
				      authTime, &sig),
			     EC_SUCCESS));
	ccprintf("sig: %ph\n", HEX_BUF(&sig, sizeof(sig)));

	ccprintf("u2f_attest - %s\n",
		 expect_bool(u2f_attest(&state, &kh, 1, user, origin, authTime,
					&pubKey, authTime, sizeof(authTime),
					&sig),
			     EC_SUCCESS));
	ccprintf("sig: %ph\n", HEX_BUF(&sig, sizeof(sig)));

	/* Should fail with incorrect key handle. */
	kh.v1.origin_seed[0] ^= 0x10;
	ccprintf("u2f_sign - %s\n",
		 expect_bool(u2f_sign(&state, &kh, 1, user, origin, authTime,
				      authTime, &sig),
			     EC_ERROR_ACCESS_DENIED));
	ccprintf("sig: %ph\n", HEX_BUF(&sig, sizeof(sig)));

	cflush();

	return 0;
}

DECLARE_SAFE_CONSOLE_COMMAND(u2f_test, cmd_u2f_test, NULL,
			     "Test U2F functionality");

#endif
