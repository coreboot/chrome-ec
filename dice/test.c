/*
 * Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

#include "cbor_dice.h"
#include "dice.h"
#include "platform.h"
#include "platform_host.h"

#undef TEST_PLATFORM

#ifdef TEST_PLATFORM

#define DBGDUMP(name)                        \
	do {                                 \
		printf("%s: ", #name);       \
		hexdump(name, sizeof(name)); \
		printf("\n");                \
	} while (0)

static void hexdump(const uint8_t *buf, size_t size)
{
	size_t i;

	for (i = 0; i < size; i++)
		printf("%02x ", buf[i]);
}

static void test_hkdf(void)
{
	uint8_t derived[DIGEST_BYTES];
	const struct slice_ref_s ikm = {
		3, (const uint8_t *)"key"
	};
	const struct slice_ref_s salt = {
		4, (const uint8_t *)"salt"
	};
	const struct slice_ref_s info = {
		5, (const uint8_t *)"label"
	};
	const struct slice_mut_s result = {DIGEST_BYTES, derived };

	if (__platform_hkdf_sha256(ikm, salt, info, result)) {
		__platform_log_str("HKDF: success");
		DBGDUMP(derived);
	} else {
		__platform_log_str("HKDF: failed");
	}
}

static void test_sha(void)
{
	uint8_t digest[DIGEST_BYTES];
	const struct slice_ref_s input = {
		4, (const uint8_t *)"test"
	};

	if (__platform_sha256(input, digest)) {
		__platform_log_str("SHA256: success");
		DBGDUMP(digest);
	} else {
		__platform_log_str("SHA256: failed");
	}
}

static void test_ecdsa(void)
{
	uint8_t seed[DIGEST_BYTES] = { 0 };
	const void *key;
	const struct slice_ref_s input = {
		4, (const uint8_t *)"test"
	};
	uint8_t signature[ECDSA_SIG_BYTES];
	struct ecdsa_public_s pub_key;

	if (!__platform_ecdsa_p256_keygen_hmac_drbg(seed, &key)) {
		__platform_log_str("ECDSA: keygen failed");
		return;
	}

	if (!__platform_ecdsa_p256_sign(key, input, signature)) {
		__platform_log_str("ECDSA: sign failed");
		__platform_ecdsa_p256_free(key);
		return;
	}

	if (!__platform_ecdsa_p256_get_pub_key(key, &pub_key)) {
		__platform_log_str("ECDSA: get pubkey failed");
		__platform_ecdsa_p256_free(key);
		return;
	}

	__platform_ecdsa_p256_free(key);
	__platform_log_str("ECDSA: success");
	DBGDUMP(signature);
	DBGDUMP(pub_key.x);
	DBGDUMP(pub_key.y);
}
#endif /* TEST_PLATFORM */

static bool save_to_file(
	const char *filename,
	uint8_t *buf,
	size_t size
)
{
	FILE *f;
	size_t written;

	f = fopen(filename, "wb");
	if (f == NULL) {
		printf("Failed to open file \"%s\"\n", filename);
		return false;
	}
	written = fwrite(buf, 1, size, f);
	fclose(f);
	return written == size;
}

const size_t kDiceChainOffset = sizeof(struct dice_handover_hdr_s);
static bool test_dice_handover(
	const char *filename_handover,
	const char *filename_chain
)
{
	uint8_t dice_handover[kDiceHandoverSize];
	size_t res =
		get_dice_handover_bytes(dice_handover, 0, kDiceHandoverSize);

	if (res != kDiceHandoverSize) {
		printf("get_dice_handover_bytes failed");
		return false;
	}
	return
		save_to_file(filename_handover,
			     dice_handover,
			     kDiceHandoverSize) &&
		save_to_file(filename_chain,
			     dice_handover + kDiceChainOffset,
			     kDiceHandoverSize - kDiceChainOffset);
}

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
static const uint8_t kPcr0DebugMode[DIGEST_BYTES] = {
	0x23, 0xE1, 0x4D, 0xD9, 0xBB, 0x51, 0xA5, 0x0E,
	0x16, 0x91, 0x1F, 0x7E, 0x11, 0xDF, 0x1E, 0x1A,
	0xAF, 0x0B, 0x17, 0x13, 0x4D, 0xC7, 0x39, 0xC5,
	0x65, 0x36, 0x07, 0xA1, 0xEC, 0x8D, 0xD3, 0x7A
};
static const uint8_t kPcr0Zeroes[DIGEST_BYTES] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static void set_pcr0(const char *param)
{
	const uint8_t *pcr0 = kPcr0DebugMode;

	switch (param[0]) {
	case '0':
		pcr0 = kPcr0Zeroes;
		break;
	case 'n':
		pcr0 = kPcr0NormalMode;
		break;
	case 'r':
		pcr0 = kPcr0RecoveryNormalMode;
		break;
	}

	memcpy(g_dice_config.pcr0, pcr0, DIGEST_BYTES);
}

int main(int argc, char *argv[])
{
#ifdef TEST_PLATFORM
	printf("kDiceHandoverSize = %zu\n", kDiceHandoverSize);
	test_hkdf();
	test_sha();
	test_ecdsa();
#endif /* TEST_PLATFORM */

	if (argc != 1 + 3) {
		printf("Syntax: %s <handover> <dice_chain> <bootmode>\n"
		       "  where\n"
		       "    <handover> - filename for writing the "
		       "dice handover structure\n"
		       "    <dice_chain> - filename for writing the "
		       "dice chain structure\n"
		       "    <bootmode> - sets PCR0 value: when starts with\n"
		       "        0 = all zeroes\n"
		       "        n = normal mode\n"
		       "        r = recovery mode\n"
		       "        <anything else> = debug mode\n"
		       "",
		       argv[0]);
		return 2;
	}
	set_pcr0(argv[3]);
	return test_dice_handover(argv[1], argv[2]) ? 0 : 1;
}
