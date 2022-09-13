/* Copyright 2016 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "dcrypto.h"
#include "internal.h"
#include "registers.h"

#ifdef CRYPTO_TEST_SETUP

/* test and benchmark */
#include "common.h"
#include "console.h"
#include "hooks.h"
#include "task.h"
#include "watchdog.h"

#define cyclecounter() GREG32(M3, DWT_CYCCNT)
#define START_PROFILE(x)                                                       \
	{                                                                      \
		x -= cyclecounter();                                           \
	}
#define END_PROFILE(x)                                                         \
	{                                                                      \
		x += cyclecounter();                                           \
	}
static uint32_t t_sw;
static uint32_t t_hw;
static uint32_t t_transform;
static uint32_t t_dcrypto;

#else /* CRYPTO_TEST_SETUP */

#define START_PROFILE(x)
#define END_PROFILE(x)

#endif /* CRYPTO_TEST_SETUP */

/* Firmware blob for crypto accelerator */
#include "dcrypto_sha512.inc"

struct DMEM_sha512 {
	uint64_t H0[4];
	uint64_t H1[4];
	uint64_t H2[4];
	uint64_t H3[4];
	uint64_t H4[4];
	uint64_t H5[4];
	uint64_t H6[4];
	uint64_t H7[4];
	uint32_t nblocks;
	uint32_t unused[2 * 8 - 1];
	uint32_t input[4 * 8 * 8]; // dmem[10..41]
};

static void copy_words(const void *in, uint32_t *dst, size_t nwords)
{
	const uint32_t *src = (const uint32_t *) in;

	do {
		uint32_t w1 = __builtin_bswap32(*src++);
		uint32_t w2 = __builtin_bswap32(*src++);
		*dst++ = w2;
		*dst++ = w1;
	} while (nwords -= 2);
}

static void dcrypto_SHA512_setup(void)
{
	dcrypto_imem_load(0, IMEM_dcrypto_sha512,
			  ARRAY_SIZE(IMEM_dcrypto_sha512));
}

static void dcrypto_SHA512_Transform(struct sha512_ctx *ctx,
				     const uint32_t *buf, size_t nwords)
{
	int result = 0;
	struct DMEM_sha512 *p512 =
	    (struct DMEM_sha512 *) GREG32_ADDR(CRYPTO, DMEM_DUMMY);

	START_PROFILE(t_transform)

	/* Pass in H[] */
	p512->H0[0] = ctx->state[0];
	p512->H1[0] = ctx->state[1];
	p512->H2[0] = ctx->state[2];
	p512->H3[0] = ctx->state[3];
	p512->H4[0] = ctx->state[4];
	p512->H5[0] = ctx->state[5];
	p512->H6[0] = ctx->state[6];
	p512->H7[0] = ctx->state[7];

	p512->nblocks = nwords / 32;

	/* Pass in buf[] */
	copy_words(buf, p512->input, nwords);

	START_PROFILE(t_dcrypto)
	result |= dcrypto_call(CF_compress_adr);
	END_PROFILE(t_dcrypto)

	/* Retrieve new H[] */
	ctx->state[0] = p512->H0[0];
	ctx->state[1] = p512->H1[0];
	ctx->state[2] = p512->H2[0];
	ctx->state[3] = p512->H3[0];
	ctx->state[4] = p512->H4[0];
	ctx->state[5] = p512->H5[0];
	ctx->state[6] = p512->H6[0];
	ctx->state[7] = p512->H7[0];

	/* TODO: errno or such to capture errors */
	(void) (result == 0);

	END_PROFILE(t_transform)
}

void SHA512_hw_update(struct sha512_ctx *ctx, const void *data, size_t len)
{
	int i = (int)(ctx->count & (sizeof(ctx->b8) - 1));
	const uint8_t *p = (const uint8_t *)data;
	uint8_t *d = &ctx->b8[i];

	ctx->count += len;

	dcrypto_init_and_lock();
	dcrypto_SHA512_setup();

	/* Take fast path for 32-bit aligned 1KB inputs */
	if (i == 0 && len == 1024 && (((intptr_t)data) & 3) == 0) {
		dcrypto_SHA512_Transform(ctx, (const uint32_t *)data, 8 * 32);
	} else {
		if (len <= sizeof(ctx->b8) - i) {
			memcpy(d, p, len);
			if (len == sizeof(ctx->b8) - i) {
				dcrypto_SHA512_Transform(
					ctx, (uint32_t *)(ctx->b8), 32);
			}
		} else {
			memcpy(d, p, sizeof(ctx->b8) - i);
			dcrypto_SHA512_Transform(ctx, ctx->b32, 32);
			d = ctx->b8;
			len -= (sizeof(ctx->b8) - i);
			p += (sizeof(ctx->b8) - i);
			while (len >= sizeof(ctx->b8)) {
				memcpy(d, p, sizeof(ctx->b8));
				p += sizeof(ctx->b8);
				len -= sizeof(ctx->b8);
				dcrypto_SHA512_Transform(
					ctx, (uint32_t *)(ctx->b8), 32);
			}
			/* Leave remainder in ctx->buf */
			memcpy(d, p, len);
		}
	}
	dcrypto_unlock();
}

struct sha512_digest *SHA512_hw_final(struct sha512_ctx *ctx)
{
	uint64_t cnt = ctx->count * 8;
	size_t i = (size_t) (ctx->count & (sizeof(ctx->b8) - 1));
	uint8_t *p = &ctx->b8[i];

	*p++ = 0x80;
	i++;

	dcrypto_init_and_lock();
	dcrypto_SHA512_setup();

	if (i > sizeof(ctx->b8) - 16U) {
		memset(p, 0, sizeof(ctx->b8) - i);
		dcrypto_SHA512_Transform(ctx, ctx->b32, 32);
		i = 0;
		p = ctx->b8;
	}

	memset(p, 0, sizeof(ctx->b8) - 8 - i);
	p += sizeof(ctx->b8) - 8 - i;

	for (i = 0; i < 8; ++i) {
		uint8_t tmp = (uint8_t)(cnt >> 56);
		cnt <<= 8;
		*p++ = tmp;
	}

	dcrypto_SHA512_Transform(ctx, ctx->b32, 32);

	p = ctx->b8;
	for (i = 0; i < 8; i++) {
		uint64_t tmp = ctx->state[i];
		*p++ = (uint8_t)(tmp >> 56);
		*p++ = (uint8_t)(tmp >> 48);
		*p++ = (uint8_t)(tmp >> 40);
		*p++ = (uint8_t)(tmp >> 32);
		*p++ = (uint8_t)(tmp >> 24);
		*p++ = (uint8_t)(tmp >> 16);
		*p++ = (uint8_t)(tmp >> 8);
		*p++ = (uint8_t)(tmp >> 0);
	}

	dcrypto_unlock();
	return &ctx->digest;
}

BUILD_ASSERT(sizeof(union hash_ctx) >= sizeof(struct sha512_ctx));
BUILD_ASSERT(sizeof(union hash_ctx) >= sizeof(struct sha384_ctx));

static void SHA512_hw_init_as_hash(union hash_ctx *const ctx)
	__alias(SHA512_hw_init);

static void SHA384_hw_init_as_hash(union hash_ctx *const ctx)
	__alias(SHA384_hw_init);

static void SHA512_hw_update_as_hash(union hash_ctx *const ctx,
				     const void *data, size_t len)
	__alias(SHA512_hw_update);

static const union sha_digests *SHA512_final_as_hash(union hash_ctx *const ctx)
	__alias(SHA512_hw_final);

void SHA384_hw_update(struct sha384_ctx *const ctx, const void *data,
		      size_t len) __alias(SHA512_hw_update);

const struct sha384_digest *SHA384_hw_final(struct sha384_ctx *const ctx)
	__alias(SHA512_hw_final);

const struct sha512_digest *SHA512_hw_hash(const void *data, size_t len,
					   struct sha512_digest *digest)
{
	struct sha512_ctx ctx;

	SHA512_hw_init(&ctx);
	SHA512_hw_update(&ctx, data, len);
	memcpy(digest, SHA512_hw_final(&ctx), SHA512_DIGEST_SIZE);

	return digest;
}

void SHA512_hw_init(struct sha512_ctx *ctx)
{
	static const struct hash_vtable dcrypto_SHA512_VTAB = {
		SHA512_hw_init_as_hash,	  SHA512_hw_update_as_hash,
		SHA512_final_as_hash,	  HMAC_sw_final,
		SHA512_DIGEST_SIZE,	  SHA512_BLOCK_SIZE,
		sizeof(struct sha512_ctx)
	};
	static const uint64_t sha512_init[SHA512_DIGEST_DWORDS] = {
		0x6a09e667f3bcc908ll, 0xbb67ae8584caa73bll,
		0x3c6ef372fe94f82bll, 0xa54ff53a5f1d36f1ll,
		0x510e527fade682d1ll, 0x9b05688c2b3e6c1fll,
		0x1f83d9abfb41bd6bll, 0x5be0cd19137e2179ll
	};

	memcpy(ctx->state, sha512_init, sizeof(ctx->state));
	ctx->count = 0;
	ctx->f = &dcrypto_SHA512_VTAB;
}

const struct sha384_digest *SHA384_hw_hash(const void *data, size_t len,
			      struct sha384_digest *digest)
{
	struct sha384_ctx ctx;

	SHA384_hw_init(&ctx);
	SHA384_hw_update(&ctx, data, len);
	memcpy(digest->b8, SHA384_hw_final(&ctx)->b8, SHA384_DIGEST_SIZE);
	return digest;
}

void SHA384_hw_init(struct sha512_ctx *ctx)
{
	static const struct hash_vtable dcrypto_SHA384_VTAB = {
		SHA384_hw_init_as_hash,	  SHA512_hw_update_as_hash,
		SHA512_final_as_hash,	  HMAC_sw_final,
		SHA384_DIGEST_SIZE,	  SHA512_BLOCK_SIZE,
		sizeof(struct sha384_ctx)
	};
	static const uint64_t sha384_init[SHA512_DIGEST_DWORDS] = {
		0xcbbb9d5dc1059ed8ll, 0x629a292a367cd507ll,
		0x9159015a3070dd17ll, 0x152fecd8f70e5939ll,
		0x67332667ffc00b31ll, 0x8eb44a8768581511ll,
		0xdb0c2e0d64f98fa7ll, 0x47b5481dbefa4fa4ll
	};

	memcpy(ctx->state, sha384_init, sizeof(ctx->state));

	ctx->count = 0;
	ctx->f = &dcrypto_SHA384_VTAB;
}

#if defined(CONFIG_SHA512_HW_EQ_SW)
/**
 * Make sw version equal to hw. Unlike SHA2-256, dcrypto implementation
 * of SHA2-512/384 allows to save context, so can fully replace software
 * implementation.
 */
const struct sha512_digest *SHA512_sw_hash(const void *data, size_t len,
					   struct sha512_digest *digest)
	__alias(SHA512_hw_hash);


void SHA512_sw_init(struct sha512_ctx *const ctx) __alias(SHA512_hw_init);

void SHA512_sw_update(struct sha512_ctx *const ctx, const void *data,
		      size_t len) __alias(SHA512_hw_update);

const struct sha512_digest *SHA512_sw_final(struct sha512_ctx *const ctx)
	__alias(SHA512_hw_final);


const struct sha384_digest *SHA384_sw_hash(const void *data, size_t len,
					   struct sha384_digest *digest)
	__alias(SHA384_hw_hash);

void SHA384_sw_init(struct sha384_ctx *const ctx) __alias(SHA384_hw_init);

void SHA384_sw_update(struct sha384_ctx *const ctx, const void *data,
		      size_t len) __alias(SHA384_hw_update);

const struct sha384_digest *SHA384_sw_final(struct sha384_ctx *const ctx)
	__alias(SHA384_hw_final);

#endif

#ifdef CRYPTO_TEST_SETUP

static uint32_t msg[256]; // 1KB
static int msg_len;
static int msg_loops;
static struct sha512_ctx sw;
static struct sha512_ctx hw;
static const struct sha512_digest *sw_digest;
static const struct sha512_digest *hw_digest;
static uint32_t t_sw;
static uint32_t t_hw;

static void run_sha512_cmd(void)
{
	int i;

	t_transform = 0;
	t_dcrypto = 0;
	t_sw = 0;
	t_hw = 0;

	START_PROFILE(t_sw)
	SHA512_sw_init(&sw);
	for (i = 0; i < msg_loops; ++i) {
		SHA512_update(&sw, msg, msg_len);
	}
	sw_digest = SHA512_final(&sw);
	END_PROFILE(t_sw)

	watchdog_reload();

	START_PROFILE(t_hw)
	SHA512_hw_init(&hw);
	for (i = 0; i < msg_loops; ++i) {
		SHA512_update(&hw, msg, msg_len);
	}
	hw_digest = SHA512_final(&hw);
	END_PROFILE(t_hw)

	watchdog_reload();

	ccprintf("sw(%u):\n", t_sw);
	for (i = 0; i < SHA512_DIGEST_SIZE; ++i)
		ccprintf("%02x", sw_digest->b8[i]);
	ccprintf("\n");

	ccprintf("hw(%u/%u/%u):\n", t_hw, t_transform, t_dcrypto);
	for (i = 0; i < SHA512_DIGEST_SIZE; ++i)
		ccprintf("%02x", hw_digest->b8[i]);
	ccprintf("\n");
}

static int cmd_sha512_bench(int argc, char *argv[])
{
	memset(msg, '!', sizeof(msg));

	if (argc > 1) {
		msg_loops = 1;
		msg_len = strlen(argv[1]);
		memcpy(msg, argv[1], msg_len);
	} else {
		msg_loops = 64; // benchmark 64K
		msg_len = sizeof(msg);
	}

	run_sha512_cmd();

	return EC_SUCCESS;
}
DECLARE_SAFE_CONSOLE_COMMAND(sha512_bench, cmd_sha512_bench, NULL, NULL);

static int cmd_sha512_test(int argc, char *argv[])
{
	size_t i;

	ccprintf("sha512 self-test started!\n");

	for (i = 0; i < 129; ++i) {
		memset(msg, i, i);
		watchdog_reload();
		SHA512_sw_init(&sw);
		SHA512_update(&sw, msg, i);
		sw_digest = SHA512_final(&sw);

		SHA512_hw_init(&hw);
		SHA512_update(&hw, msg, i);
		hw_digest = SHA512_final(&hw);

		if (memcmp(sw_digest, hw_digest, SHA512_DIGEST_SIZE) != 0) {
			ccprintf("sha512 self-test fail at %d!\n", i);
			cflush();
		}
	}

	ccprintf("sha512 self-test PASS!\n");

	return EC_SUCCESS;
}
DECLARE_SAFE_CONSOLE_COMMAND(sha512_test, cmd_sha512_test, NULL, NULL);

#endif /* CRYPTO_TEST_SETUP */
