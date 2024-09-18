/*
 * Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __GSC_UTILS_DICE_CBOR_DICE_H
#define __GSC_UTILS_DICE_CBOR_DICE_H

#include "cbor_basic.h"
#include "dice_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Boot mode decisions.
 * Boot mode == "Not configured" is not allowed
 */
#define BOOT_MODE_NORMAL   1
#define BOOT_MODE_DEBUG	   2
#define BOOT_MODE_RECOVERY 3

/* Configuration descriptor - see go/gsc-dice
 */
#define CFG_DESCR_LABEL_COMP_NAME     CBOR_NINT32(-70002)
#define CFG_DESCR_LABEL_RESETTABLE    CBOR_NINT32(-70004)
#define CFG_DESCR_LABEL_SEC_VER	      CBOR_NINT32(-70005)
#define CFG_DESCR_LABEL_APROV_STATUS  CBOR_NINT32(-71000)
#define CFG_DESCR_LABEL_VBOOT_STATUS  CBOR_NINT32(-71001)
#define CFG_DESCR_LABEL_AP_FW_VERSION CBOR_NINT32(-71002)

#define CFG_DESCR_COMP_NAME_VALUE_LEN 10 /* "CrOS AP FW" */
#define CFG_DESCR_COMP_NAME_LEN	      (1 + CFG_DESCR_COMP_NAME_VALUE_LEN)
#define CFG_DESCR_COMP_NAME                                                \
	{                                                                  \
		CBOR_HDR1(CBOR_MAJOR_TSTR, CFG_DESCR_COMP_NAME_VALUE_LEN), \
			'C', 'r', 'O', 'S', ' ', 'A', 'P', ' ', 'F', 'W'   \
	}

struct cfg_descr_s {
	/* Map header: 6 entries */
	uint8_t map_hdr;
	/* 1. Comp name: nint(-70002, 4bytes) => tstr("CrOS AP FW") */
	uint8_t comp_name_label[CBOR_NINT32_LEN];
	uint8_t comp_name[CFG_DESCR_COMP_NAME_LEN];
	/* 2. Resettable: nint(-70004, 4bytes) => null */
	uint8_t resettable_label[CBOR_NINT32_LEN];
	uint8_t resettable;
	/* 3. Sec ver: nint(-70005, 4bytes) => uint(Security ver, 4bytes) */
	uint8_t sec_ver_label[CBOR_NINT32_LEN];
	struct cbor_uint32_s sec_ver;
	/* 4. APROV status: nint(-71000, 4bytes) => uint(APROV_sts, 4bytes) */
	uint8_t aprov_status_label[CBOR_NINT32_LEN];
	struct cbor_uint32_s aprov_status;
	/* 5. Vboot status: nint(-71001, 4bytes) => bstr(PCR0, 32bytes) */
	uint8_t vboot_status_label[CBOR_NINT32_LEN];
	struct cbor_bstr32_s vboot_status;
	/* 6. AP FW version: nint(-71002, 4bytes) => bstr(PCR10, 32bytes) */
	uint8_t ap_fw_version_label[CBOR_NINT32_LEN];
	struct cbor_bstr32_s ap_fw_version;
};

#define CFG_DESCR_LEN sizeof(struct cfg_descr_s)
struct cfg_descr_bstr_s {
	uint8_t bstr_hdr[2]; /* bstr(sizeof(struct cfg_descr_s), 1byte) */
	struct cfg_descr_s data;
};
#define CFG_DESCR_BSTR_HDR CBOR_BSTR_HDR8(CFG_DESCR_LEN)

/* COSE keys - see go/gsc-dice
 */
#define COSE_KEY_LABEL_KTY     CBOR_UINT0(1)
#define COSE_KEY_LABEL_ALG     CBOR_UINT0(3)
#define COSE_KEY_LABEL_KEY_OPS CBOR_UINT0(4)
#define COSE_KEY_LABEL_CRV     CBOR_NINT0(-1)
#define COSE_KEY_LABEL_X       CBOR_NINT0(-2)
#define COSE_KEY_LABEL_Y       CBOR_NINT0(-3)

/* Configuration descriptor per go/gsc-dice
 */
struct cose_key_ecdsa_s {
	/* Map header: 6 entries */
	uint8_t map_hdr;
	/* 1. Key type: uint(1, 0bytes) => uint(2, 0bytes) */
	uint8_t kty_label;
	uint8_t kty;
	/* 2. Algorithm: uint(3, 0bytes) => nint(-1, 0bytes) */
	uint8_t alg_label;
	uint8_t alg;
	/* 3. Key ops: uint(4, 0bytes) => array(1) { uint(2, 0bytes) } */
	uint8_t key_ops_label;
	uint8_t key_ops_array_hdr;
	uint8_t key_ops;
	/* 4. Curve: nint(-1, 0bytes) => uint(1, 0bytes) */
	uint8_t crv_label;
	uint8_t crv;
	/* 5. X: nint(-2, 0bytes) => bstr(X, 32bytes) */
	uint8_t x_label;
	struct cbor_bstr32_s x;
	/* 6. X: nint(-3, 0bytes) => bstr(Y, 32bytes) */
	uint8_t y_label;
	struct cbor_bstr32_s y;
};

#define COSE_KEY_ECDSA_LEN sizeof(struct cose_key_ecdsa_s)
struct cose_key_ecdsa_bstr_s {
	uint8_t bstr_hdr[2]; /* bstr(sizeof(struct cose_key_ecdsa_s), 1byte) */
	struct cose_key_ecdsa_s data;
};
#define COSE_KEY_ECDSA_BSTR_HDR CBOR_BSTR_HDR8(COSE_KEY_ECDSA_LEN)

/* CWT claims - see go/gsc-dice
 * Size of TSTR containing an {UDS,CDI}_ID: (24 =< DICE_ID_HEX_LEN < 255)
 * => 1 byte size encoding
 */
#define DICE_ID_TSTR_LEN (2 + DICE_ID_HEX_BYTES)

#define CWT_LABEL_ISS	       CBOR_UINT0(1)
#define CWT_LABEL_SUB	       CBOR_UINT0(2)
#define CWT_LABEL_CODE_HASH    CBOR_NINT32(-4670545)
#define CWT_LABEL_CFG_HASH     CBOR_NINT32(-4670547)
#define CWT_LABEL_CFG_DESCR    CBOR_NINT32(-4670548)
#define CWT_LABEL_AUTH_HASH    CBOR_NINT32(-4670549)
#define CWT_LABEL_MODE	       CBOR_NINT32(-4670551)
#define CWT_LABEL_SUBJECT_PK   CBOR_NINT32(-4670552)
#define CWT_LABEL_KEY_USAGE    CBOR_NINT32(-4670553)
#define CWT_LABEL_PROFILE_NAME CBOR_NINT32(-4670554)

#define CWT_PROFILE_NAME_VALUE_LEN 10 /* "android.16" */
#define CWT_PROFILE_NAME_LEN	   (1 + CWT_PROFILE_NAME_VALUE_LEN)
#define CWT_PROFILE_NAME                                                 \
	{                                                                \
		CBOR_HDR1(CBOR_MAJOR_TSTR, CWT_PROFILE_NAME_VALUE_LEN),  \
			'a', 'n', 'd', 'r', 'o', 'i', 'd', '.', '1', '6' \
	}

struct cwt_claims_s {
	/* Map header: 10 entries */
	uint8_t map_hdr;
	/* 1. ISS: uint(1, 0bytes) => tstr(hex(UDS_ID)) */
	uint8_t iss_label;
	struct cbor_tstr40_s iss;
	/* 2. SUB: uint(2, 0bytes) => tstr(hex(CDI_ID)) */
	uint8_t sub_label;
	struct cbor_tstr40_s sub;
	/* 3. Code Hash: nint(-4670545, 4bytes) => bstr(32bytes) */
	uint8_t code_hash_label[CBOR_NINT32_LEN];
	struct cbor_bstr32_s code_hash;
	/* 4. Cfg Hash: nint(-4670547, 4bytes) => bstr(32bytes) */
	uint8_t cfg_hash_label[CBOR_NINT32_LEN];
	struct cbor_bstr32_s cfg_hash;
	/* 5. Cfg Descr: nint(-4670548, 4bytes) => bstr(struct cfg_descr_s) */
	uint8_t cfg_descr_label[CBOR_NINT32_LEN];
	struct cfg_descr_bstr_s cfg_descr;
	/* 6. Auth Hash: nint(-4670549, 4bytes) => bstr(32bytes) */
	uint8_t auth_hash_label[CBOR_NINT32_LEN];
	struct cbor_bstr32_s auth_hash;
	/* 7. Mode: nint(-4670551, 4bytes) => bstr(1byte) */
	uint8_t mode_label[CBOR_NINT32_LEN];
	struct cbor_bstr1_s mode;
	/* 8. Subject PK: nint(-4670552, 4bytes) => bstr(COSE_Key) */
	uint8_t subject_pk_label[CBOR_NINT32_LEN];
	struct cose_key_ecdsa_bstr_s subject_pk;
	/* 9. Key Usage: nint(-4670553, 4bytes) => bstr(1byte) */
	uint8_t key_usage_label[CBOR_NINT32_LEN];
	struct cbor_bstr1_s key_usage;
	/* 10. Profile name: nint(-4670554, 4bytes) => tstr("android.16") */
	uint8_t profile_name_label[CBOR_NINT32_LEN];
	uint8_t profile_name[CWT_PROFILE_NAME_LEN];
};

#define CWT_CLAIMS_LEN sizeof(struct cwt_claims_s)
struct cwt_claims_bstr_s {
	uint8_t bstr_hdr[3]; /* bstr(sizeof(struct cose_key_ecdsa_s), 2bytes) */
	struct cwt_claims_s data;
};
#define CWT_CLAIMS_BSTR_HDR CBOR_BSTR_HDR16(CWT_CLAIMS_LEN)

/* Protected COSE header parameters - see go/gsc-dice
 */
#define COSE_PARAM_LABEL_ALG CBOR_UINT0(1)
struct cose_param_bstr_s {
	/* BSTR of size 3 - see the rest of the struct */
	uint8_t bstr_hdr;
	/* Map header: 1 element */
	uint8_t map_hdr;
	/* 1. Alg: uint(1, 0bytes) => nint(-7, 0bytes) */
	uint8_t alg_label;
	uint8_t alg;
};

#define COSE_PARAM_BSTR                                                       \
	{                                                                     \
		/* BSTR of size 3 - see the rest of the struct */             \
		CBOR_HDR1(CBOR_MAJOR_BSTR, 3),                                \
		/* Map header: 1 elem */                                      \
		CBOR_HDR1(CBOR_MAJOR_MAP, 1),                                 \
		/* 1. Alg: uint(1) => nint(-7) */                             \
		COSE_PARAM_LABEL_ALG,                                         \
		CBOR_NINT0(-7) /* ECDSA w/ SHA-256 */                         \
	}

/* Sig structure for CDI certificate - see go/gsc-dice
 */
#define CDI_SIG_STRUCT_CONTEXT_VALUE_LEN 10 /* "Signature1" */
#define CDI_SIG_STRUCT_CONTEXT_LEN	 (1 + CDI_SIG_STRUCT_CONTEXT_VALUE_LEN)
#define CDI_SIG_STRUCT_CONTEXT                                                \
	{                                                                     \
		CBOR_HDR1(CBOR_MAJOR_TSTR, CDI_SIG_STRUCT_CONTEXT_VALUE_LEN), \
			'S', 'i', 'g', 'n', 'a', 't', 'u', 'r', 'e', '1'      \
	}

struct cdi_sig_struct_hdr_s {
	/* Array header: 4 elements */
	uint8_t array_hdr;
	/* 1. Context: tstr("Signature1") */
	uint8_t context[CDI_SIG_STRUCT_CONTEXT_LEN];
	/* 2. Body protected: bstr(COSE param) */
	struct cose_param_bstr_s body_protected;
	/* 3. External AAD: bstr(0 bytes) */
	uint8_t external_aad;
	/* 4. Payload - not fixed, contained in cdi_sig_struct_t */
};

#define CDI_SIG_STRUCT_LEN \
	(sizeof(struct cdi_sig_struct_hdr_s) + sizeof(struct cwt_claims_bstr_s))

/* CDI certificate = COSE_Sign1 structure - see go/gsc-dice
 */

/* Header of the certificate that includes protected & unprotected
 * parameters, but not payload (CWT claims) and signature.
 * This header is actually fixed (no variable fields).
 * We need it to be able to prepend either it or Sig_structure header to
 * payload: the former to return as a part of AndroidDiceHandover,
 * the latter to calculate certificate signature.
 */
struct cdi_cert_hdr_s {
	/* Array header: 4 elements */
	uint8_t array_hdr;
	/* 1. Protected: bstr(COSE param) */
	struct cose_param_bstr_s protected;
	/* 2. Unprotected: empty map */
	uint8_t unprotected;
	/* 3. Payload: bstr(CWT claims) - not fixed, struct cwt_claims_bstr_s */
	/* 4. Signature: bstr(64 bytes) - not fixed, struct cbor_bstr64_s */
};

#define CDI_CERT_LEN                        \
	(sizeof(struct cdi_cert_hdr_s) +    \
	 sizeof(struct cwt_claims_bstr_s) + \
	 sizeof(struct cbor_bstr64_s))

/* DICE cert chain. In our case, CBOR array consisting of exactly 2 entries
 * DiceCertChain = [
 * COSE_Key,       ; UDS pub key
 * COSE_Sign1,     ; CDI DICE cert
 * ]
 */

/* Header of DICE cert chain that includes UDS pubkey, but not CDI DICE cert.
 * This header contains variable fields (UDS key).
 * We need it to be able to prepend either it + cert header or Sig_structure
 * header to CWT claims: the former to return as a part of
 * AndroidDiceHandover, the latter to calculate certificate signature.
 */
struct dice_cert_chain_hdr_s {
	/* Array header: 2 elements */
	uint8_t array_hdr;
	/* 1. UDS pub key: COSE_Key */
	struct cose_key_ecdsa_s uds_pub_key;
	/* 2. CDI DICE cert: - not included,
	 * consists of hdr=cdi_cert_hdr_s, payload=cwt_claims_bstr_s,
	 * sig=cbor_bstr64_s
	 */
};

/* Dice Handover struct. In our case, CBOR map with exactly 3 entries:
 * AndroidDiceHandover = {
 * 1 : bstr .size 32,     ; CDI_Attest
 * 2 : bstr .size 32,     ; CDI_Seal
 * 3 : DiceCertChain,     ; DICE chain - see above
 * }
 */

#define DICE_HANDOVER_LABEL_CDI_ATTEST CBOR_UINT0(1)
#define DICE_HANDOVER_LABEL_CDI_SEAL   CBOR_UINT0(2)
#define DICE_HANDOVER_LABEL_DICE_CHAIN CBOR_UINT0(3)

/* Header of the DICE handover structure that contains the CDIs, but not the
 * DICE chain. This header contains variable fields (CDIs). We need it to be
 * able to prepend either it + DICE chain header + cert header or
 * Sig_structure header to CWT claims: the former to return as a part of
 * AndroidDiceHandover, the latter to calculate certificate signature.
 */
struct dice_handover_hdr_s {
	/* Map header: 3 elements */
	uint8_t map_hdr;
	/* 1. CDI_Attest: uint(1, 0bytes) => bstr(32bytes) */
	uint8_t cdi_attest_label;
	struct cbor_bstr32_s cdi_attest;
	/* 2. CDI_Seal: uint(2, 0bytes) => bstr(32bytes) */
	uint8_t cdi_seal_label;
	struct cbor_bstr32_s cdi_seal;
	/* 3. DICE chain: uint(3, 0bytes) => DICE cert chain */
	uint8_t dice_chain_label;
	/* DICE cert chain is not included */
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __GSC_UTILS_DICE_CBOR_DICE_H */
