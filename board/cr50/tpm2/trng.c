/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "CryptoEngine.h"

CRYPT_RESULT _cpri__StirRandom(int32_t num, uint8_t *entropy)
{
	return CRYPT_SUCCESS;     /* NO-OP on CR50. */
}

#ifdef CRYPTO_TEST_SETUP
#include "endian.h"
#include "extension.h"
#include "trng.h"
/*
 * This extension command is similar to TPM2_GetRandom, but made
 * available for CRYPTO_TEST = 1 which disables TPM.
 * Command structure, shared out of band with the test driver running
 * on the host:
 *
 * field     |    size  |                  note
 * =========================================================================
 * text_len  |    2     | the number of random bytes to generate, big endian
 * type      |    1     | 0 = TRNG, other values reserved for extensions
 */
static enum vendor_cmd_rc trng_test(enum vendor_cmd_cc code, void *buf,
				    size_t input_size, size_t *response_size)
{
	uint16_t text_len;
	uint8_t *cmd = buf;
	uint8_t op_type = 0;

	if (input_size != sizeof(text_len) + 1) {
		*response_size = 0;
		return VENDOR_RC_BOGUS_ARGS;
	}

	text_len = be16toh(*(uint16_t *)cmd);
	op_type = cmd[sizeof(text_len)];

	if (text_len > *response_size) {
		*response_size = 0;
		return VENDOR_RC_BOGUS_ARGS;
	}

	switch (op_type) {
	case 0:
		rand_bytes(buf, text_len);
		break;
	default:
		return VENDOR_RC_BOGUS_ARGS;
	}
	*response_size = text_len;
	return VENDOR_RC_SUCCESS;
}

DECLARE_VENDOR_COMMAND(VENDOR_CC_TRNG_TEST, trng_test);
#endif /* CRYPTO_TEST_SETUP */
