/* Copyright 2015 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "internal.h"
#include "flash_log.h"
#include "dcrypto_regs.h"

/**
 * Define KEYMGR SHA/HMAC access structure.
 */
static volatile struct trng_reg *reg_trng = (void *)(GC_TRNG_BASE_ADDR);

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
 * Attempts to read TRNG_EMPTY before reporting a stall. Practically data should
 * be available in less than 0xfff cycles under normal conditions.
 * Test on boards with slow TRNG before reducing this number.
 */
#define TRNG_EMPTY_COUNT 0xfff

/**
 * Number of attempts to reset TRNG after stall is detected.
 */
#define TRNG_RESET_COUNT 32

void fips_init_trng(void)
{
	/*
	 * Most of the trng initialization requires high permissions. If RO has
	 * dropped the permission level, dont try to read or write these high
	 * permission registers because it will cause rolling reboots. RO
	 * should do the TRNG initialization before dropping the level.
	 *
	 * For Cr50 RO doesn't drop permission level and init_trng() is called
	 * by board_init() before dropping permissions.
	 */

	/**
	 * According to NIST SP 800-90B only vetted conditioning mechanism
	 * should be used for post-processing raw entropy.
	 * See SP 800-90B, 3.1.5.1 Using Vetted Conditioning Components.
	 * Use of non-vetted algorithms is governed in 3.1.5.2, but
	 * assumes conservative coefficient 0.85 for entropy estimate,
	 * which increase number of requests to TRNG to get desirable
	 * entropy and prevents from getting full entropy.
	 */
	reg_trng->post_processing  = 0;

	/**
	 * TRNG can return up to 16 bits at a time, but highest bits
	 * have lower entropy. Practically on Cr50 only 13 bits can be
	 * used - setting to higher value makes TRNG_EMPTY always set.
	 * Entropy assessed to be reasonable (one bit H > 0.85)
	 * for up to 8 bits [7..0].
	 * Time to get 32bit random is roughly 160/TRNG_SAMPLE_BITS us.
	 */
	reg_trng->slice_max_upper_limit = TRNG_SAMPLE_BITS - 1;

	/* lowest bit have highest entropy, so always start from it */
	reg_trng->slice_min_lower_limit = 0;

	/**
	 * Analog logic cannot create a value < 8 under normal operating
	 * conditions, but there's a chance that an attacker could coax
	 * them out.
	 * Bit 0 - Enable rejection for values outside of range specified
	 * by TRNG_ALLOWED_VALUES register
	 */
	reg_trng->secure_post_processing = 0x1;

	/**
	 * Since for FIPS settings we use TRNG_SAMPLE_BITS = 1,
	 * and take only bit 0 from internal 16 bit reading, no bias is
	 * created for bit 0 if allowed_min is set to 6, which
	 * actually means min accepted value is 8 (RTL adds +2).
	 * TRNG_ALLOWED_VALUES_MAX=0x04 (accept all values up to 2^16-1).
	 * So, range will be [8..65535], with probability for bit 0 and 1
	 * remaining 50/50.
	 */
	reg_trng->allowed_values = 0x26;

	reg_trng->timeout_counter = TRNG_EMPTY_COUNT;
	reg_trng->timeout_max_try = 4;
	reg_trng->power_down_b = 1;
	reg_trng->go_event = 1;
}

uint64_t read_rand(void)
{
	uint32_t empty_count = 0;
	uint8_t reset_count = 0;

#ifdef CRYPTO_TEST_SETUP
	/* Do we need to simulate error? */
	if (fips_break_cmd == FIPS_BREAK_TRNG)
		return (uint64_t)1UL << 32; /* Valid result, but value = 0. */
#endif

	/**
	 * Make sure we never hang in the loop - try at max TRNG_RESET_COUNT
	 * reset attempts, then return error
	 */
	while (reg_trng->empty && (reset_count < TRNG_RESET_COUNT)) {
		if ((reg_trng->fsm_state & GC_TRNG_FSM_STATE_FSM_IDLE_MASK) ||
		    empty_count > TRNG_EMPTY_COUNT) {
			/* TRNG timed out, restart */
			reg_trng->stop_work = 1;
			empty_count = 0;
			reset_count++;
			reg_trng->go_event = 1;
		}
		empty_count++;
	}
#ifdef CONFIG_FLASH_LOG
	/* Log stall count if happened .*/
	if (reset_count > 0)
		fips_vtable->flash_log_add_event(
			FE_LOG_TRNG_STALL, sizeof(reset_count), &reset_count);
#endif
	/**
	 * High 32-bits set to zero in case of error;
	 * otherwise value >> 32 == 1
	 */
	return (uint64_t)reg_trng->read_data |
	       ((uint64_t)(reset_count < TRNG_RESET_COUNT) << 32);
}

/* Local switch to test command. Enable when work on it. */
#ifndef CRYPTO_TEST_CMD_RAND
#define CRYPTO_TEST_CMD_RAND 0
#endif

#if !defined(SECTION_IS_RO) && defined(CRYPTO_TEST_SETUP)
#include "console.h"
#include "endian.h"
#include "extension.h"
#include "timer.h"
#include "watchdog.h"

#if CRYPTO_TEST_CMD_RAND
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
		uint64_t rnd;
		uint32_t rvalue;
		size_t size;

		rnd = fips_trng_rand32();
		if (!rand_valid(rnd)) {
			ccprintf("Failed reading TRNG.\n");
			return EC_ERROR_HW_INTERNAL;
		}
		rvalue = (uint32_t)rnd;
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

#endif /* CRYPTO_TEST_CMD_RAND */

/* For testing we need unchecked values from TRNG. */
static bool raw_rand_bytes(void *buffer, size_t len)
{
	size_t random_togo = 0;
	size_t buffer_index = 0;
	uint32_t random_value;
	uint8_t *buf = (uint8_t *) buffer;

	/*
	 * Retrieve random numbers in 4 byte quantities and pack as many bytes
	 * as needed into 'buffer'. If len is not divisible by 4, the
	 * remaining random bytes get dropped.
	 */
	while (buffer_index < len) {
		if (!random_togo) {
			uint64_t rnd = read_rand();

			if (!rand_valid(rnd))
				return false;

			random_value = (uint32_t)rnd;
			random_togo = sizeof(random_value);
		}
		buf[buffer_index++] = random_value >>
			((random_togo-- - 1) * 8);
	}
	return true;
}

/*
 * This extension command is similar to TPM2_GetRandom, but made
 * available for CRYPTO_TEST = 1 which disables TPM.
 * Command structure, shared out of band with the test driver running
 * on the host:
 *
 * field     |    size  |                  note
 * =========================================================================
 * text_len  |    2     | the number of random bytes to generate, big endian
 * type      |    1     | 0 - TRNG, 1 = FIPS TRNG, 2 = FIPS DRBG
 *           |          | 3 - TRNG after restart
 *           |          | other values reserved for extensions
 */
static enum vendor_cmd_rc trng_test(enum vendor_cmd_cc code, void *buf,
				    size_t input_size, size_t *response_size)
{
	uint16_t text_len;
	uint8_t *cmd = buf;
	uint8_t op_type = 0;

	if (input_size != sizeof(text_len) + 1) {
		*response_size = 0;
		return VENDOR_RC_BOGUS_ARGS;
	}

	text_len = be16toh(*(uint16_t *)cmd);
	op_type = cmd[sizeof(text_len)];

	if (text_len > *response_size) {
		*response_size = 0;
		return VENDOR_RC_BOGUS_ARGS;
	}

	switch (op_type) {
	case 3:
		/* Power down LDO, wait 1ms, power up. */
		reg_trng->power_down_b = 0;
		udelay(1000);
		reg_trng->power_down_b = 1;
		reg_trng->go_event = 1;
		/* Fall through */
	case 0:
		if (!raw_rand_bytes(buf, text_len))
			return VENDOR_RC_INTERNAL_ERROR;
		break;
	case 1:
		if (!fips_trng_bytes(buf, text_len))
			return VENDOR_RC_INTERNAL_ERROR;
		break;
	case 2:
		if (!fips_rand_bytes(buf, text_len))
			return VENDOR_RC_INTERNAL_ERROR;
		break;

	default:
		return VENDOR_RC_BOGUS_ARGS;
	}
	*response_size = text_len;
	return VENDOR_RC_SUCCESS;
}

DECLARE_VENDOR_COMMAND(VENDOR_CC_TRNG_TEST, trng_test);

#endif /* CRYPTO_TEST_SETUP */
