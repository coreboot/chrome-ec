/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Crypto wrapper library for the g chip.
 */
#ifndef __EC_BOARD_CR50_DCRYPTO_DCRYPTO_H
#define __EC_BOARD_CR50_DCRYPTO_DCRYPTO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "crypto_common.h"
#include "internal.h"

#include <stddef.h>

enum cipher_mode {
	CIPHER_MODE_ECB = 0, /* NIST SP 800-38A */
	CIPHER_MODE_CTR = 1, /* NIST SP 800-38A */
	CIPHER_MODE_CBC = 2, /* NIST SP 800-38A */
	CIPHER_MODE_GCM = 3 /* NIST SP 800-38D */
};

enum encrypt_mode { DECRYPT_MODE = 0, ENCRYPT_MODE = 1 };

enum hashing_mode {
	HASH_SHA1 = 0,
	HASH_SHA256 = 1,
	HASH_SHA384 = 2, /* Only supported for PKCS#1 signing */
	HASH_SHA512 = 3, /* Only supported for PKCS#1 signing */
	HASH_NULL = 4 /* Only supported for PKCS#1 signing */
};

/*
 * AES implementation, based on a hardware AES block.
 * FIPS Publication 197, The Advanced Encryption Standard (AES)
 */
#define AES256_BLOCK_CIPHER_KEY_SIZE 32

int DCRYPTO_aes_init(const uint8_t *key, uint32_t key_len, const uint8_t *iv,
		     enum cipher_mode c_mode, enum encrypt_mode e_mode);
int DCRYPTO_aes_block(const uint8_t *in, uint8_t *out);

void DCRYPTO_aes_write_iv(const uint8_t *iv);
void DCRYPTO_aes_read_iv(uint8_t *iv);

/* AES-CTR-128/192/256
 * NIST Special Publication 800-38A
 */
int DCRYPTO_aes_ctr(uint8_t *out, const uint8_t *key, uint32_t key_bits,
		    const uint8_t *iv, const uint8_t *in, size_t in_len);

/* AES-GCM-128/192/256
 * NIST Special Publication 800-38D, IV is provided externally
 * Caller should use IV length according to section 8.2 of SP 800-38D
 * And choose appropriate IV construction method, constrain number
 * of invocations according to section 8.3 of SP 800-38D
 */
struct GCM_CTX {
	union {
		uint32_t d[4];
		uint8_t c[16];
	} block, Ej0;

	uint64_t aad_len;
	uint64_t count;
	size_t remainder;
};

/* Initialize the GCM context structure. */
void DCRYPTO_gcm_init(struct GCM_CTX *ctx, uint32_t key_bits,
		      const uint8_t *key, const uint8_t *iv, size_t iv_len);
/* Additional authentication data to include in the tag calculation. */
void DCRYPTO_gcm_aad(struct GCM_CTX *ctx, const uint8_t *aad_data, size_t len);
/* Encrypt & decrypt return the number of bytes written to out
 * (always an integral multiple of 16), or -1 on error.  These functions
 * may be called repeatedly with incremental data.
 *
 * NOTE: if in_len is not a integral multiple of 16, then out_len must
 * be atleast in_len - (in_len % 16) + 16 bytes.
 */
int DCRYPTO_gcm_encrypt(struct GCM_CTX *ctx, uint8_t *out, size_t out_len,
			const uint8_t *in, size_t in_len);
int DCRYPTO_gcm_decrypt(struct GCM_CTX *ctx, uint8_t *out, size_t out_len,
			const uint8_t *in, size_t in_len);
/* Encrypt & decrypt a partial final block, if any.  These functions
 * return the number of bytes written to out (<= 15), or -1 on error.
 */
int DCRYPTO_gcm_encrypt_final(struct GCM_CTX *ctx, uint8_t *out,
			      size_t out_len);
int DCRYPTO_gcm_decrypt_final(struct GCM_CTX *ctx, uint8_t *out,
			      size_t out_len);
/* Compute the tag over AAD + encrypt or decrypt data, and return the
 * number of bytes written to tag.  Returns -1 on error.
 */
int DCRYPTO_gcm_tag(struct GCM_CTX *ctx, uint8_t *tag, size_t tag_len);
/* Cleanup secrets. */
void DCRYPTO_gcm_finish(struct GCM_CTX *ctx);

/* AES-CMAC-128
 * NIST Special Publication 800-38B, RFC 4493
 * K: 128-bit key, M: message, len: number of bytes in M
 * Writes 128-bit tag to T; returns 0 if an error is encountered and 1
 * otherwise.
 */
int DCRYPTO_aes_cmac(const uint8_t *K, const uint8_t *M, const uint32_t len,
		     uint32_t T[4]);
/* key: 128-bit key, M: message, len: number of bytes in M,
 *    T: tag to be verified
 * Returns 1 if the tag is correct and 0 otherwise.
 */
int DCRYPTO_aes_cmac_verify(const uint8_t *key, const uint8_t *M, const int len,
			    const uint32_t T[4]);

/*
 * SHA implementation.  This abstraction is backed by either a
 * software or hardware implementation.
 *
 * There could be only a single hardware SHA context in progress. The init
 * functions will try using the HW context, if available, unless 'sw_required'
 * is TRUE, in which case there will be no attempt to use the hardware for
 * this particular hashing session.
 */

void SHA1_hw_init(struct sha1_ctx *ctx);
void SHA256_hw_init(struct sha256_ctx *ctx);
const struct sha1_digest *SHA1_hw_hash(const void *data, size_t len,
				       struct sha1_digest *digest);
const struct sha256_digest *SHA256_hw_hash(const void *data, size_t len,
					   struct sha256_digest *digest);
#ifdef CONFIG_UPTO_SHA512
void SHA384_hw_init(struct sha384_ctx *ctx);
void SHA512_hw_init(struct sha512_ctx *ctx);
const struct sha384_digest *SHA384_hw_hash(const void *data, size_t len,
					   struct sha384_digest *digest);

const struct sha512_digest *SHA512_hw_hash(const void *data, size_t len,
					   struct sha512_digest *digest);
#endif

const uint8_t *DCRYPTO_SHA1_hash(const void *data, size_t n, uint8_t *digest);

/* TODO: remove dependency on board/cr50/dcrypto/dcrypto.h for RO. */
const uint8_t *DCRYPTO_SHA256_hash(const void *data, size_t n, uint8_t *digest);

/*
 *  HMAC. FIPS 198-1
 */
void HMAC_SHA256_hw_init(struct hmac_sha256_ctx *ctx, const void *key,
			      size_t len);
/* DCRYPTO HMAC-SHA256 final */
const struct sha256_digest *HMAC_SHA256_hw_final(struct hmac_sha256_ctx *ctx);

/*
 * BIGNUM utility methods.
 */
void DCRYPTO_bn_wrap(struct LITE_BIGNUM *b, void *buf, size_t len);

/*
 *  RSA.
 */

/* Largest supported key size for signing / encryption: 2048-bits.
 * Verification is a special case and supports 4096-bits (signing /
 * decryption could also support 4k-RSA, but is disabled since support
 * is not required, and enabling support would result in increased
 * stack usage for all key sizes.)
 */
#define RSA_BYTES_2K 256
#define RSA_BYTES_4K 512
#define RSA_WORDS_2K (RSA_BYTES_2K / sizeof(uint32_t))
#define RSA_WORDS_4K (RSA_BYTES_4K / sizeof(uint32_t))
#ifndef RSA_MAX_BYTES
#define RSA_MAX_BYTES RSA_BYTES_2K
#endif
#define RSA_MAX_WORDS (RSA_MAX_BYTES / sizeof(uint32_t))
#define RSA_F4	      65537

struct RSA {
	uint32_t e;
	struct LITE_BIGNUM N;
	struct LITE_BIGNUM d;
};

enum padding_mode {
	PADDING_MODE_PKCS1 = 0,
	PADDING_MODE_OAEP = 1,
	PADDING_MODE_PSS = 2,
	/* USE OF NULL PADDING IS NOT RECOMMENDED.
	 * SUPPORT EXISTS AS A REQUIREMENT FOR TPM2 OPERATION. */
	PADDING_MODE_NULL = 3
};

/* RSA support, FIPS PUB 186-4 *
 * Calculate r = m ^ e mod N
 */
int DCRYPTO_rsa_encrypt(struct RSA *rsa, uint8_t *out, uint32_t *out_len,
			const uint8_t *in, uint32_t in_len,
			enum padding_mode padding, enum hashing_mode hashing,
			const char *label);

/* Calculate r = m ^ d mod N
 * return 0 if error
 */
int DCRYPTO_rsa_decrypt(struct RSA *rsa, uint8_t *out, uint32_t *out_len,
			const uint8_t *in, const uint32_t in_len,
			enum padding_mode padding, enum hashing_mode hashing,
			const char *label);

/* Calculate r = m ^ d mod N
 * return 0 if error
 */
int DCRYPTO_rsa_sign(struct RSA *rsa, uint8_t *out, uint32_t *out_len,
		     const uint8_t *in, const uint32_t in_len,
		     enum padding_mode padding, enum hashing_mode hashing);

/* Calculate r = m ^ e mod N
 * return 0 if error
 */
int DCRYPTO_rsa_verify(const struct RSA *rsa, const uint8_t *digest,
		       uint32_t digest_len, const uint8_t *sig,
		       const uint32_t sig_len, enum padding_mode padding,
		       enum hashing_mode hashing);

/* Calculate n = p * q, d = e ^ -1 mod phi. */
int DCRYPTO_rsa_key_compute(struct LITE_BIGNUM *N, struct LITE_BIGNUM *d,
			    struct LITE_BIGNUM *p, struct LITE_BIGNUM *q,
			    uint32_t e);

/*
 *  EC.
 */

/**
 * Check if point is on NIST P-256 curve
 *
 * @param x point coordinate
 * @param y point coordinate
 * @return DCRYPTO_OK if (x,y) is a valid point, DCRYPTO_FAIL otherwise
 */
enum dcrypto_result DCRYPTO_p256_is_valid_point(const p256_int *x,
						const p256_int *y);

/* DCRYPTO_p256_base_point_mul sets {out_x,out_y} = nG, where n is < the
 * order of the group.
 */
int DCRYPTO_p256_base_point_mul(p256_int *out_x, p256_int *out_y,
				const p256_int *n);

/**
 * DCRYPTO_p256_point_mul sets {out_x,out_y} = n*{in_x,in_y}, where n is <
 * the order of the group. Prior to computation check than {in_x,in_y} is
 * on NIST P-256 curve.
 *
 * @param out_x output shared coordinate x
 * @param out_y output shared coordinate y
 * @param n private key
 * @param in_x input public point x
 * @param in_y input public point y
 * @return 1 if success
 */
int DCRYPTO_p256_point_mul(p256_int *out_x, p256_int *out_y, const p256_int *n,
			   const p256_int *in_x, const p256_int *in_y);
/*
 * Key selection based on FIPS-186-4, section B.4.2 (Key Pair
 * Generation by Testing Candidates).
 * Produce uniform private key from seed.
 * If x or y is NULL, the public key part is not computed.
 * Returns !0 on success.
 */
int DCRYPTO_p256_key_from_bytes(p256_int *x, p256_int *y, p256_int *d,
				const uint8_t bytes[P256_NBYTES]);

/**
 * Pair-wise consistency test for private and public key.
 *
 * @param drbg - DRBG to use for nonce generation
 * @param d - private key (scalar)
 * @param x - public key part
 * @param y - public key part
 * @return !0 on success
 */
int DCRYPTO_p256_key_pwct(struct drbg_ctx *drbg, const p256_int *d,
			  const p256_int *x, const p256_int *y);

/* P256 based integration encryption (DH+AES128+SHA256).
 * Not FIPS 140-2 compliant, not used other than for tests
 * Authenticated data may be provided, where the first auth_data_len
 * bytes of in will be authenticated but not encrypted. *
 * Supports in-place encryption / decryption. *
 * The output format is:
 * 0x04 || PUBKEY || AUTH_DATA || AES128_CTR(PLAINTEXT) ||
 *         HMAC_SHA256(AUTH_DATA || CIPHERTEXT)
 */
size_t DCRYPTO_ecies_encrypt(void *out, size_t out_len, const void *in,
			     size_t in_len, size_t auth_data_len,
			     const uint8_t *iv, const p256_int *pub_x,
			     const p256_int *pub_y, const uint8_t *salt,
			     size_t salt_len, const uint8_t *info,
			     size_t info_len);
size_t DCRYPTO_ecies_decrypt(void *out, size_t out_len, const void *in,
			     size_t in_len, size_t auth_data_len,
			     const uint8_t *iv, const p256_int *d,
			     const uint8_t *salt, size_t salt_len,
			     const uint8_t *info, size_t info_len);

/*
 * HKDF as per RFC 5869. Mentioned as conforming NIST SP 800-56C Rev.1
 * [RFC 5869] specifies a version of the above extraction-then-expansion
 * key-derivation procedure using HMAC for both the extraction and expansion
 * steps.
 */
int DCRYPTO_hkdf(uint8_t *OKM, size_t OKM_len, const uint8_t *salt,
		 size_t salt_len, const uint8_t *IKM, size_t IKM_len,
		 const uint8_t *info, size_t info_len);

/*
 *  BN.
 */

/* Apply Miller-Rabin test for prime candidate p.
 * Returns 1 if test passed, 0 otherwise
 */
int DCRYPTO_bn_generate_prime(struct LITE_BIGNUM *p);
void DCRYPTO_bn_wrap(struct LITE_BIGNUM *b, void *buf, size_t len);
void DCRYPTO_bn_mul(struct LITE_BIGNUM *c, const struct LITE_BIGNUM *a,
		    const struct LITE_BIGNUM *b);
int DCRYPTO_bn_div(struct LITE_BIGNUM *quotient, struct LITE_BIGNUM *remainder,
		   const struct LITE_BIGNUM *input,
		   const struct LITE_BIGNUM *divisor);

/*
 * ASN.1 DER
 */
size_t DCRYPTO_asn1_sigp(uint8_t *buf, const p256_int *r, const p256_int *s);
size_t DCRYPTO_asn1_pubp(uint8_t *buf, const p256_int *x, const p256_int *y);

/*
 *  X509.
 */
/* DCRYPTO_x509_verify verifies that the provided X509 certificate was issued
 * by the specified certifcate authority.
 *
 * cert is a pointer to a DER encoded X509 certificate, as specified
 * in https://tools.ietf.org/html/rfc5280#section-4.1.  In ASN.1
 * notation, the certificate has the following structure:
 *
 *   Certificate  ::=  SEQUENCE  {
 *        tbsCertificate       TBSCertificate,
 *        signatureAlgorithm   AlgorithmIdentifier,
 *        signatureValue       BIT STRING  }
 *
 *   TBSCertificate  ::=  SEQUENCE  { }
 *   AlgorithmIdentifier  ::=  SEQUENCE  { }
 *
 * where signatureValue = SIGN(HASH(tbsCertificate)), with SIGN and
 * HASH specified by signatureAlgorithm.
 * Accepts only certs with OID: sha256WithRSAEncryption:
 * 30 0d 06 09 2a 86 48 86 f7 0d 01 01 0b 05 00
 */
int DCRYPTO_x509_verify(const uint8_t *cert, size_t len,
			const struct RSA *ca_pub_key);

/* Generate U2F Certificate and sign it
 * Use ECDSA with NIST P-256 curve, and SHA2-256 digest
 * @param d: key handle, used for NIST SP 800-90A HMAC DRBG
 * @param pk_x, pk_y: public key
 * @param serial: serial number for certificate
 * @param name: certificate issuer and subject
 * @param cert: output buffer for certificate
 * @param n: max size of cert
 */
int DCRYPTO_x509_gen_u2f_cert_name(const p256_int *d, const p256_int *pk_x,
				   const p256_int *pk_y, const p256_int *serial,
				   const char *name, uint8_t *cert,
				   const int n);

/* Generate U2F Certificate with DCRYPTO_x509_gen_u2f_cert_name
 * Providing certificate issuer as BOARD or U2F
 * @param d: key handle, used for NIST SP 800-90A HMAC DRBG
 * @param pk_x, pk_y: public key
 * @param serial: serial number for certificate
 * @param name: certificate issuer and subject
 * @param cert: output buffer for certificate
 * @param n: max size of cert
 */
int DCRYPTO_x509_gen_u2f_cert(const p256_int *d, const p256_int *pk_x,
			      const p256_int *pk_y, const p256_int *serial,
			      uint8_t *cert, const int n);

/*
 * Memory related functions.
 */
enum dcrypto_result DCRYPTO_equals(const void *a, const void *b, size_t len);

/*
 * Key-ladder and application key related functions.
 */
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

int DCRYPTO_ladder_compute_frk2(size_t major_fw_version, uint8_t *frk2);
void DCRYPTO_ladder_revoke(void);

int DCRYPTO_appkey_init(enum dcrypto_appid id);
void DCRYPTO_appkey_finish(void);
int DCRYPTO_appkey_derive(enum dcrypto_appid appid, const uint32_t input[8],
			  uint32_t output[8]);

/*
 * Encrypt/decrypt a flat blob.
 *
 * Encrypt or decrypt the input buffer, and write the correspondingly
 * ciphered output to out.  The number of bytes produced is equal to
 * the number of input bytes.  Note that the input and output pointers
 * MUST be word-aligned.
 *
 * This API is expected to be applied to a single contiguous region.

 * WARNING: A given salt/"in" pair MUST be unique, i.e. re-using a
 * salt with a logically different input buffer is catastrophic.  An
 * example of a suitable salt is one that is derived from "in", e.g. a
 * digest of the input data.
 *
 * @param appid the application-id of the calling context.
 * @param salt pointer to a unique value to be associated with this blob,
 *	       used for derivation of the proper IV, the size of the value
 *	       is as defined by DCRYPTO_CIPHER_SALT_SIZE above.
 * @param out Destination pointer where to write plaintext / ciphertext.
 * @param in  Source pointer where to read ciphertext / plaintext.
 * @param len Number of bytes to read from in / write to out.
 * @return non-zero on success, and zero otherwise.
 */
int DCRYPTO_app_cipher(enum dcrypto_appid appid, const void *salt, void *out,
		       const void *in, size_t len);

/*
 * Query whether Key Ladder is enabled.
 *
 * @return 1 if Key Ladder is enabled, and 0 otherwise.
 */
int DCRYPTO_ladder_is_enabled(void);

/**
 * Random number generation functions.
 */

/**
 * Returns random number from TRNG with indication wherever reading is valid.
 * This is different from rand() which doesn't provide any indication.
 * High 32-bits set to zero in case of error; otherwise value >> 32 == 1
 * Use of uint64_t vs. struct results in more efficient code.
 * Random is passed continuous TRNG health tests.
 *
 * @return uint64_t, low 32 bits - random  high 32 bits - validity status
 */
uint64_t fips_trng_rand32(void);

/**
 * Return true if fips_trng_rand() result contains valid random from TRNG.
 * @param rand value from fips_trng_rand32() or read_rand()
 *
 * @return true if rand contains valid random
 */

static inline bool rand_valid(uint64_t rand)
{
	return (rand >> 32) != 0;
}

/**
 * Fill buffer with FIPS health checked randoms directly from TRNG.
 *
 * @param buffer buffer to fill
 * @param len size of buffer in bytes
 * @return true if successful
 * @return false if TRNG failed, values didn't pass health test
 *         or module crypto failed
 */
bool fips_trng_bytes(void *buffer, size_t len)
	__attribute__((warn_unused_result));

/**
 * Fill buffer with random bytes from FIPS-compliant HMAC_DRBG_SHA256,
 * instantiated during system start-up and reseeded as needed.
 *
 * @param buffer buffer to fill
 * @param len size of buffer in bytes
 * @return true if successful
 * @return false if any errors occurred or module crypto failed
 */
bool fips_rand_bytes(void *buffer, size_t len)
	__attribute__((warn_unused_result));

#ifdef __cplusplus
}
#endif

#endif /* ! __EC_BOARD_CR50_DCRYPTO_DCRYPTO_H */
