/* Copyright 2020 The ChromiumOS Authors
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
 * Probability of false positive in single APT/RCT test
 * defined as 2^(-TRNG_TEST_ALPHA).
 */
#define TRNG_TEST_ALPHA 39

/* Entropy estimate for H1 = 0.77 = 77/100 */
#define H_ENTROPY 77
#define H_ENTROPY_DIVISOR 100

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
 * RCT_CUTOFF = CEIL(1+(ALPHA/H)) = CEIL(1+(ALPHA*(1/H)))
 */
#define RCT_CUTOFF_SAMPLES                                              \
	(1 + (((TRNG_TEST_ALPHA * H_ENTROPY_DIVISOR) + H_ENTROPY - 1) / \
	      H_ENTROPY))

/* Our implementation supports only certain range of RCT_CUTOFF values. */
BUILD_ASSERT((RCT_CUTOFF_SAMPLES >= 1) && (RCT_CUTOFF_SAMPLES <= 63));

#if TRNG_TEST_ALPHA == 39
BUILD_ASSERT(RCT_CUTOFF_SAMPLES == 52);
#elif TRNG_TEST_ALPHA == 30
BUILD_ASSERT(RCT_CUTOFF_SAMPLES == 40);
#endif

/**
 * Number of 32-bit words containing RCT_CUTOFF_SAMPLES samples
 */
#define RCT_CUTOFF_WORDS (BITS_TO_WORDS(RCT_CUTOFF_SAMPLES))

/**
 * (2) Adaptive Proportion Test (APT), NIST SP 800-90B 4.4.2, Table 2
 */
/* We only support 1-bit alphabet for TRNG. */
BUILD_ASSERT(TRNG_SAMPLE_BITS == 1);
/* APT Windows size W = 1024 for 1 bit samples */
#define APT_WINDOW_SIZE_SAMPLES 1024
#define APT_WINDOW_SIZE_BITS   (APT_WINDOW_SIZE_SAMPLES * TRNG_SAMPLE_BITS)
#define APT_WINDOW_SIZE_NWORDS (BITS_TO_WORDS(APT_WINDOW_SIZE_BITS))

/**
 * Cut off value = CRITBINOM(W, power(2,(-H)),1-α).
 * 708 = CRITBINOM(1024, power(2,(-0.77)), 1 - 2^(-39))
 */
#if TRNG_TEST_ALPHA == 39
#define APT_CUTOFF_SAMPLES 708
#elif TRNG_TEST_ALPHA == 30
/* APT cut off for TRNG_TEST_ALPHA == 30 */
#define APT_CUTOFF_SAMPLES 694
#endif

/**
 * APT_CUTOFF should be larger than half of window size, but less
 * than windows size.
 */
BUILD_ASSERT((APT_CUTOFF_SAMPLES >= (APT_WINDOW_SIZE_SAMPLES / 2)) &&
	     (APT_CUTOFF_SAMPLES < APT_WINDOW_SIZE_SAMPLES));

#ifdef __cplusplus
}
#endif
#endif /* ! __EC_BOARD_CR50_FIPS_RAND_H */
