/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Common U2F raw message format header - Review Draft
// 2014-10-08
// Editor: Jakob Ehrensvard, Yubico, jakob@yubico.com

#ifndef __U2F_H_INCLUDED__
#define __U2F_H_INCLUDED__

#ifdef _MSC_VER /* Windows */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long int uint64_t;
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Load platform hooks/definitions */
#include "tpm_vendor_cmds.h"
#include "u2f_impl.h"

/* General constants */

#define U2F_MAX_ATT_CERT_SIZE 2048 /* Max size of attestation certificate */
#define U2F_MAX_EC_SIG_SIZE   72 /* Max size of DER coded EC signature */
#define U2F_CTR_SIZE	      4 /* Size of counter field */
#define U2F_APPID_SIZE	      32 /* Size of application id */
#define U2F_USER_SECRET_SIZE  32 /* Size of user secret */

#define U2F_CHAL_SIZE	      32 /* Size of challenge */
#define U2F_MAX_ATTEST_SIZE   256 /* Size of largest blob to sign */

#define SHA256_DIGEST_SIZE 32

#define U2F_AUTH_TIME_SECRET_SIZE 32
#define U2F_MESSAGE_DIGEST_SIZE SHA256_DIGEST_SIZE


#define ENC_SIZE(x) ((x + 7) & 0xfff8)

/* Request Flags. */

#define U2F_AUTH_ENFORCE    0x03 /* Enforce user presence and sign */
#define U2F_AUTH_CHECK_ONLY 0x07 /* Check only */
#define U2F_AUTH_FLAG_TUP   0x01 /* Test of user presence set */
/**
 * The key handle can be used with fingerprint or PIN fro WebAuthn.
 * Implies key handle version = 1.
 */
#define U2F_UV_ENABLED_KH 0x08

#define U2F_KH_VERSION_1	    0x01

/* TODO(louiscollard): Add Descriptions. */

struct u2f_generate_req {
	uint8_t appId[U2F_APPID_SIZE]; /* Application id */
	uint8_t userSecret[U2F_USER_SECRET_SIZE];
	uint8_t flags;
	/*
	 * If generating versioned KH, derive an hmac from it and append to
	 * the key handle. Otherwise unused.
	 */
	uint8_t authTimeSecretHash[U2F_AUTH_TIME_SECRET_SIZE];
};

struct u2f_generate_resp {
	struct u2f_ec_point pubKey; /* Generated public key */
	struct u2f_key_handle_v0 keyHandle;
};

struct u2f_generate_versioned_resp {
	struct u2f_ec_point pubKey; /* Generated public key */
	struct u2f_key_handle_v1 keyHandle;
};

struct u2f_sign_req {
	uint8_t appId[U2F_APPID_SIZE]; /* Application id */
	uint8_t userSecret[U2F_USER_SECRET_SIZE];
	struct u2f_key_handle_v0 keyHandle;
	uint8_t hash[U2F_MESSAGE_DIGEST_SIZE];
	uint8_t flags;
};

struct u2f_sign_versioned_req {
	uint8_t appId[U2F_APPID_SIZE]; /* Application id */
	uint8_t userSecret[U2F_USER_SECRET_SIZE];
	uint8_t authTimeSecret[U2F_AUTH_TIME_SECRET_SIZE];
	uint8_t hash[U2F_MESSAGE_DIGEST_SIZE];
	uint8_t flags;
	struct u2f_key_handle_v1 keyHandle;
};


struct u2f_sign_resp {
	struct u2f_signature sig; /* Signature */
};

struct u2f_attest_req {
	uint8_t userSecret[U2F_USER_SECRET_SIZE];
	uint8_t format;
	uint8_t dataLen;
	uint8_t data[U2F_MAX_ATTEST_SIZE];
};

struct u2f_attest_resp {
	struct u2f_signature sig; /* Signature */
};

struct g2f_register_msg {
	uint8_t reserved;
	uint8_t app_id[U2F_APPID_SIZE];
	uint8_t challenge[U2F_CHAL_SIZE];
	struct u2f_key_handle_v0 key_handle;
	struct u2f_ec_point public_key;
};

/* Command status responses */

#define U2F_SW_NO_ERROR	  0x9000 /* SW_NO_ERROR */
#define U2F_SW_WRONG_DATA 0x6A80 /* SW_WRONG_DATA */
#define U2F_SW_CONDITIONS_NOT_SATISFIED       \
	0x6985 /* SW_CONDITIONS_NOT_SATISFIED \
		*/
#define U2F_SW_COMMAND_NOT_ALLOWED 0x6986 /* SW_COMMAND_NOT_ALLOWED */
#define U2F_SW_INS_NOT_SUPPORTED   0x6D00 /* SW_INS_NOT_SUPPORTED */

/* Protocol extensions */

/* Non-standardized command status responses */
#define U2F_SW_CLA_NOT_SUPPORTED 0x6E00
#define U2F_SW_WRONG_LENGTH	 0x6700
#define U2F_SW_WTF		 0x6f00

/* Additional flags for P1 field */
#define G2F_ATTEST  0x80 /* Fixed attestation key */
#define G2F_CONSUME 0x02 /* Consume presence */

/*
 * The key handle format was changed when support for user secrets was added.
 * U2F_SIGN requests that specify this flag will first try to validate the key
 * handle as a new format key handle, and if that fails, will fall back to
 * treating it as a legacy key handle (without user secrets).
 */
#define SIGN_LEGACY_KH 0x40

/* U2F Attest format for U2F Register Response. */
#define U2F_ATTEST_FORMAT_REG_RESP 0

/* Vendor command to enable/disable the extensions */
#define U2F_VENDOR_MODE U2F_VENDOR_LAST


/**
 * U2F_GENERATE command handler. Generates a key handle according to input
 * parameters.
 */
enum vendor_cmd_rc u2f_generate_cmd(enum vendor_cmd_cc code, void *buf,
				    size_t input_size, size_t *response_size);

/**
 * U2F_SIGN command handler. Verifies a key handle is owned and signs data with
 * it.
 */
enum vendor_cmd_rc u2f_sign_cmd(enum vendor_cmd_cc code, void *buf,
				size_t input_size, size_t *response_size);


/* Maximum size in bytes of G2F attestation certificate. */
#define G2F_ATTESTATION_CERT_MAX_LEN 315

/**
 * Gets the x509 certificate for the attestation key pair returned
 * by g2f_individual_keypair().
 *
 * @param buf pointer to a buffer that must be at least
 * G2F_ATTESTATION_CERT_MAX_LEN bytes.
 * @return size of certificate written to buf, 0 on error.
 */
size_t g2f_attestation_cert(uint8_t *buf);


#ifdef __cplusplus
}
#endif

#endif /* __U2F_H_INCLUDED__ */
