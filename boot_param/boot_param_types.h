/*
 * Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __GSC_UTILS_BOOT_PARAM_BOOT_PARAM_TYPES_H
#define __GSC_UTILS_BOOT_PARAM_BOOT_PARAM_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DIGEST_BYTES	  32
#define ECDSA_POINT_BYTES 32
#define ECDSA_SIG_BYTES	  (2 * ECDSA_POINT_BYTES) /* 32 byte R + 32 byte S */

/* Sizes of GGSCBootParam fields */
#define EARLY_ENTROPY_BYTES     64
#define KEY_SEED_BYTES          32


/* UDS_ID and CDI_ID sizes */
#define DICE_ID_BYTES	  20
#define DICE_ID_HEX_BYTES (DICE_ID_BYTES * 2)

struct slice_mut_s {
	size_t size;
	uint8_t *data;
};
struct slice_ref_s {
	const size_t size;
	const uint8_t *data;
};

#define digest_as_slice(digest) \
	{ DIGEST_BYTES, digest }

#define digest_as_slice_mut(digest) \
	{ DIGEST_BYTES, digest }

struct ecdsa_public_s {
	uint8_t x[ECDSA_POINT_BYTES];
	uint8_t y[ECDSA_POINT_BYTES];
};

struct dice_config_s {
	/* APROV status */
	uint32_t aprov_status;
	/* GSCVD version (or 0 where not available) */
	uint32_t sec_ver;
	/* UDS */
	uint8_t uds[DIGEST_BYTES];
	/* derived from owner seed, changes on owner clear */
	uint8_t hidden_digest[DIGEST_BYTES];
	/* GSCVD digest (or 0..0 where not available) */
	uint8_t code_digest[DIGEST_BYTES];
	/* PCR0 value */
	uint8_t pcr0[DIGEST_BYTES];
	/* PCR10 value */
	uint8_t pcr10[DIGEST_BYTES];
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __GSC_UTILS_BOOT_PARAM_BOOT_PARAM_TYPES_H */
