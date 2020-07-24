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

/* General constants */

#define U2F_EC_KEY_SIZE	      32 /* EC key size in bytes */
#define U2F_EC_POINT_SIZE     ((U2F_EC_KEY_SIZE * 2) + 1) /* Size of EC point */
#define U2F_MAX_KH_SIZE	      128 /* Max size of key handle */
#define U2F_MAX_ATT_CERT_SIZE 2048 /* Max size of attestation certificate */
#define U2F_MAX_EC_SIG_SIZE   72 /* Max size of DER coded EC signature */
#define U2F_CTR_SIZE	      4 /* Size of counter field */
#define U2F_APPID_SIZE	      32 /* Size of application id */
#define U2F_CHAL_SIZE	      32 /* Size of challenge */
#define U2F_MAX_ATTEST_SIZE   256 /* Size of largest blob to sign */
#define U2F_P256_SIZE	      32

#define ENC_SIZE(x) ((x + 7) & 0xfff8)

/* EC (uncompressed) point */

#define U2F_POINT_UNCOMPRESSED 0x04 /* Uncompressed point format */

struct u2f_ec_point {
	uint8_t pointFormat; /* Point type */
	uint8_t x[U2F_EC_KEY_SIZE]; /* X-value */
	uint8_t y[U2F_EC_KEY_SIZE]; /* Y-value */
};

/* Request Flags. */

#define U2F_AUTH_ENFORCE    0x03 /* Enforce user presence and sign */
#define U2F_AUTH_CHECK_ONLY 0x07 /* Check only */
#define U2F_AUTH_FLAG_TUP   0x01 /* Test of user presence set */
/* The key handle can be used with fingerprint or PIN. */
#define U2F_UV_ENABLED_KH 0x08

#define U2F_KH_VERSION_1 0x01

#define U2F_AUTHORIZATION_SALT_SIZE 16

struct u2f_key_handle {
	uint8_t origin_seed[U2F_P256_SIZE];
	uint8_t hmac[U2F_P256_SIZE];
};

struct u2f_versioned_key_handle_header {
	uint8_t version;
	uint8_t origin_seed[U2F_P256_SIZE];
	uint8_t kh_hmac[U2F_P256_SIZE];
};

struct u2f_versioned_key_handle {
	struct u2f_versioned_key_handle_header header;
	/* Optionally checked in u2f_sign. */
	uint8_t authorization_salt[U2F_AUTHORIZATION_SALT_SIZE];
	uint8_t authorization_hmac[U2F_P256_SIZE];
};

/* TODO(louiscollard): Add Descriptions. */

struct u2f_generate_req {
	uint8_t appId[U2F_APPID_SIZE]; /* Application id */
	uint8_t userSecret[U2F_P256_SIZE];
	uint8_t flags;
	/*
	 * If generating versioned KH, derive an hmac from it and append to
	 * the key handle. Otherwise unused.
	 */
	uint8_t authTimeSecretHash[U2F_P256_SIZE];
};

struct u2f_generate_resp {
	struct u2f_ec_point pubKey; /* Generated public key */
	struct u2f_key_handle keyHandle;
};

struct u2f_generate_versioned_resp {
	struct u2f_ec_point pubKey; /* Generated public key */
	struct u2f_versioned_key_handle keyHandle;
};

struct u2f_sign_req {
	uint8_t appId[U2F_APPID_SIZE]; /* Application id */
	uint8_t userSecret[U2F_P256_SIZE];
	struct u2f_key_handle keyHandle;
	uint8_t hash[U2F_P256_SIZE];
	uint8_t flags;
};

struct u2f_sign_versioned_req {
	uint8_t appId[U2F_APPID_SIZE]; /* Application id */
	uint8_t userSecret[U2F_P256_SIZE];
	uint8_t authTimeSecret[U2F_P256_SIZE];
	uint8_t hash[U2F_P256_SIZE];
	uint8_t flags;
	struct u2f_versioned_key_handle keyHandle;
};

struct u2f_sign_resp {
	uint8_t sig_r[U2F_P256_SIZE]; /* Signature */
	uint8_t sig_s[U2F_P256_SIZE]; /* Signature */
};

struct u2f_attest_req {
	uint8_t userSecret[U2F_P256_SIZE];
	uint8_t format;
	uint8_t dataLen;
	uint8_t data[U2F_MAX_ATTEST_SIZE];
};

struct u2f_attest_resp {
	uint8_t sig_r[U2F_P256_SIZE];
	uint8_t sig_s[U2F_P256_SIZE];
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

#ifdef __cplusplus
}
#endif

#endif /* __U2F_H_INCLUDED__ */
