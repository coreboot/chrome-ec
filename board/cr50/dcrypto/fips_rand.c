/* Copyright 2020 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "console.h"
#include "internal.h"
#include "flash_log.h"
#include "init_chip.h"
#include "registers.h"
#include "task.h"
#include "timer.h"
#include "util.h"

/**
 * FIPS-compliant CR50-wide DRBG
 * Since raw TRNG input shouldn't be used as random number generator,
 * all FIPS-compliant code use DRBG, seeded from TRNG
 */
struct drbg_ctx fips_drbg;

/**
 * NIST FIPS TRNG health tests (NIST SP 800-90B 4.3)
 * If any of the approved continuous health tests are used by the entropy
 * source, the false positive probability for these tests shall be set to
 * at least 2^-50
 */


/* state data for TRNG health test */
static struct {
	uint32_t count; /* number of 1-bits in APT test window */
	int last_clz; /* running count of leading 0 bits */
	int last_clo; /* running count of leading 1 bits */
	uint8_t pops[APT_WINDOW_SIZE_NWORDS];
	uint8_t rct_count; /* current windows size for RCT */
	uint8_t oldest; /* position in APT window */
	bool apt_initialized; /* flag APT window is filled with data */
} rand_state;

/**
 * We use FIPS_UNINITIALIZED as default (zero) value to accumulate
 * errors, so check it is really zero.
 */
BUILD_ASSERT(FIPS_UNINITIALIZED == 0);

/**
 * NIST SP 800-90B 4.4.1
 * The repetition count test detects abnormal runs of 0s or 1s.
 * RCT_CUTOFF_BITS must be >= 32.
 * If a single value appears more than 100/H times in a row,
 * the tests must detect this with high probability.
 *
 * This implementation assumes TRNG is configured to produce 1-bit
 * readings, packed into 32-bit words.
 * @return false if test failed
 */
static enum fips_status repetition_count_test(uint32_t rnd)
{
	uint32_t clz, ctz, clo, cto;

	/* count repeating 0 and 1 bits from each side */
	clz = count_leading_zeros(rnd);  /* # of leading 0s */
	ctz = count_trailing_zeros(rnd);  /* # of trailing 0s */
	clo = count_leading_zeros(~rnd); /* # of leading 1s */
	cto = count_trailing_zeros(~rnd); /* # of trailing 1s */

	/**
	 * check that number of trailing 0/1 in current sample added to
	 * leading 0/1 of previous sample is less than cut off value, so
	 * we don't have long repetitive series of 0s or 1s
	 */
	if ((ctz + rand_state.last_clz >= RCT_CUTOFF_SAMPLES) ||
	    (cto + rand_state.last_clo >= RCT_CUTOFF_SAMPLES))
		return FIPS_FATAL_TRNG_RCT;

	/**
	 * merge series of repetitive values - update running counters in
	 * such way that if current sample is all 0s then add previous
	 * counter of zeros to current number which will be 32,
	 * otherwise (we had 1s) - just use current value. Same for 1s
	 */
	if (rnd == 0U) /* if all 32 samples are 0s */
		clz += rand_state.last_clz;

	if (rnd == ~0U) /* if all 32 samples are 1s */
		clo += rand_state.last_clo;
	rand_state.last_clz = clz;
	rand_state.last_clo = clo;

	/* check we collected enough bits for statistics */
	if (rand_state.rct_count < RCT_CUTOFF_WORDS)
		++rand_state.rct_count;
	return FIPS_UNINITIALIZED;
}

static int misbalanced(uint32_t count)
{
	return count > APT_CUTOFF_SAMPLES ||
	       count < APT_WINDOW_SIZE_BITS - APT_CUTOFF_SAMPLES;
}

/* Returns number of set bits (1s) in 32-bit word */
static int popcount(uint32_t x)
{
	x = x - ((x >> 1) & 0x55555555);
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	x = (x + (x >> 4)) & 0x0F0F0F0F;
	x = x + (x >> 8);
	x = x + (x >> 16);
	return x & 0x0000003F;
}

/**
 * NIST SP 800-90B 4.4.2 Adaptive Proportion Test.
 * Implementation for 1-bit alphabet.
 * Instead of storing actual samples we can store pop counts
 * of each 32bit reading, which would fit in 8-bit.
 */
static enum fips_status adaptive_proportion_test(uint32_t rnd)
{
	/* update rolling count */
	rand_state.count -= rand_state.pops[rand_state.oldest];
	/**
	 * Since we have 1-bit samples, in order to have running window to
	 * count ratio of 1s and 0s in it we can just store number of 1s in
	 * each 32-bit sample which requires 1 byte vs. 4 bytes.
	 * number of zeros  =  32  - number of 1s
	 */
	rand_state.pops[rand_state.oldest] = popcount(rnd);

	/* update rolling count with current sample statistics */
	rand_state.count += rand_state.pops[rand_state.oldest];
	if (++rand_state.oldest >= APT_WINDOW_SIZE_NWORDS) {
		rand_state.apt_initialized = true; /* saw full window */
		rand_state.oldest = 0;
	}
	/* check when initialized */
	if (rand_state.apt_initialized && misbalanced(rand_state.count))
		return FIPS_FATAL_TRNG_APT;
	return FIPS_UNINITIALIZED;
}

static bool fips_powerup_passed(void)
{
	return rand_state.rct_count >= RCT_CUTOFF_WORDS &&
	       rand_state.apt_initialized;
}

/**
 * Get random from TRNG and run continuous health tests.
 * it is also can simulate stuck-bit error
 * @param power_up if true indicates warm-up mode
 * @return random value from TRNG
 */
static uint64_t fips_trng32(void)
{
	uint64_t r;
	uint32_t remaining_tries = 4;
	enum fips_status error = FIPS_UNINITIALIZED;

	do {
		r = read_rand();

		/* We can't read from TRNG. read_rand() made several tries. */
		if (!rand_valid(r)) {
			fips_set_status(FIPS_FATAL_TRNG_OTHER);
			break;
		}
		error = repetition_count_test((uint32_t)r);
		error |= adaptive_proportion_test((uint32_t)r);
		remaining_tries--;
		/* Repeat several times if statistical tests doesn't pass. */
	} while (remaining_tries && error != FIPS_UNINITIALIZED);

	if (error != FIPS_UNINITIALIZED) {
		fips_set_status(error);
		r = (uint32_t)r; /* Set result as invalid. */
	}

	return r;
}

uint64_t fips_trng_rand32(void)
{
	if (!fips_crypto_allowed())
		return 0;

	return fips_trng32();
}

bool fips_trng_bytes(void *buffer, size_t len)
{
	uint8_t *buf = (uint8_t *)buffer;
	size_t random_togo = 0;
	uint64_t rand;
	uint32_t r;

	if (!fips_crypto_allowed())
		return false;
	/**
	 * Retrieve random numbers in 4 byte quantities and pack as many bytes
	 * as needed into 'buffer'. If len is not divisible by 4, the
	 * remaining random bytes get dropped.
	 */
	while (len--) {
		if (!random_togo) {
			rand = fips_trng32();
			if (!rand_valid(rand))
				return false;
			r = (uint32_t)rand;
			random_togo = sizeof(r);
		}
		*buf++ = (uint8_t)r;
		random_togo--;
		r >>= 8;
	}
	return true;
}

/* FIPS TRNG power-up tests */
bool fips_trng_startup(int stage)
{
	uint64_t r;

	if (!stage) {
		/**
		 * To hide TRNG latency, split it into 2 stages.
		 * at stage 0, initialize variables
		 */
		rand_state.rct_count = 0;
		rand_state.apt_initialized = 0;
	}
	/* Startup tests per NIST SP800-90B, Section 4 */
	/* 4096 1-bit samples, in 2 steps, 2048 bit in each */
	for (uint32_t i = 0; i < (TRNG_INIT_WORDS) / 2; i++) {
		r = fips_trng32();

		if (!rand_valid(r))
			return false;
	}
	/* Also update seed for fast randoms. */
	set_fast_random_seed((uint32_t)r);
	return fips_powerup_passed();
}

/* Assuming H=0.77, we need 571 bits from TRNG to get 440 bits. */
#define ENTROPY_SIZE_BITS  571
#define ENTROPY_SIZE_WORDS (BITS_TO_WORDS(ENTROPY_SIZE_BITS))

bool fips_drbg_init(void)
{
	/* Buffer for Entropy + Nonce for DRBG initialization. */
	uint32_t entropy_input[ENTROPY_SIZE_WORDS];

	if (!fips_crypto_allowed())
		return false;

	if (hmac_drbg_ctx_valid(&fips_drbg))
		return true;

	/**
	 * Get entropy + nonce from TRNG. Assume H>=0.77.
	 */
	if (!fips_trng_bytes(entropy_input, sizeof(entropy_input)))
		return false;

	/**
	 * Pass combined seed containing total 571 bits of entropy and nonce,
	 * and assuming H=0.77, we will get total entropy in seed as 440 bits
	 * as defined for HMAC DBRG in NIST SP 800-90Ar1 B.2.
	 * Required minimum entropy for the entropy input at instantiation =
	 * (3/2) security_strength (this includes the entropy required for the
	 * nonce). For 256-bit security, this means at least 384 bits.
	 *
	 * Maximum length of the personalization string = 160 bits.
	 * Maximum length of the entropy input = 1000 bits.
	 *
	 * Reseed_interval = 1000 requests.
	 */
	hmac_drbg_init(&fips_drbg, &entropy_input, sizeof(entropy_input), NULL,
		       0, NULL, 0, 1000);

	always_memset(entropy_input, 0, sizeof(entropy_input));
	return true;
}

bool fips_rand_bytes(void *buffer, size_t len)
{
	/**
	 * make sure cr50 DRBG is initialized after power-on or resume,
	 * but do it on first use to minimize latency of board_init()
	 *
	 * fips_drbg_init() also checks for fips_crypto_allowed().
	 */
	if (!fips_drbg_init())
		return false;

	/* HMAC_DRBG can only return up to 7500 bits in a single request */
	while (len) {
		size_t request = MIN(len, HMAC_DRBG_MAX_OUTPUT_SIZE);

		if (hmac_drbg_generate(&fips_drbg, buffer, request, NULL, 0) !=
		    DCRYPTO_OK)
			return false;
		len -= request;
		buffer += request;
	}
	return true;
}

#ifndef CRYPTO_TEST_CMD_RAND_PERF
#define CRYPTO_TEST_CMD_RAND_PERF 0
#endif

#if defined(CRYPTO_TEST_SETUP) && CRYPTO_TEST_CMD_RAND_PERF
#include "endian.h"
#include "extension.h"
#include "watchdog.h"

static int cmd_rand_perf(int argc, char **argv)
{
	uint64_t starttime;
	static uint32_t buf[SHA256_DIGEST_WORDS];
	int j, k;

	starttime = get_time().val;

	/* run power-up tests to measure raw performance */
	if (fips_trng_startup(0) && fips_trng_startup(1)) {
		starttime = get_time().val - starttime;
		ccprintf("time for fips_trng_startup = %llu\n", starttime);
	} else
		ccprintf("TRNG power up test failed\n");

	cflush();

	starttime = get_time().val;
	fips_drbg_init();
	starttime = get_time().val - starttime;
	ccprintf("time for drbg_init = %llu\n", starttime);
	cflush();
	starttime = get_time().val;
	for (k = 0; k < 10; k++) {
		for (j = 0; j < 100; j++)
			if (!fips_rand_bytes(buf, sizeof(buf))) {
				ccprintf("DRBG test failed\n");
				return EC_ERROR_HW_INTERNAL;
			}
				;
		watchdog_reload();
		cflush();
	}
	starttime = get_time().val - starttime;
	ccprintf("time for 1000 drbg reads = %llu\n", starttime);
	cflush();

	starttime = get_time().val;
	for (k = 0; k < 10; k++) {
		for (j = 0; j < 100; j++)
			if (!fips_trng_bytes(&buf, sizeof(buf))) {
				ccprintf("FIPS TRNG error\n");
				return EC_ERROR_HW_INTERNAL;
			}
		watchdog_reload();
	}
	starttime = get_time().val - starttime;
	ccprintf("time for 1000 fips_trng_byte() = %llu\n", starttime);
	cflush();

	return 0;
}

DECLARE_SAFE_CONSOLE_COMMAND(rand_perf, cmd_rand_perf, NULL, NULL);

#endif /* CRYPTO_TEST_SETUP */
