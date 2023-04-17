/* Copyright 2023 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */
#ifndef __CR50_INCLUDE_AP_RO_STATUS_H
#define __CR50_INCLUDE_AP_RO_STATUS_H

enum ap_ro_status {
	/* All AP RO Verification V1 statuses are less than 20 */
	AP_RO_NOT_RUN = 0,
	AP_RO_PASS_UNVERIFIED_GBB = 1,
	AP_RO_FAIL = 2,
	AP_RO_UNSUPPORTED_UNKNOWN = 3, /* Deprecated */
	AP_RO_UNSUPPORTED_NOT_TRIGGERED = 4,
	AP_RO_UNSUPPORTED_TRIGGERED = 5,
	AP_RO_PASS = 6,
	AP_RO_IN_PROGRESS = 7,
	/* All AP RO Verification V2 status are 20 or greater */
	AP_RO_V2_SUCCESS = 20,
	AP_RO_V2_FAILED_VERIFICATION = 21,
	AP_RO_V2_INCONSISTENT_GSCVD = 22,
	AP_RO_V2_INCONSISTENT_KEYBLOCK = 23,
	AP_RO_V2_INCONSISTENT_KEY = 24,
	AP_RO_V2_SPI_READ = 25,
	AP_RO_V2_UNSUPPORTED_CRYPTO_ALGORITHM = 26,
	AP_RO_V2_VERSION_MISMATCH = 27,
	AP_RO_V2_OUT_OF_MEMORY = 28,
	AP_RO_V2_INTERNAL = 29,
	AP_RO_V2_TOO_BIG = 30,
	AP_RO_V2_MISSING_GSCVD = 31,
	AP_RO_V2_BOARD_ID_MISMATCH = 32,
	AP_RO_V2_SETTING_NOT_PROVISIONED = 33,
	/*
	 * Do not use values 34 and 35. They are ambiguous depending on
	 * ti50 FW version.
	 */
	AP_RO_V2_NON_ZERO_GBB_FLAGS = 36,
	AP_RO_V2_WRONG_ROOT_KEY = 37,
	AP_RO_V2_UNKNOWN = 255,
};

#endif /* ! __CR50_INCLUDE_AP_RO_STATUS_H */
