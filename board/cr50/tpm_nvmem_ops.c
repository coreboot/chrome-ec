/*
 * Copyright 2017 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stddef.h>

#include "common.h"
#include "console.h"
#include "tpm_nvmem_ops.h"

/* These come from the tpm2 tree. */
#include "Global.h"
#include "Implementation.h"
#include "NV_fp.h"
#include "PCR_fp.h"
#include "tpm_types.h"

#define CPRINTF(format, args...) cprintf(CC_TASK, format, ## args)

enum tpm_read_rv read_tpm_nvmem(uint16_t obj_index,
				uint16_t obj_size, void *obj_value)
{
	TPM_HANDLE       object_handle;
	NV_INDEX         nvIndex;
	uint32_t         handle_addr;

	object_handle = HR_NV_INDEX + obj_index;

	handle_addr = NvEarlyStageFindHandle(object_handle);
	if (!handle_addr) {
		CPRINTF("%s: object at 0x%x not found\n", __func__, obj_index);
		return TPM_READ_NOT_FOUND;
	}

	/* Get properties of this index as stored in nvmem. */
	NvReadIndexInfo(object_handle, handle_addr, &nvIndex);

	/*
	 * Check that the index was written to. Otherwise, behave as if the
	 * index doesn't exist.
	 */
	if (nvIndex.publicArea.attributes.TPMA_NV_WRITTEN == 0) {
		CPRINTF("%s: object at 0x%x not written\n",
				__func__, obj_index);
		return TPM_READ_NOT_FOUND;
	}

	/*
	 * We presume it is readable and are not checking the access
	 * limitations.
	 */

	/*
	 * Does the caller ask for too much? Note that we always read from the
	 * beginning of the space, unlike the actual TPM2_NV_Read command
	 * which can start at an offset.
	 */
	if (obj_size > nvIndex.publicArea.dataSize) {
		CPRINTF("%s: object at 0x%x is smaller than %d\n",
			__func__, obj_index, obj_size);
		return TPM_READ_TOO_SMALL;
	}

	/* Perform the read. */
	NvReadIndexData(object_handle, &nvIndex, handle_addr, 0, obj_size,
			   obj_value);

	return TPM_READ_SUCCESS;
}

enum tpm_read_rv read_tpm_nvmem_hidden(uint16_t object_index,
				       uint16_t object_size,
				       void *obj_value)
{
	if (NvGetHiddenObject(HR_HIDDEN | object_index,
			      object_size,
			      obj_value) == TPM_RC_SUCCESS) {
		return TPM_READ_SUCCESS;
	} else {
		return TPM_READ_NOT_FOUND;
	}
}

enum tpm_write_rv write_tpm_nvmem_hidden(uint16_t object_index,
					 uint16_t object_size,
					 void *obj_value,
					 int commit)
{
	enum tpm_write_rv ret = TPM_WRITE_FAIL;

	uint32_t handle = object_index | HR_HIDDEN;

	if (!NvIsDefinedHiddenObject(handle) &&
	    NvAddHiddenObject(handle,
			      object_size,
			      obj_value) == TPM_RC_SUCCESS) {
		ret = TPM_WRITE_CREATED;
	} else if (NvWriteHiddenObject(handle,
				       object_size,
				       obj_value) == TPM_RC_SUCCESS) {
		ret = TPM_WRITE_UPDATED;
	}

	if (commit && !NvCommit())
		ret = TPM_WRITE_FAIL;

	return ret;
}

size_t read_tpm_nvmem_size(uint16_t obj_index)
{
	UINT16 size;

	if (NvGetHiddenObjectSize(HR_HIDDEN | obj_index, &size) !=
	    TPM_RC_SUCCESS)
		return 0;

	return size;
}

BUILD_ASSERT(TPM_ORDERLY_STATE_SIZE >= RAM_INDEX_SPACE);

void tpm_orderly_state_capture(char copy[TPM_ORDERLY_STATE_SIZE])
{
	NvStateCapture(copy);
}

void tpm_orderly_state_restore(const char copy[TPM_ORDERLY_STATE_SIZE])
{
	NvStateRestore(copy);
}

bool get_tpm_pcr_value(uint32_t pcr_num, uint8_t value[SHA256_DIGEST_SIZE])
{
	return PCRGetValue(TPM_ALG_SHA256, pcr_num, SHA256_DIGEST_SIZE, value);
}

uint16_t tpm_nv_tpm2b_len(uint32_t index)
{
	NV_RESERVED_ITEM ri;
	/*
	 * Make sure we read length of TPM2B struct properly. Note, it is
	 * stored in machine format, so no ending conversion is needed.
	 */
	uint16_t tpm2b_len = 0; /* Shall match TPM2B size type. */

	BUILD_ASSERT(sizeof(gp.EPSeed.t.size) == sizeof(tpm2b_len));
	/* This check is only valid for a range of TPM2B spaces. */
	if (index < NV_OWNER_POLICY || index > NV_EH_PROOF)
		return 0;

	NvGetReserved(index, &ri);
	if (ri.size >= sizeof(tpm2b_len)) {
		_plat__NvMemoryRead(ri.offset, sizeof(tpm2b_len), &tpm2b_len);
		if (tpm2b_len > ri.size - sizeof(tpm2b_len))
			tpm2b_len = 0;
	}
	return tpm2b_len;
}
