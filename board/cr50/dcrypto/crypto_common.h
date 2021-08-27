/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef __EC_FIPS_MODULE_COMMON_H
#define __EC_FIPS_MODULE_COMMON_H

/**
 * This header file contains types shared between public API in dcrypto.h and
 * internal functions in internal.h.
 */

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Result codes for crypto operations, targeting
 * high Hamming distance from each other.
 */
enum dcrypto_result {
	DCRYPTO_OK = 0xAA33AAFF, /* Success. */
	DCRYPTO_FAIL = 0x55665501, /* Failure. */
	DCRYPTO_RETRY = 0xA5775A33,
	DCRYPTO_RESEED_NEEDED = 0x36AA6355,
};

#ifdef __cplusplus
}
#endif

#endif /* __EC_FIPS_MODULE_COMMON_H */
