/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * TPM NVMEM definitions.
 */
#ifndef __CROS_EC_TPM_NVMEM_H
#define __CROS_EC_TPM_NVMEM_H
#include "common.h"

/**
 * A range of NV indices are reserved for use by the TPM manufacturer,
 * and can be allocated arbitrarily, without the need to consult the TCG,
 * or any expectation that these be consistent across different models
 * of TPM. See 'Registry of reserved TPM 2.0 handles and localities' for
 * more details.
 * The range allocated to TPM manufacturers is 0x01000000 - 0x013fffff
 * This file documents indices that have been allocated as part of the TPM
 * implementation in cr50.
 *
 * Index          Description
 *
 * Source: src/third_party/coreboot/src/security/vboot/antirollback.h
 * 0x01001007     FIRMWARE_NV_INDEX
 * 0x01001008     NV_INDEX_KERNEL
 * 0x01001009     BACKUP_NV_INDEX
 * 0x0100100a     NV_INDEX_FWMP
 * 0x0100100b     MRC_REC_HASH_NV_INDEX
 *
 *                Virtual NV indices
 * Source:  src/platform/ec/board/cr50/tpm2/virtual_nvmem.c
 * 0x013fff00     BOARD_ID
 * 0x013fff01     SN_BITS
 * 0x013fff02     G2F_CERT
 * 0x013fff03     RSU_DEV_ID
 * 0x013fff04     RMA_BYTES (UNIMPLEMENTED)
 * 0x013fff05     WV_UDS_BYTES (UNIMPLEMENTED)
 * 0x013fff06     FACTORY_CONFIG,
 * - to -         Reserved
 * 0x013fffff
 */

/*
 * These NV space definitions were manually copied from
 * src/third_party/coreboot/src/security/vboot/antirollback.h
 * at git hash a03ebe7fc5.
 */
enum nv_index {
	NV_INDEX_FIRMWARE = 0x1007, /* FIRMWARE_NV_INDEX */
	NV_INDEX_KERNEL = 0x1008, /* KERNEL_NV_INDEX */
	NV_INDEX_FWMP = 0x100a, /* FWMP_NV_INDEX */
	/* 0x100b: Hash of MRC_CACHE training data for recovery boot */
	NV_INDEX_MRC_REC_HASH = 0x100b, /* MRC_REC_HASH_NV_INDEX */
	/* 0x100c: OOBE autoconfig public key hashes */
	/* 0x100d: Hash of MRC_CACHE training data for non-recovery boot */
	NV_INDEX_MRC_RW_HASH = 0x100d, /* MRC_RW_HASH_NV_INDEX */
	NV_INDEX_ENT_ROLLBACK_SPACE = 0x100e, /* ENT_ROLLBACK_SPACE_INDEX */
	NV_INDEX_VBIOS_CACHE = 0x100f, /* VBIOS_CACHE_NV_INDEX */
	/* Widevine Secure Counter space */
	NV_INDEX_WIDEVINE_COUNTER_0 = 0x3000, /*WIDEVINE_COUNTER_NV_INDEX(0)*/
	NV_INDEX_WIDEVINE_COUNTER_1 = 0x3001, /*WIDEVINE_COUNTER_NV_INDEX(1)*/
	NV_INDEX_WIDEVINE_COUNTER_2 = 0x3002, /*WIDEVINE_COUNTER_NV_INDEX(2)*/
	NV_INDEX_WIDEVINE_COUNTER_3 = 0x3003, /*WIDEVINE_COUNTER_NV_INDEX(3)*/
};

enum tpm_nv_hidden_object {
	TPM_HIDDEN_U2F_KEK,
	TPM_HIDDEN_U2F_KH_SALT,
};


/*
 * These definitions and the structure layout were manually copied from
 * src/platform/vboot_reference/firmware/2lib/include/2secdata.h. at
 * git sha 38d7d1c.
 */
#define FWMP_HASH_SIZE		    32
#define FWMP_DEV_DISABLE_CCD_UNLOCK BIT(6)
#define FWMP_DEV_DISABLE_BOOT       BIT(0)
#define FIRMWARE_FLAG_DEV_MODE      0x02

struct RollbackSpaceFirmware {
	/* Struct version, for backwards compatibility */
	uint8_t struct_version;
	/* Flags (see FIRMWARE_FLAG_* above) */
	uint8_t flags;
	/* Firmware versions */
	uint32_t fw_versions;
	/* Reserved for future expansion */
	uint8_t reserved[3];
	/* Checksum (v2 and later only) */
	uint8_t crc8;
} __packed;

/* Firmware management parameters */
struct RollbackSpaceFwmp {
	/* CRC-8 of fields following struct_size */
	uint8_t crc;
	/* Structure size in bytes */
	uint8_t struct_size;
	/* Structure version */
	uint8_t struct_version;
	/* Reserved; ignored by current reader */
	uint8_t reserved0;
	/* Flags; see enum fwmp_flags */
	uint32_t flags;
	/* Hash of developer kernel key */
	uint8_t dev_key_hash[FWMP_HASH_SIZE];
} __packed;


#endif /* __CROS_EC_TPM_NVMEM_H */
