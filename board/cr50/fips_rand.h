/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef __EC_BOARD_CR50_FIPS_RAND_H
#define __EC_BOARD_CR50_FIPS_RAND_H

#include <stddef.h>
#include <string.h>

#include "common.h"
#include "dcrypto.h"
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TRNG_SAMPLE_BITS 1

/**
 * TRNG Health Tests
 *
 * If any of the approved continuous health tests are used by the entropy
 * source, the false positive probability for these tests shall be set to
 * at least 2^(-50)  (NIST SP 800-90B 4.3).
 * Reason for 2^(-50) vs 2^(-40) is to minimize impact to user experience
 * due to false positives.
 *
 * For H1 minimal assessed entropy H >=0.85 for 1-bit samples
 * using NIST Entropy Assessment tool.
 */

/**
 * The entropy source's startup tests shall run the continuous health tests
 * over at least 4096 consecutive samples. We use 1-bit samples
 */
#define TRNG_INIT_BITS	(4096 * TRNG_SAMPLE_BITS)
#define TRNG_INIT_WORDS (BITS_TO_WORDS(TRNG_INIT_BITS))

/**
 * (1) Repetition Count Test (RCT) NIST SP 800-90B 4.4.1
 * Cut off value is computed as:
 * c = ceil(1 + (-log2 alpha)/H);
 * alpha = 2^-50, H = 0.85; RCT_CUTOFF = CEIL(1+(50/0.85))
 */
#define RCT_CUTOFF_SAMPLES 60

/**
 * Number of 32-bit words containing RCT_CUTOFF_SAMPLES samples
 */
#define RCT_CUTOFF_WORDS (BITS_TO_WORDS(RCT_CUTOFF_SAMPLES))

/**
 * (2) Adaptive Proportion Test (APT), NIST SP 800-90B 4.4.2, Table 2
 */
#if TRNG_SAMPLE_BITS == 1
/* APT Windows size W = 1024 for 1 bit samples */
#define APT_WINDOW_SIZE_SAMPLES 1024
#else
/* or 512 samples if more than 1 bit per sample */
#define APT_WINDOW_SIZE_SAMPLES 512
#endif
#define APT_WINDOW_SIZE_BITS   (APT_WINDOW_SIZE_SAMPLES * TRNG_SAMPLE_BITS)
#define APT_WINDOW_SIZE_NWORDS (BITS_TO_WORDS(APT_WINDOW_SIZE_BITS))
/**
 * Cut off value = CRITBINOM(W, power(2,(-H)),1-α).
 * 692 = CRITBINOM(1024, power(2,(-0.85)), 1 - 2^(-50))
 */
#define APT_CUTOFF_SAMPLES 692

/**
 * FIPS-compliant TRNG startup.
 * The entropy source's startup tests shall run the continuous health tests
 * over at least 4096 consecutive samples.
 * Note: This function can throw FIPS_FATAL_TRNG error
 *
 * To hide latenccy of reading TRNG data, this test is executed in 2 stages
 * @param stage is 0 or 1, choosing the stage. On each stage 2048
 * samples are processed. Assuming that some other tasks can be executed
 * between stages, when TRNG FIFO if filled with samples.
 *
 * Some number of samples will be available in entropy_fifo
 */
bool fips_trng_startup(int stage);

bool fips_trng_bytes(void *buffer, size_t len);

/* initialize cr50-wide DRBG replacing rand */
bool fips_drbg_init(void);
/* mark cr50-wide DRBG as not initialized */
void fips_drbg_init_clear(void);

/* random bytes using FIPS-compliant HMAC_DRBG */
bool fips_rand_bytes(void *buffer, size_t len);

/* wrapper around dcrypto_p256_ecdsa_sign using FIPS-compliant HMAC_DRBG */
int fips_p256_ecdsa_sign(const p256_int *key, const p256_int *message,
			 p256_int *r, p256_int *s);
/**
 * wrapper around hmac_drbg_generate to automatically reseed drbg
 * when needed.
 */
enum hmac_result fips_hmac_drbg_generate_reseed(struct drbg_ctx *ctx, void *out,
						size_t out_len,
						const void *input,
						size_t input_len);
#ifdef __cplusplus
}
#endif
#endif /* ! __EC_BOARD_CR50_FIPS_RAND_H */
