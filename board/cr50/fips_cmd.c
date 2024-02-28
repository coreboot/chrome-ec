/* Copyright 2021 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "builtin/endian.h"
#include "console.h"
#include "dcrypto.h"
#include "ec_commands.h"
#include "extension.h"
#include "fips.h"
#include "fips_rand.h"
#include "flash_log.h"
#include "hooks.h"
#include "new_nvmem.h"
#include "nvmem.h"
#include "nvmem_vars.h"
#include "registers.h"
#include "scratch_reg1.h"
#include "shared_mem.h"
#include "system.h"
#include "task.h"
#include "tpm_nvmem.h"
#include "tpm_nvmem_ops.h"
#include "u2f_impl.h"

/**
 * Create IRQ handler calling FIPS module's dcrypto_done_interrupt() on
 * interrupt. Generated code calls some of the EC OS task management
 * functions which are not security/crypto related, so to avoid rewriting
 * macro using FIPS vtable, move it outside FIPS module.
 */
DECLARE_IRQ(GC_IRQNUM_CRYPTO0_HOST_CMD_DONE_INT, dcrypto_done_interrupt, 1);

#define CPRINTS(format, args...) cprints(CC_SYSTEM, format, ##args)

/* Print on console current FIPS mode. */
static void fips_print_mode(void)
{
	enum fips_status status = fips_status();

	CPRINTS("FIPS module digest %ph...",
		HEX_BUF(&fips_integrity, 8));
	if (status == FIPS_UNINITIALIZED)
		CPRINTS("FIPS mode not initialized");
	else if (status & FIPS_ERROR_MASK)
		CPRINTS("FIPS error code 0x%08x, not-approved", status);
	else
		CPRINTS("Running in FIPS 140-2 %s mode",
			((status & FIPS_MODE_ACTIVE) &&
			 (status & FIPS_POWER_UP_TEST_DONE)) ?
				"approved" :
				      "not-approved");
}

/* Print time it took tests to run or print error message. */
static void fips_print_test_time(void)
{
	if (fips_last_kat_test_duration != -1ULL)
		CPRINTS("FIPS power-up tests completed in %llu",
			fips_last_kat_test_duration);
}

static void fips_print_status(void)
{
	/* Once FIPS power-up tests completed we can enable console output. */
	console_enable_output();

	fips_print_test_time();
	fips_print_mode();
}
DECLARE_HOOK(HOOK_INIT, fips_print_status, HOOK_PRIO_INIT_PRINT_FIPS_STATUS);

#if defined(CRYPTO_TEST_SETUP)
static const uint8_t k_salt = NVMEM_VAR_G2F_SALT;

static void print_u2f_keys_status(void)
{
	struct u2f_state state;
	bool load_result;
	size_t hmac_len, drbg_len;

	hmac_len = read_tpm_nvmem_size(TPM_HIDDEN_U2F_KEK);
	drbg_len = read_tpm_nvmem_size(TPM_HIDDEN_U2F_KH_SALT);
	load_result = u2f_load_or_create_state(&state, false, false);

	CPRINTS("U2F HMAC len: %u, U2F Entropy len: %u, U2F load:%u, "
		"State DRBG len:%u", hmac_len,
		drbg_len, load_result, state.drbg_entropy_size);
}

static void u2f_keys(void)
{
	CPRINTS("U2F state %x", (uintptr_t)u2f_get_state());
	print_u2f_keys_status();
}

/* Set U2F keys as old. */
static int fips_set_old_u2f_keys(void)
{
	uint8_t random[32];

	u2f_zeroize_keys();

	/* Create fake u2f keys old style */
	if (!fips_trng_bytes(random, sizeof(random)))
		return EC_ERROR_HW_INTERNAL;
	setvar(&k_salt, sizeof(k_salt), random, sizeof(random));

	if (!fips_trng_bytes(random, sizeof(random)))
		return EC_ERROR_HW_INTERNAL;
	write_tpm_nvmem_hidden(TPM_HIDDEN_U2F_KEK, sizeof(random), random, 1);
	if (!fips_trng_bytes(random, sizeof(random)))
		return EC_ERROR_HW_INTERNAL;
	write_tpm_nvmem_hidden(TPM_HIDDEN_U2F_KH_SALT, sizeof(random), random,
			       1);
	return 0;
}
#endif

/* Console command 'fips' to report and change status, run tests */
static int cmd_fips_status(int argc, char **argv)
{
	fips_print_mode();
	fips_print_test_time();
	CPRINTS("FIPS crypto allowed: %u", fips_crypto_allowed());

	cflush();

	if (argc == 2) {
		if (!strncmp(argv[1], "test", 4)) {
			fips_power_up_tests();
			fips_print_test_time();
			fips_print_mode();
		}
#ifdef CRYPTO_TEST_SETUP
		else if (!strncmp(argv[1], "new", 3))
			CPRINTS("u2f update status: %d", u2f_update_keys());
		else if (!strncmp(argv[1], "del", 3))
			CPRINTS("u2f zeroization status: %d",
				u2f_zeroize_keys());
		else if (!strncmp(argv[1], "old", 3))
			return fips_set_old_u2f_keys();
		else if (!strncmp(argv[1], "kek", 3))
			return u2f_gen_kek_seed();
		else if (!strncmp(argv[1], "u2f", 3))
			print_u2f_keys_status();
		else if (!strncmp(argv[1], "gen", 3))
			u2f_keys();
		else if (!strncmp(argv[1], "trng", 4))
			fips_break_cmd = FIPS_BREAK_TRNG;
		else if (!strncmp(argv[1], "sha", 3))
			fips_break_cmd = FIPS_BREAK_SHA256;
		else if (!strncmp(argv[1], "hmac", 4))
			fips_break_cmd = FIPS_BREAK_HMAC_SHA256;
		else if (!strncmp(argv[1], "drbg", 4))
			fips_break_cmd = FIPS_BREAK_HMAC_DRBG;
		else if (!strncmp(argv[1], "ecver", 5))
			fips_break_cmd = FIPS_BREAK_ECDSA_VER;
		else if (!strncmp(argv[1], "ecsign", 6))
			fips_break_cmd = FIPS_BREAK_ECDSA_SIGN;
		else if (!strncmp(argv[1], "pwct", 4))
			fips_break_cmd = FIPS_BREAK_ECDSA_PWCT;
		else if (!strncmp(argv[1], "none", 4))
			fips_break_cmd = FIPS_NO_BREAK;
#endif
	}
	return 0;
}

DECLARE_SAFE_CONSOLE_COMMAND(
	fips, cmd_fips_status,
#ifdef CRYPTO_TEST_SETUP
	"[test | new | old | u2f | gen | trng | sha]",
	"Report FIPS status, switch U2F key, run tests, simulate errors");
#else
	"[test]", "Report FIPS status, run tests");
#endif

/**
 * Vendor command implementation to report & change status, run tests.
 * Command structure:
 *
 * field     |    size  |                  note
 * =========================================================================
 * op        |    1     | 0 - get status, 1 - set FIPS ON (remove old U2F)
 *           |          | 2 - run tests, 3 .. 8 - simulate errors
 */
static enum vendor_cmd_rc fips_cmd(enum vendor_cmd_cc code, void *buf,
				   size_t input_size, size_t *response_size)
{
	uint8_t *cmd = buf;
	uint32_t fips_reverse;

	*response_size = 0;
	if (input_size != 1)
		return VENDOR_RC_BOGUS_ARGS;

	switch ((enum fips_cmd) *cmd) {
	case FIPS_CMD_GET_STATUS:
		fips_reverse = htobe32(fips_status());
		memcpy(buf, &fips_reverse, sizeof(fips_reverse));
		*response_size = sizeof(fips_reverse);
		break;
	case FIPS_CMD_TEST:
		fips_power_up_tests();
		fips_reverse = htobe32(fips_status());
		memcpy(buf, &fips_reverse, sizeof(fips_reverse));
		*response_size = sizeof(fips_reverse);
		break;
	case FIPS_CMD_ON:
		if (u2f_update_keys() != EC_SUCCESS)
			return VENDOR_RC_INTERNAL_ERROR;
		break;
	case FIPS_CMD_U2F_STATUS:
		*cmd = u2f_keys_are_fips();
		*response_size = sizeof(*cmd);
		break;

#ifdef CRYPTO_TEST_SETUP
	case FIPS_CMD_BREAK_TRNG:
		fips_break_cmd = FIPS_BREAK_TRNG;
		break;
	case FIPS_CMD_BREAK_SHA256:
		fips_break_cmd = FIPS_BREAK_SHA256;
		break;
	case FIPS_CMD_BREAK_HMAC_SHA256:
		fips_break_cmd = FIPS_BREAK_HMAC_SHA256;
		break;
	case FIPS_CMD_BREAK_HMAC_DRBG:
		fips_break_cmd = FIPS_BREAK_HMAC_DRBG;
		break;
	case FIPS_CMD_BREAK_ECDSA_VER:
		fips_break_cmd = FIPS_BREAK_ECDSA_VER;
		break;
	case FIPS_CMD_BREAK_ECDSA_SIGN:
		fips_break_cmd = FIPS_BREAK_ECDSA_SIGN;
		break;
#ifdef CONFIG_FIPS_AES_CBC_256
	case FIPS_CMD_BREAK_AES256:
		fips_break_cmd = FIPS_BREAK_AES256;
		break;
#endif /* CONFIG_FIPS_AES_CBC_256 */
#ifdef CONFIG_FIPS_RSA2048
	case FIPS_CMD_BREAK_RSA2048:
		fips_break_cmd = FIPS_BREAK_RSA2048;
		break;
#endif /* CONFIG_FIPS_RSA2048 */
	case FIPS_CMD_NO_BREAK:
		fips_break_cmd = FIPS_NO_BREAK;
		break;
#endif /* CRYPTO_TEST_SETUP */
	default:
		return VENDOR_RC_BOGUS_ARGS;
	}

	return VENDOR_RC_SUCCESS;
}

DECLARE_VENDOR_COMMAND(VENDOR_CC_FIPS_CMD, fips_cmd);
