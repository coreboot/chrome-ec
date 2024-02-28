/*
 * Copyright 2017 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __EC_BOARD_CR50_TPM_NVMEM_OPS_H
#define __EC_BOARD_CR50_TPM_NVMEM_OPS_H

#ifndef SHA256_DIGEST_SIZE
#define SHA256_DIGEST_SIZE 32
#endif /* SHA256_DIGEST_SIZE */

#define TPM_ORDERLY_STATE_SIZE 512

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

void tpm_orderly_state_capture(char copy[TPM_ORDERLY_STATE_SIZE]);

void tpm_orderly_state_restore(const char copy[TPM_ORDERLY_STATE_SIZE]);

/*
 * Read value of the specified PCR in SHA-256 bank into a buffer,
 * or return false if no such PCR
 */
bool get_tpm_pcr_value(uint32_t pcr_num, uint8_t value[SHA256_DIGEST_SIZE]);

/* Return length of TPM2B reserved space from NVmem. */
uint16_t tpm_nv_tpm2b_len(uint32_t index);

#endif  /* ! __EC_BOARD_CR50_TPM_NVMEM_OPS_H */
