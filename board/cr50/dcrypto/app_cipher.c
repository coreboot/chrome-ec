/*
 * Copyright 2017 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "crypto_api.h"
#include "internal.h"
#include "registers.h"
#include "dcrypto_regs.h"
/**
 * Define KEYMGR AES access structure.
 */
static volatile struct keymgr_aes *reg_aes = (void *)(GC_KEYMGR_BASE_ADDR);

static inline void read_aes_fifo(uint32_t *outw)
{
	uint32_t w0, w1, w2, w3;

	w0 = reg_aes->rfifo_data;
	w1 = reg_aes->rfifo_data;
	w2 = reg_aes->rfifo_data;
	w3 = reg_aes->rfifo_data;
	outw[0] = w0;
	outw[1] = w1;
	outw[2] = w2;
	outw[3] = w3;
}

static inline void write_aes_fifo(const uint32_t *inw)
{
	uint32_t w0, w1, w2, w3;

	w0 = inw[0];
	w1 = inw[1];
	w2 = inw[2];
	w3 = inw[3];
	reg_aes->wfifo_data = w0;
	reg_aes->wfifo_data = w1;
	reg_aes->wfifo_data = w2;
	reg_aes->wfifo_data = w3;
}

/* The default build options compile for size (-Os); instruct the
 * compiler to optimize for speed here.  Incidentally -O produces
 * faster code than -O2!
 *
 * AES FIFOs are 32-bits wide by 16 entries deep (64-Bytes), so to
 * get maximum throughput try to load it at least at 50% and that's why we don't
 * read after the first 4 words are loaded, unless they are the only 4 words
 */
size_t __optimize("O")
	dcrypto_aes_process(uint32_t **out, const uint32_t **in, size_t len)
{
	uint32_t *outw = *out;
	const uint32_t *inw = *in;

	if (len >= 16) {
		write_aes_fifo(inw);
		inw += 4;
		len -= 16;

		while (len >= 16) {
			write_aes_fifo(inw);
			inw += 4;
			len -= 16;
			while (reg_aes->rfifo_empty)
				;

			read_aes_fifo(outw);
			outw += 4;
		}
		while (reg_aes->rfifo_empty)
			;

		read_aes_fifo(outw);
		outw += 4;
	}
	*in = inw;
	*out = outw;
	return len;
}

static int aes_init(enum dcrypto_appid appid)
{
	uint32_t aes_config;

	/* Setup USR-based application key. */
	if (!DCRYPTO_appkey_init(appid))
		return 0;

	/* Configure AES engine. */
	aes_config = (CTRL_NO_SOFT_RESET << GC_KEYMGR_AES_CTRL_RESET_LSB) |
		     (2 << GC_KEYMGR_AES_CTRL_KEYSIZE_LSB) |
		     (CIPHER_MODE_CTR << GC_KEYMGR_AES_CTRL_CIPHER_MODE_LSB) |
		     (ENCRYPT_MODE << GC_KEYMGR_AES_CTRL_ENC_MODE_LSB) |
		     (CTRL_CTR_BIG_ENDIAN << GC_KEYMGR_AES_CTRL_CTR_ENDIAN_LSB);
	reg_aes->ctrl = aes_config;

	/*
	 * For fixed-key, bulk ciphering, turn off random nops (which
	 * are enabled by default).
	 */
	reg_aes->rand_stall = 0;

	/* Enable hidden key usage, each appid gets its own
	 * USR, with USR0 starting at 0x2a0.
	 */
	reg_aes->use_hidden_key = GC_KEYMGR_AES_USE_HIDDEN_KEY_ENABLE_MASK |
				  ((0x2a0 + (appid * 2))
				   << GC_KEYMGR_AES_USE_HIDDEN_KEY_INDEX_LSB);

	reg_aes->ctrl = aes_config |
			(CTRL_ENABLE << GC_KEYMGR_AES_CTRL_ENABLE_LSB);

	/* Wait for key-expansion. */
	reg_aes->key_start = 1;
	while (reg_aes->key_start)
		;

	/* Check for errors (e.g. USR not correctly setup. */
	if (GREG32(KEYMGR, HKEY_ERR_FLAGS))
		return 0;

	return 1;
}

int DCRYPTO_app_cipher(enum dcrypto_appid appid, const void *salt,
		void *out, const void *in, size_t len)
{
	const uint32_t *inw = in;
	uint32_t *outw = out;

	/* Test pointers for word alignment. */
	if (((uintptr_t) in & 0x03) || ((uintptr_t) out & 0x03))
		return 0;

	{
		/* Initialize key, and AES engine. */
		BUILD_ASSERT(CIPHER_SALT_SIZE == 16);
		if (!aes_init(appid))
			return 0;
		/* Set IV. */
		DCRYPTO_aes_write_iv(salt);
	}

	len = dcrypto_aes_process(&outw, &inw, len);

	if (len) {
		/* Cipher the final partial block */
		uint32_t tmpin[4];
		uint32_t tmpout[4];
		const uint32_t *tmpinw;
		uint32_t *tmpoutw;

		tmpinw = tmpin;
		tmpoutw = tmpout;

		memcpy(tmpin, inw, len);
		dcrypto_aes_process(&tmpoutw, &tmpinw, 16);
		memcpy(outw, tmpout, len);
	}

	DCRYPTO_appkey_finish();
	return 1;
}

#ifndef CRYPTO_TEST_CIPHER
#define CRYPTO_TEST_CIPHER 0
#endif

#if defined(CRYPTO_TEST_SETUP) && CRYPTO_TEST_CIPHER

#include "common.h"
#include "console.h"
#include "hooks.h"
#include "shared_mem.h"
#include "task.h"
#include "timer.h"
#include "watchdog.h"

#define HEAP_HEAD_ROOM 0x400
static uint32_t number_of_iterations;
static uint8_t result;

/* Staticstics for ecrypt and decryp passes. */
struct ciph_stats {
	uint16_t min_time;
	uint16_t max_time;
	uint32_t total_time;
} __packed; /* Just in case. */

/* A common structure to contain information about the test run. */
struct test_info {
	size_t test_blob_size;
	struct ciph_stats enc_stats;
	struct ciph_stats dec_stats;
	char *p; /* Pointer to an allcoated buffer of test_blob_size bytes. */
};

static void init_stats(struct ciph_stats *stats)
{
	stats->min_time = ~0;
	stats->max_time = 0;
	stats->total_time = 0;
}

static void update_stats(struct ciph_stats *stats, uint32_t time)
{
	if (time < stats->min_time)
		stats->min_time = time;

	if (time > stats->max_time)
		stats->max_time = time;

	stats->total_time += time;
}

static void report_stats(const char *direction, struct ciph_stats *stats)
{
	ccprintf("%s results: min %d us, max %d us, average %d us\n",
		 direction, stats->min_time, stats->max_time,
		 stats->total_time / number_of_iterations);
}

/*
 * Prepare to run the test: allocate memory, initialize stats structures.
 *
 * Returns EC_SUCCESS if everything is fine, EC_ERROR_OVERFLOW on malloc
 * failures.
 */
static int prepare_running(struct test_info *pinfo)
{
	memset(pinfo, 0, sizeof(*pinfo));


	pinfo->test_blob_size = shared_mem_size();
	/*
	 * Leave some room for crypto functions if they need to allocate
	 * something, just in case. 0x20 extra bytes are needed to be able to
	 * modify size alignment of the allocated buffer.
	 */
	if (pinfo->test_blob_size < (HEAP_HEAD_ROOM + 0x20)) {
		ccprintf("Not enough memory to run the test\n");
		return EC_ERROR_OVERFLOW;
	}
	pinfo->test_blob_size = (pinfo->test_blob_size - HEAP_HEAD_ROOM);

	if (shared_mem_acquire(pinfo->test_blob_size,
			       (char **)&(pinfo->p)) != EC_SUCCESS) {
		ccprintf("Failed to allocate %d bytes\n",
			 pinfo->test_blob_size);
		return EC_ERROR_OVERFLOW;
	}

	/*
	 * Use odd block size to make sure unaligned length blocks are handled
	 * properly. This leaves room in the end of the buffer to check if the
	 * decryption routine scratches it.
	 */
	pinfo->test_blob_size &= ~0x1f;
	pinfo->test_blob_size |= 7;

	ccprintf("running %d iterations\n", number_of_iterations);
	ccprintf("blob size %d at %pP\n", pinfo->test_blob_size, pinfo->p);

	init_stats(&(pinfo->enc_stats));
	init_stats(&(pinfo->dec_stats));

	return EC_SUCCESS;
}

/*
 * Let's split the buffer in two equal halves, encrypt the lower half into the
 * upper half and compare them word by word. There should be no repetitions.
 *
 * The side effect of this is starting the test with random clear text data.
 *
 * The first 16 bytes of the allocated buffer are used as the encryption IV.
 */
static int basic_check(struct test_info *pinfo)
{
	size_t half;
	size_t i;
	uint32_t *p;

	ccprintf("original data  %ph\n", HEX_BUF(pinfo->p, 16));

	half = (pinfo->test_blob_size/2) & ~3;
	if (!DCRYPTO_app_cipher(NVMEM, pinfo->p, pinfo->p,
				pinfo->p + half, half)) {
		ccprintf("first encryption run failed\n");
		return EC_ERROR_UNKNOWN;
	}

	p = (uint32_t *)pinfo->p;
	half /= sizeof(*p);

	for (i = 0; i < half; i++)
		if (p[i] == p[i + half]) {
			ccprintf("repeating 32 bit word detected"
				 " at offset 0x%x!\n", i * 4);
			return EC_ERROR_UNKNOWN;
		}

	ccprintf("hashed data    %ph\n", HEX_BUF(pinfo->p, 16));

	return EC_SUCCESS;
}

/*
 * Main iteration of the console command, runs ecnryption/decryption cycles,
 * vefifying that decrypted text's hash matches the original, and accumulating
 * timing statistics.
 */
static int command_loop(struct test_info *pinfo)
{
	uint8_t sha[SHA_DIGEST_SIZE];
	uint8_t sha_after[SHA_DIGEST_SIZE];
	uint32_t iteration;
	uint8_t *p_last_byte;
	int rv;

	/*
	 * Prepare the hash of the original data to be able to verify
	 * results.
	 */
	DCRYPTO_SHA1_hash(pinfo->p, pinfo->test_blob_size, sha);

	/* Use the hash as an IV for the cipher. */
	memcpy(sha_after, sha, sizeof(sha_after));

	iteration = number_of_iterations;
	p_last_byte = pinfo->p + pinfo->test_blob_size;

	while (iteration--) {
		char last_byte = (char) iteration;
		uint32_t tstamp;

		*p_last_byte = last_byte;

		if (!(iteration % 512))
			watchdog_reload();

		tstamp = get_time().val;
		rv = DCRYPTO_app_cipher(NVMEM, sha_after, pinfo->p,
					pinfo->p, pinfo->test_blob_size);
		tstamp = get_time().val - tstamp;

		if (!rv) {
			ccprintf("encryption failed\n");
			return EC_ERROR_UNKNOWN;
		}
		if (*p_last_byte != last_byte) {
			ccprintf("encryption overflowed\n");
			return EC_ERROR_UNKNOWN;
		}
		update_stats(&pinfo->enc_stats, tstamp);

		tstamp = get_time().val;
		rv = DCRYPTO_app_cipher(NVMEM, sha_after, pinfo->p,
					pinfo->p, pinfo->test_blob_size);
		tstamp = get_time().val - tstamp;

		if (!rv) {
			ccprintf("decryption failed\n");
			return EC_ERROR_UNKNOWN;
		}
		if (*p_last_byte != last_byte) {
			ccprintf("decryption overflowed\n");
			return EC_ERROR_UNKNOWN;
		}

		DCRYPTO_SHA1_hash(pinfo->p, pinfo->test_blob_size, sha_after);
		if (memcmp(sha, sha_after, sizeof(sha))) {
			ccprintf("\n"
				 "sha1 before and after mismatch, %d to go!\n",
				 iteration);
			return EC_ERROR_UNKNOWN;
		}

		update_stats(&pinfo->dec_stats, tstamp);

		/* get a new IV */
		DCRYPTO_SHA1_hash(sha_after, sizeof(sha), sha_after);
	}

	return EC_SUCCESS;
}

/*
 * Run cipher command on the hooks task context, as dcrypto's stack
 * requirements exceed console tasks' allowance.
 */
static void run_cipher_cmd(void)
{
	struct test_info info;

	result = prepare_running(&info);

	if (result == EC_SUCCESS)
		result = basic_check(&info);

	if (result == EC_SUCCESS)
		result = command_loop(&info);

	if (result == EC_SUCCESS) {
		report_stats("Encryption", &info.enc_stats);
		report_stats("Decryption", &info.dec_stats);
	} else if (info.p) {
		ccprintf("current data   %ph\n", HEX_BUF(info.p, 16));
	}

	if (info.p)
		shared_mem_release(info.p);

	task_set_event(TASK_ID_CONSOLE, TASK_EVENT_CUSTOM_BIT(0), 0);
}
DECLARE_DEFERRED(run_cipher_cmd);

static int cmd_cipher(int argc, char **argv)
{
	uint32_t events;
	uint32_t max_time;

	/* Ignore potential input errors, let the user handle them. */
	if (argc > 1)
		number_of_iterations = strtoi(argv[1], NULL, 0);
	else
		number_of_iterations = 1000;

	if (!number_of_iterations) {
		ccprintf("not running zero iterations\n");
		return EC_ERROR_PARAM1;
	}

	hook_call_deferred(&run_cipher_cmd_data, 0);

	/* Roughly, .5 us per byte should be more than enough. */
	max_time = number_of_iterations * shared_mem_size() / 2;

	ccprintf("Will wait up to %d ms\n", (max_time + 500)/1000);

	events = task_wait_event_mask(TASK_EVENT_CUSTOM_BIT(0), max_time);
	if (!(events & TASK_EVENT_CUSTOM_BIT(0))) {
		ccprintf("Timed out, you might want to reboot...\n");
		return EC_ERROR_TIMEOUT;
	}

	return result;
}
DECLARE_SAFE_CONSOLE_COMMAND(cipher, cmd_cipher, NULL, NULL);
#endif
