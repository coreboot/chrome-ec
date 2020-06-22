/*
 * Copyright 2017 The Chromium OS Authors. All rights reserved.
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

enum tpm_nv_hidden_object {
	TPM_HIDDEN_U2F_KEK,
	TPM_HIDDEN_U2F_KH_SALT,
};

enum tpm_read_rv read_tpm_nvmem(uint16_t object_index,
				uint16_t object_size,
				void *obj_value);

/*
 * The following functions must only be called from the TPM task,
 * and only after TPM initialization is complete (specifically,
 * after NvInitStatic).
 */

enum tpm_read_rv read_tpm_nvmem_hidden(uint16_t object_index,
				       uint16_t object_size,
				       void *obj_value);

enum tpm_write_rv write_tpm_nvmem_hidden(uint16_t object_index,
					 uint16_t object_size,
					 void *obj_value,
					 int commit);

/* return size of hidden nvmem object, 0 if not found */
size_t read_tpm_nvmem_size(uint16_t obj_index);

#endif  /* ! __EC_BOARD_CR50_TPM_NVMEM_OPS_H */
