/* Copyright 2017 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* U2F implementation-specific callbacks and parameters. */

#ifndef __CROS_EC_U2F_IMPL_H
#define __CROS_EC_U2F_IMPL_H

#include "common.h"
#include "dcrypto.h"
#include "u2f.h"

/* ---- platform cryptography hooks ---- */

/* ---- non-volatile U2F state, shared with common code ---- */
struct u2f_state {
	/* G2F key gen seed. */
	uint32_t salt[8];
	/* HMAC key for U2F key handle authentication. */
	uint32_t hmac_key[SHA256_DIGEST_SIZE / sizeof(uint32_t)];
	/* Stored DRBG entropy. */
	uint32_t drbg_entropy[16];
	size_t drbg_entropy_size;
};

/* Make sure common declaration is compatible. */
BUILD_ASSERT(U2F_EC_KEY_SIZE == P256_NBYTES);
BUILD_ASSERT(sizeof(struct u2f_ec_point) == U2F_EC_POINT_SIZE);

BUILD_ASSERT(sizeof(struct u2f_key_handle_v0) <= U2F_MAX_KH_SIZE);
BUILD_ASSERT(sizeof(struct u2f_key_handle_v0) == U2F_V0_KH_SIZE);

BUILD_ASSERT(sizeof(struct u2f_key_handle_v1) <= U2F_MAX_KH_SIZE);
BUILD_ASSERT(sizeof(struct u2f_key_handle_v1) == U2F_V1_KH_SIZE);


BUILD_ASSERT(sizeof(union u2f_key_handle_variant) <= U2F_MAX_KH_SIZE);

/**
 * Create or update DRBG entropy in U2F state. Used when changing ownership
 * to cryptographically discard previously generated keys.
 *
 * @param state u2f state to update
 *
 * @return EC_SUCCESS if successful
 */
enum ec_error_list u2f_generate_drbg_entropy(struct u2f_state *state);

/**
 * Create or update HMAC key in U2F state. Used when changing ownership to
 * cryptographically discard previously generated keys.
 *
 * @param state u2f state to update
 *
 * @return EC_SUCCESS if successful
 */
enum ec_error_list u2f_generate_hmac_key(struct u2f_state *state);

/**
 * Create or update G2F secret in U2F state.
 *
 * @param state u2f state to update
 *
 * @return EC_SUCCESS if successful
 */
enum ec_error_list u2f_generate_g2f_secret(struct u2f_state *state);

/**
 * Create a randomized key handle for specified origin, user secret.
 * Generate associated signing key.
 *
 * @param state initialized u2f state
 * @param origin pointer to origin id
 * @param user pointer to user secret
 * @param authTimeSecretHash authentication time secret
 * @param kh output key handle header
 * @param kh_version - key handle version to generate
 * @param pubKey - generated public key
 *
 * @return EC_SUCCESS if successful
 */
enum ec_error_list u2f_generate(const struct u2f_state *state,
				const uint8_t *user, const uint8_t *origin,
				const uint8_t *authTimeSecretHash,
				union u2f_key_handle_variant *kh,
				uint8_t kh_version,
				struct u2f_ec_point *pubKey);

/**
 * Create a randomized key handle for specified origin, user secret.
 * Generate associated signing key.
 *
 * @param state initialized u2f state
 * @param kh output key handle header
 * @param kh_version - key handle version to generate
 * @param origin pointer to origin id
 * @param user pointer to user secret
 * @param authTimeSecretHash pointer to user's authentication secret.
 *        can be set to NULL if authorization_hmac check is not needed.
 * @param r - generated part of signature
 * @param s - generated part of signature
 *
 * @return EC_SUCCESS if a valid key pair was created
 *         EC_ACCESS_DENIED if key handle can't authenticated
 */
enum ec_error_list u2f_sign(const struct u2f_state *state,
			    const union u2f_key_handle_variant *kh,
			    uint8_t kh_version, const uint8_t *user,
			    const uint8_t *origin,
			    const uint8_t *authTimeSecretHash,
			    const uint8_t *hash, struct u2f_signature *sig);

/**
 * Verify that key handle matches provided origin, user and user's
 * authentication secret and was created on this device (signed with
 * U2F state HMAC key).
 *
 * @param state initialized u2f state
 * @param kh input key handle
 * @param kh_version - key handle version to verify
 * @param user pointer to user secret
 * @param origin pointer to origin id
 * @param authTimeSecretHash pointer to user's authentication secret.
 *        can be set to NULL if authorization_hmac check is not needed.
 *
 * @return EC_SUCCESS if handle can be authenticated
 */
enum ec_error_list u2f_authorize_keyhandle(const struct u2f_state *state,
			     const union u2f_key_handle_variant *kh,
			     uint8_t kh_version, const uint8_t *user,
			     const uint8_t *origin,
			     const uint8_t *authTimeSecretHash);

/**
 * Gets the x509 certificate for the attestation key pair returned
 * by g2f_individual_keypair().
 *
 * @param state U2F state parameters
 * @param serial Device serial number
 * @param buf pointer to a buffer that must be at least
 *
 * G2F_ATTESTATION_CERT_MAX_LEN bytes.
 * @return size of certificate written to buf, 0 on error.
 */
size_t g2f_attestation_cert_serial(const struct u2f_state *state,
				   const p256_int *serial, uint8_t *buf);

/**
 * Verify that provided key handle and public key match.
 * @param state U2F state parameters
 * @param key_handle key handle
 * @param kh_version key handle version (0 - legacy, 1 - versioned)
 * @param user pointer to user secret
 * @param origin pointer to origin id
 * @param authTimeSecretHash pointer to user's authentication secret.
 *        can be set to NULL if authorization_hmac check is not needed.
 * @param public_key pointer to public key point (big endian)
 * @param data data to sign
 * @param data_size data size in bytes
 *
 * @param r part of generated signature
 * @param s part of generated signature
 *
 * @return EC_SUCCESS if public key matches key handle,
 *         (r,s) set to valid signature
 *         EC_ACCESS_DENIED if key handle can't authenticated
 */
enum ec_error_list u2f_attest(const struct u2f_state *state,
			      const union u2f_key_handle_variant *kh,
			      uint8_t kh_version, const uint8_t *user,
			      const uint8_t *origin,
			      const uint8_t *authTimeSecretHash,
			      const struct u2f_ec_point *public_key,
			      const uint8_t *data, size_t data_size,
			      struct u2f_signature *sig);


/**
 *
 * Board U2F key management part implemented.
 *
 */

/**
 * Get the current u2f state from the board.
 *
 * Create a state if one does not exist, commit the created state into NVMEM.
 *
 * @return pointer to static state if successful, NULL otherwise
 */
struct u2f_state *u2f_get_state(void);

/**
 * Get the current u2f state from the board.
 *
 * Create a state if one does not exist, do not force committing of the
 * created state into NVMEM.
 *
 * @return pointer to static state if successful, NULL otherwise
 */
struct u2f_state *u2f_get_state_no_commit(void);

/**
 * Try to load U2F keys or create if failed.
 *
 * @param state - buffer for state to load/create
 * @param force_create - if true, always create all keys
 *
 * @return true if state is properly initialized and will persist in flash.
 */
bool u2f_load_or_create_state(struct u2f_state *state, bool force_create,
			      bool commit);

/***
 * Generates and persists to nvram a new key that will be used to
 * sign U2F key handles and check they were created on this device.
 *
 * @return EC_SUCCESS if seed was successfully created
 * (and persisted if requested).
 */
enum ec_error_list u2f_gen_kek_seed(void);

/**
 * Zeroize U2F keys. Can be used to switch to FIPS-compliant path by
 * destroying old keys.
 *
 * @return true if state is properly initialized and will persist in flash.
 */
enum ec_error_list u2f_zeroize_keys(void);

/**
 * Update keys to a newer (FIPS-compliant) version if needed. Do nothing if
 * keys are already updated.
 *
 * @return EC_SUCCESS or error code.
 */
enum ec_error_list u2f_update_keys(void);

/**
 * Check status of U2F secrets used for key generation to be FIPS
 *
 * @return Return `true` if U2F keys are FIPS-compliant
 */
bool u2f_keys_are_fips(void);

#endif /* __CROS_EC_U2F_IMPL_H */
