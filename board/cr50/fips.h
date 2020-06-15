/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef __EC_BOARD_CR50_FIPS_H__
#define __EC_BOARD_CR50_FIPS_H__

#include "common.h"

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
	FIPS_FATAL_RSA2048 = 1 << 8,
	FIPS_FATAL_AES256 = 1 << 9,
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
	FIPS_BREAK_ECDSA = 5,
	FIPS_BREAK_AES256 = 6
};
extern uint8_t fips_break_cmd;

/* Command codes for VENDOR_CC_FIPS_CMD. */
enum fips_cmd {
	FIPS_CMD_GET_STATUS = 0,
	FIPS_CMD_ON = 1,
	FIPS_CMD_TEST = 2,
	FIPS_CMD_BREAK_TRNG = 3,
	FIPS_CMD_BREAK_SHA256 = 4,
	FIPS_CMD_BREAK_HMAC_SHA256 = 5,
	FIPS_CMD_BREAK_HMAC_DRBG = 6,
	FIPS_CMD_BREAK_ECDSA = 7,
	FIPS_CMD_BREAK_AES256 = 8,
	FIPS_CMD_NO_BREAK = 9
};

/* Return current FIPS status of operations. */
enum fips_status fips_status(void);

/* return true if in FIPS-approved mode. */
bool fips_mode(void);

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
 * Switch FIPS status, zeroize keys if needed. For Production it's a one way
 * to 'FIPS on'. For development board it allows creation of non-FIPS keys.
 */
void fips_set_policy(bool active);

#ifdef __cplusplus
}
#endif
#endif /* __EC_BOARD_CR50_FIPS_H__ */
