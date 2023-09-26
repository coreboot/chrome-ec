/* Copyright 2015 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/*
 * TODO(ngm): only the NIST-P256 curve is currently supported.
 */

#include "CryptoEngine.h"
#include "TPMB.h"

#include "console.h"
#include "fips_rand.h"
#include "util.h"
#include "dcrypto.h"

TPM2B_BYTE_VALUE(4);
TPM2B_BYTE_VALUE(32);

BOOL _cpri__EccIsPointOnCurve(TPM_ECC_CURVE curve_id, TPMS_ECC_POINT *q)
{
	p256_int x, y;

	switch (curve_id) {
	case TPM_ECC_NIST_P256:

		if (!p256_from_be_bin_size(q->x.b.buffer, q->x.b.size, &x) ||
		    !p256_from_be_bin_size(q->y.b.buffer, q->y.b.size, &y))
			return FALSE;

		if (DCRYPTO_p256_is_valid_point(&x, &y) == DCRYPTO_OK)
			return TRUE;
		else
			return FALSE;
	default:
		return FALSE;
	}
}

/* out = n1*G, or out = n2*in */
CRYPT_RESULT _cpri__EccPointMultiply(
	TPMS_ECC_POINT *out, TPM_ECC_CURVE curve_id,
	TPM2B_ECC_PARAMETER *n1, TPMS_ECC_POINT *in, TPM2B_ECC_PARAMETER *n2)
{
	enum dcrypto_result result;
	p256_int n, in_x, in_y, out_x, out_y;

	switch (curve_id) {
	case TPM_ECC_NIST_P256:
		if ((n1 != NULL && n2 != NULL) || (n1 == NULL && n2 == NULL))
			/* Only one of n1 or n2 must be specified. */
			return CRYPT_PARAMETER;
		if ((n2 != NULL && in == NULL) || (n2 == NULL && in != NULL))
			return CRYPT_PARAMETER;
		if (n1 != NULL &&
		    !p256_from_be_bin_size(n1->b.buffer, n1->b.size, &n))
			return CRYPT_PARAMETER;
		if (in != NULL && !_cpri__EccIsPointOnCurve(curve_id, in))
			return CRYPT_POINT;
		if (n2 != NULL &&
		    !p256_from_be_bin_size(n2->b.buffer, n2->b.size, &n))
			return CRYPT_PARAMETER;

		if (n1 != NULL) {
			result =
				DCRYPTO_p256_base_point_mul(&out_x, &out_y, &n);
		} else {
			if (!p256_from_be_bin_size(in->x.b.buffer, in->x.b.size,
						   &in_x) ||
			    !p256_from_be_bin_size(in->y.b.buffer, in->y.b.size,
						   &in_y))
				return CRYPT_PARAMETER;

			result = DCRYPTO_p256_point_mul(&out_x, &out_y, &n,
							&in_x, &in_y);
		}

		p256_clear(&n);
		if (result == DCRYPTO_OK) {
			out->x.b.size = sizeof(p256_int);
			out->y.b.size = sizeof(p256_int);
			p256_to_bin(&out_x, out->x.b.buffer);
			p256_to_bin(&out_y, out->y.b.buffer);
			return CRYPT_SUCCESS;
		} else {
			return CRYPT_NO_RESULT;
		}
	default:
		return CRYPT_PARAMETER;
	}
}

/* The name field of TPM2B_NAME is a TPMT_HA */
static const uint8_t TPM2_ECC_EK_NAME_TEMPLATE[] = {
	/* TPM_ALG_SHA256 in big endian. */
	0x00, 0x0b,
	/* SHA256 digest of the default template TPMT_PUBLIC. */
	0x0f, 0x12, 0x77, 0xa2, 0xf3, 0xf3, 0x82, 0xe7,
	0xf7, 0x5d, 0xb4, 0x66, 0xfa, 0xc2, 0x34, 0x18,
	0x2a, 0x8d, 0x62, 0xf9, 0x7d, 0xfb, 0xaa, 0xe7,
	0xb0, 0x6f, 0xdf, 0x52, 0xbd, 0xa5, 0x14, 0x67
};
BUILD_ASSERT(sizeof(TPM2_ECC_EK_NAME_TEMPLATE) == 2 + SHA256_DIGEST_SIZE);

/* The first 4 bytes of the wrong template used in factory
 * c2e0319340fb48f102539ea98363f81e2d306e918dd778abf05473a2a60dae09
 */
static const TPM2B_4_BYTE_VALUE TPM2_ECC_EK_NAME_CR50 = {
	.t = {
		.size = 4,
		.buffer = {
			0xc2, 0xe0, 0x31, 0x93
		},
	}
};

/* Key generation based on FIPS-186.4 section B.1.2 (Key Generation by
 * Testing Candidates) */
CRYPT_RESULT _cpri__GenerateKeyEcc(
	TPMS_ECC_POINT *q, TPM2B_ECC_PARAMETER *d,
	TPM_ECC_CURVE curve_id,	TPM_ALG_ID hash_alg,
	TPM2B *seed, const char *label,	TPM2B *extra, UINT32 *counter)
{
	TPM2B_4_BYTE_VALUE marshaled_counter = { .t = {4} };
	TPM2B_32_BYTE_VALUE local_seed = { .t = {32} };
	TPM2B *local_extra;
	uint32_t count = 0;
	uint8_t key_bytes[P256_NBYTES];
	struct hmac_sha256_ctx hmac;

	if (curve_id != TPM_ECC_NIST_P256)
		return CRYPT_PARAMETER;

	/* extra may be empty, but seed must be specified. */
	if (seed == NULL || seed->size < PRIMARY_SEED_SIZE) {
		/* TODO(b/262324344). Remove when zero sized EPS fixed. */
		cprintf(CC_EXTENSION, "%s: seed size %d invalid\n", __func__,
			(seed) ? seed->size : -1);
		cflush();
		return CRYPT_PARAMETER;
	}

	if (counter != NULL)
		count = *counter;
	if (count == 0)
		count++;

	/* Hash down the primary seed for ECC key generation, so that
	 * the derivation tree is distinct from RSA key derivation. */
	if (DCRYPTO_hw_hmac_sha256_init(&hmac, seed->buffer, seed->size) !=
	    DCRYPTO_OK)
		return CRYPT_NO_RESULT;

	HMAC_SHA256_update(&hmac, "ECC", 4);
	memcpy(local_seed.t.buffer, HMAC_SHA256_final(&hmac),
	       local_seed.t.size);
	always_memset(&hmac, 0, sizeof(hmac));
	/* b/35576109: the personalize code uses only the first 4 bytes
	 * of extra.
	 */
	if (extra && extra->size == sizeof(TPM2_ECC_EK_NAME_TEMPLATE) &&
		memcmp(extra->buffer,
		       TPM2_ECC_EK_NAME_TEMPLATE,
		       sizeof(TPM2_ECC_EK_NAME_TEMPLATE)) == 0) {
		local_extra = (TPM2B *) &TPM2_ECC_EK_NAME_CR50.b;
	} else {
		local_extra = extra;
	}

	for (; count != 0; count++) {
		p256_int x, y, key;
		enum dcrypto_result result;

		memcpy(marshaled_counter.t.buffer, &count, sizeof(count));
		_cpri__KDFa(hash_alg, &local_seed.b, label, local_extra,
			&marshaled_counter.b, sizeof(key_bytes) * 8, key_bytes,
			NULL, FALSE);

		result = DCRYPTO_p256_key_from_bytes(&x, &y, &key, key_bytes);
		if (result == DCRYPTO_RETRY)
			continue;
		else if (result == DCRYPTO_OK) {
			q->x.b.size = sizeof(p256_int);
			p256_to_bin(&x, q->x.b.buffer);

			q->y.b.size = sizeof(p256_int);
			p256_to_bin(&y, q->y.b.buffer);

			d->b.size = sizeof(p256_int);
			p256_to_bin(&key, d->b.buffer);
			p256_clear(&key);
			break;
		} else {
			/* Any other value for result is failure. */
			count = 0;
			break;
		}
	}

	always_memset(local_seed.t.buffer, 0, local_seed.t.size);
	always_memset(key_bytes, 0, sizeof(key_bytes));

	if (counter != NULL)
		*counter = count;
	if (count == 0) {
		FAIL(FATAL_ERROR_CRYPTO);
		return CRYPT_HW_FAILURE; /* Produce TPM_RC_FAILURE */
	}

	return CRYPT_SUCCESS;
}

CRYPT_RESULT _cpri__SignEcc(
	TPM2B_ECC_PARAMETER *r, TPM2B_ECC_PARAMETER *s,
	TPM_ALG_ID scheme, TPM_ALG_ID hash_alg, TPM_ECC_CURVE curve_id,
	TPM2B_ECC_PARAMETER *d, TPM2B *digest, TPM2B_ECC_PARAMETER *k)
{
	uint8_t digest_local[sizeof(p256_int)];
	const size_t digest_len = MIN(digest->size, sizeof(digest_local));
	p256_int p256_digest, key, p256_r, p256_s;
	enum dcrypto_result result;

	if (curve_id != TPM_ECC_NIST_P256)
		return CRYPT_PARAMETER;

	switch (scheme) {
	case TPM_ALG_ECDSA: {
		if (!p256_from_be_bin_size(d->b.buffer, d->b.size, &key))
			return CRYPT_PARAMETER;

		/* Truncate / zero-pad the digest as appropriate. */
		memset(digest_local, 0, sizeof(digest_local));
		memcpy(digest_local + sizeof(digest_local) - digest_len,
		       digest->buffer, digest_len);
		p256_from_bin(digest_local, &p256_digest);

		result = DCRYPTO_p256_ecdsa_sign(&key, &p256_digest, &p256_r,
						 &p256_s);

		p256_clear(&key);
		r->b.size = sizeof(p256_int);
		s->b.size = sizeof(p256_int);
		p256_to_bin(&p256_r, r->b.buffer);
		p256_to_bin(&p256_s, s->b.buffer);

		if (result == DCRYPTO_OK)
			return CRYPT_SUCCESS;
		else
			return CRYPT_FAIL;
	}
	default:
		return CRYPT_PARAMETER;
	}
}

CRYPT_RESULT _cpri__ValidateSignatureEcc(
	TPM2B_ECC_PARAMETER *r,	TPM2B_ECC_PARAMETER *s,
	TPM_ALG_ID scheme, TPM_ALG_ID hash_alg,
	TPM_ECC_CURVE curve_id,	TPMS_ECC_POINT *q, TPM2B *digest)
{
	uint8_t digest_local[sizeof(p256_int)];
	const size_t digest_len = MIN(digest->size, sizeof(digest_local));
	p256_int p256_digest, q_x, q_y, p256_r, p256_s;

	if (curve_id != TPM_ECC_NIST_P256)
		return CRYPT_PARAMETER;

	switch (scheme) {
	case TPM_ALG_ECDSA: {
		if (!p256_from_be_bin_size(q->x.b.buffer, q->x.b.size, &q_x) ||
		    !p256_from_be_bin_size(q->y.b.buffer, q->y.b.size, &q_y) ||
		    !p256_from_be_bin_size(r->b.buffer, r->b.size, &p256_r) ||
		    !p256_from_be_bin_size(s->b.buffer, s->b.size, &p256_s))
			return CRYPT_PARAMETER;

		/* Truncate / zero-pad the digest as appropriate. */
		memset(digest_local, 0, sizeof(digest_local));
		memcpy(digest_local + sizeof(digest_local) - digest_len,
		       digest->buffer, digest_len);
		p256_from_bin(digest_local, &p256_digest);

		if (DCRYPTO_p256_ecdsa_verify(&q_x, &q_y, &p256_digest, &p256_r,
					      &p256_s) == DCRYPTO_OK)
			return CRYPT_SUCCESS;
		return CRYPT_FAIL;
	}
	default:
		return CRYPT_PARAMETER;
	}
}

CRYPT_RESULT _cpri__GetEphemeralEcc(TPMS_ECC_POINT *q, TPM2B_ECC_PARAMETER *d,
				TPM_ECC_CURVE curve_id)
{
	enum dcrypto_result result;
	uint8_t key_bytes[P256_NBYTES] __aligned(4);
	p256_int x, y, key;

	if (curve_id != TPM_ECC_NIST_P256)
		return CRYPT_PARAMETER;

	if (!fips_rand_bytes(key_bytes, sizeof(key_bytes)))
		return CRYPT_FAIL;

	result = DCRYPTO_p256_key_from_bytes(&x, &y, &key, key_bytes);

	always_memset(key_bytes, 0, sizeof(key_bytes));

	if (result == DCRYPTO_OK) {
		q->x.b.size = sizeof(p256_int);
		q->y.b.size = sizeof(p256_int);
		p256_to_bin(&x, q->x.b.buffer);
		p256_to_bin(&y, q->y.b.buffer);

		d->b.size = sizeof(p256_int);
		p256_to_bin(&key, d->b.buffer);
		p256_clear(&key);

		return CRYPT_SUCCESS;
	} else {
		return CRYPT_FAIL;
	}
}

#ifdef CRYPTO_TEST_SETUP

#include "console.h"
#include "extension.h"

enum {
	TEST_SIGN = 0,
	TEST_VERIFY = 1,
	TEST_KEYGEN = 2,
	TEST_KEYDERIVE = 3,
	TEST_POINT = 4,
	TEST_VERIFY_ANY = 5,
	TEST_SIGN_ANY = 6,
};

struct TPM2B_ECC_PARAMETER_aligned {
	uint16_t pad;
	TPM2B_ECC_PARAMETER d;
} __packed __aligned(4);

struct TPM2B_MAX_BUFFER_aligned {
	uint16_t pad;
	TPM2B_MAX_BUFFER d;
} __packed __aligned(4);

static const struct TPM2B_ECC_PARAMETER_aligned NIST_P256_d = {
	.d = {
		.t = {32, {
				0xfc, 0x44, 0x1e, 0x07, 0x74, 0x4e, 0x48, 0xf1,
				0x09, 0xb7, 0xe6, 0x6b, 0x29, 0x48, 0x2f, 0x7b,
				0x7e, 0x3e, 0xc9, 0x1f, 0xa2, 0x7f, 0xd4, 0x87,
				0x09, 0x91, 0xb2, 0x89, 0xfe, 0xa0, 0xd2, 0x0a
			}
		}
	}
};

static const struct TPM2B_ECC_PARAMETER_aligned NIST_P256_qx = {
	.d = {
		.t = {32, {
				0x12, 0xc3, 0xd6, 0xa2, 0x67, 0x9c, 0xa8, 0xee,
				0x3c, 0x4d, 0x92, 0x7f, 0x20, 0x4e, 0xd5, 0xbc,
				0xb4, 0x57, 0x7a, 0x04, 0xb0, 0xac, 0x02, 0xb2,
				0xa3, 0x6a, 0xb3, 0xe9, 0xe1, 0x07, 0x81, 0xde
			}
		}
	}
};

static const struct TPM2B_ECC_PARAMETER_aligned NIST_P256_qy = {
	.d = {
		.t = {32, {
				0x5c, 0x85, 0xad, 0x74, 0x13, 0x97, 0x11, 0x72,
				0xfc, 0xa5, 0x73, 0x8f, 0xee, 0x9d, 0x0e, 0x7b,
				0xc5, 0x9f, 0xfd, 0x8a, 0x62, 0x6d, 0x68, 0x9b,
				0xc6, 0xcc, 0xa4, 0xb5, 0x86, 0x65, 0x52, 0x1d
			}
		}
	}
};

#define MAX_MSG_BYTES MAX_DIGEST_BUFFER

static int point_equals(const TPMS_ECC_POINT *a, const TPMS_ECC_POINT *b)
{
	int diff = 0;

	diff = a->x.b.size - b->x.b.size;
	diff |= a->y.b.size - b->y.b.size;
	diff |= DCRYPTO_equals(a->x.b.buffer, b->x.b.buffer, a->x.b.size) -
		DCRYPTO_OK;
	diff |= DCRYPTO_equals(a->y.b.buffer, b->y.b.buffer, a->y.b.size) -
		DCRYPTO_OK;

	return !diff;
}

/**
 * Parse TPM_2B data, copy data to destination, update position
 * @param ptr - pointer to current position, updated if parsed successfully
 * @param cmd_size - remaining number of bytes, [in/out]
 * @param len [out] - length of parsed data
 * @param out [out] - destination buffer to copy data
 * @param out_len - size of destination buffer
 */
static int parse_2b(uint8_t **ptr, size_t *cmd_size, uint16_t *len,
		    uint8_t *out, size_t out_len)
{
	uint16_t length;
	uint8_t *local_ptr = *ptr;

	*len = 0;
	/* if there is no space for 2 bytes of length, exit */
	if (*cmd_size < sizeof(length))
		return 0;
	length = local_ptr[0];
	length = (length << 8) | local_ptr[1];
	*cmd_size -= sizeof(length);
	local_ptr += 2;
	if (*cmd_size < length || length > out_len) {
		ccprintf("%s: length = %d (0x%4x),"
			 " data available = %d, out_len = %d\n",
			 __func__, length, length, *cmd_size, out_len);
		*cmd_size = 0;
		return 0;
	}
	*len = length;
	memcpy(out, local_ptr, length);
	*ptr = local_ptr + length;
	*cmd_size -= length;
	return 1;
}

static void store_2b(uint8_t **ptr, size_t *rsp_size, TPM2B_ECC_PARAMETER *p)
{
	uint8_t *out = *ptr;

	*out++ = p->t.size >> 8;
	*out++ = p->t.size & 0xff;
	memcpy(out, p->t.buffer, p->t.size);
	*ptr = out + p->t.size;
	*rsp_size += 2 + p->t.size;
}

static void ecc_command_handler(void *cmd_body, size_t cmd_size,
			size_t *response_size_out)
{
	uint8_t *cmd;
	uint8_t op;
	uint8_t curve_id;
	uint8_t sign_mode = 0;
	uint8_t hashing = 0;
	TPM2B_SEED seed;

	struct TPM2B_MAX_BUFFER_aligned digest;
	uint8_t *out = (uint8_t *) cmd_body;
	size_t *response_size = response_size_out;

	TPMS_ECC_POINT q;
	TPM2B_ECC_PARAMETER d;
	struct TPM2B_ECC_PARAMETER_aligned r;
	struct TPM2B_ECC_PARAMETER_aligned s;
	/**
	 * Command formats:
	 *
	 * TEST_SIGN:
	 * OP | CURVE_ID | SIGN_MODE | HASHING | DIGEST_LEN | DIGEST
	 *    @returns 0/1 | R_LEN | R | S_LEN | S
	 *
	 * TEST_SIGN_ANY:
	 * OP | CURVE_ID | SIGN_MODE | HASHING | DIGEST_LEN | DIGEST |
	 *   D_LEN | D
	 *    @returns 0/1 | R_LEN | R | S_LEN | S
	 *
	 * TEST_VERIFY:
	 * OP | CURVE_ID | SIGN_MODE | HASHING | R_LEN | R | S_LEN | S
	 *   DIGEST_LEN | DIGEST
	 *    @returns 0/1 |  if successful
	 *
	 * TEST_VERIFY_ANY:
	 * OP | CURVE_ID | SIGN_MODE | HASHING | R_LEN | R | S_LEN | S |
	 *   DIGEST_LEN | DIGEST | QX_LEN | QX | QY_LEN | QY
	 *    @returns 1 if successful
	 *
	 * TEST_KEYDERIVE :
	 * OP | CURVE_ID | SEED_LEN | SEED
	 *    @returns 1 if successful
	 *
	 * TEST_POINT:
	 * OP | CURVE_ID | QX_LEN | QX | QY_LEN | QY
	 *    @returns 1 if point is on curve
	 *
	 * TEST_KEYGEN:
	 * OP | CURVE_ID
	 *    @returns 0/1 | D_LEN | D | QX_LEN | QX | QY_LEN | QY
	 *
	 * Field size:
	 * FIELD          LENGTH
	 * OP             1
	 * CURVE_ID       1
	 * SIGN_MODE      1
	 * HASHING        1
	 * MSG_LEN        2 (big endian)
	 * MSG            MSG_LEN
	 * SEED_LEN       2 (big endian)
	 * SEED           SEED_LEN
	 * R_LEN          2 (big endian)
	 * R              R_LEN
	 * S_LEN          2 (big endian)
	 * S              S_LEN
	 * DIGEST_LEN     2 (big endian)
	 * DIGEST         DIGEST_LEN
	 * D_LEN          2 (big endian)
	 * D              D_LEN
	 * QX_LEN         2 (big endian)
	 * QX             QX_LEN
	 * QY_LEN         2 (big endian)
	 * QY             QX_LEN
	 */
	*response_size = 0;
	if (cmd_size < 2)
		return;

	cmd = (uint8_t *)cmd_body;
	op = *cmd++;
	curve_id = *cmd++;
	cmd_size -= 2;

	if (op == TEST_SIGN || op == TEST_VERIFY || op == TEST_VERIFY_ANY ||
	    op == TEST_SIGN_ANY) {
		if (cmd_size >= 2) {
			sign_mode = *cmd++;
			hashing = *cmd++;
			cmd_size -= 2;
		} else
			return;
	}

	if (op == TEST_SIGN || op == TEST_SIGN_ANY) {
		if (!parse_2b(&cmd, &cmd_size, &digest.d.t.size,
			      digest.d.t.buffer, sizeof(digest.d.t.buffer)))
			return;
		if (op == TEST_SIGN_ANY &&
		    !parse_2b(&cmd, &cmd_size, &d.t.size, d.t.buffer,
			      sizeof(d.t.buffer)))
			return;
		if (op == TEST_SIGN) {
			if (curve_id == TPM_ECC_NIST_P256)
				d = NIST_P256_d.d;
			else
				return;
		}
	}

	if (op == TEST_KEYDERIVE &&
	    !parse_2b(&cmd, &cmd_size, &seed.t.size, seed.t.buffer,
		      sizeof(seed.t.buffer)))
		return;

	if (op == TEST_VERIFY || op == TEST_VERIFY_ANY) {
		if (!parse_2b(&cmd, &cmd_size, &r.d.t.size, r.d.t.buffer,
			      sizeof(r.d.t.buffer)))
			return;
		if (!parse_2b(&cmd, &cmd_size, &s.d.t.size, s.d.t.buffer,
			      sizeof(s.d.t.buffer)))
			return;
		if (!parse_2b(&cmd, &cmd_size, &digest.d.t.size,
			      digest.d.t.buffer, sizeof(digest.d.t.buffer)))
			return;
	}

	if (op == TEST_VERIFY_ANY || op == TEST_POINT) {
		if (!parse_2b(&cmd, &cmd_size, &q.x.t.size, q.x.t.buffer,
			      sizeof(q.x.t.buffer)))
			return;
		if (!parse_2b(&cmd, &cmd_size, &q.y.t.size, q.y.t.buffer,
			      sizeof(q.y.t.buffer)))
			return;
	} else {
		/* use fixed signature */
		q.x = NIST_P256_qx.d;
		q.y = NIST_P256_qy.d;
	}

	/* there should be no other data as command is parsed */
	if (cmd_size != 0)
		return;

	*out = 0;
	*response_size = 1;
	switch (op) {
	case TEST_SIGN:
	case TEST_SIGN_ANY:
		if (hashing != TPM_ALG_SHA256 && hashing != TPM_ALG_NULL)
			return;
		if (_cpri__SignEcc(&r.d, &s.d, sign_mode, hashing, curve_id, &d,
				   &digest.d.b, NULL) != CRYPT_SUCCESS) {
			ccprintf("test_sign: error signing\n");
			return;
		}
		*out++ = 1;
		store_2b(&out, response_size, &r.d);
		store_2b(&out, response_size, &s.d);
		break;
	case TEST_VERIFY:
	case TEST_VERIFY_ANY:
		if (_cpri__ValidateSignatureEcc(&r.d, &s.d, sign_mode, hashing,
						curve_id, &q,
						&digest.d.b) != CRYPT_SUCCESS) {
			ccprintf("test_verify: verification failed\n");
		} else
			*out = 1;
		return;
	case TEST_KEYGEN: {
		struct TPM2B_ECC_PARAMETER_aligned d_local;
		TPMS_ECC_POINT q_local;

		if (_cpri__GetEphemeralEcc(&q, &d_local.d, curve_id)
			!= CRYPT_SUCCESS)
			return;


		if (_cpri__EccIsPointOnCurve(curve_id, &q) != TRUE)
			return;

		/* Verify correspondence of secret with the public point. */
		if (_cpri__EccPointMultiply(
				&q_local, curve_id, &d_local.d,
				NULL, NULL) != CRYPT_SUCCESS)
			return;
		if (!point_equals(&q, &q_local))
			return;

		*out++ = 1;
		store_2b(&out, response_size, &d_local.d);
		store_2b(&out, response_size, &q.x);
		store_2b(&out, response_size, &q.y);
		return;
	}
	case TEST_KEYDERIVE: {
		struct TPM2B_ECC_PARAMETER_aligned d_local;
		TPMS_ECC_POINT q_local;
		const char *label = "ecc_test";

		if (_cpri__GenerateKeyEcc(&q, &d_local.d, curve_id, hashing,
					  &seed.b, label, NULL,
					  NULL) != CRYPT_SUCCESS)
			return;

		if (_cpri__EccIsPointOnCurve(curve_id, &q) != TRUE)
			return;

		/* Verify correspondence of secret with the public point. */
		if (_cpri__EccPointMultiply(&q_local, curve_id, &d_local.d,
					    NULL, NULL) != CRYPT_SUCCESS)
			return;

		if (!point_equals(&q, &q_local))
			return;

		*out = 1;
		break;
	}
	case TEST_POINT:
		*out = _cpri__EccIsPointOnCurve(curve_id, &q);
		break;

	default:
		break;
	}
}

DECLARE_EXTENSION_COMMAND(EXTENSION_ECC, ecc_command_handler);

#endif   /* CRYPTO_TEST_SETUP */
