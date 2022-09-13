/* Copyright 2015 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "dcrypto.h"
#include "internal.h"

#include "util.h"

#include <assert.h>

/* Extend the MSB throughout the word. */
static uint32_t msb_extend(uint32_t a)
{
	return 0u - (a >> 31);
}

/* Return 0xFF..FF if a is zero, and zero otherwise. */
static uint32_t is_zero(uint32_t a)
{
	return msb_extend(~a & (a - 1));
}

/* Select a or b based on mask.  Mask expected to be 0xFF..FF or 0. */
static uint32_t select(uint32_t mask, uint32_t a, uint32_t b)
{
	return (mask & a) | (~mask & b);
}

/* We use SHA256 context to store SHA1 context, so make sure it's ok. */
BUILD_ASSERT(sizeof(struct sha256_ctx) >= sizeof(struct sha1_ctx));

static enum dcrypto_result MGF1_xor(uint8_t *dst, uint32_t dst_len,
				    const uint8_t *seed, uint32_t seed_len,
				    enum hashing_mode hashing)
{
	union hash_ctx ctx;

	struct {
		uint8_t b3;
		uint8_t b2;
		uint8_t b1;
		uint8_t b0;
	} cnt;
	const uint8_t *digest;
	const size_t hash_size = DCRYPTO_hash_size(hashing);

	if (!hash_size)
		return DCRYPTO_FAIL;

	cnt.b0 = cnt.b1 = cnt.b2 = cnt.b3 = 0;
	while (dst_len) {
		size_t i;
		enum dcrypto_result result;

		result = DCRYPTO_hw_hash_init(&ctx, hashing);
		if (result != DCRYPTO_OK)
			return result;

		HASH_update(&ctx, seed, seed_len);
		HASH_update(&ctx, (uint8_t *)&cnt, sizeof(cnt));
		digest = HASH_final(&ctx)->b8;
		for (i = 0; i < dst_len && i < hash_size; ++i)
			*dst++ ^= *digest++;
		dst_len -= i;
		if (!++cnt.b0)
			++cnt.b1;
	}
	return DCRYPTO_OK;
}

/*
 * struct OAEP {                  // MSB to LSB.
 *      uint8_t zero;
 *      uint8_t seed[HASH_SIZE];
 *      uint8_t phash[HASH_SIZE];
 *      uint8_t PS[];             // Variable length (optional) zero-pad.
 *      uint8_t one;              // 0x01, message demarcator.
 *      uint8_t msg[];            // Input message.
 * };
 */
/* encrypt */
static enum dcrypto_result oaep_pad(uint8_t *output, uint32_t output_len,
				    const uint8_t *msg, uint32_t msg_len,
				    enum hashing_mode hashing,
				    const char *label)
{
	const size_t hash_size = DCRYPTO_hash_size(hashing);
	uint8_t *const seed = output + 1;
	uint8_t *const phash = seed + hash_size;
	uint8_t *const PS = phash + hash_size;
	const uint32_t max_msg_len = output_len - 2 - 2 * hash_size;
	const uint32_t ps_len = max_msg_len - msg_len;
	uint8_t *const one = PS + ps_len;
	union hash_ctx ctx;
	enum dcrypto_result result;

	if (!hash_size)
		return DCRYPTO_FAIL;
	if (output_len < 2 + 2 * hash_size)
		return DCRYPTO_FAIL; /* Key size too small for chosen hash. */
	if (msg_len > output_len - 2 - 2 * hash_size)
		return DCRYPTO_FAIL; /* Input message too large for key size. */

	always_memset(output, 0, output_len);
	if (!fips_rand_bytes(seed, hash_size))
		return DCRYPTO_FAIL;

	result = DCRYPTO_hw_hash_init(&ctx, hashing);
	if (result != DCRYPTO_OK)
		return result;

	HASH_update(&ctx, label, label ? strlen(label) + 1 : 0);
	memcpy(phash, HASH_final(&ctx)->b8, hash_size);
	*one = 1;
	memcpy(one + 1, msg, msg_len);
	result = MGF1_xor(phash, hash_size + 1 + max_msg_len, seed, hash_size,
			  hashing);
	result |= MGF1_xor(seed, hash_size, phash, hash_size + 1 + max_msg_len,
			   hashing);

	if (result != DCRYPTO_OK)
		return DCRYPTO_FAIL;

	return result;
}

/* decrypt */
static enum dcrypto_result check_oaep_pad(uint8_t *out, size_t *out_len,
			uint8_t *padded, size_t padded_len,
			enum hashing_mode hashing, const char *label)
{
	const size_t hash_size = DCRYPTO_hash_size(hashing);
	uint8_t *seed = padded + 1;
	uint8_t *phash = seed + hash_size;
	uint8_t *PS = phash + hash_size;
	const uint32_t max_msg_len = padded_len - 2 - 2 * hash_size;
	union hash_ctx ctx;
	size_t one_index = 0;
	uint32_t looking_for_one_byte = ~0;
	int bad;
	size_t i;
	enum dcrypto_result result;

	if (!hash_size)
		return DCRYPTO_FAIL;

	if (padded_len < 2 + 2 * hash_size)
		return DCRYPTO_FAIL;       /* Invalid input size. */

	/* Recover seed. */
	result = MGF1_xor(seed, hash_size, phash, hash_size + 1 + max_msg_len,
		      hashing);
	/* Recover db. */
	result |= MGF1_xor(phash, hash_size + 1 + max_msg_len, seed, hash_size,
		      hashing);

	if (result != DCRYPTO_OK)
		return DCRYPTO_FAIL;

	result = DCRYPTO_hw_hash_init(&ctx, hashing);
	if (result != DCRYPTO_OK)
		return result;
	HASH_update(&ctx, label, label ? strlen(label) + 1 : 0);

	/* bad should be zero if DCRYPTO_OK is returned. */
	result = DCRYPTO_equals(phash, HASH_final(&ctx)->b8, hash_size);
	bad = result - DCRYPTO_OK; /* bad = 0 if result == DCRYPTO_OK */
	bad |= padded[0];

	for (i = PS - padded; i < padded_len; i++) {
		uint32_t equals0 = is_zero(padded[i]);
		uint32_t equals1 = is_zero(padded[i] ^ 1);

		one_index = select(looking_for_one_byte & equals1,
				i, one_index);
		looking_for_one_byte = select(equals1, 0, looking_for_one_byte);

		/* Bad padding if padded[i] is neither 1 nor 0. */
		bad |= looking_for_one_byte & ~equals0;
	}

	bad |= looking_for_one_byte;

	if (bad)
		return DCRYPTO_FAIL;

	one_index++;
	if (*out_len < padded_len - one_index)
		return DCRYPTO_FAIL;
	memcpy(out, padded + one_index, padded_len - one_index);
	*out_len = padded_len - one_index;
	/* Result should be DCRYPTO_OK after DCRYPTO_equals() */
	return result;
}

/* Constants from RFC 3447. */
#define RSA_PKCS1_PADDING_SIZE 11

/* encrypt */
static enum dcrypto_result pkcs1_type2_pad(uint8_t *padded, size_t padded_len,
					   const uint8_t *in, size_t in_len)
{
	size_t PS_len;

	if (padded_len < RSA_PKCS1_PADDING_SIZE)
		return DCRYPTO_FAIL;
	if (in_len > padded_len - RSA_PKCS1_PADDING_SIZE)
		return DCRYPTO_FAIL;
	PS_len = padded_len - 3 - in_len;

	*(padded++) = 0;
	*(padded++) = 2;
	while (PS_len) {
		size_t i;
		uint8_t r[SHA256_DIGEST_SIZE];

		if (!fips_rand_bytes(r, sizeof(r)))
			return DCRYPTO_FAIL;

		/**
		 * zero byte has special meaning in PKCS1, so copy
		 * only non-zero random bytes.
		 */
		for (i = 0; i < sizeof(r) && PS_len; i++) {
			if (r[i]) {
				*padded++ = r[i];
				PS_len--;
			}
		}
	}
	*(padded++) = 0;
	memcpy(padded, in, in_len);
	return DCRYPTO_OK;
}

/* decrypt */
static enum dcrypto_result check_pkcs1_type2_pad(uint8_t *out, size_t *out_len,
				const uint8_t *padded, size_t padded_len)
{
	size_t i;
	int valid;
	uint32_t zero_index = 0;
	uint32_t looking_for_index = ~0;

	if (padded_len < RSA_PKCS1_PADDING_SIZE)
		return DCRYPTO_FAIL;

	valid = (padded[0] == 0);
	valid &= (padded[1] == 2);

	for (i = 2; i < padded_len; i++) {
		uint32_t found = is_zero(padded[i]);

		zero_index = select(looking_for_index & found, i, zero_index);
		looking_for_index = select(found, 0, looking_for_index);
	}

	zero_index++;

	valid &= ~looking_for_index;
	valid &= (zero_index >= RSA_PKCS1_PADDING_SIZE);
	if (!valid)
		return DCRYPTO_FAIL;

	if (*out_len < padded_len - zero_index)
		return DCRYPTO_FAIL;

	memcpy(out, &padded[zero_index], padded_len - zero_index);
	*out_len = padded_len - zero_index;
	return DCRYPTO_OK;
}

static const uint8_t SHA1_DER[] = {
	0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e,
	0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14
};
static const uint8_t SHA256_DER[] = {
	0x30, 0x31, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86,
	0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
	0x00, 0x04, 0x20
};
static const uint8_t SHA384_DER[] = {
	0x30, 0x41, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
	0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02, 0x05,
	0x00, 0x04, 0x30
};
static const uint8_t SHA512_DER[] = {
	0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
	0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03, 0x05,
	0x00, 0x04, 0x40
};

static enum dcrypto_result pkcs1_get_der(enum hashing_mode hashing,
					 const uint8_t **der, size_t *der_size,
					 size_t *hash_size)
{
	switch (hashing) {
	case HASH_SHA1:
		*der = &SHA1_DER[0];
		*der_size = sizeof(SHA1_DER);
		*hash_size = SHA1_DIGEST_SIZE;
		break;
	case HASH_SHA256:
		*der = &SHA256_DER[0];
		*der_size = sizeof(SHA256_DER);
		*hash_size = SHA256_DIGEST_SIZE;
		break;
	case HASH_SHA384:
		*der = &SHA384_DER[0];
		*der_size = sizeof(SHA384_DER);
		*hash_size = SHA384_DIGEST_SIZE;
		break;
	case HASH_SHA512:
		*der = &SHA512_DER[0];
		*der_size = sizeof(SHA512_DER);
		*hash_size = SHA512_DIGEST_SIZE;
		break;
	case HASH_NULL:
		*der = NULL;
		*der_size = 0;
		*hash_size = 0; /* any size allowed */
		break;
	default:
		return DCRYPTO_FAIL;
	}

	return DCRYPTO_OK;
}

/* sign */
static enum dcrypto_result pkcs1_type1_pad(uint8_t *padded, size_t padded_len,
					   const uint8_t *in, size_t in_len,
					   enum hashing_mode hashing)
{
	const uint8_t *der;
	size_t der_size;
	size_t hash_size;
	size_t ps_len;
	enum dcrypto_result result;

	result = pkcs1_get_der(hashing, &der, &der_size, &hash_size);
	if (result != DCRYPTO_OK)
		return result;
	if (padded_len < RSA_PKCS1_PADDING_SIZE + der_size)
		return DCRYPTO_FAIL;
	if (!in_len || (hash_size && in_len != hash_size))
		return DCRYPTO_FAIL;
	if (in_len > padded_len - RSA_PKCS1_PADDING_SIZE - der_size)
		return DCRYPTO_FAIL;
	ps_len = padded_len - 3 - der_size - in_len;

	*(padded++) = 0;
	*(padded++) = 1;
	always_memset(padded, 0xFF, ps_len);
	padded += ps_len;
	*(padded++) = 0;
	memcpy(padded, der, der_size);
	padded += der_size;
	memcpy(padded, in, in_len);
	return DCRYPTO_OK;
}

/* verify */
static enum dcrypto_result check_pkcs1_type1_pad(const uint8_t *msg,
						 size_t msg_len,
						 const uint8_t *padded,
						 size_t padded_len,
						 enum hashing_mode hashing)
{
	size_t i;
	const uint8_t *der;
	size_t der_size;
	size_t hash_size;
	size_t ps_len;
	enum dcrypto_result result;

	result = pkcs1_get_der(hashing, &der, &der_size, &hash_size);
	if (result != DCRYPTO_OK)
		return result;
	if (msg_len != hash_size)
		return DCRYPTO_FAIL;
	if (padded_len < RSA_PKCS1_PADDING_SIZE + der_size + hash_size)
		return DCRYPTO_FAIL;
	ps_len = padded_len - 3 - der_size - hash_size;

	if (padded[0] != 0 || padded[1] != 1)
		return DCRYPTO_FAIL;
	for (i = 2; i < ps_len + 2; i++) {
		if (padded[i] != 0xFF)
			return DCRYPTO_FAIL;
	}

	if (padded[i++] != 0)
		return DCRYPTO_FAIL;

	result = DCRYPTO_equals(&padded[i], der, der_size);
	i += der_size;
	result |= DCRYPTO_equals(msg, &padded[i], hash_size);
	if (result != DCRYPTO_OK)
		return DCRYPTO_FAIL;
	return result;
}

static const uint8_t ZEROS[8] = { 0 };

/* sign */
static enum dcrypto_result pkcs1_pss_pad(uint8_t *padded, size_t padded_len,
					 const uint8_t *in, size_t in_len,
					 enum hashing_mode hashing)
{
	const uint32_t hash_size = DCRYPTO_hash_size(hashing);
	const uint32_t salt_len = MIN(padded_len - hash_size - 2, hash_size);
	size_t db_len;
	size_t ps_len;
	union hash_ctx ctx;
	enum dcrypto_result result;

	if (!hash_size)
		return DCRYPTO_FAIL;
	if (in_len != hash_size)
		return DCRYPTO_FAIL;
	if (padded_len < hash_size + 2)
		return DCRYPTO_FAIL;
	db_len = padded_len - hash_size - 1;

	if (!fips_rand_bytes(padded, salt_len))
		return DCRYPTO_FAIL;

	result = DCRYPTO_hw_hash_init(&ctx, hashing);
	if (result != DCRYPTO_OK)
		return result;

	HASH_update(&ctx, ZEROS, sizeof(ZEROS));
	HASH_update(&ctx, in, in_len);
	HASH_update(&ctx, padded, salt_len);

	/* Output hash. */
	memcpy(padded + db_len, HASH_final(&ctx)->b8, hash_size);

	/* Prepare DB. */
	ps_len = db_len - salt_len - 1;
	memmove(padded + ps_len + 1, padded, salt_len);
	memset(padded, 0, ps_len);
	padded[ps_len] = 0x01;

	result = MGF1_xor(padded, db_len, padded + db_len, hash_size, hashing);
	/* Clear most significant bit. */
	padded[0] &= 0x7F;
	/* Set trailing byte. */
	padded[padded_len - 1] = 0xBC;
	return result;
}

/* verify */
static enum dcrypto_result check_pkcs1_pss_pad(const uint8_t *in, size_t in_len,
					       uint8_t *padded,
					       size_t padded_len,
					       enum hashing_mode hashing)
{
	const uint32_t hash_size = DCRYPTO_hash_size(hashing);
	uint32_t db_len;
	uint32_t max_ps_len;
	uint32_t salt_len;
	union hash_ctx ctx;
	int bad = 0;
	size_t i;
	enum dcrypto_result result;

	if (!hash_size)
		return DCRYPTO_FAIL;
	if (in_len != hash_size)
		return DCRYPTO_FAIL;
	if (padded_len < hash_size + 2)
		return DCRYPTO_FAIL;
	db_len = padded_len - hash_size - 1;

	/* Top bit should be zero. */
	bad |= padded[0] & 0x80;
	/* Check trailing byte. */
	bad |= padded[padded_len - 1] ^ 0xBC;

	/* Recover DB. */
	result = MGF1_xor(padded, db_len, padded + db_len, hash_size, hashing);
	bad |= result - DCRYPTO_OK;

	/* Clear top bit. */
	padded[0] &= 0x7F;
	/* Verify padding2. */
	max_ps_len = db_len - 1;
	for (i = 0; i < max_ps_len; i++) {
		if (padded[i] == 0x01)
			break;
		else
			bad |= padded[i];
	}
	bad |= (padded[i] ^ 0x01);
	/* Continue with zero-length salt if 0x01 was not found. */
	salt_len = max_ps_len - i;

	result |= DCRYPTO_hw_hash_init(&ctx, hashing);
	if (result != DCRYPTO_OK)
		return DCRYPTO_FAIL;

	HASH_update(&ctx, ZEROS, sizeof(ZEROS));
	HASH_update(&ctx, in, in_len);
	HASH_update(&ctx, padded + db_len - salt_len, salt_len);
	result |= DCRYPTO_equals(padded + db_len, HASH_final(&ctx), hash_size);
	bad |= result - DCRYPTO_OK;
	if (bad)
		result = DCRYPTO_FAIL;
	return result;
}

static enum dcrypto_result check_modulus_params(const struct RSA *rsa,
						size_t *out_len)
{
	/* We don't check for prime exponents, but at least check it is odd. */
	if ((rsa->e & 1) == 0)
		return DCRYPTO_FAIL;

	/* Check key size is within limits and 256-bit aligned for dcrypto. */
	if ((bn_words(&rsa->N) > RSA_WORDS_4K) || (bn_words(&rsa->N) & 7))
		return DCRYPTO_FAIL;
	if (!bn_check_topbit(&rsa->N)) /* Check that top bit is set. */
		return DCRYPTO_FAIL;
	if (out_len && *out_len < bn_size(&rsa->N))
		return DCRYPTO_FAIL; /* Output buffer too small. */
	return DCRYPTO_OK;
}

enum dcrypto_result DCRYPTO_rsa_encrypt(struct RSA *rsa, uint8_t *out,
					size_t *out_len, const uint8_t *in,
					size_t in_len,
					enum padding_mode padding,
					enum hashing_mode hashing,
					const char *label)
{
	uint8_t *p;
	size_t n_len;
	uint8_t *padded_buf = NULL;

	struct LITE_BIGNUM padded;
	struct LITE_BIGNUM encrypted;
	enum dcrypto_result ret;

	if (!fips_crypto_allowed())
		return DCRYPTO_FAIL;

	ret = check_modulus_params(rsa, out_len);
	if (ret != DCRYPTO_OK)
		return ret;

	n_len = bn_size(&rsa->N);
	padded_buf = alloca(n_len);

	bn_init(&padded, padded_buf, n_len);
	bn_init(&encrypted, out, n_len);

	switch (padding) {
	case PADDING_MODE_OAEP:
		ret = oaep_pad((uint8_t *)padded.d, bn_size(&padded),
			     (const uint8_t *)in, in_len, hashing,
			     label);
		break;
	case PADDING_MODE_PKCS1:
		ret = pkcs1_type2_pad((uint8_t *)padded.d, bn_size(&padded),
				    (const uint8_t *)in, in_len);
		break;
	case PADDING_MODE_NULL:
		/* Input is allowed to have more bytes than N, in
		 * which case the excess must be zero. */
		for (; in_len > bn_size(&padded); in_len--)
			if (*in++ != 0)
				return DCRYPTO_FAIL;
		p = (uint8_t *) padded.d;
		/* If in_len < bn_size(&padded), padded will
		 * have leading zero bytes. */
		memcpy(&p[bn_size(&padded) - in_len], in, in_len);
		/* TODO(ngm): in may be > N, bn_mod_exp() should
		 * handle this case. */
		break;
	default:
		/* Unsupported padding mode. */
		ret = DCRYPTO_FAIL;
		break;
	}
	if (ret != DCRYPTO_OK)
		return ret;
	/* Reverse from big-endian to little-endian notation. */
	reverse((uint8_t *) padded.d, bn_size(&padded));
	ret = bn_modexp_word(&encrypted, &padded, rsa->e, &rsa->N);
	/* Back to big-endian notation. */
	reverse((uint8_t *) encrypted.d, bn_size(&encrypted));
	*out_len = bn_size(&encrypted);

	always_memset(padded_buf, 0, n_len);
	return ret;
}

enum dcrypto_result DCRYPTO_rsa_decrypt(struct RSA *rsa, uint8_t *out,
					size_t *out_len, const uint8_t *in,
					const size_t in_len,
					enum padding_mode padding,
					enum hashing_mode hashing,
					const char *label)
{
	uint8_t *buf = NULL;

	struct LITE_BIGNUM encrypted;
	struct LITE_BIGNUM padded;
	enum dcrypto_result ret;

	if (!fips_crypto_allowed())
		return DCRYPTO_FAIL;

	ret = check_modulus_params(rsa, NULL);
	if (ret != DCRYPTO_OK)
		return ret;
	if (in_len != bn_size(&rsa->N)) /* Invalid input length. */
		return DCRYPTO_FAIL;
	buf = alloca(in_len * 2);
	/* TODO: this copy can be eliminated if input may be modified. */
	bn_init(&encrypted, buf, in_len);
	memcpy(buf, in, in_len);
	bn_init(&padded, buf + in_len, in_len);

	/* Reverse from big-endian to little-endian notation. */
	reverse((uint8_t *) encrypted.d, encrypted.dmax * LITE_BN_BYTES);
	ret = bn_modexp_blinded(&padded, &encrypted, &rsa->d, &rsa->N, rsa->e);
	/* Back to big-endian notation. */
	reverse((uint8_t *) padded.d, padded.dmax * LITE_BN_BYTES);

	switch (padding) {
	case PADDING_MODE_OAEP:
		ret |= check_oaep_pad(out, out_len, (uint8_t *)padded.d,
				   bn_size(&padded), hashing,
				   label);
		break;
	case PADDING_MODE_PKCS1:
		ret |= check_pkcs1_type2_pad(out, out_len,
					  (const uint8_t *)padded.d,
					  bn_size(&padded));
		break;
	case PADDING_MODE_NULL:
		if (*out_len < bn_size(&padded)) {
			ret = DCRYPTO_FAIL;
		} else {
			*out_len = bn_size(&padded);
			memcpy(out, padded.d, *out_len);
		}
		break;
	default:
		/* Unsupported padding mode. */
		ret = DCRYPTO_FAIL;
		break;
	}
	if (ret != DCRYPTO_OK)
		ret = DCRYPTO_FAIL;
	always_memset(buf, 0, in_len * 2);
	return ret;
}

enum dcrypto_result DCRYPTO_rsa_sign(struct RSA *rsa, uint8_t *out,
				     size_t *out_len, const uint8_t *in,
				     const size_t in_len,
				     enum padding_mode padding,
				     enum hashing_mode hashing)
{
	uint8_t *padded_buf;
	size_t n_len;

	struct LITE_BIGNUM padded;
	struct LITE_BIGNUM signature;
	enum dcrypto_result ret;

	if (!fips_crypto_allowed())
		return DCRYPTO_FAIL;

	ret = check_modulus_params(rsa, out_len);
	if (ret != DCRYPTO_OK)
		return ret;

	n_len = bn_size(&rsa->N);
	padded_buf = alloca(n_len);
	bn_init(&padded, padded_buf, n_len);
	bn_init(&signature, out, n_len);

	switch (padding) {
	case PADDING_MODE_PKCS1:
		ret = pkcs1_type1_pad((uint8_t *)padded.d, bn_size(&padded),
				    (const uint8_t *)in, in_len,
				    hashing);
		break;
	case PADDING_MODE_PSS:
		ret = pkcs1_pss_pad((uint8_t *)padded.d, bn_size(&padded),
				  (const uint8_t *)in, in_len,
				  hashing);
		break;
	default:
		ret = DCRYPTO_FAIL;
	}
	if (ret != DCRYPTO_OK)
		return ret;

	/* Reverse from big-endian to little-endian notation. */
	reverse((uint8_t *) padded.d, bn_size(&padded));
	ret = bn_modexp_blinded(&signature, &padded, &rsa->d, &rsa->N, rsa->e);
	/* Back to big-endian notation. */
	reverse((uint8_t *) signature.d, bn_size(&signature));
	*out_len = n_len;

	always_memset(padded_buf, 0, n_len);
	return ret;
}

enum dcrypto_result DCRYPTO_rsa_verify(const struct RSA *rsa,
				       const uint8_t *digest, size_t digest_len,
				       const uint8_t *sig, const size_t sig_len,
				       enum padding_mode padding,
				       enum hashing_mode hashing)
{
	uint8_t *buf;

	struct LITE_BIGNUM padded;
	struct LITE_BIGNUM signature;
	enum dcrypto_result ret;

	if (!fips_crypto_allowed())
		return DCRYPTO_FAIL;

	ret = check_modulus_params(rsa, NULL);
	if (ret != DCRYPTO_OK)
		return ret;
	if (sig_len != bn_size(&rsa->N))
		return DCRYPTO_FAIL; /* Invalid input length. */

	buf = alloca(sig_len * 2);
	bn_init(&signature, buf, sig_len);
	memcpy(buf, sig, sig_len);
	bn_init(&padded, buf + sig_len, sig_len);

	/* Reverse from big-endian to little-endian notation. */
	reverse((uint8_t *)signature.d, bn_size(&signature));
	ret = bn_modexp_word(&padded, &signature, rsa->e, &rsa->N);
	/* Back to big-endian notation. */
	reverse((uint8_t *)padded.d, bn_size(&padded));

	switch (padding) {
	case PADDING_MODE_PKCS1:
		ret = check_pkcs1_type1_pad(digest, digest_len,
					    (uint8_t *)padded.d,
					    bn_size(&padded), hashing);
		break;
	case PADDING_MODE_PSS:
		ret = check_pkcs1_pss_pad(digest, digest_len,
					  (uint8_t *)padded.d, bn_size(&padded),
					  hashing);
		break;
	default:
		/* Unsupported padding mode. */
		ret = DCRYPTO_FAIL;
		break;
	}

	always_memset(buf, 0, sig_len * 2);
	return ret;
}

enum dcrypto_result DCRYPTO_rsa_key_compute(struct LITE_BIGNUM *N,
					    struct LITE_BIGNUM *d,
					    struct LITE_BIGNUM *p,
					    struct LITE_BIGNUM *q,
					    uint32_t e_buf)
{
	uint32_t ONE_buf = 1;
	uint8_t *buf;
	size_t n_len, p_len;
	struct LITE_BIGNUM ONE;
	struct LITE_BIGNUM e;
	struct LITE_BIGNUM phi;
	struct LITE_BIGNUM q_local;

	if (!fips_crypto_allowed())
		return DCRYPTO_FAIL;

	n_len = bn_size(N);
	p_len = bn_size(p);

	if (n_len > RSA_BYTES_4K || p_len > RSA_BYTES_2K || !(e_buf & 1))
		return DCRYPTO_FAIL;

	buf = alloca(n_len + p_len);
	DCRYPTO_bn_wrap(&ONE, &ONE_buf, sizeof(ONE_buf));
	DCRYPTO_bn_wrap(&phi, buf, n_len);
	if (!q) {
		/* q not provided, calculate it. */
		bn_init(&q_local, buf + n_len, p_len);
		q = &q_local;

		if (!DCRYPTO_bn_div(q, NULL, N, p))
			return DCRYPTO_FAIL;

		/* Check that p * q == N */
		DCRYPTO_bn_mul(&phi, p, q);
		if (!bn_eq(N, &phi))
			return DCRYPTO_FAIL;
	} else {
		DCRYPTO_bn_mul(N, p, q);
		memcpy(phi.d, N->d, n_len);
	}

	bn_sub(&phi, p);
	bn_sub(&phi, q);
	bn_add(&phi, &ONE);
	DCRYPTO_bn_wrap(&e, &e_buf, sizeof(e_buf));
	return bn_modinv_vartime(d, &e, &phi);
}
