/*
 * Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __GSC_UTILS_BOOT_PARAM_DICE_H
#define __GSC_UTILS_BOOT_PARAM_DICE_H

#include "dice_types.h"
#include "boot_param_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Size of DICE handover structure in bytes */
extern const size_t kDiceHandoverSize;

/* Get (part of) DICE handover structure: [offset .. offset + size) */
size_t get_dice_handover_bytes(
	/* [OUT] destination buffer to fill */
	uint8_t *dest,
	/* [IN] starting offset in the DICE handover struct */
	size_t offset,
	/* [IN] size of the DICE handover struct to copy */
	size_t size
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __GSC_UTILS_BOOT_PARAM_DICE_H */
