/* Copyright 2020 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef __EC_BOARD_CR50_FIPS_H__
#define __EC_BOARD_CR50_FIPS_H__

#include "common.h"
#include "timer.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Signals start in the top most bits, errors in the least significant bits. */
enum fips_status {
	/* FIPS status */
	FIPS_MODE_ACTIVE = 1U << 31,
	FIPS_POWER_UP_TEST_DONE = 1U << 30,

	FIPS_UNINITIALIZED = 0, /* Default value */

	/* FIPS errors */
	FIPS_FATAL_TRNG_RCT = 1 << 1,
	FIPS_FATAL_TRNG_APT = 1 << 2,
	FIPS_FATAL_TRNG_OTHER = 1 << 3,
	FIPS_FATAL_SHA256 = 1 << 4,
	FIPS_FATAL_HMAC_SHA256 = 1 << 5,
	FIPS_FATAL_HMAC_DRBG = 1 << 6,
	FIPS_FATAL_ECDSA = 1 << 7,
#ifdef CONFIG_FIPS_RSA2048
	FIPS_FATAL_RSA2048 = 1 << 8,
#endif
#ifdef CONFIG_FIPS_AES_CBC_256
	FIPS_FATAL_AES256 = 1 << 9,
#endif
	FIPS_FATAL_SELF_INTEGRITY = 1 << 10,
	FIPS_FATAL_BN_MATH = 1 << 11,
	FIPS_FATAL_ECDSA_PWCT = 1 << 12,
	FIPS_FATAL_OTHER = 1 << 15,

	FIPS_ERROR_MASK = 0xffff,

	FIPS_RFU_MASK = 0x7fff0000
};

/* Simulate error in specific block. */
enum fips_break {
	FIPS_NO_BREAK = 0,
	FIPS_BREAK_TRNG = 1,
	FIPS_BREAK_SHA256 = 2,
	FIPS_BREAK_HMAC_SHA256 = 3,
	FIPS_BREAK_HMAC_DRBG = 4,
	FIPS_BREAK_ECDSA_VER = 5,
	FIPS_BREAK_ECDSA_SIGN = 6,
	FIPS_BREAK_ECDSA_PWCT = 7,
#ifdef CONFIG_FIPS_AES_CBC_256
	FIPS_BREAK_AES256 = 8,
#endif
#ifdef CONFIG_FIPS_RSA2048
	FIPS_BREAK_RSA2048 = 9,
#endif
};

#ifdef CRYPTO_TEST_SETUP
extern uint8_t fips_break_cmd;
#endif

/* Duration of last FIPS KAT / Power-up tests. */
extern uint64_t fips_last_kat_test_duration;

/* Command codes for VENDOR_CC_FIPS_CMD. */
enum fips_cmd {
	FIPS_CMD_GET_STATUS = 0,
	FIPS_CMD_ON = 1,
	FIPS_CMD_TEST = 2,
	FIPS_CMD_BREAK_TRNG = 3,
	FIPS_CMD_BREAK_SHA256 = 4,
	FIPS_CMD_BREAK_HMAC_SHA256 = 5,
	FIPS_CMD_BREAK_HMAC_DRBG = 6,
	FIPS_CMD_BREAK_ECDSA_VER = 7,
	FIPS_CMD_BREAK_ECDSA_SIGN = 8,
	FIPS_CMD_BREAK_ECDSA_PWCT = 9,
#ifdef CONFIG_FIPS_AES_CBC_256
	FIPS_CMD_BREAK_AES256 = 10,
#endif
#ifdef CONFIG_FIPS_RSA2048
	FIPS_CMD_BREAK_RSA2048 = 11,
#endif
	FIPS_CMD_NO_BREAK = 12
};

/* These symbols defined in core/cortex-m/ec.lds.S. */
extern uint8_t __fips_module_start;
extern uint8_t __fips_module_end;

/* Return current FIPS status of operations. */
enum fips_status fips_status(void);

/**
 * Crypto is enabled when either FIPS mode is not enforced,
 * or if it is enforced and in good health
 * @returns non-zero if crypto can be executed.
 */
bool fips_crypto_allowed(void);

/**
 * Update FIPS status without updating log
 */
void fips_set_status(enum fips_status status);

/**
 * Update FIPS status with error code, write error in the log.
 */
void fips_throw_err(enum fips_status err);

/**
 * FIPS Power-up and known-answer tests.
 * Single point of initialization for all FIPS-compliant
 * cryptography. Responsible for KATs, TRNG testing, and signalling a
 * fatal error.
 *
 * Set FIPS status globally as a result.
 */
void fips_power_up_tests(void);

struct fips_vtable {
	enum ec_error_list (*shared_mem_acquire)(int size, char **dest_ptr);
	void (*shared_mem_release)(void *ptr);
#ifdef CONFIG_FLASH_LOG
	void (*flash_log_add_event)(uint8_t type, uint8_t size, void *payload);
#endif
	void (*cflush)(void);
	timestamp_t (*get_time)(void);

	void (*task_enable_irq)(int irq);
	uint32_t (*task_wait_event_mask)(uint32_t event_mask, int timeout_us);
	uint32_t (*task_set_event)(task_id_t tskid, uint32_t event, int wait);
	task_id_t (*task_get_current)(void);
	void (*task_start_irq_handler)(void *excep_return);
	void (*task_resched_if_needed)(void *excep_return);
	void (*mutex_lock)(struct mutex *mtx);
	void (*mutex_unlock)(struct mutex *mtx);
#ifdef CONFIG_WATCHDOG
	void (*watchdog_reload)(void);
#endif
};

/* Pointer to external callbacks used by FIPS module. */
extern const struct fips_vtable *fips_vtable;

/**
 * Set FIPS module vtable. Called during board_init() phase to provide
 * pointers to several system functions required by module to function.
 *
 * This should be called before any other FIPS functions are invoked as
 * vtable is used during FIPS power-up tests. Internally it checks that
 * provided vtable and referenced functions are in the same flash bank
 * as the FIPS module for additional security.
 */
void fips_set_callbacks(const struct fips_vtable *vtable);

/**
 * Run FIPS self-integrity, power-on and known-answer tests.
 * Called from board_init() during power-up and resume from sleep.
 * Enables crypto operation on successful completion.
 */
void fips_power_on(void);

#ifdef __cplusplus
}
#endif
#endif /* __EC_BOARD_CR50_FIPS_H__ */
