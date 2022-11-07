// Common U2F raw message format header - Review Draft
// 2014-10-08
// Editor: Jakob Ehrensvard, Yubico, jakob@yubico.com

#ifndef __U2F_H_INCLUDED__
#define __U2F_H_INCLUDED__

/**
 * Note: This header file should be self-sufficient as it is shared
 * with other boards and with userland daemons (u2fd).
 *
 * chromeos-ec-headers package installs it in ChromeOS development environment.
 *
 */

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

#define U2F_EC_KEY_SIZE	      32 /* EC key size in bytes, NIST P-256 Curve */
#define U2F_EC_POINT_SIZE     ((U2F_EC_KEY_SIZE * 2) + 1) /* Size of EC point */
#define U2F_MAX_KH_SIZE	      128 /* Max size of key handle */
#define U2F_MAX_ATT_CERT_SIZE 2048 /* Max size of attestation certificate */
#define U2F_MAX_EC_SIG_SIZE   72 /* Max size of DER coded EC signature */
#define U2F_CTR_SIZE	      4 /* Size of counter field */
#define U2F_APPID_SIZE	      32 /* Size of application id */
#define U2F_CHAL_SIZE	      32 /* Size of challenge */
#define U2F_MAX_ATTEST_SIZE   256 /* Size of largest blob to sign */
#define U2F_P256_SIZE	      32
/* Origin seed is a random nonce generated during key handle creation. */
#define U2F_ORIGIN_SEED_SIZE 32
#define U2F_USER_SECRET_SIZE 32 /* Size of user secret */

#define U2F_AUTH_TIME_SECRET_SIZE 32

#define SHA256_DIGEST_SIZE	32
#define U2F_MESSAGE_DIGEST_SIZE SHA256_DIGEST_SIZE

#define CORP_CHAL_SIZE 16
#define CORP_SALT_SIZE 65

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

/* Request v2 key handle. Should be used with U2F_UV_ENABLED_KH */
#define U2F_V2_KH      0x10
#define U2F_V2_KH_MASK (U2F_V2_KH | U2F_UV_ENABLED_KH)

#define U2F_KH_VERSION_1 0x01
#define U2F_KH_VERSION_2 0x02

#define U2F_AUTHORIZATION_SALT_SIZE 16
#define U2F_V0_KH_SIZE		    64

/**
 * Key handle version = 1 for WebAuthn, bound to device and user.
 */
#define U2F_V1_KH_SIZE 113

/* Header is composed of version || origin_seed || kh_hmac */
#define U2F_V1_KH_HEADER_SIZE (U2F_ORIGIN_SEED_SIZE + SHA256_DIGEST_SIZE + 1)

/**
 * Key handle version = 2 for WebAuthn, bound to device and user.
 */
#define U2F_V2_KH_SIZE 81

/* Header is composed of version || origin_seed */
#define U2F_V2_KH_HEADER_SIZE (U2F_ORIGIN_SEED_SIZE + 1)

struct u2f_signature {
	uint8_t sig_r[U2F_EC_KEY_SIZE]; /* Signature */
	uint8_t sig_s[U2F_EC_KEY_SIZE]; /* Signature */
};

struct u2f_key_handle {
	uint8_t origin_seed[U2F_ORIGIN_SEED_SIZE];
	uint8_t hmac[SHA256_DIGEST_SIZE];
};

struct u2f_versioned_key_handle_header {
	uint8_t version;
	uint8_t origin_seed[U2F_ORIGIN_SEED_SIZE];
	uint8_t kh_hmac[SHA256_DIGEST_SIZE];
};

struct u2f_versioned_key_handle {
	struct u2f_versioned_key_handle_header header;
	/* Optionally checked in u2f_sign. */
	uint8_t authorization_salt[U2F_AUTHORIZATION_SALT_SIZE];
	uint8_t authorization_hmac[SHA256_DIGEST_SIZE];
};

/**
 * Alternative definitions of key handles.
 *
 *  struct u2f_key_handle_v0 == struct u2f_key_handle
 *  struct u2f_key_handle_v1 == struct u2f_versioned_key_handle
 *
 */

/* Key handle version = 0, only bound to device. */
struct u2f_key_handle_v0 {
	uint8_t origin_seed[U2F_ORIGIN_SEED_SIZE];
	uint8_t hmac[SHA256_DIGEST_SIZE];
};

/* Key handle version = 1, bound to device and user. */
struct u2f_key_handle_v1 {
	uint8_t version;
	uint8_t origin_seed[U2F_ORIGIN_SEED_SIZE];
	/* HMAC(u2f_hmac_key, origin || user || origin seed || version) */
	uint8_t kh_hmac[SHA256_DIGEST_SIZE];
	/* Optionally checked in u2f_sign. */
	uint8_t authorization_salt[U2F_AUTHORIZATION_SALT_SIZE];
	/**
	 * HMAC(u2f_hmac_key,
	 *      auth_salt || version || origin_seed
	 *      || kh_hmac || authTimeSecretHash)
	 */
	uint8_t authorization_hmac[SHA256_DIGEST_SIZE];
};

/* Key handle version = 2, bound to device and user. */
struct u2f_key_handle_v2 {
	uint8_t version;
	uint8_t origin_seed[U2F_ORIGIN_SEED_SIZE];
	/* Always checked in u2f_sign. */
	uint8_t authorization_salt[U2F_AUTHORIZATION_SALT_SIZE];
	/**
	 * HMAC(u2f_hmac_key,
	 *      auth_salt || version || origin_seed || origin ||
	 *      user || authTimeSecretHash)
	 */
	uint8_t authorization_hmac[SHA256_DIGEST_SIZE];
};

union u2f_key_handle_variant {
	struct u2f_key_handle_v0 v0;
	struct u2f_key_handle_v1 v1;
	struct u2f_key_handle_v2 v2;
};

/* TODO(louiscollard): Add Descriptions. */

struct u2f_generate_req {
	uint8_t appId[U2F_APPID_SIZE]; /* Application id */
	uint8_t userSecret[U2F_USER_SECRET_SIZE];
	uint8_t flags;
	/*
	 * If generating versioned KH, derive an hmac from it and append to
	 * the key handle. Otherwise unused.
	 */
	uint8_t authTimeSecretHash[SHA256_DIGEST_SIZE];
};

struct u2f_generate_resp {
	struct u2f_ec_point pubKey; /* Generated public key */
	struct u2f_key_handle keyHandle;
};

struct u2f_generate_versioned_resp {
	struct u2f_ec_point pubKey; /* Generated public key */
	struct u2f_versioned_key_handle keyHandle;
};

struct u2f_generate_versioned_resp_v2 {
	struct u2f_ec_point pubKey; /* Generated public key */
	struct u2f_key_handle_v2 keyHandle;
};

/**
 * Combined type for U2F_GENERATE response. Length of response size
 * should be used to determine which version of key handle is generated.
 * Caller may check that response matches request flags.
 */
union u2f_generate_response {
	struct u2f_generate_resp v0;
	struct u2f_generate_versioned_resp v1;
	struct u2f_generate_versioned_resp_v2 v2;
};

struct u2f_sign_req {
	uint8_t appId[U2F_APPID_SIZE]; /* Application id */
	uint8_t userSecret[U2F_USER_SECRET_SIZE];
	struct u2f_key_handle keyHandle;
	uint8_t hash[U2F_P256_SIZE];
	uint8_t flags;
};

struct u2f_sign_versioned_req {
	uint8_t appId[U2F_APPID_SIZE]; /* Application id */
	uint8_t userSecret[U2F_USER_SECRET_SIZE];
	uint8_t authTimeSecret[U2F_AUTH_TIME_SECRET_SIZE];
	uint8_t hash[U2F_P256_SIZE];
	uint8_t flags;
	struct u2f_versioned_key_handle keyHandle;
};

struct u2f_sign_versioned_req_v2 {
	uint8_t appId[U2F_APPID_SIZE]; /* Application id */
	uint8_t userSecret[U2F_USER_SECRET_SIZE];
	uint8_t authTimeSecret[U2F_AUTH_TIME_SECRET_SIZE];
	uint8_t hash[U2F_P256_SIZE];
	uint8_t flags;
	struct u2f_key_handle_v2 keyHandle;
};

/**
 * Combined type for U2F_SIGN request. Length of request size
 * is used to determine which version of key handle is provided.
 */
union u2f_sign_request {
	struct u2f_sign_req v0;
	struct u2f_sign_versioned_req v1;
	struct u2f_sign_versioned_req_v2 v2;
};

struct u2f_sign_resp {
	uint8_t sig_r[U2F_P256_SIZE]; /* Signature */
	uint8_t sig_s[U2F_P256_SIZE]; /* Signature */
};

struct u2f_attest_req {
	uint8_t userSecret[U2F_USER_SECRET_SIZE];
	uint8_t format;
	uint8_t dataLen;
	/* struct g2f_register_msg_vX or corp_register_msg_vX */
	uint8_t data[U2F_MAX_ATTEST_SIZE];
};

struct g2f_register_msg_v0 {
	uint8_t reserved;
	uint8_t app_id[U2F_APPID_SIZE];
	uint8_t challenge[U2F_CHAL_SIZE];
	struct u2f_key_handle_v0 key_handle;
	struct u2f_ec_point public_key;
};

struct corp_attest_data {
	uint8_t challenge[CORP_CHAL_SIZE];
	struct u2f_ec_point public_key;
	uint8_t salt[CORP_SALT_SIZE];
};

struct corp_register_msg_v0 {
	struct corp_attest_data data;
	uint8_t app_id[U2F_APPID_SIZE];
	struct u2f_key_handle_v0 key_handle;
};

union u2f_attest_msg_variant {
	struct g2f_register_msg_v0 g2f;
	struct corp_register_msg_v0 corp;
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

/* Corp Attest format for U2F Register Response. */
#define CORP_ATTEST_FORMAT_REG_RESP 1

/* Vendor command to enable/disable the extensions */
#define U2F_VENDOR_MODE U2F_VENDOR_LAST

#ifdef __cplusplus
}
#endif

#endif /* __U2F_H_INCLUDED__ */
