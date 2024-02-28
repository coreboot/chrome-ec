/*
 * Copyright 2020 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __EC_BOARD_CR50_TPM_NVMEM_OPS_H
#define __EC_BOARD_CR50_TPM_NVMEM_OPS_H

enum tpm_read_rv {
	TPM_READ_SUCCESS,
	TPM_READ_NOT_FOUND,
	TPM_READ_TOO_SMALL,
};

enum tpm_write_rv {
	TPM_WRITE_CREATED,
	TPM_WRITE_UPDATED,
	TPM_WRITE_FAIL,
};

enum tpm_read_rv read_tpm_nvmem(uint16_t object_index,
				uint16_t object_size,
				void *obj_value);

#endif  /* ! __EC_BOARD_CR50_TPM_NVMEM_OPS_H */
