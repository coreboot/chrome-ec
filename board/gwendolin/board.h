/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Buccaneer board configuration */

#include "base_board.h"

#ifndef __CROS_EC_BOARD_GWENDOLIN_BOARD_H
#define __CROS_EC_BOARD_GWENDOLIN_BOARD_H

/*
 *-------------------------------------------------------------------------*
 * Fingerprint Specific
 *-------------------------------------------------------------------------*
 */

/**
 * See description in baseboard/helpilot/base_board.h.
 */
#undef HELIPILOT_CODE_RAM_SIZE_BYTES
#define HELIPILOT_CODE_RAM_SIZE_BYTES (256 * 1024)

/**
 * See description in baseboard/helpilot/base_board.h.
 */
#undef HELIPILOT_DATA_RAM_SIZE_BYTES
#define HELIPILOT_DATA_RAM_SIZE_BYTES (252 * 1024)

#ifdef SECTION_IS_RW
#define CONFIG_FP_SENSOR_EGIS630
#define CONFIG_FP_SENSOR_INT_ACTIVE_LOW
#endif /* SECTION_IS_RW */

#endif /* __CROS_EC_BOARD_GWENDOLIN_BOARD_H */
