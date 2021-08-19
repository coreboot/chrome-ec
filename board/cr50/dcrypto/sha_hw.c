/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "dcrypto.h"
#include "fips.h"
#include "internal.h"
#include "registers.h"

#ifdef SECTION_IS_RO
/* RO is single threaded. */
#define mutex_lock(x)
#define mutex_unlock(x)
static inline int dcrypto_grab_sha_hw(void)
{
	return 1;
}
static inline void dcrypto_release_sha_hw(void)
{
}
#else
#include "task.h"
static struct mutex hw_busy_mutex;

static bool hw_busy;

int dcrypto_grab_sha_hw(void)
{
	int rv = 0;

	fips_vtable->mutex_lock(&hw_busy_mutex);
	if (!hw_busy) {
		rv = 1;
		hw_busy = true;
	}
	fips_vtable->mutex_unlock(&hw_busy_mutex);

	return rv;
}

void dcrypto_release_sha_hw(void)
{
	fips_vtable->mutex_lock(&hw_busy_mutex);
	hw_busy = false;
	fips_vtable->mutex_unlock(&hw_busy_mutex);
}

#endif /* ! SECTION_IS_RO */

static void dcrypto_sha_wait(enum sha_mode mode, uint32_t *digest)
{
	int i;
	const int digest_len = (mode == SHA1_MODE) ? SHA1_DIGEST_SIZE :
							   SHA256_DIGEST_SIZE;

	/* Stop LIVESTREAM mode. */
	GREG32(KEYMGR, SHA_TRIG) = GC_KEYMGR_SHA_TRIG_TRIG_STOP_MASK;

	/* Wait for SHA DONE interrupt. */
	while (!GREG32(KEYMGR, SHA_ITOP))
		;

	/* Read out final digest. */
	for (i = 0; i < digest_len / 4; ++i)
		*digest++ = GR_KEYMGR_SHA_HASH(i);
	dcrypto_release_sha_hw();
}

static void dcrypto_sha_update(union hash_ctx *unused, const void *data,
			       size_t n)
{
	const uint8_t *bp = (const uint8_t *)data;
	const uint32_t *wp;

	/* Feed unaligned start bytes. */
	while (n != 0 && ((uint32_t)bp & 3)) {
		GREG8(KEYMGR, SHA_INPUT_FIFO) = *bp++;
		n -= 1;
	}

	/* Feed groups of aligned words. */
	wp = (uint32_t *)bp;
	while (n >= 8 * 4) {
		GREG32(KEYMGR, SHA_INPUT_FIFO) = *wp++;
		GREG32(KEYMGR, SHA_INPUT_FIFO) = *wp++;
		GREG32(KEYMGR, SHA_INPUT_FIFO) = *wp++;
		GREG32(KEYMGR, SHA_INPUT_FIFO) = *wp++;
		GREG32(KEYMGR, SHA_INPUT_FIFO) = *wp++;
		GREG32(KEYMGR, SHA_INPUT_FIFO) = *wp++;
		GREG32(KEYMGR, SHA_INPUT_FIFO) = *wp++;
		GREG32(KEYMGR, SHA_INPUT_FIFO) = *wp++;
		n -= 8 * 4;
	}
	/* Feed individual aligned words. */
	while (n >= 4) {
		GREG32(KEYMGR, SHA_INPUT_FIFO) = *wp++;
		n -= 4;
	}

	/* Feed remaining bytes. */
	bp = (uint8_t *)wp;
	while (n != 0) {
		GREG8(KEYMGR, SHA_INPUT_FIFO) = *bp++;
		n -= 1;
	}
}

static void dcrypto_sha_init(enum sha_mode mode)
{
	int val;

	/* Stop LIVESTREAM mode, in case final() was not called. */
	GREG32(KEYMGR, SHA_TRIG) = GC_KEYMGR_SHA_TRIG_TRIG_STOP_MASK;
	/* Clear interrupt status. */
	GREG32(KEYMGR, SHA_ITOP) = 0;

	/* Enable streaming mode. */
	val = GC_KEYMGR_SHA_CFG_EN_LIVESTREAM_MASK;
	/* Enable SHA DONE interrupt. */
	val |= GC_KEYMGR_SHA_CFG_EN_INT_EN_DONE_MASK;
	/* Select SHA mode. */
	if (mode == SHA1_MODE)
		val |= GC_KEYMGR_SHA_CFG_EN_SHA1_MASK;
	GREG32(KEYMGR, SHA_CFG_EN) = val;

	/* Turn off random nops (which are enabled by default). */
	GWRITE_FIELD(KEYMGR, SHA_RAND_STALL_CTL, STALL_EN, 0);
	/* Configure random nop percentage at 12%. */
	GWRITE_FIELD(KEYMGR, SHA_RAND_STALL_CTL, FREQ, 2);
	/* Now turn on random nops. */
	GWRITE_FIELD(KEYMGR, SHA_RAND_STALL_CTL, STALL_EN, 1);

	/* Start SHA engine. */
	GREG32(KEYMGR, SHA_TRIG) = GC_KEYMGR_SHA_TRIG_TRIG_GO_MASK;
}

static const struct sha1_digest *dcrypto_sha1_final(union hash_ctx *ctx)
{
	dcrypto_sha_wait(SHA1_MODE, ctx->sha1.digest.b32);
	return &ctx->sha1.digest;
}

static const struct sha256_digest *dcrypto_sha256_final(union hash_ctx *ctx)
{
	dcrypto_sha_wait(SHA256_MODE, ctx->sha256.digest.b32);
	return &ctx->sha256.digest;
}

static const union sha_digests *dcrypto_sha256_final_as_hash(
	union hash_ctx *const ctx) __alias(dcrypto_sha256_final);
static const union sha_digests *dcrypto_sha1_final_as_hash(
	union hash_ctx *const ctx) __alias(dcrypto_sha1_final);

static void dcrypto_sha_hash(enum sha_mode mode, const uint8_t *data, size_t n,
			     uint32_t *digest)
{
	dcrypto_sha_init(mode);
	dcrypto_sha_update(NULL, data, n);
	dcrypto_sha_wait(mode, digest);
}

/* Requires dcrypto_grab_sha_hw() to be called first. */
static void dcrypto_sha1_init(union hash_ctx *ctx)
{
	static const struct hash_vtable hw_sha1_vtab = {
		dcrypto_sha1_init,	    dcrypto_sha_update,
		dcrypto_sha1_final_as_hash, HMAC_sw_final,
		SHA1_DIGEST_SIZE,	    SHA1_BLOCK_SIZE,
		sizeof(struct sha1_ctx)
	};

	ctx->f = &hw_sha1_vtab;
	dcrypto_sha_init(SHA1_MODE);
}

static void dcrypto_sha256_init(union hash_ctx *ctx)
{
	/* Hardware SHA implementation. */
	static const struct hash_vtable HW_SHA256_VTAB = {
		dcrypto_sha256_init,	      dcrypto_sha_update,
		dcrypto_sha256_final_as_hash, HMAC_sw_final,
		SHA256_DIGEST_SIZE,	      SHA256_BLOCK_SIZE,
		sizeof(struct sha256_ctx)
	};

	ctx->f = &HW_SHA256_VTAB;
	dcrypto_sha_init(SHA256_MODE);
}

/**
 * Select and initialize either the software or hardware
 * implementation.  If "multi-threaded" behaviour is required, then
 * callers must specifically use software version SHA1_sw_init(). This
 * is because SHA1 state internal to the hardware cannot be extracted, so
 * it is not possible to suspend and resume a hardware based SHA operation.
 *
 * Hardware implementation is selected based on availability. Hardware is
 * considered to be in use between init() and finished() calls. If hardware
 * is not available, fall back to software implementation.
 */
void SHA1_hw_init(struct sha1_ctx *ctx)
{
	if (dcrypto_grab_sha_hw())
		dcrypto_sha1_init((union hash_ctx *)ctx);
	else
		SHA1_sw_init(ctx);
}

/**
 * Select and initialize either the software or hardware
 * implementation.  If "multi-threaded" behaviour is required, then
 * callers must specifically use software version SHA256_sw_init(). This
 * is because SHA256 state internal to the hardware cannot be extracted, so
 * it is not possible to suspend and resume a hardware based SHA operation.
 *
 * Hardware implementation is selected based on availability. Hardware is
 * considered to be in use between init() and finished() calls. If hardware
 * is not available, fall back to software implementation.
 */
void SHA256_hw_init(struct sha256_ctx *ctx)
{
	if (dcrypto_grab_sha_hw())
		dcrypto_sha256_init((union hash_ctx *)ctx);
	else
		SHA256_sw_init(ctx);
}

const struct sha1_digest *SHA1_hw_hash(const void *data, size_t n,
					struct sha1_digest *digest)
{
	if (dcrypto_grab_sha_hw())
		/* dcrypto_sha_wait() will release the hw. */
		dcrypto_sha_hash(SHA1_MODE, data, n,  digest->b32);
	else
		SHA1_sw_hash(data, n, digest);
	return digest;
}

const struct sha256_digest *SHA256_hw_hash(const void *data, size_t n,
					  struct sha256_digest *digest)
{
	if (dcrypto_grab_sha_hw())
		/* dcrypto_sha_wait() will release the hw. */
		dcrypto_sha_hash(SHA256_MODE, data, n, digest->b32);
	else
		SHA256_sw_hash(data, n, digest);
	return digest;
}

/* For compatibility with chip/g code. */
const uint8_t *DCRYPTO_SHA1_hash(const void *data, size_t n, uint8_t *digest)
	__alias(SHA1_hw_hash);

/* TODO(b/195092622): initialize HW HMAC instead. */
void HMAC_SHA256_hw_init(struct hmac_sha256_ctx *ctx, const void *key,
			      size_t len)
{
	SHA256_hw_init(&ctx->hash);
	HMAC_sw_init((union hmac_ctx *)ctx, key, len);
}

const struct sha256_digest *
HMAC_SHA256_hw_final(struct hmac_sha256_ctx *ctx)
{
	return HMAC_SHA256_final(ctx);
}
