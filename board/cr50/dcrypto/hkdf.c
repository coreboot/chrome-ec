/* Copyright 2016 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* An implementation of HKDF as per RFC 5869. */

#include "dcrypto.h"
#include "internal.h"

static int hkdf_extract(uint8_t *PRK, const uint8_t *salt, size_t salt_len,
			const uint8_t *IKM, size_t IKM_len)
{
	struct hmac_sha256_ctx ctx;

	if (PRK == NULL)
		return 0;
	if (salt == NULL && salt_len > 0)
		return 0;
	if (IKM == NULL && IKM_len > 0)
		return 0;

	HMAC_SHA256_hw_init(&ctx, salt, salt_len);
	HMAC_SHA256_update(&ctx, IKM, IKM_len);
	memcpy(PRK, HMAC_SHA256_hw_final(&ctx), SHA256_DIGEST_SIZE);
	return 1;
}

static int hkdf_expand(uint8_t *OKM, size_t OKM_len, const uint8_t *PRK,
		const uint8_t *info, size_t info_len)
{
	uint8_t count = 1;
	const uint8_t *T = OKM;
	size_t T_len = 0;
	uint32_t num_blocks = (OKM_len / SHA256_DIGEST_SIZE) +
		(OKM_len % SHA256_DIGEST_SIZE ? 1 : 0);

	if (OKM == NULL || OKM_len == 0)
		return 0;
	if (PRK == NULL)
		return 0;
	if (info == NULL && info_len > 0)
		return 0;
	if (num_blocks > 255)
		return 0;

	while (OKM_len > 0) {
		struct hmac_sha256_ctx ctx;
		const size_t block_size = OKM_len < SHA256_DIGEST_SIZE ?
			OKM_len : SHA256_DIGEST_SIZE;

		HMAC_SHA256_hw_init(&ctx, PRK, SHA256_DIGEST_SIZE);
		HMAC_SHA256_update(&ctx, T, T_len);
		HMAC_SHA256_update(&ctx, info, info_len);
		HMAC_SHA256_update(&ctx, &count, sizeof(count));
		memcpy(OKM, HMAC_SHA256_hw_final(&ctx), block_size);

		T += T_len;
		T_len = SHA256_DIGEST_SIZE;
		count += 1;
		OKM += block_size;
		OKM_len -= block_size;
	}
	return 1;
}

int DCRYPTO_hkdf(uint8_t *OKM, size_t OKM_len,
		const uint8_t *salt, size_t salt_len,
		const uint8_t *IKM, size_t IKM_len,
		const uint8_t *info, size_t info_len)
{
	int result;
	uint8_t PRK[SHA256_DIGEST_SIZE];

	if (!hkdf_extract(PRK, salt, salt_len, IKM, IKM_len))
		return 0;

	result = hkdf_expand(OKM, OKM_len, PRK, info, info_len);
	always_memset(PRK, 0, sizeof(PRK));
	return result;
}

#ifdef CRYPTO_TEST_SETUP

#include "extension.h"

enum {
	TEST_RFC = 0,
};

#define MAX_OKM_BYTES 1024

static void hkdf_command_handler(void *cmd_body,
				size_t cmd_size,
				size_t *response_size)
{
	uint8_t *cmd;
	uint8_t *out;
	uint8_t op;
	size_t salt_len;
	const uint8_t *salt;
	size_t IKM_len;
	const uint8_t *IKM;
	size_t info_len;
	const uint8_t *info;
	size_t OKM_len;
	uint8_t OKM[MAX_OKM_BYTES];

	/* Command format.
	 *
	 *   WIDTH	   FIELD
	 *   1		   OP
	 *   1             MSB SALT LEN
	 *   1             LSB SALT LEN
	 *   SALT_LEN      SALT
	 *   1             MSB IKM LEN
	 *   1             LSB IKM LEN
	 *   IKM_LEN       IKM
	 *   1             MSB INFO LEN
	 *   1             LSB INFO LEN
	 *   INFO_LEN      INFO
	 *   1             MSB OKM LEN
	 *   1             LSB OKM LEN
	 */
	cmd = (uint8_t *) cmd_body;
	out = (uint8_t *) cmd_body;
	op = *cmd++;

	salt_len = ((uint16_t) (cmd[0] << 8)) | cmd[1];
	cmd += 2;
	salt = cmd;
	cmd += salt_len;

	IKM_len = ((uint16_t) (cmd[0] << 8)) | cmd[1];
	cmd += 2;
	IKM = cmd;
	cmd += IKM_len;

	info_len = ((uint16_t) (cmd[0] << 8)) | cmd[1];
	cmd += 2;
	info = cmd;
	cmd += info_len;

	OKM_len = ((uint16_t) (cmd[0] << 8)) | cmd[1];

	if (OKM_len > MAX_OKM_BYTES) {
		*response_size = 0;
		return;
	}

	switch (op) {
	case TEST_RFC:
		if (DCRYPTO_hkdf(OKM, OKM_len, salt, salt_len,
					IKM, IKM_len, info, info_len)) {
			memcpy(out, OKM, OKM_len);
			*response_size = OKM_len;
		} else {
			*response_size = 0;
		}
		break;
	default:
		*response_size = 0;
	}
}

DECLARE_EXTENSION_COMMAND(EXTENSION_HKDF, hkdf_command_handler);

#endif   /* CRYPTO_TEST_SETUP */
