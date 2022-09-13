/* Copyright 2018 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "console.h"
#include "dcrypto.h"
#include "extension.h"
#include "internal.h"

/* Assuming H=0.8, we need 320 bits from TRNG to get 256 bits. */
#define RESEED_ENTROPY_SIZE_BITS  320
#define RESEED_ENTROPY_SIZE_WORDS (BITS_TO_WORDS(RESEED_ENTROPY_SIZE_BITS))

/* HMAC_DRBG flow in NIST SP 800-90Ar1, 10.2, RFC 6979
 */
/* V = HMAC(K, V) */
static void update_v(const uint32_t *k, uint32_t *v)
{
	struct hmac_sha256_ctx ctx;

	HMAC_SHA256_hw_init(&ctx, k, SHA256_DIGEST_SIZE);
	HMAC_SHA256_update(&ctx, v, SHA256_DIGEST_SIZE);
	memcpy(v, HMAC_SHA256_final(&ctx), SHA256_DIGEST_SIZE);
}

/* K = HMAC(K, V || tag || p0 || p1 || p2) */
/* V = HMAC(K, V) */
static void update_kv(uint32_t *k, uint32_t *v, uint8_t tag, const void *p0,
		      size_t p0_len, const void *p1, size_t p1_len,
		      const void *p2, size_t p2_len)
{
	struct hmac_sha256_ctx ctx;

	HMAC_SHA256_hw_init(&ctx, k, SHA256_DIGEST_SIZE);
	HMAC_SHA256_update(&ctx, v, SHA256_DIGEST_SIZE);
	HMAC_SHA256_update(&ctx, &tag, 1);
	HMAC_SHA256_update(&ctx, p0, p0_len);
	HMAC_SHA256_update(&ctx, p1, p1_len);
	HMAC_SHA256_update(&ctx, p2, p2_len);
	memcpy(k, HMAC_SHA256_final(&ctx), SHA256_DIGEST_SIZE);
	update_v(k, v);
}

static void update(struct drbg_ctx *ctx,
		   const void *p0, size_t p0_len,
		   const void *p1, size_t p1_len,
		   const void *p2, size_t p2_len)
{
	/* K = HMAC(K, V || 0x00 || provided_data) */
	/* V = HMAC(K, V) */
	update_kv(ctx->k, ctx->v, 0x00,
		  p0, p0_len, p1, p1_len, p2, p2_len);

	/* If no provided_data, stop. */
	if (p0_len + p1_len + p2_len == 0)
		return;

	/* K = HMAC(K, V || 0x01 || provided_data) */
	/* V = HMAC(K, V) */
	update_kv(ctx->k, ctx->v,
		  0x01,
		  p0, p0_len, p1, p1_len, p2, p2_len);
}

void hmac_drbg_init(struct drbg_ctx *ctx, const void *entropy,
		    size_t entropy_len, const void *nonce, size_t nonce_len,
		    const void *perso, size_t perso_len,
		    uint32_t reseed_threshold)
{
	/**
	 * Clear the context. Also set
	 * K = 0x00 0x00 0x00 ... 0x00
	 * magic_cookie = 0
	 */
	always_memset(ctx, 0x00, sizeof(*ctx));
	/* V = 0x01 0x01 0x01 ... 0x01 */
	always_memset(ctx->v, 0x01, sizeof(ctx->v));

	/* seed_material = entropy_input || nonce || personalization_string. */
	update(ctx, entropy, entropy_len, nonce, nonce_len, perso, perso_len);

	ctx->reseed_counter = 1;
	ctx->reseed_threshold = reseed_threshold;
	ctx->magic_cookie = DCRYPTO_OK;
}

void hmac_drbg_init_rfc6979(struct drbg_ctx *ctx, const p256_int *key,
			    const p256_int *message)
{
	hmac_drbg_init(ctx, key->a, sizeof(key->a), message->a,
		       sizeof(message->a), NULL, 0,
		       HMAC_DRBG_DO_NOT_AUTO_RESEED);
}

void hmac_drbg_reseed(struct drbg_ctx *ctx, const void *entropy,
		      size_t entropy_len, const void *additional_input,
		      size_t additional_input_len)
{
	/* seed_material = entropy_input || additional_input. */
	update(ctx, entropy, entropy_len, additional_input,
	       additional_input_len, NULL, 0);
	ctx->reseed_counter = 1;
}

enum dcrypto_result hmac_drbg_generate(struct drbg_ctx *ctx, void *out,
				       size_t out_len,
				       const void *additional_input,
				       size_t additional_input_len)
{
	/* Prevent misuse of uninitialized DRBG context. */
	if (!hmac_drbg_ctx_valid(ctx))
		return DCRYPTO_FAIL;

	/**
	 * In addition to output length, check also additional input
	 * length to be reasonable.
	 */

	if (out_len > HMAC_DRBG_MAX_OUTPUT_SIZE ||
	    additional_input_len > HMAC_DRBG_MAX_OUTPUT_SIZE)
		return DCRYPTO_FAIL;

	/**
	 * Special case when no auto reseed is needed. Note, as we use unsigned
	 * 32-bit values, ctx->reseed_counter can never be larger
	 * than HMAC_DRBG_DO_NOT_AUTO_RESEED, so check explicitly.
	 */
	if (ctx->reseed_counter == HMAC_DRBG_DO_NOT_AUTO_RESEED)
		return DCRYPTO_RESEED_NEEDED;

	if (ctx->reseed_counter > ctx->reseed_threshold) {
		uint32_t entropy[RESEED_ENTROPY_SIZE_WORDS];

		if (!fips_trng_bytes(&entropy, sizeof(entropy)))
			return DCRYPTO_FAIL;

		hmac_drbg_reseed(ctx, entropy, sizeof(entropy),
				 additional_input, additional_input_len);
		additional_input_len = 0;
	}

	ctx->reseed_counter++;

	if (additional_input_len)
		update(ctx, additional_input, additional_input_len, NULL, 0,
		       NULL, 0);

	while (out_len) {
		size_t n = MIN(out_len, sizeof(ctx->v));

		update_v(ctx->k, ctx->v);

		memcpy(out, ctx->v, n);
		out += n;
		out_len -= n;
	}

	update(ctx, additional_input, additional_input_len, NULL, 0, NULL, 0);

	return DCRYPTO_OK;
}

void drbg_exit(struct drbg_ctx *ctx)
{
	always_memset(ctx, 0, sizeof(*ctx));
}

#ifndef CRYPTO_TEST_CMD_HMAC_DRBG
#define CRYPTO_TEST_CMD_HMAC_DRBG 0
#endif

#ifdef CRYPTO_TEST_SETUP

#if CRYPTO_TEST_CMD_HMAC_DRBG
/*
 * from the RFC 6979 A.2.5 example:
 *
 * curve: NIST P-256
 *
 * q = FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551
 * (qlen = 256 bits)
 *
 * private key:
 * x = C9AFA9D845BA75166B5C215767B1D6934E50C3DB36E89B127B8A622B120F6721
 *
 * public key: U = xG
 * Ux = 60FED4BA255A9D31C961EB74C6356D68C049B8923B61FA6CE669622E60F29FB6
 * Uy = 7903FE1008B8BC99A41AE9E95628BC64F2F1B20C2D7E9F5177A3C294D4462299
 *
 * Signature:
 * With SHA-256, message = "sample":
 * k = A6E3C57DD01ABE90086538398355DD4C3B17AA873382B0F24D6129493D8AAD60
 * r = EFD48B2AACB6A8FD1140DD9CD45E81D69D2C877B56AAF991C34D0EA84EAF3716
 * s = F7CB1C942D657C41D436C7A1B6E29F65F3E900DBB9AFF4064DC4AB2F843ACDA8
 */
static int cmd_rfc6979(int argc, char **argv)
{
	static p256_int h1;
	static p256_int k;
	static const char message[] = "sample";
	static struct drbg_ctx drbg;

	static struct sha256_ctx ctx;
	int result;
	static const uint8_t priv_from_rfc[] = {
		0xC9, 0xAF, 0xA9, 0xD8, 0x45, 0xBA, 0x75, 0x16,
		0x6B, 0x5C, 0x21, 0x57, 0x67, 0xB1, 0xD6, 0x93,
		0x4E, 0x50, 0xC3, 0xDB, 0x36, 0xE8, 0x9B, 0x12,
		0x7B, 0x8A, 0x62, 0x2B, 0x12, 0x0F, 0x67, 0x21
	};
	static const uint8_t k_from_rfc[] = {
		0xA6, 0xE3, 0xC5, 0x7D, 0xD0, 0x1A, 0xBE, 0x90,
		0x08, 0x65, 0x38, 0x39, 0x83, 0x55, 0xDD, 0x4C,
		0x3B, 0x17, 0xAA, 0x87, 0x33, 0x82, 0xB0, 0xF2,
		0x4D, 0x61, 0x29, 0x49, 0x3D, 0x8A, 0xAD, 0x60
	};
	p256_int *x = (p256_int *)priv_from_rfc;
	p256_int *reference_k = (p256_int *)k_from_rfc;

	/* h1 = H(m) */
	SHA256_hw_init(&ctx);
	SHA256_update(&ctx, message, sizeof(message) - 1);
	memcpy(&h1, SHA256_final(&ctx)->b8, SHA256_DIGEST_SIZE);

	hmac_drbg_init_rfc6979(&drbg, x, &h1);
	if (hmac_drbg_generate(&drbg, k.a, sizeof(k), NULL, 0) != DCRYPTO_OK) {
		ccprintf("HMAC DRBG generate failed\n");
		return EC_ERROR_HW_INTERNAL;
	}
	ccprintf("K = %ph\n", HEX_BUF(&k, 32));
	drbg_exit(&drbg);
	result = memcmp(&k, reference_k, sizeof(reference_k));
	ccprintf("K generation: %s\n", result ? "FAIL" : "PASS");

	return result ? EC_ERROR_INVAL : EC_SUCCESS;
}
DECLARE_SAFE_CONSOLE_COMMAND(rfc6979, cmd_rfc6979, NULL, NULL);

/*
 * Test vectors from the NIST Cryptographic Algorithm Validation Program.
 *
 * These are the first two examples from the SHA-256, without prediction
 * resistance, and with reseed supported.
 */
#define HMAC_TEST_COUNT 2
static int cmd_hmac_drbg(int argc, char **argv)
{
	static struct drbg_ctx ctx;

	static const uint8_t init_entropy[HMAC_TEST_COUNT][32] = {
		{
			0x06, 0x03, 0x2C, 0xD5, 0xEE, 0xD3, 0x3F, 0x39, 0x26,
			0x5F, 0x49, 0xEC, 0xB1, 0x42, 0xC5, 0x11, 0xDA, 0x9A,
			0xFF, 0x2A, 0xF7, 0x12, 0x03, 0xBF, 0xFA, 0xF3, 0x4A,
			0x9C, 0xA5, 0xBD, 0x9C, 0x0D
		},
		{
			0xAA, 0xDC, 0xF3, 0x37, 0x78, 0x8B, 0xB8, 0xAC, 0x01,
			0x97, 0x66, 0x40, 0x72, 0x6B, 0xC5, 0x16, 0x35, 0xD4,
			0x17, 0x77, 0x7F, 0xE6, 0x93, 0x9E, 0xDE, 0xD9, 0xCC,
			0xC8, 0xA3, 0x78, 0xC7, 0x6A
		},
	};

	static const uint8_t init_nonce[HMAC_TEST_COUNT][16] = {
		{
			0x0E, 0x66, 0xF7, 0x1E, 0xDC, 0x43, 0xE4, 0x2A, 0x45,
			0xAD, 0x3C, 0x6F, 0xC6, 0xCD, 0xC4, 0xDF
		},
		{
			0x9C, 0xCC, 0x9D, 0x80, 0xC8, 0x9A, 0xC5, 0x5A, 0x8C,
			0xFE, 0x0F, 0x99, 0x94, 0x2F, 0x5A, 0x4D
		},
	};

	static const uint8_t reseed_entropy[HMAC_TEST_COUNT][32] = {
		{
			0x01, 0x92, 0x0A, 0x4E, 0x66, 0x9E, 0xD3, 0xA8, 0x5A,
			0xE8, 0xA3, 0x3B, 0x35, 0xA7, 0x4A, 0xD7, 0xFB, 0x2A,
			0x6B, 0xB4, 0xCF, 0x39, 0x5C, 0xE0, 0x03, 0x34, 0xA9,
			0xC9, 0xA5, 0xA5, 0xD5, 0x52
		},
		{
			0x03, 0xA5, 0x77, 0x92, 0x54, 0x7E, 0x0C, 0x98, 0xEA,
			0x17, 0x76, 0xE4, 0xBA, 0x80, 0xC0, 0x07, 0x34, 0x62,
			0x96, 0xA5, 0x6A, 0x27, 0x0A, 0x35, 0xFD, 0x9E, 0xA2,
			0x84, 0x5C, 0x7E, 0x81, 0xE2
		}
	};

	static const uint8_t expected_output[HMAC_TEST_COUNT][128] = {
		{
			0x76, 0xFC, 0x79, 0xFE, 0x9B, 0x50, 0xBE, 0xCC, 0xC9,
			0x91, 0xA1, 0x1B, 0x56, 0x35, 0x78, 0x3A, 0x83, 0x53,
			0x6A, 0xDD, 0x03, 0xC1, 0x57, 0xFB, 0x30, 0x64, 0x5E,
			0x61, 0x1C, 0x28, 0x98, 0xBB, 0x2B, 0x1B, 0xC2, 0x15,
			0x00, 0x02, 0x09, 0x20, 0x8C, 0xD5, 0x06, 0xCB, 0x28,
			0xDA, 0x2A, 0x51, 0xBD, 0xB0, 0x38, 0x26, 0xAA, 0xF2,
			0xBD, 0x23, 0x35, 0xD5, 0x76, 0xD5, 0x19, 0x16, 0x08,
			0x42, 0xE7, 0x15, 0x8A, 0xD0, 0x94, 0x9D, 0x1A, 0x9E,
			0xC3, 0xE6, 0x6E, 0xA1, 0xB1, 0xA0, 0x64, 0xB0, 0x05,
			0xDE, 0x91, 0x4E, 0xAC, 0x2E, 0x9D, 0x4F, 0x2D, 0x72,
			0xA8, 0x61, 0x6A, 0x80, 0x22, 0x54, 0x22, 0x91, 0x82,
			0x50, 0xFF, 0x66, 0xA4, 0x1B, 0xD2, 0xF8, 0x64, 0xA6,
			0xA3, 0x8C, 0xC5, 0xB6, 0x49, 0x9D, 0xC4, 0x3F, 0x7F,
			0x2B, 0xD0, 0x9E, 0x1E, 0x0F, 0x8F, 0x58, 0x85, 0x93,
			0x51, 0x24
		},
		{
			0x17, 0xD0, 0x9F, 0x40, 0xA4, 0x37, 0x71, 0xF4, 0xA2,
			0xF0, 0xDB, 0x32, 0x7D, 0xF6, 0x37, 0xDE, 0xA9, 0x72,
			0xBF, 0xFF, 0x30, 0xC9, 0x8E, 0xBC, 0x88, 0x42, 0xDC,
			0x7A, 0x9E, 0x3D, 0x68, 0x1C, 0x61, 0x90, 0x2F, 0x71,
			0xBF, 0xFA, 0xF5, 0x09, 0x36, 0x07, 0xFB, 0xFB, 0xA9,
			0x67, 0x4A, 0x70, 0xD0, 0x48, 0xE5, 0x62, 0xEE, 0x88,
			0xF0, 0x27, 0xF6, 0x30, 0xA7, 0x85, 0x22, 0xEC, 0x6F,
			0x70, 0x6B, 0xB4, 0x4A, 0xE1, 0x30, 0xE0, 0x5C, 0x8D,
			0x7E, 0xAC, 0x66, 0x8B, 0xF6, 0x98, 0x0D, 0x99, 0xB4,
			0xC0, 0x24, 0x29, 0x46, 0x45, 0x23, 0x99, 0xCB, 0x03,
			0x2C, 0xC6, 0xF9, 0xFD, 0x96, 0x28, 0x47, 0x09, 0xBD,
			0x2F, 0xA5, 0x65, 0xB9, 0xEB, 0x9F, 0x20, 0x04, 0xBE,
			0x6C, 0x9E, 0xA9, 0xFF, 0x91, 0x28, 0xC3, 0xF9, 0x3B,
			0x60, 0xDC, 0x30, 0xC5, 0xFC, 0x85, 0x87, 0xA1, 0x0D,
			0xE6, 0x8C
		}
	};

	static uint8_t output[128];

	int i, cmp_result;
	enum dcrypto_result err;

	for (i = 0; i < HMAC_TEST_COUNT; i++) {
		hmac_drbg_init(&ctx, init_entropy[i], sizeof(init_entropy[i]),
			       init_nonce[i], sizeof(init_nonce[i]), NULL, 0,
			       10000);

		hmac_drbg_reseed(&ctx, reseed_entropy[i],
				 sizeof(reseed_entropy[i]), NULL, 0);

		err = hmac_drbg_generate(&ctx, output, sizeof(output), NULL, 0);

		err |= hmac_drbg_generate(&ctx, output, sizeof(output), NULL,
					  0);

		if (err != DCRYPTO_OK) {
			ccprintf("HMAC DRBG generate failed.\n");
			return EC_ERROR_HW_INTERNAL;
		}

		cmp_result = memcmp(output, expected_output[i], sizeof(output));
		ccprintf("HMAC DRBG generate test %d, %s\n",
			 i, cmp_result ? "failed" : "passed");
	}

	return 0;
}
DECLARE_SAFE_CONSOLE_COMMAND(hmac_drbg, cmd_hmac_drbg, NULL, NULL);

/*
 * Validity check to exercise random initialization.
 */
static int cmd_hmac_drbg_rand(int argc, char **argv)
{
	static struct drbg_ctx ctx;
	static uint8_t output[128];

	size_t i;

	/* Seed with 256 bits from TRNG. */
	if (!fips_trng_bytes(output, 32))
		return EC_ERROR_HW_INTERNAL;
	hmac_drbg_init(&ctx, output, 32, NULL, 0, NULL, 0, 10000);

	if (hmac_drbg_generate(&ctx, output, sizeof(output), NULL, 0) !=
	    DCRYPTO_OK) {
		ccprintf("HMAC_DRBG generate failed.\n");
		return EC_ERROR_HW_INTERNAL;
	}
	ccprintf("Randomly initialized HMAC DRBG, 1024 bit output: ");

	for (i = 0; i < sizeof(output); i++)
		ccprintf("%x", output[i]);
	ccprintf("\n");

	return 0;
}
DECLARE_SAFE_CONSOLE_COMMAND(hmac_drbg_rand, cmd_hmac_drbg_rand, NULL, NULL);

#endif /* CRYPTO_TEST_CMD_HMAC_DRBG */

enum drbg_command {
	DRBG_INIT = 0,
	DRBG_RESEED = 1,
	DRBG_GENERATE = 2
};

/*
 * DRBG_TEST command structure:
 *
 * field       |    size  |              note
 * ==========================================================================
 * mode        |    1     | 0 - DRBG_INIT, 1 - DRBG_RESEED, 2 - DRBG_GENERATE
 * p0_len      |    2     | size of first input in bytes
 * p0          |  p0_len  | entropy for INIT & SEED, input for GENERATE
 * p1_len      |    2     | size of second input in bytes (for INIT & RESEED)
 *             |          | or size of expected output for GENERATE
 * p1          |  p1_len  | nonce for INIT & SEED
 * p2_len      |    2     | size of third input in bytes for DRBG_INIT
 * p2          |  p2_len  | personalization for INIT & SEED
 *
 * DRBG_INIT (entropy, nonce, perso)
 * DRBG_RESEED (entropy, additional input 1, additional input 2)
 * DRBG_INIT and DRBG_RESEED returns empty response
 * DRBG_GENERATE (p0_len, p0 - additional input 1, p1_len - size of output)
 * DRBG_GENERATE returns p1_len bytes of generated data
 * (up to a maximum of 128 bytes)
 */
static enum vendor_cmd_rc drbg_test(enum vendor_cmd_cc code, void *buf,
				    size_t input_size, size_t *response_size)
{
	static struct drbg_ctx drbg_ctx;
	static uint8_t output[512];
	uint8_t *p0 = NULL, *p1 = NULL, *p2 = NULL;
	uint16_t p0_len = 0, p1_len = 0, p2_len = 0;
	uint8_t *cmd = (uint8_t *)buf;
	size_t max_out_len = *response_size;
	enum drbg_command drbg_op;

	*response_size = 0;
	/* there is always op + first parameter, even if zero length */
	if (input_size < sizeof(p0_len) + 1)
		return VENDOR_RC_BOGUS_ARGS;
	drbg_op = *cmd++;
	p0_len = *cmd++;
	p0_len = p0_len * 256 + *cmd++;
	input_size -= 3;
	if (p0_len > input_size)
		return VENDOR_RC_BOGUS_ARGS;
	input_size -= p0_len;
	if (p0_len)
		p0 = cmd;
	cmd += p0_len;

	/* there should be enough space for p1_len */
	if (input_size && input_size < sizeof(p1_len))
		return VENDOR_RC_BOGUS_ARGS;

	/* DRBG_GENERATE should just have p1_len defined */
	if (drbg_op == DRBG_GENERATE && input_size != sizeof(p1_len))
		return VENDOR_RC_BOGUS_ARGS;

	if (input_size) {
		p1_len = *cmd++;
		p1_len = p1_len * 256 + *cmd++;
		input_size -= 2;

		if (drbg_op != DRBG_GENERATE) {
			if (p1_len > input_size)
				return VENDOR_RC_BOGUS_ARGS;
			input_size -= p1_len;
			if (p1_len)
				p1 = cmd;
			cmd += p1_len;
		}
	}

	if (input_size) {
		if (drbg_op == DRBG_GENERATE)
			return VENDOR_RC_BOGUS_ARGS;
		p2_len = *cmd++;
		p2_len = p2_len * 256 + *cmd++;
		input_size -= 2;
		if (p2_len > input_size)
			return VENDOR_RC_BOGUS_ARGS;
		if (p2_len)
			p2 = cmd;
	}

	switch (drbg_op) {
	case DRBG_INIT: {
		hmac_drbg_init(&drbg_ctx, p0, p0_len, p1, p1_len, p2, p2_len,
			       10000);
		break;
	}
	case DRBG_RESEED: {
		hmac_drbg_reseed(&drbg_ctx, p0, p0_len, p1, p1_len);
		break;
	}
	case DRBG_GENERATE: {
		if (p1_len > sizeof(output) || max_out_len < p1_len)
			return VENDOR_RC_BOGUS_ARGS;

		if (hmac_drbg_generate(&drbg_ctx, output, p1_len, p0, p0_len) !=
		    DCRYPTO_OK)
			return VENDOR_RC_INTERNAL_ERROR;

		memcpy(buf, output, p1_len);
		*response_size = p1_len;
		break;
	}
	default:
		return VENDOR_RC_BOGUS_ARGS;
	}

	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_DRBG_TEST, drbg_test);

#endif /* CRYPTO_TEST_SETUP */
