/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "tpm_manufacture.h"
#include "tpm_registers.h"

#include "Global.h"
#include "Hierarchy_fp.h"
#include "InternalRoutines.h"
#include "Manufacture_fp.h"
#include "NV_Write_fp.h"
#include "NV_DefineSpace_fp.h"
#include "TPM_Types.h"

#include "console.h"
#include "extension.h"
#include "flash.h"
#include "flash_config.h"
#include "flash_info.h"
#include "printf.h"
#include "registers.h"

#include "dcrypto.h"

#include <cryptoc/sha256.h>

#include <endian.h>
#include <string.h>

#define CROS_ENDORSEMENT_CERT_MAX_SIZE     1932

#define INFO1_SENTINEL_MANUFACTURE_DONE    0x00000000

#define INFO1_EPS_SIZE                     PRIMARY_SEED_SIZE

#define INFO1_SENTINEL_OFFSET FLASH_INFO_MANUFACTURE_STATE_OFFSET
#define INFO1_EPS_OFFSET      FLASH_INFO_MANUFACTURE_STATE_OFFSET

#define CPRINTF(format, args...) cprintf(CC_TPM, format, ## args)

#define FRAME_SIZE             1024
#define KEY_SIZE               32
#define PAYLOAD_MAGIC          0xb1
#define HW_CAT                 0
#define PRODUCT_TYPE           2
#define RWR0_CROS              2
#define PAYLOAD_VERSION        0x8000
#define DEVKEY_ID              0xb93d6539
#define TEST_REGISTRATION_FLAG 0x80

struct cros_ack_response_v0 {
	uint32_t magic;
	uint16_t payload_version;
	uint16_t n_keys;
	struct {
		char name[KEY_SIZE];
	} keys[(FRAME_SIZE - SHA256_DIGEST_SIZE - 4 - 2 - 2) / KEY_SIZE];
	uint8_t _filler[12];  /* Pad out to get exactly to FRAME_SIZE. */
	uint32_t checksum[SHA256_DIGEST_WORDS];
} __packed;
BUILD_ASSERT(sizeof(struct cros_ack_response_v0) == 1012);

enum cros_product_type {
	PRODUCT_TYPE_CROS_TPM = 2,
};

enum cros_perso_component_type {
	CROS_PERSO_COMPONENT_TYPE_EPS = 128,
	CROS_PERSO_COMPONENT_TYPE_RSA_CERT = 129,
	CROS_PERSO_COMPONENT_TYPE_P256_CERT = 130
};

struct haven_perso_response_header_v0 {
	uint8_t  magic[4];
	uint16_t payload_version;
	uint8_t  hwcat[2];
	uint8_t  hwid[8];
	uint8_t rwr[32];
	uint8_t fwr[32];
	uint16_t product_type;
	uint16_t num_components;
	uint8_t  registration_outcome;
	uint8_t  reserved[3];
} __packed;  /* Size: 88B */

struct cros_perso_response_component_info_v0 {
	uint16_t component_size;
	uint8_t  component_type;
	uint8_t  reserved[5];
} __packed;                                             /* Size: 8B */

/* key_id: key for which this is the certificate */
/* cert_len: length of the following certificate */
/* cert: the certificate bytes */
struct cros_perso_certificate_response_v0 {
	uint8_t key_id[4];
	uint32_t cert_len;
	uint8_t cert[CROS_ENDORSEMENT_CERT_MAX_SIZE];
} __packed;                                             /* Size: 1940B */

/* Personalization response. */
struct cros_perso_response_v0 {
	struct haven_perso_response_header_v0 haven_perso_response_header;
	struct cros_perso_response_component_info_v0 cert_info;
	struct cros_perso_certificate_response_v0 cert;
} __packed;                                             /* Size: 2036B */
BUILD_ASSERT(sizeof(struct haven_perso_response_header_v0) == 88);
BUILD_ASSERT(sizeof(struct cros_perso_response_component_info_v0) == 8);
BUILD_ASSERT(sizeof(struct cros_perso_certificate_response_v0) == 1940);
BUILD_ASSERT(sizeof(struct cros_perso_response_v0) == 2036);
/* Adding the TPM header brings the total frame size to 2048-bytes. */

#define RESPONSE_OK     0x8080
#define RESPONSE_NOT_OK 0x8181

struct cros_perso_ok_response_v0 {
	uint16_t ok;
} __packed;

/* TODO(ngm): replace with real pub key. */
static const uint32_t ENDORSEMENT_CA_RSA_N[64] = {
	0xfa3b34ed, 0x3c59ad05, 0x912d6623, 0x83302402,
	0xd43b6755, 0x5777021a, 0xaf37e9a1, 0x45c0e8ad,
	0x9728f946, 0x4391523d, 0xdf7a9164, 0x88f1a9ae,
	0x036c557e, 0x5d9df43e, 0x3e65de68, 0xe172008a,
	0x709dc81f, 0x27a75fe0, 0x3e77f89e, 0x4f400ecc,
	0x51a17dae, 0x2ff9c652, 0xd1d83cdb, 0x20d26349,
	0xbbad71dd, 0x30051b2b, 0x276b2459, 0x809bb8e1,
	0xb8737049, 0xdbe94466, 0x8287072b, 0x070ef311,
	0x6e2a26de, 0x29d69f11, 0x96463d95, 0xb4dc6950,
	0x097d4dfe, 0x1b4a88cc, 0xbd6b50c8, 0x9f7a5b34,
	0xda22c199, 0x9d1ac04b, 0x136af5e5, 0xb1a0e824,
	0x4a065b34, 0x1f67fb46, 0xa1f91ab1, 0x27bb769f,
	0xb704c992, 0xb669cbf4, 0x9299bb6c, 0xcb1b2208,
	0x2dc0d9db, 0xe1513e13, 0xc7f24923, 0xa74c6bcc,
	0xca1a9a69, 0x1b994244, 0x4f64b0d9, 0x78607fd6,
	0x486fb315, 0xa1098c31, 0x5dc50dd6, 0xcdc10874
};

static const struct RSA ENDORSEMENT_CA_RSA_PUB = {
	.e = RSA_F4,
	.N = {
		.dmax = sizeof(ENDORSEMENT_CA_RSA_N) / sizeof(uint32_t),
		.d = (struct access_helper *) ENDORSEMENT_CA_RSA_N,
	},
	.d = {
		.dmax = 0,
		.d = NULL,
	},
};

void uart_hexdump(const char *label, const uint8_t *p, size_t len)
{
	int i;

	uart_printf("%s [%d bytes]\n", label, len);
	for (i = 0; i < len; i++) {
		uart_printf("%02X:", p[i]);
		if ((i + 1) % 8 == 0)
			uart_printf("\n");
	}
	if (len % 8 != 0)
		uart_printf("\n");
}

static int validate_cert(
	const struct cros_perso_response_component_info_v0 *cert_info,
	const struct cros_perso_certificate_response_v0 *cert)
{
	if (cert_info->component_type != CROS_PERSO_COMPONENT_TYPE_RSA_CERT &&
		cert_info->component_type !=
		CROS_PERSO_COMPONENT_TYPE_P256_CERT)
		return 0;  /* Invalid component type. */

	if (cert_info->component_size != sizeof(
			struct cros_perso_certificate_response_v0))
		return 0;  /* Invalid component size. */

	/* TODO(ngm): verify key_id against HIK/FRK0. */
	if (cert->cert_len > CROS_ENDORSEMENT_CERT_MAX_SIZE ||
		cert->cert_len > MAX_NV_BUFFER_SIZE)
		return 0;

	return DCRYPTO_x509_verify(cert->cert, cert->cert_len,
				&ENDORSEMENT_CA_RSA_PUB);
}

static int store_cert(enum cros_perso_component_type component_type,
		const struct cros_perso_certificate_response_v0 *cert)
{
	const uint32_t ek_nv_index_0 = 0x01C00000;
	const uint32_t ek_nv_index_1 = ek_nv_index_0 + 1;
	uint32_t nv_index;
	NV_DefineSpace_In define_space;
	TPMA_NV space_attributes;
	NV_Write_In in;

	/* Indicate that a system reset has occurred, and currently
	 * running with Platform auth.
	 */
	HierarchyStartup(SU_RESET);

	if (component_type == CROS_PERSO_COMPONENT_TYPE_RSA_CERT)
		nv_index = ek_nv_index_0;
	else   /* P256 certificate. */
		nv_index = ek_nv_index_1;

	memset(&space_attributes, 0, sizeof(space_attributes));

	/* Writeable under platform auth. */
	space_attributes.TPMA_NV_PPWRITE = 1;
	/* Not modifyable by OWNER; require PLATFORM auth. */
	/* POLICY_DELETE requires PLATFORM_AUTH */
	space_attributes.TPMA_NV_POLICY_DELETE = 1;
	/* Mark as write-once; space must be deleted to be
	 * re-written.
	 */
	space_attributes.TPMA_NV_WRITEDEFINE = 1;
	/* Space created with platform auth. */
	space_attributes.TPMA_NV_PLATFORMCREATE = 1;
	/* Readable under empty password? */
	space_attributes.TPMA_NV_AUTHREAD = 1;

	define_space.authHandle = TPM_RH_PLATFORM;
	define_space.auth.t.size = 0;
	define_space.publicInfo.t.size = sizeof(
		define_space.publicInfo.t.nvPublic);
	define_space.publicInfo.t.nvPublic.nvIndex = nv_index;
	define_space.publicInfo.t.nvPublic.nameAlg = TPM_ALG_SHA256;
	define_space.publicInfo.t.nvPublic.attributes = space_attributes;
	define_space.publicInfo.t.nvPublic.authPolicy.t.size = 0;
	define_space.publicInfo.t.nvPublic.dataSize = cert->cert_len;

	/* Define the required space first. */
	if (TPM2_NV_DefineSpace(&define_space) != TPM_RC_SUCCESS)
		return 0;

	/* TODO(ngm): call TPM2_NV_WriteLock(nvIndex) on tpm_init();
	 * this prevents delete?
	 */

	in.nvIndex = nv_index;
	in.authHandle = TPM_RH_PLATFORM;
	in.data.t.size = cert->cert_len;
	memcpy(in.data.t.buffer, cert->cert, cert->cert_len);
	in.offset = 0;

	if (TPM2_NV_Write(&in) == TPM_RC_SUCCESS)
		return 1;
	else
		return 0;
}

static uint32_t hw_key_ladder_step(uint32_t cert)
{
	uint32_t itop;

	GREG32(KEYMGR, SHA_ITOP) = 0;  /* clear status */

	GREG32(KEYMGR, SHA_USE_CERT_INDEX) =
		(cert << GC_KEYMGR_SHA_USE_CERT_INDEX_LSB) |
		GC_KEYMGR_SHA_USE_CERT_ENABLE_MASK;

	GREG32(KEYMGR, SHA_CFG_EN) =
		GC_KEYMGR_SHA_CFG_EN_INT_EN_DONE_MASK;
	GREG32(KEYMGR, SHA_TRIG) =
		GC_KEYMGR_SHA_TRIG_TRIG_GO_MASK;

	do {
		itop = GREG32(KEYMGR, SHA_ITOP);
	} while (!itop);

	GREG32(KEYMGR, SHA_ITOP) = 0;  /* clear status */

	return !!GREG32(KEYMGR, HKEY_ERR_FLAGS);
}


#define KEYMGR_CERT_0 0
#define KEYMGR_CERT_3 3
#define KEYMGR_CERT_4 4
#define KEYMGR_CERT_5 5
#define KEYMGR_CERT_7 7
#define KEYMGR_CERT_15 15
#define KEYMGR_CERT_20 20
#define KEYMGR_CERT_25 25
#define KEYMGR_CERT_26 26

#define K_CROS_FW_MAJOR_VERSION 0
static const uint8_t k_cr50_max_fw_major_version = 254;

static int compute_frk2(uint8_t frk2[AES256_BLOCK_CIPHER_KEY_SIZE])
{
	int i;

	if (hw_key_ladder_step(KEYMGR_CERT_0))
		return 0;
	/* Derive HC_PHIK --> Deposited into ISR0 */
	if (hw_key_ladder_step(KEYMGR_CERT_3))
		return 0;

	/* Cryptographically mix OBS-FBS --> Deposited into ISR1 */
	if (hw_key_ladder_step(KEYMGR_CERT_4))
		return 0;
	/* Derive HIK_RT --> Deposited into ISR0 */
	if (hw_key_ladder_step(KEYMGR_CERT_5))
		return 0;
	/* Derive BL_HIK --> Deposited into ISR0 */
	if (hw_key_ladder_step(KEYMGR_CERT_7))
		return 0;

	/* Generate FRK2 by executing certs 15, 20, 25, and 26 */
	if (hw_key_ladder_step(KEYMGR_CERT_15))
		return 0;
	if (hw_key_ladder_step(KEYMGR_CERT_20))
		return 0;

	for (i = 0; i < k_cr50_max_fw_major_version -
			K_CROS_FW_MAJOR_VERSION; i++) {
		if (hw_key_ladder_step(KEYMGR_CERT_25))
			return 0;
	}
	if (hw_key_ladder_step(KEYMGR_CERT_26))
		return 0;
	memcpy(frk2, (void *) GREG32_ADDR(KEYMGR, HKEY_FRR0),
		AES256_BLOCK_CIPHER_KEY_SIZE);
	return 1;
}

/* EPS is stored XOR'd with FRK2, so make sure that the sizes match. */
BUILD_ASSERT(AES256_BLOCK_CIPHER_KEY_SIZE == PRIMARY_SEED_SIZE);
static int decrypt_and_copy_eps(void)
{
	int i;
	uint8_t frk2[AES256_BLOCK_CIPHER_KEY_SIZE];

	if (!compute_frk2(frk2))
		return 0;

	for (i = 0; i < INFO1_EPS_SIZE / sizeof(uint32_t);
	     i += sizeof(uint32_t)) {
		uint32_t word;

		if (flash_physical_info_read_word(
				INFO1_EPS_OFFSET + i, &word) != EC_SUCCESS)
			return 0;     /* Flash read INFO1 failed. */
		/* gp is a TPM global state structure , declared in Global.h. */
		memcpy(gp.EPSeed.t.buffer + (i * sizeof(word)),
		       &word, sizeof(word));
	}

	/* One-time-pad decrypt EPS. */
	for (i = 0; i < PRIMARY_SEED_SIZE; i++)
		gp.EPSeed.t.buffer[i] ^= frk2[i];
	/* Persist the seed to flash. */
	NvWriteReserved(NV_EP_SEED, &gp.EPSeed);
	return 1;
}

static void manufacture_complete(void)
{
	int i;
	const uint32_t erase = INFO1_SENTINEL_MANUFACTURE_DONE;

	/* Wipe encrypted EPS from INFO1... just wipe all of INFO1. */
	for (i = 0; i < FLASH_INFO_MANUFACTURE_STATE_SIZE; i += sizeof(erase))
		flash_info_physical_write(
			FLASH_INFO_MANUFACTURE_STATE_OFFSET + i, sizeof(erase),
			(unsigned char *) &erase);

	/* TODO(ngm): lock HIK export. */
}

int tpm_manufactured(void)
{
	uint32_t sentinel;

	flash_physical_info_read_word(INFO1_SENTINEL_OFFSET, &sentinel);
	return sentinel == INFO1_SENTINEL_MANUFACTURE_DONE;
}

static void ack_command_handler(void *request, size_t command_size,
				size_t *response_size)
{
	uint32_t dev_id0;
	uint32_t dev_id1;
	uint32_t product_type;
	struct cros_ack_response_v0 *ack_response = request;

	CPRINTF("%s size %d\n", __func__, command_size);
	*response_size = 0;

	if (tpm_manufactured())
		return;

	if (command_size) {
		CPRINTF("%s command size is %d\n", __func__, command_size);
		return;
	}

	memset(ack_response, 0, sizeof(struct cros_ack_response_v0));

	ack_response->magic = PAYLOAD_MAGIC;
	ack_response->payload_version = PAYLOAD_VERSION;
	ack_response->n_keys = 1;

	dev_id0 = htobe32(GREG32(FUSE, DEV_ID0));
	dev_id1 = htobe32(GREG32(FUSE, DEV_ID1));
	product_type = htobe16(PRODUCT_TYPE);
	snprintf(ack_response->keys[0].name, KEY_SIZE,
		"%02x:%08x%08x:%02x%02x%02x:%04x", HW_CAT, dev_id0, dev_id1,
		RWR0_CROS, TEST_REGISTRATION_FLAG, DEVKEY_ID & 255,
		product_type);

	/* Compute a checksum over all previous fields. */
	SHA256_hash(ack_response,
		    sizeof(struct cros_ack_response_v0) - SHA256_DIGEST_SIZE,
		    (uint8_t *) ack_response->checksum);
	*response_size = sizeof(*ack_response);
}

static int rsa_cert_done;
static int p256_cert_done;

static void perso_command_handler(void *request, size_t command_size,
				size_t *response_size)
{
	uint16_t ok = RESPONSE_NOT_OK;
	const struct cros_perso_response_v0  *perso_response = request;
	struct cros_perso_ok_response_v0 *ok_response =
		(struct cros_perso_ok_response_v0 *) request;

	CPRINTF("%s size %d\n", __func__, command_size);
	*response_size = 0;

	do {
		if (tpm_manufactured())
			break;

		if (command_size != sizeof(struct cros_perso_response_v0))
			break;

		/* Write RSA / P256 endorsement certificate. */
		if (!validate_cert(&perso_response->cert_info,
					&perso_response->cert))
			break;  /* Invalid cert. */

		if (!rsa_cert_done && !p256_cert_done)
			/* Input validated; initialize flash, TPM globals. */
			if (TPM_Manufacture(1) != 0)
				break;

		if (!store_cert(perso_response->cert_info.component_type,
					&perso_response->cert))
			break;  /* Internal failure. */

		/* TODO(ngm): verify that storage succeeded. */

		if (perso_response->cert_info.component_type ==
			CROS_PERSO_COMPONENT_TYPE_RSA_CERT)
			rsa_cert_done = 1;
		else
			p256_cert_done = 1;

		if (rsa_cert_done && p256_cert_done) {
			/* Setup flash region mapping. */
			flash_info_write_enable();

			/* Copy EPS from INFO1 to flash data region. */
			if (!decrypt_and_copy_eps())
				break;

			/* TODO: generate RSA and ECC keys,
			 *  and verify matching cert.
			 */

			/* Mark as manufactured. */
			manufacture_complete();
		}

		ok = RESPONSE_OK;
	} while (0);

	*response_size = sizeof(*ok_response);
	ok_response->ok = ok;
}

DECLARE_EXTENSION_COMMAND(EXTENSION_MANUFACTURE_ACK, ack_command_handler);
DECLARE_EXTENSION_COMMAND(EXTENSION_MANUFACTURE_PERSO, perso_command_handler);
