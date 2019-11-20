/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.h"
#include "flash_log.h"
#include "init_chip.h"
#include "registers.h"
#include "trng.h"
#include "watchdog.h"
#include "console.h"

/**
 * The H1 TRNG uses the collapse time of a ring oscillator (RO) that is
 * initialized in a 3x mode (three enable pulses) and eventually collapses
 * to a stable 1x mode as a result of accumulated jitter (thermal noise).
 * A Phase-Frequency Detector (PFD) compares the 3x RO to a reference
 * RO (1.5x) and captures the state of a counter that is incremented
 * from the reference RO. The resulting reference-cycles-to-collapse
 * distribution is log-normal, and truncation of the counter bits results in
 * a distribution that approaches uniform.
 *
 * TRNG_SAMPLE_BITS defines how many bits to use from the 16 bit counter
 * output coming from the analog unit. Entropy is highest in least significant
 * bits of counter. For FIPS-certified code use just Bit 0 - it provides
 * highest entropy, allows better security settings for TRNG and simplifies
 * implementation of continuous health tests.
 */
#ifndef TRNG_SAMPLE_BITS
#define TRNG_SAMPLE_BITS 1
#endif

/**
 * Attempts to read TRNG_EMPTY before reporting a stall.
 * Practically data should be available in less than 400
 * cycles under normal conditions.
 */
#define TRNG_EMPTY_COUNT 400

void init_trng(void)
{
#if (!(defined(CONFIG_CUSTOMIZED_RO) && defined(SECTION_IS_RO)))
	/*
	 * Most of the trng initialization requires high permissions. If RO has
	 * dropped the permission level, dont try to read or write these high
	 * permission registers because it will cause rolling reboots. RO
	 * should do the TRNG initialization before dropping the level.
	 */
	if (!runlevel_is_high())
		return;
#endif
	/**
	 * According to NIST SP 800-90B only vetted conditioning mechanism
	 * should be used for post-processing raw entropy.
	 * See SP 800-90B, 3.1.5.1 Using Vetted Conditioning Components.
	 * Use of non-vetted algorithms is governed in 3.1.5.2, but
	 * assumes conservative coefficient 0.85 for entropy estimate,
	 * which increase number of requests to TRNG to get desirable
	 * entropy and prevents from getting full entropy.
	 */
	GWRITE(TRNG, POST_PROCESSING_CTRL, 0);

	/**
	 * TRNG can return up to 16 bits at a time, but highest bits
	 * have lower entropy. Practically on Cr50 only 13 bits can be
	 * used - setting to higher value makes TRNG_EMPTY always set.
	 * Entropy assessed to be reasonable (one bit H > 0.85)
	 * for up to 8 bits [7..0].
	 * Time to get 32bit random is roughly 160/TRNG_SAMPLE_BITS us.
	 */
	GWRITE(TRNG, SLICE_MAX_UPPER_LIMIT, TRNG_SAMPLE_BITS - 1);

	/* lowest bit have highest entropy, so always start from it */
	GWRITE(TRNG, SLICE_MIN_LOWER_LIMIT, 0);

	/**
	 * Analog logic cannot create a value < 8 under normal operating
	 * conditions, but there's a chance that an attacker could coax
	 * them out.
	 * Bit 0 - Enable rejection for values outside of range specified
	 * by TRNG_ALLOWED_VALUES register
	 */
	GWRITE(TRNG, SECURE_POST_PROCESSING_CTRL, 0x1);

	/**
	 * Since for FIPS settings we use TRNG_SAMPLE_BITS = 1,
	 * and take only bit 0 from internal 16 bit reading, no bias is
	 * created for bit 0 if allowed_min is set to 6, which
	 * actually means min accepted value is 8 (RTL adds +2).
	 * TRNG_ALLOWED_VALUES_MAX=0x04 (accept all values up to 2^16-1).
	 * So, range will be [8..65535], with probability for bit 0 and 1
	 * remaining 50/50.
	 */
	GWRITE(TRNG, ALLOWED_VALUES_MIN, 0x26);

	GWRITE(TRNG, TIMEOUT_COUNTER, 0x7ff);
	GWRITE(TRNG, TIMEOUT_MAX_TRY_NUM, 4);
	GWRITE(TRNG, POWER_DOWN_B, 1);
	GWRITE(TRNG, GO_EVENT, 1);
}

uint32_t rand(void)
{	uint32_t empty_count = 0;

	while (GREAD(TRNG, EMPTY)) {
		if (GREAD_FIELD(TRNG, FSM_STATE, FSM_IDLE) ||
		    empty_count > TRNG_EMPTY_COUNT) {
			/* TRNG timed out, restart */
			GWRITE(TRNG, STOP_WORK, 1);
#if !defined(SECTION_IS_RO) && defined(CONFIG_FLASH_LOG)
			flash_log_add_event(FE_LOG_TRNG_STALL, 0, NULL);
#endif
			GWRITE(TRNG, GO_EVENT, 1);
			empty_count = 0;
		}
		empty_count++;
	}
	return GREAD(TRNG, READ_DATA);
}

void rand_bytes(void *buffer, size_t len)
{
	int random_togo = 0;
	int buffer_index = 0;
	uint32_t random_value;
	uint8_t *buf = (uint8_t *) buffer;

	/*
	 * Retrieve random numbers in 4 byte quantities and pack as many bytes
	 * as needed into 'buffer'. If len is not divisible by 4, the
	 * remaining random bytes get dropped.
	 */
	while (buffer_index < len) {
		if (!random_togo) {
			random_value = rand();
			random_togo = sizeof(random_value);
		}
		buf[buffer_index++] = random_value >>
			((random_togo-- - 1) * 8);
	}
}

#if !defined(SECTION_IS_RO) && defined(CRYPTO_TEST_SETUP)
#include "console.h"
#include "watchdog.h"

static void  print_rand_stat(uint32_t *histogram, size_t size)
{
	struct pair {
		uint32_t value;
		uint32_t count;
	};
	struct pair min;
	struct pair max;
	size_t count;

	min.count = ~0;
	max.count = 0;
	max.value = ~0;
	min.value = ~0;

	for (count = 0; count < size; count++) {
		if (histogram[count] > max.count) {
			max.count = histogram[count];
			max.value = count;
		}
		if (histogram[count] < min.count) {
			min.count = histogram[count];
			min.value = count;
		}
	}

	ccprintf("min %d(%d), max %d(%d)", min.count, min.value,
		 max.count, max.value);

	for (count = 0; count < size; count++) {
		if (!(count % 8)) {
			ccprintf("\n");
			cflush();
		}
		ccprintf(" %6d", histogram[count]);
	}
	ccprintf("\n");
}

/* histogram at byte level */
static uint32_t histogram[256];
/* histogram at level of TRNG samples */
static uint32_t histogram_trng[1 << TRNG_SAMPLE_BITS];

static int command_rand(int argc, char **argv)
{
	int count = 1000; /* Default number of cycles. */
	uint32_t val = 0, bits = 0;

	if (argc == 2)
		count = strtoi(argv[1], NULL, 10);

	memset(histogram, 0, sizeof(histogram));
	memset(histogram_trng, 0, sizeof(histogram_trng));
	ccprintf("Retrieving %d 32-bit random words.\n", count);
	while (count-- > 0) {
		uint32_t rvalue;
		int size;

		rvalue = rand();
		/* update byte-level histogram */
		for (size = 0; size < sizeof(rvalue); size++)
			histogram[((uint8_t *)&rvalue)[size]]++;

		/* update histogram on TRNG sample size level */
		val = (val | (rvalue << bits)) & ((1 << TRNG_SAMPLE_BITS) - 1);
		rvalue >>= TRNG_SAMPLE_BITS - bits;
		bits += 32;
		while (bits >= TRNG_SAMPLE_BITS) {
			histogram_trng[val]++;
			val = rvalue & ((1 << TRNG_SAMPLE_BITS) - 1);
			rvalue >>= TRNG_SAMPLE_BITS;
			bits -= TRNG_SAMPLE_BITS;
		};

		if (!(count % 10000))
			watchdog_reload();
	}
	ccprintf("Byte-level histogram:\n");
	print_rand_stat(histogram, ARRAY_SIZE(histogram));
	ccprintf("\nSample-level (%d bits) histogram:\n", TRNG_SAMPLE_BITS);
	print_rand_stat(histogram_trng, ARRAY_SIZE(histogram_trng));

	return EC_SUCCESS;
}
DECLARE_SAFE_CONSOLE_COMMAND(rand, command_rand, NULL, NULL);

#endif /* CRYPTO_TEST_SETUP */
