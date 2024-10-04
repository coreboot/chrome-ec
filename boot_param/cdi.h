/*
 * Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __GSC_UTILS_BOOT_PARAM_CDI_H
#define __GSC_UTILS_BOOT_PARAM_CDI_H

#include "dice_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct cdi_seal_inputs_s {
	uint8_t auth_data_digest[DIGEST_BYTES];
	uint8_t mode;
	uint8_t hidden_digest[DIGEST_BYTES];
};

struct cdi_attest_inputs_s {
	uint8_t code_digest[DIGEST_BYTES];
	uint8_t cfg_desr_digest[DIGEST_BYTES];
	struct cdi_seal_inputs_s seal_inputs;
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __GSC_UTILS_BOOT_PARAM_CDI_H */
