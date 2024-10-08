/* Copyright 2018 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __EC_BOARD_CR50_TPM2_VIRTUAL_NVMEM_H
#define __EC_BOARD_CR50_TPM2_VIRTUAL_NVMEM_H

/*
 * Currently supported virtual NV indexes.
 *
 * The range for virtual NV indexes is chosen such that all indexes
 * fall within a range designated by the TCG for use by TPM manufacturers,
 * without expectation of consultation with the TCG, or consistent behavior
 * across TPM models. See Table 3 in the 'Registry of reserved TPM 2.0
 * handles and localities' for more details.
 *
 * To return data, entries in this enum must be registered in virtual_nvmem.c.
 *
 * Values in this enum must be consecutive.
 */
enum virtual_nv_index {
	VIRTUAL_NV_INDEX_START = 0x013fff00,
	VIRTUAL_NV_INDEX_BOARD_ID = VIRTUAL_NV_INDEX_START,
	VIRTUAL_NV_INDEX_SN_DATA,
	VIRTUAL_NV_INDEX_G2F_CERT,
	VIRTUAL_NV_INDEX_RSU_DEV_ID,
	/* Reserved for generic TPM2.0 stand-alone counter. */
	/* TODO(b/191163997): support reading RMA bytes via this index. */
	VIRTUAL_NV_INDEX_RMA_BYTES_UNIMPLEMENTED,
	VIRTUAL_NV_INDEX_WV_UDS_BYTES_UNIMPLEMENTED,
	VIRTUAL_NV_INDEX_FACTORY_CONFIG,
	VIRTUAL_NV_INDEX_WV_ROT_CERT_UNIMPLEMENTED,
	VIRTUAL_NV_INDEX_ID_CERT_UNIMPLEMENTED,
	VIRTUAL_NV_INDEX_DICE_CHAIN,
	VIRTUAL_NV_INDEX_BOOT_PARAM,
	VIRTUAL_NV_INDEX_END,
};
/* Reserved space for future virtual indexes; this is the last valid index. */
#define VIRTUAL_NV_INDEX_MAX 0x013fffff

/*
 * Data sizes (in bytes) of currently defined indexes.
 */
#define VIRTUAL_NV_INDEX_BOARD_ID_SIZE	12
#define VIRTUAL_NV_INDEX_SN_DATA_SIZE	16
#define VIRTUAL_NV_INDEX_G2F_CERT_SIZE	315
#define VIRTUAL_NV_INDEX_RSU_DEV_ID_SIZE 32
#define VIRTUAL_NV_INDEX_FACTORY_CONFIG_SIZE INFO_FACTORY_CFG_SIZE

#endif /* __EC_BOARD_CR50_TPM2_VIRTUAL_NVMEM_H */
