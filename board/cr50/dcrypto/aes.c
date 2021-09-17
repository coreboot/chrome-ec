/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "dcrypto.h"
#include "internal.h"
#include "registers.h"

static void set_control_register(
	unsigned mode, unsigned key_size, unsigned encrypt)
{
	GWRITE_FIELD(KEYMGR, AES_CTRL, RESET, CTRL_NO_SOFT_RESET);
	GWRITE_FIELD(KEYMGR, AES_CTRL, KEYSIZE, key_size);
	GWRITE_FIELD(KEYMGR, AES_CTRL, CIPHER_MODE, mode);
	GWRITE_FIELD(KEYMGR, AES_CTRL, ENC_MODE, encrypt);
	GWRITE_FIELD(KEYMGR, AES_CTRL, CTR_ENDIAN, CTRL_CTR_BIG_ENDIAN);
	GWRITE_FIELD(KEYMGR, AES_CTRL, ENABLE, CTRL_ENABLE);

	/* Turn off random nops (which are enabled by default). */
	GWRITE_FIELD(KEYMGR, AES_RAND_STALL_CTL, STALL_EN, 0);
	/* Configure random nop percentage at 25%. */
	GWRITE_FIELD(KEYMGR, AES_RAND_STALL_CTL, FREQ, 1);
	/* Now turn on random nops. */
	GWRITE_FIELD(KEYMGR, AES_RAND_STALL_CTL, STALL_EN, 1);
}

static int wait_read_data(volatile uint32_t *addr)
{
	int empty;
	int count = 20;     /* Wait these many ~cycles. */

	do {
		empty = REG32(addr);
		count--;
	} while (count && empty);

	return empty ? 0 : 1;
}

int DCRYPTO_aes_init(const uint8_t *key, uint32_t key_len, const uint8_t *iv,
		enum cipher_mode c_mode, enum encrypt_mode e_mode)
{
	int i;
	const struct access_helper *p;
	uint32_t key_mode;

	switch (key_len) {
	case 128:
		key_mode = 0;
		break;
	case 192:
		key_mode = 1;
		break;
	case 256:
		key_mode = 2;
		break;
	default:
		/* Invalid key length specified. */
		return 0;
	}
	set_control_register(c_mode, key_mode, e_mode);

	/* Initialize hardware with AES key */
	p = (struct access_helper *) key;
	for (i = 0; i < (key_len >> 5); i++)
		GR_KEYMGR_AES_KEY(i) = p[i].udata;
	/* Trigger key expansion. */
	GREG32(KEYMGR, AES_KEY_START) = 1;

	/* Wait for key expansion. */
	if (!wait_read_data(GREG32_ADDR(KEYMGR, AES_KEY_START))) {
		/* Should not happen. */
		return 0;
	}

	/* Initialize IV for modes that require it. */
	if (iv)
		DCRYPTO_aes_write_iv(iv);

	return 1;
}

int DCRYPTO_aes_block(const uint8_t *in, uint8_t *out)
{
	int i;
	uint32_t buf[4];
	const uint32_t *inw;
	uint32_t *outw;

	if (is_not_aligned(in)) {
		memcpy(buf, in, sizeof(buf));
		inw = buf;
	} else
		inw = (const uint32_t *)in;

	/* Write plaintext. */
	for (i = 0; i < 4; i++)
		GREG32(KEYMGR, AES_WFIFO_DATA) = inw[i];

	/* Wait for the result. */
	if (!wait_read_data(GREG32_ADDR(KEYMGR, AES_RFIFO_EMPTY))) {
		/* Should not happen, ciphertext not ready. */
		return 0;
	}

	/* Read ciphertext. */
	if (is_not_aligned(out))
		outw = buf;
	else
		outw = (uint32_t *)out;

	for (i = 0; i < 4; i++)
		outw[i] = GREG32(KEYMGR, AES_RFIFO_DATA);

	if (out != (uint8_t *)outw)
		memcpy(out, outw, sizeof(buf));

	return 1;
}

void DCRYPTO_aes_write_iv(const uint8_t *iv)
{
	int i;
	uint32_t buf[4];
	const uint32_t *ivw;

	if (is_not_aligned(iv)) {
		memcpy(buf, iv, sizeof(buf));
		ivw = buf;
	} else
		ivw = (uint32_t *)iv;

	for (i = 0; i < 4; i++)
		GR_KEYMGR_AES_CTR(i) = ivw[i];
}

void DCRYPTO_aes_read_iv(uint8_t *iv)
{
	int i;
	uint32_t buf[4];
	uint32_t *ivw;

	if (is_not_aligned(iv))
		ivw = buf;
	else
		ivw = (uint32_t *)iv;

	for (i = 0; i < 4; i++)
		ivw[i] = GR_KEYMGR_AES_CTR(i);

	if (iv != (uint8_t *)ivw)
		memcpy(iv, ivw, sizeof(buf));
}

int DCRYPTO_aes_ctr(uint8_t *out, const uint8_t *key, uint32_t key_bits,
		const uint8_t *iv, const uint8_t *in, size_t in_len)
{
	/* Initialize AES hardware. */
	if (!DCRYPTO_aes_init(key, key_bits, iv,
				CIPHER_MODE_CTR, ENCRYPT_MODE))
		return 0;

	while (in_len > 0) {
		uint32_t tmpin[4];
		uint32_t tmpout[4];
		const uint8_t *inp;
		uint8_t *outp;
		const size_t count = MIN(in_len, 16);

		if (count < 16) {
			memcpy(tmpin, in, count);
			inp = (uint8_t *)tmpin;
			outp = (uint8_t *)tmpout;
		} else {
			inp = in;
			outp = out;
		}
		if (!DCRYPTO_aes_block(inp, outp))
			return 0;
		if (outp != out)
			memcpy(out, outp, count);

		in += count;
		out += count;
		in_len -= count;
	}
	return 1;
}
