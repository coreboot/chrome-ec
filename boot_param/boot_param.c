/*
 * Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cbor_boot_param.h"
#include "cdi.h"
#include "boot_param.h"
#include "boot_param_platform.h"

/* Common structure to build Sig_structure or DICE Handover structure
 */

/* Combined headers before cert payload (CWT claims) in DICE handover
 */
struct combined_hdr_s {
	struct dice_cert_chain_hdr_s cert_chain;
	struct cdi_cert_hdr_s cert;
};

union header_options_s {
	uint8_t sig_struct[sizeof(struct combined_hdr_s)];
	struct combined_hdr_s dice_handover;
};

/* We need the following to be able to fill CDIs in dice_handover_s::hdr
 * before we temporarily use dice_handover_s::options.sig_struct to calc
 * signature.
 */
_Static_assert(
	sizeof(struct combined_hdr_s) >= sizeof(struct cdi_sig_struct_hdr_s)
);

struct dice_handover_s {
	struct dice_handover_hdr_s hdr;
	union header_options_s options;
	struct cwt_claims_bstr_s payload;
	struct cbor_bstr64_s signature;
};
const size_t kDiceChainSize =
	sizeof(struct dice_handover_s) - sizeof(struct dice_handover_hdr_s);

/* BootParam = {
 *   1  : uint,               ; structure version (0)
 *   2  : GSCBootParam,
 *   3  : AndroidDiceHandover,
 * }
 */
#define BOOT_PARAM_VERSION 0
struct boot_param_s {
	/* Map header: 3 entries */
	uint8_t map_hdr;
	/* 1. Version: uint(1, 0bytes) => uint(BOOT_PARAM_VERSION, 0bytes) */
	uint8_t version_label;
	uint8_t version;
	/* 2. GSCBootParam: uint(2, 0bytes) => GSCBootParam */
	uint8_t gsc_boot_param_label;
	struct gsc_boot_param_s gsc_boot_param;
	/* 3. AndroidDiceHandover: uint(3, 0bytes) => AndroidDiceHandover */
	uint8_t dice_handover_label;
	struct dice_handover_s dice_handover;
};
const size_t kBootParamSize = sizeof(struct boot_param_s);

/* Context to pass between functions that build the DICE handover structure
 */
struct dice_ctx_s {
	struct boot_param_s output;
	struct dice_config_s cfg;
};

/* PCR0 values for various modes - see go/pcr0-tpm2 */
static const uint8_t kPcr0NormalMode[DIGEST_BYTES] = {
	0x89, 0xEA, 0xF3, 0x51, 0x34, 0xB4, 0xB3, 0xC6,
	0x49, 0xF4, 0x4C, 0x0C, 0x76, 0x5B, 0x96, 0xAE,
	0xAB, 0x8B, 0xB3, 0x4E, 0xE8, 0x3C, 0xC7, 0xA6,
	0x83, 0xC4, 0xE5, 0x3D, 0x15, 0x81, 0xC8, 0xC7
};
static const uint8_t kPcr0RecoveryNormalMode[DIGEST_BYTES] = {
	0x9F, 0x9E, 0xA8, 0x66, 0xD3, 0xF3, 0x4F, 0xE3,
	0xA3, 0x11, 0x2A, 0xE9, 0xCB, 0x1F, 0xBA, 0xBC,
	0x6F, 0xFE, 0x8C, 0xD2, 0x61, 0xD4, 0x24, 0x93,
	0xBC, 0x68, 0x42, 0xA9, 0xE4, 0xF9, 0x3B, 0x3D
};

/* Const salts - see go/gsc-dice */
static const uint8_t kIdSalt[64] = {
	0xDB, 0xDB, 0xAE, 0xBC, 0x80, 0x20, 0xDA, 0x9F,
	0xF0, 0xDD, 0x5A, 0x24, 0xC8, 0x3A, 0xA5, 0xA5,
	0x42, 0x86, 0xDF, 0xC2, 0x63, 0x03, 0x1E, 0x32,
	0x9B, 0x4D, 0xA1, 0x48, 0x43, 0x06, 0x59, 0xFE,
	0x62, 0xCD, 0xB5, 0xB7, 0xE1, 0xE0, 0x0F, 0xC6,
	0x80, 0x30, 0x67, 0x11, 0xEB, 0x44, 0x4A, 0xF7,
	0x72, 0x09, 0x35, 0x94, 0x96, 0xFC, 0xFF, 0x1D,
	0xB9, 0x52, 0x0B, 0xA5, 0x1C, 0x7B, 0x29, 0xEA
};
static const uint8_t kAsymSalt[64] = {
	0x63, 0xB6, 0xA0, 0x4D, 0x2C, 0x07, 0x7F, 0xC1,
	0x0F, 0x63, 0x9F, 0x21, 0xDA, 0x79, 0x38, 0x44,
	0x35, 0x6C, 0xC2, 0xB0, 0xB4, 0x41, 0xB3, 0xA7,
	0x71, 0x24, 0x03, 0x5C, 0x03, 0xF8, 0xE1, 0xBE,
	0x60, 0x35, 0xD3, 0x1F, 0x28, 0x28, 0x21, 0xA7,
	0x45, 0x0A, 0x02, 0x22, 0x2A, 0xB1, 0xB3, 0xCF,
	0xF1, 0x67, 0x9B, 0x05, 0xAB, 0x1C, 0xA5, 0xD1,
	0xAF, 0xFB, 0x78, 0x9C, 0xCD, 0x2B, 0x0B, 0x3B
};
static const uint8_t kSigHdr[2] = CBOR_BSTR64_HDR;

static const struct cwt_claims_bstr_s kCwtClaimsTemplate = {
	CWT_CLAIMS_BSTR_HDR,
	{
		/* Map header: 10 entries */
		CBOR_HDR1(CBOR_MAJOR_MAP, 10),
		/* 1. ISS: uint(1, 0bytes) => tstr(hex(UDS_ID)) */
		CWT_LABEL_ISS,
		CBOR_TSTR40_EMPTY, /* CALC - calc from UDS */
		/* 2. SUB: uint(2, 0bytes) =>
		 *    tstr(hex(CDI_ID))
		 */
		CWT_LABEL_SUB,
		CBOR_TSTR40_EMPTY, /* CALC - calc from CDI */
		/* 3. Code Hash: nint(-4670545, 4bytes) =>
		 *    bstr(32bytes)
		 */
		CWT_LABEL_CODE_HASH,
		CBOR_BSTR32_EMPTY, /* VARIABLE */
		/* 4. Cfg Hash: nint(-4670547, 4bytes) =>
		 *    bstr(32bytes)
		 */
		CWT_LABEL_CFG_HASH,
		CBOR_BSTR32_EMPTY, /* CALC - calc from CfgDescr */
		/* 5. Cfg Descr: nint(-4670548, 4bytes) =>
		 *    bstr(struct cfg_descr_s)
		 */
		CWT_LABEL_CFG_DESCR,
		/* struct cfg_descr_bstr_s */
		{
			CFG_DESCR_BSTR_HDR,
			/* struct cfg_descr_s */
			{
				/* Map header: 6 entries */
				CBOR_HDR1(CBOR_MAJOR_MAP, 6),
				/* 1. Comp name: nint(-70002, 4bytes) =>
				 * tstr("CrOS AP FW")
				 */
				CFG_DESCR_LABEL_COMP_NAME,
				CFG_DESCR_COMP_NAME,
				/* 2. Resettable: nint(-70004, 4bytes) => null
				 */
				CFG_DESCR_LABEL_RESETTABLE,
				CBOR_NULL,
				/* 3. Sec ver: nint(-70005, 4bytes) =>
				 *    uint(Security ver, 4bytes)
				 */
				CFG_DESCR_LABEL_SEC_VER,
				CBOR_UINT32_ZERO, /* VARIABLE */
				/* 4. APROV status:
				 *    nint(-71000, 4bytes) =>
				 *    uint(APROV status, 4bytes)
				 */
				CFG_DESCR_LABEL_APROV_STATUS,
				CBOR_UINT32_ZERO, /* VARIABLE */
				/* 5. Vboot status: */
				/* nint(-71000, 4bytes) => */
				/* bstr(PCR0, 32bytes) */
				CFG_DESCR_LABEL_VBOOT_STATUS,
				CBOR_BSTR32_EMPTY, /* VARIABLE */
				/* 6. AP FW version: */
				/* nint(-71002, 4bytes) => */
				/* bstr(PCR10, 32bytes) */
				CFG_DESCR_LABEL_AP_FW_VERSION,
				CBOR_BSTR32_EMPTY, /* VARIABLE */
			},
		},
		/* 6. Auth Hash: nint(-4670549, 4bytes) => bstr(32bytes) */
		CWT_LABEL_AUTH_HASH,
		CBOR_BSTR32_EMPTY, /* always zero */
		/* 7. Mode: nint(-4670551, 4bytes) => bstr(1byte) */
		CWT_LABEL_MODE,
		CBOR_BSTR1_EMPTY, /* VARIABLE */
		/* 8. Subject PK: nint(-4670552, 4bytes) => bstr(COSE_Key) */
		CWT_LABEL_SUBJECT_PK,
		/* struct cose_key_ecdsa_bstr_s */
		{
			COSE_KEY_ECDSA_BSTR_HDR,
			/* struct cose_key_ecdsa_s */
			{
				/* Map header: 6 entries */
				CBOR_HDR1(CBOR_MAJOR_MAP, 6),
				/* 1. Key type: uint(1, 0bytes) =>
				 * uint(2, 0bytes)
				 */
				COSE_KEY_LABEL_KTY,
				CBOR_UINT0(2), /* EC2 */
				/* 2. Algorithm: uint(3, 0bytes) =>
				 *    nint(-1, 0bytes)
				 */
				COSE_KEY_LABEL_ALG,
				CBOR_NINT0(-7), /* ECDSA w/SHA-256 */
				/* 3. Key ops: uint(4, 0bytes) =>
				 *    array(1) { uint(2, 0bytes) }
				 */
				COSE_KEY_LABEL_KEY_OPS,
				CBOR_HDR1(CBOR_MAJOR_ARR, 1),
				CBOR_UINT0(2),
				/* 4. Curve: nint(-1, 0bytes) => uint(1, 0bytes)
				 */
				COSE_KEY_LABEL_CRV,
				CBOR_UINT0(1), /* P-256 */
				/* 5. X: nint(-2, 0bytes) =>
				 *    bstr(X, 32bytes)
				 */
				COSE_KEY_LABEL_X,
				CBOR_BSTR32_EMPTY, /* VARIABLE */
				/* 6. X: nint(-3, 0bytes) =>
				 *    bstr(Y, 32bytes)
				 */
				COSE_KEY_LABEL_Y,
				CBOR_BSTR32_EMPTY, /* VARIABLE */
			},
		},
		/* 9. Key Usage: nint(-4670553, 4bytes) => bstr(1byte) */
		CWT_LABEL_KEY_USAGE,
		{
			CBOR_HDR1(CBOR_MAJOR_BSTR, 1),
			0x20, /* keyCertSign */
		},
		/* 10. Profile name: nint(-4670554, 4bytes) => */
		/* tstr("android.16") */
		CWT_LABEL_PROFILE_NAME,
		CWT_PROFILE_NAME,
	},
};

static const struct dice_handover_hdr_s kDiceHandoverHdrTemplate = {
	/* Map header: 3 elements */
	CBOR_HDR1(CBOR_MAJOR_MAP, 3),
	/* 1. CDI_Attest: uint(1, 0bytes) => bstr(32bytes) */
	DICE_HANDOVER_LABEL_CDI_ATTEST,
	CBOR_BSTR32_EMPTY, /* CALC - CDI_attest */
	/* 2. CDI_Seal: uint(2, 0bytes) => bstr(32bytes) */
	DICE_HANDOVER_LABEL_CDI_SEAL,
	CBOR_BSTR32_EMPTY, /* CALC - CDI_seal */
	/* 3. DICE chain: uint(3, 0bytes) => DICE cert chain */
	DICE_HANDOVER_LABEL_DICE_CHAIN
	/* DICE cert chain is not included */
};

static const struct cdi_sig_struct_hdr_s kSigStructFixedHdr = {
	/* Array header: 4 elements */
	CBOR_HDR1(CBOR_MAJOR_ARR, 4),
	/* 1. Context: tstr("Signature1") */
	CDI_SIG_STRUCT_CONTEXT,
	/* 2. Body protected: bstr(COSE param) */
	COSE_PARAM_BSTR,
	/* 3. External AAD: Bstr(0 bytes) */
	CBOR_HDR1(CBOR_MAJOR_BSTR, 0),
	/* 4. Payload - not fixed, contained in cdi_sig_struct_t */
};

static const struct combined_hdr_s kCombinedHdrTemplate = {
	/* struct dice_cert_chain_hdr_s cert_chain */
	{
		/* Array header: 2 elements */
		CBOR_HDR1(CBOR_MAJOR_ARR, 2),
		/* 1. UDS pub key: COSE_Key
		 * struct cose_key_ecdsa_s
		 */
		{
			/* Map header: 6 entries */
			CBOR_HDR1(CBOR_MAJOR_MAP, 6),
			/* 1. Key type: uint(1, 0bytes) => uint(2, 0bytes) */
			COSE_KEY_LABEL_KTY,
			CBOR_UINT0(2), /* EC2 */
			/* 2. Algorithm: uint(3, 0bytes) => nint(-1, 0bytes) */
			COSE_KEY_LABEL_ALG,
			CBOR_NINT0(-7), /* ECDSA w/ SHA-256 */
			/* 3. Key ops: uint(4, 0bytes) =>
			 *    array(1) { uint(2, 0bytes) }
			 */
			COSE_KEY_LABEL_KEY_OPS,
			CBOR_HDR1(CBOR_MAJOR_ARR, 1),
			CBOR_UINT0(2),
			/* 4. Curve: nint(-1, 0bytes) => uint(1, 0bytes) */
			COSE_KEY_LABEL_CRV,
			CBOR_UINT0(1), /* P-256 */
			/* 5. X: nint(-2, 0bytes) => bstr(X, 32bytes) */
			COSE_KEY_LABEL_X,
			CBOR_BSTR32_EMPTY, /* VARIABLE */
			/* 6. X: nint(-3, 0bytes) => bstr(Y, 32bytes) */
			COSE_KEY_LABEL_Y,
			CBOR_BSTR32_EMPTY, /* VARIABLE */
		},
		/* 2. CDI DICE cert: - not included, */
		/* consists of hdr=cdi_cert_hdr_s, payload=cwt_claims_bstr_s, */
		/* sig=cbor_bstr64_s */
	},
	/* struct cdi_cert_hdr_s cert */
	{
		/* Array header: 4 elements */
		CBOR_HDR1(CBOR_MAJOR_ARR, 4),
		/* 1. Protected: bstr(COSE param) */
		COSE_PARAM_BSTR,
		/* 2. Unprotected: empty map */
		CBOR_HDR1(CBOR_MAJOR_MAP, 0),
		/* 3. Payload: bstr(CWT claims) - not fixed, contained in */
		/* cdi_cert_t */
		/* 4. Signature: bstr(64 bytes) - not fixed, contained in */
		/* cdi_cert_t */
	}
};

static const struct slice_ref_s kCdiAttestLabel = {
	10 /* strlen("CDI_Attest") */,
	(const uint8_t *)"CDI_Attest"
};
static const struct slice_ref_s kCdiSealLabel = {
	8 /* strlen("CDI_Seal") */,
	(const uint8_t *)"CDI_Seal"
};
static const struct slice_ref_s kIdSaltSlice = {
	64,
	(const uint8_t *)kIdSalt
};
static const struct slice_ref_s kAsymSaltSlice = {
	64,
	(const uint8_t *)kAsymSalt
};
static const struct slice_ref_s kIdLabel = {
	2, /* streln("ID") */
	(const uint8_t *)"ID"
};
static const struct slice_ref_s kKeyPairLabel = {
	8, /* strelen("Key Pair") */
	(const uint8_t *)"Key Pair"
};

/* Calculates CDI from {UDS, inputs_digest, label}
 */
static inline bool calc_cdi_from_digest(
	/* [IN] UDS */
	const uint8_t uds[DIGEST_BYTES],
	/* [IN] digest of inputs */
	const uint8_t inputs_digest[DIGEST_BYTES],
	/* [IN] label */
	const struct slice_ref_s label,
	/* [OUT] CDI */
	uint8_t cdi[DIGEST_BYTES]
)
{
	const struct slice_ref_s uds_slice = digest_as_slice(uds);
	const struct slice_ref_s inputs_digest_slice =
		digest_as_slice(inputs_digest);
	const struct slice_mut_s cdi_slice = digest_as_slice_mut(cdi);

	return __platform_hkdf_sha256(uds_slice, inputs_digest_slice, label,
				      cdi_slice);
}

/* Calculates CDI from {UDS, inputs, label}
 */
static bool calc_cdi(
	/* [IN] UDS */
	const uint8_t uds[DIGEST_BYTES],
	/* [IN] inputs */
	const struct slice_ref_s inputs,
	/* [IN] label */
	const struct slice_ref_s label,
	/* [OUT] CDI */
	uint8_t cdi[DIGEST_BYTES]
)
{
	uint8_t inputs_digest[DIGEST_BYTES];

	if (!__platform_sha256(inputs, inputs_digest)) {
		__platform_log_str("Failed to hash inputs");
		return false;
	}
	return calc_cdi_from_digest(uds, inputs_digest, label, cdi);
}

/* Fills inputs for sealing CDI.
 * Assumes that ctx->cfg and CfgDescr in ctx->output are already filled.
 */
static inline void fill_inputs_seal(
	/* [IN] dice context */
	const struct dice_ctx_s *ctx,
	/* [OUT] inputs */
	struct cdi_seal_inputs_s *inputs
)
{
	__platform_memset(inputs->auth_data_digest, 0, DIGEST_BYTES);
	__platform_memcpy(inputs->hidden_digest, ctx->cfg.hidden_digest,
			  DIGEST_BYTES);
	inputs->mode = ctx->output.dice_handover.payload.data.mode.value;
}

/* Fills inputs for attestation CDI
 * Assumes that ctx->cfg and CfgDescr in ctx->output are already filled.
 */
static inline void fill_inputs_attest(
	/* [IN] dice context */
	const struct dice_ctx_s *ctx,
	/* [OUT] inputs */
	struct cdi_attest_inputs_s *inputs
)
{
	const struct cwt_claims_s *cwt_claims =
		&ctx->output.dice_handover.payload.data;

	__platform_memcpy(inputs->code_digest,
			  cwt_claims->code_hash.value,
			  DIGEST_BYTES);
	__platform_memcpy(inputs->cfg_desr_digest,
			  cwt_claims->cfg_hash.value,
			  DIGEST_BYTES);
	fill_inputs_seal(ctx, &inputs->seal_inputs);
}

/* Calculates attestation CDI.
 * Assumes that ctx->cfg and CfgDescr in ctx->output are already filled.
 */
static inline bool calc_cdi_attest(
	/* [IN] dice context */
	const struct dice_ctx_s *ctx,
	/* [OUT] CDI */
	uint8_t cdi[DIGEST_BYTES]
)
{
	struct cdi_attest_inputs_s inputs;
	const struct slice_ref_s inputs_slice = {
		sizeof(inputs), (uint8_t *)&inputs
	};

	fill_inputs_attest(ctx, &inputs);
	return calc_cdi(ctx->cfg.uds, inputs_slice, kCdiAttestLabel, cdi);
}

/* Calculates sealing CDI.
 * Assumes that ctx->cfg and CfgDescr in ctx->output are already filled.
 */
static inline bool calc_cdi_seal(
	/* [IN] dice context */
	const struct dice_ctx_s *ctx,
	/* [OUT] CDI */
	uint8_t cdi[DIGEST_BYTES]
)
{
	struct cdi_seal_inputs_s inputs;
	const struct slice_ref_s inputs_slice = {
		sizeof(inputs), (uint8_t *)&inputs
	};

	fill_inputs_seal(ctx, &inputs);
	return calc_cdi(ctx->cfg.uds, inputs_slice, kCdiSealLabel, cdi);
}

/* Calculates boot mode from the data in ctx->cfg
 * On DT/OT with ti50:
 * - Normal if
 * - APROV succeeded or not configured from factory (for legacy devices).
 * Note: AllowUnverifiedRo counts as a failure AND
 * - Coreboot reported 'normal' boot mode
 * - Recovery if
 * - APROV succeeded or not configured from factory (for legacy devices).
 * Note: AllowUnverifiedRo counts as a failure AND
 * - Coreboot reported 'recovery-normal' boot mode
 * - Debug - in all other cases
 *
 * On H1 with Cr50:
 * - Normal if Coreboot reported 'normal' boot mode
 * - Recovery if Coreboot reported 'recovery-normal' boot mode
 * - Debug - in all other cases
 */
static inline uint8_t calc_mode(
	/* [IN] dice context */
	const struct dice_ctx_s *ctx
)
{
	if (__platform_aprov_status_allows_normal(ctx->cfg.aprov_status)) {
		if (__platform_memcmp(ctx->cfg.pcr0, kPcr0NormalMode,
				      DIGEST_BYTES) == 0) {
			return BOOT_MODE_NORMAL;
		}
		if (__platform_memcmp(ctx->cfg.pcr0, kPcr0RecoveryNormalMode,
				      DIGEST_BYTES) == 0) {
			return BOOT_MODE_RECOVERY;
		}
	}
	return BOOT_MODE_DEBUG;
}

/* Generates CDI cert signature for the initialized builder with pre-filled
 * CWT claims.
 */
static inline bool fill_cdi_cert_signature(
	/* [IN/OUT] dice context */
	struct dice_ctx_s *ctx,
	/* [IN] key handle to sign */
	const void *key
)
{
	uint8_t *sig_struct = ((uint8_t *)&ctx->output.dice_handover.payload) -
			      sizeof(struct cdi_sig_struct_hdr_s);
	const struct slice_ref_s data_to_sign = {
		CDI_SIG_STRUCT_LEN, sig_struct
	};
	struct cbor_bstr64_s *sig_bstr64 =
		&ctx->output.dice_handover.signature;

	__platform_memcpy(sig_struct, &kSigStructFixedHdr,
			  sizeof(struct cdi_sig_struct_hdr_s));
	__platform_memcpy(sig_bstr64->cbor_hdr, kSigHdr, 2);
	return __platform_ecdsa_p256_sign(key, data_to_sign,
					  sig_bstr64->value);
}

/* Generates key from UDS or CDI_Attest value.
 */
static bool generate_key(
	/* [IN] CDI_attest or UDS */
	const uint8_t input[DIGEST_BYTES],
	/* [OUT] key handle */
	const void **key
)
{
	uint8_t drbg_seed[DIGEST_BYTES];
	const struct slice_ref_s input_slice = digest_as_slice(input);
	const struct slice_mut_s drbg_seed_slice =
		digest_as_slice_mut(drbg_seed);

	if (!__platform_hkdf_sha256(input_slice, kAsymSaltSlice,
				    kKeyPairLabel, drbg_seed_slice)) {
		__platform_log_str("ASYM_KDF failed");
		return false;
	}
	return __platform_ecdsa_p256_keygen_hmac_drbg(drbg_seed, key);
}

/* Generates {UDS, CDI}_ID from {UDS, CDI} public key.
 */
static bool generate_id_from_pub_key(
	/* [IN] public key */
	const struct ecdsa_public_s *pub_key,
	/* [OUT] generated id */
	uint8_t dice_id[DICE_ID_BYTES]
)
{
	const struct slice_ref_s pub_key_slice = {
		sizeof(struct ecdsa_public_s), (const uint8_t *)pub_key
	};
	const struct slice_mut_s dice_id_slice = {
		DICE_ID_BYTES, (uint8_t *)dice_id
	};

	return __platform_hkdf_sha256(pub_key_slice, kIdSaltSlice, kIdLabel,
				      dice_id_slice);
}

/* Returns hexdump character for the half-byte.
 */
static inline uint8_t hexdump_halfbyte(uint8_t half_byte)
{
	if (half_byte < 10)
		return '0' + half_byte;
	else
		return 'a' + half_byte - 10;
}

/* Fills hexdump of the byte (lowercase).
 */
static inline void hexdump_byte(
	/* [IN] byte to hexdump */
	uint8_t byte,
	/* [OUT] str (always 2 bytes) with hexdump */
	uint8_t *str
)
{
	str[0] = hexdump_halfbyte((byte & 0xF0) >> 4);
	str[1] = hexdump_halfbyte(byte & 0x0F);
}

/* Fills {CDI, UDS} ID string from ID bytes.
 */
static void fill_dice_id_string(
	/* [IN] IDS_ID or CDI_ID */
	const uint8_t dice_id[DICE_ID_BYTES],
	/* [OUT] hexdump of this ID */
	uint8_t dice_id_str[DICE_ID_HEX_BYTES]
)
{
	size_t idx;

	for (idx = 0; idx < DICE_ID_BYTES; idx++, dice_id_str += 2)
		hexdump_byte(dice_id[idx], dice_id_str);
}

/* Fills COSE_Key structure from pubkey.
 * Assumes that all fields in COSE_Key except for X, y are already filled
 * from template.
 */
static inline void fill_cose_pubkey(
	/* [IN] pub key */
	const struct ecdsa_public_s *pub_key,
	/* [IN/OUT] COSE key structure */
	struct cose_key_ecdsa_s *cose_key
)
{
	__platform_memcpy(cose_key->x.value, pub_key->x, ECDSA_POINT_BYTES);
	__platform_memcpy(cose_key->y.value, pub_key->y, ECDSA_POINT_BYTES);
}

/* Fills CDI_attest pubkey, CDI_ID in the CDI certificate using generated
 * CDI_attest key.
 */
static inline bool fill_cdi_details_with_key(
	/* [IN/OUT] dice context */
	struct dice_ctx_s *ctx,
	/* [IN] CDI_attest key handle */
	const void *cdi_key
)
{
	struct ecdsa_public_s cdi_pub_key;
	uint8_t cdi_id[DICE_ID_BYTES];
	struct cwt_claims_s *cwt_claims =
		&ctx->output.dice_handover.payload.data;

	if (!__platform_ecdsa_p256_get_pub_key(cdi_key, &cdi_pub_key)) {
		__platform_log_str("Failed to get CDI pubkey");
		return false;
	}
	fill_cose_pubkey(&cdi_pub_key, &cwt_claims->subject_pk.data);
	if (!generate_id_from_pub_key(&cdi_pub_key, cdi_id)) {
		__platform_log_str("Failed to generate CDI_ID");
		return false;
	}
	/* SUB = hex(CDI_ID) */
	fill_dice_id_string(cdi_id, cwt_claims->sub.value);

	return true;
}

/* Fills CDI_attest pubkey, CDI_ID in the CDI certificate.
 * Assumes that ctx->cfg and CfgDescr in ctx->output are already filled.
 */
static inline bool fill_cdi_details(
	/* [IN/OUT] dice context */
	struct dice_ctx_s *ctx
)
{
	const void *cdi_key;
	bool result;
	struct dice_handover_hdr_s *hdr = &ctx->output.dice_handover.hdr;

	__platform_memcpy(hdr, &kDiceHandoverHdrTemplate,
			  sizeof(struct dice_handover_hdr_s));
	if (!calc_cdi_attest(ctx, hdr->cdi_attest.value)) {
		__platform_log_str("Failed to calc CDI_attest");
		return false;
	}
	if (!calc_cdi_seal(ctx, hdr->cdi_seal.value)) {
		__platform_log_str("Failed to calc CDI_seal");
		return false;
	}
	if (!generate_key(hdr->cdi_attest.value, &cdi_key)) {
		__platform_log_str("Failed to generate CDI key");
		return false;
	}
	result = fill_cdi_details_with_key(ctx, cdi_key);
	__platform_ecdsa_p256_free(cdi_key);

	return result;
}

/* Fills UDS_ID, signature into the certificate using generated UDS key.
 * Assumes that all other fields of CDI certificate are filled already.
 */
static inline bool fill_uds_details_with_key(
	/* [IN/OUT] dice context */
	struct dice_ctx_s *ctx,
	/* [IN] UDS key handle */
	const void *uds_key
)
{
	struct ecdsa_public_s uds_pub_key;
	uint8_t uds_id[DICE_ID_BYTES];
	struct cwt_claims_s *cwt_claims =
		&ctx->output.dice_handover.payload.data;
	struct combined_hdr_s *combined_hdr =
		&ctx->output.dice_handover.options.dice_handover;

	if (!__platform_ecdsa_p256_get_pub_key(uds_key, &uds_pub_key)) {
		__platform_log_str("Failed to get UDS pubkey");
		return false;
	}
	if (!generate_id_from_pub_key(&uds_pub_key, uds_id)) {
		__platform_log_str("Failed to generate UDS_ID");
		return false;
	}
	/* ISS = hex(UDS_ID) */
	fill_dice_id_string(uds_id, cwt_claims->iss.value);
	if (!fill_cdi_cert_signature(ctx, uds_key)) {
		__platform_log_str("Failed to sign CDI cert");
		return false;
	}

	/* We can do the rest only after we generated the signature because */
	/* signature generation uses ctx->output.hdr temporarily to build */
	/* Sig_struct for signing. */
	__platform_memcpy(combined_hdr,
			  &kCombinedHdrTemplate,
			  sizeof(struct combined_hdr_s));
	fill_cose_pubkey(
		&uds_pub_key,
		&combined_hdr->cert_chain.uds_pub_key);

	return true;
}

/* Fills UDS_ID, signature into the certificate. */
/* Assumes that all other fields of CDI certificate were filled already. */
static inline bool fill_uds_details(
	struct dice_ctx_s *ctx /* [IN/OUT] dice context */
)
{
	const void *uds_key;
	bool result;

	if (!generate_key(ctx->cfg.uds, &uds_key)) {
		__platform_log_str("Failed to generate UDS key");
		return false;
	}
	result = fill_uds_details_with_key(ctx, uds_key);
	__platform_ecdsa_p256_free(uds_key);

	return result;
}

/* Fills value in struct cbor_uint32_s */
/* Assumes that `cbor_var->cbor_hdr` is already pre-set */
static inline void set_cbor_u32(
	uint32_t value, /* [IN] value to set */
	struct cbor_uint32_s *cbor_var /* [OUT] CBOR UINT32 variable to fill */
)
{
	cbor_var->value[0] = (uint8_t)(((value) & 0xFF000000) >> 24);
	cbor_var->value[1] = (uint8_t)(((value) & 0x00FF0000) >> 16);
	cbor_var->value[2] = (uint8_t)(((value) & 0x0000FF00) >> 8);
	cbor_var->value[3] = (uint8_t)((value) & 0x000000FF);
}

/* Fills CfgDescr, CfgDescr digest and boot mode in CDI certificate */
static inline bool fill_config_details(
	struct dice_ctx_s *ctx /* [IN/OUT] dice context */
)
{
	struct cwt_claims_bstr_s *payload = &ctx->output.dice_handover.payload;
	struct cwt_claims_s *cwt_claims = &payload->data;
	struct cfg_descr_s *cfg_descr = &payload->data.cfg_descr.data;
	const struct slice_ref_s cfg_descr_slice = { sizeof(struct cfg_descr_s),
					(uint8_t *)cfg_descr };

	/* Copy fixed data from the template */
	__platform_memcpy(payload, &kCwtClaimsTemplate,
			  sizeof(struct cwt_claims_bstr_s));

	/* Fill Cfg Descriptor variables based on ctx->cfg */
	set_cbor_u32(ctx->cfg.aprov_status, &cfg_descr->aprov_status);
	set_cbor_u32(ctx->cfg.sec_ver, &cfg_descr->sec_ver);
	__platform_memcpy(cfg_descr->vboot_status.value, ctx->cfg.pcr0,
			  DIGEST_BYTES);
	__platform_memcpy(cfg_descr->ap_fw_version.value, ctx->cfg.pcr10,
			  DIGEST_BYTES);

	/* Calculate Cfg Descriptor digest */
	if (!__platform_sha256(cfg_descr_slice, cwt_claims->cfg_hash.value)) {
		__platform_log_str("Failed to calc CfgDescr digest");
		return false;
	}

	/* Calculate boot mode */
	cwt_claims->mode.value = calc_mode(ctx);

	return true;
}

/* Fills DICE handover structure in struct dice_ctx_s. */
/* Assumes ctx.cfg is already filled */
static inline bool generate_dice_handover(
	struct dice_ctx_s *ctx /* [IN/OUT] dice context */
)
{
	/* 1. Fill device configuration details in CDI certificate: CfgDescr and
	 * its digest, boot mode.
	 * 2. Fill CDI details in CDI certificate (CDI pubkey, CDI_ID) and DICE
	 * handover (CDIs) Relies on config details to be filled already
	 * 3. Fill UDS details in CDI certificate (UDS_ID, signature by UDS key)
	 * and DICE chain (UDS pubkey) Relies on the rest of CDI certificate to
	 * be filled already.
	 */

	return fill_config_details(ctx) &&
		fill_cdi_details(ctx) &&
		fill_uds_details(ctx);
}

/* Fills GSCBootParam. */
static inline bool fill_gsc_boot_param(
	struct gsc_boot_param_s *gsc_boot_param /* [IN/OUT] GSCBootParam */
)
{
	/* GSCBootParam: Map header: 3 entries */
	gsc_boot_param->map_hdr = CBOR_HDR1(CBOR_MAJOR_MAP, 3);

	/* GSCBootParam entry 1: EarlyEntropy:
	 * uint(1, 0bytes) => bstr(entropy, 64bytes)
	 */
	gsc_boot_param->early_entropy_label = CBOR_UINT0(1);
	gsc_boot_param->early_entropy.cbor_hdr[0] =
		CBOR_HDR1(CBOR_MAJOR_BSTR, CBOR_BYTES1);
	gsc_boot_param->early_entropy.cbor_hdr[1] = EARLY_ENTROPY_BYTES;

	/* GSCBootParam entry 2: SessionKeySeed:
	 * uint(2, 0bytes) => bstr(entropy, 32bytes)
	 */
	gsc_boot_param->session_key_seed_label = CBOR_UINT0(2);
	gsc_boot_param->session_key_seed.cbor_hdr[0] =
		CBOR_HDR1(CBOR_MAJOR_BSTR, CBOR_BYTES1);
	gsc_boot_param->session_key_seed.cbor_hdr[1] = KEY_SEED_BYTES;

	/* GSCBootParam entry 3: AuthTokenKeySeed:
	 * uint(3, 0bytes) => bstr(entropy, 32bytes)
	 */
	gsc_boot_param->auth_token_key_seed_label = CBOR_UINT0(3);
	gsc_boot_param->auth_token_key_seed.cbor_hdr[0] =
		CBOR_HDR1(CBOR_MAJOR_BSTR, CBOR_BYTES1);
	gsc_boot_param->auth_token_key_seed.cbor_hdr[1] = KEY_SEED_BYTES;

	if (!__platform_get_gsc_boot_param(
			gsc_boot_param->early_entropy.value,
			gsc_boot_param->session_key_seed.value,
			gsc_boot_param->auth_token_key_seed.value)) {
		__platform_log_str("Failed to get GSC boot param");
		return false;
	}
	return true;
}

/* Fills GSCBootParam and BootParam header in struct dice_ctx_s. */
/* Doesn't touch DICE handover structure */
static inline bool fill_boot_param(
	struct dice_ctx_s *ctx /* [IN/OUT] dice context */
)
{
	/* BootParam: Map header: 3 entries */
	ctx->output.map_hdr = CBOR_HDR1(CBOR_MAJOR_MAP, 3);

	/* BootParam entry 1: Version:
	 * uint(1, 0bytes) => uint(BOOT_PARAM_VERSION, 0bytes)
	 */
	ctx->output.version_label = CBOR_UINT0(1);
	ctx->output.version = CBOR_UINT0(0);

	/* BootParam entry 2: GSCBootParam:
	 * uint(2, 0bytes) => GSCBootParam (filled in fill_gsc_boot_param)
	 */
	ctx->output.gsc_boot_param_label = CBOR_UINT0(2);

	/* BootParam entry 3: AndroidDiceHandover:
	 * uint(3, 0bytes) => AndroidDiceHandover (not touched in this func)
	 */
	ctx->output.dice_handover_label = CBOR_UINT0(3);

	return fill_gsc_boot_param(&ctx->output.gsc_boot_param);
}

/* Get (part of) BootParam structure: [offset .. offset + size). */
size_t get_boot_param_bytes(
	/* [OUT] destination buffer to fill */
	uint8_t *dest,
	/* [IN] starting offset in the BootParam struct */
	size_t offset,
	/* [IN] size of the BootParam struct to copy */
	size_t size
)
{
	struct dice_ctx_s ctx;
	uint8_t *src = (uint8_t *)&ctx.output;

	if (size == 0 || offset >= kBootParamSize)
		return 0;
	if (size > kBootParamSize - offset)
		size = kBootParamSize - offset;

	if (!__platform_get_dice_config(&ctx.cfg)) {
		__platform_log_str("Failed to get DICE config");
		return 0;
	}
	if (!generate_dice_handover(&ctx))
		return 0;

	if (!fill_boot_param(&ctx))
		return 0;

	__platform_memcpy(dest, src + offset, size);
	return size;
}

/* Get (part of) DiceChain structure: [offset .. offset + size) */
size_t get_dice_chain_bytes(
	/* [OUT] destination buffer to fill */
	uint8_t *dest,
	/* [IN] starting offset in the DiceChain struct */
	size_t offset,
	/* [IN] size of the data to copy */
	size_t size
)
{
	struct dice_ctx_s ctx;
	uint8_t *src = (uint8_t *)&ctx.output.dice_handover.options;

	if (size == 0 || offset >= kDiceChainSize)
		return 0;
	if (size > kDiceChainSize - offset)
		size = kDiceChainSize - offset;

	if (!__platform_get_dice_config(&ctx.cfg)) {
		__platform_log_str("Failed to get DICE config");
		return 0;
	}
	if (!generate_dice_handover(&ctx))
		return 0;

	__platform_memcpy(dest, src + offset, size);
	return size;
}
