/* Copyright 2015 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "internal.h"
#include "dcrypto_regs.h"
/**
 * Define KEYMGR AES access structure.
 */
static volatile struct keymgr_aes *reg_aes = (void *)(GC_KEYMGR_BASE_ADDR);

static void set_control_register(enum cipher_mode mode, uint32_t key_size,
				 enum encrypt_mode encrypt)
{
	/* Make sure we don't use key coming from keyladder. */
	reg_aes->use_hidden_key = GC_KEYMGR_AES_USE_HIDDEN_KEY_ENABLE_DEFAULT;
	/* Bring AES engine FIFO into known state. */
	reg_aes->ctrl = GC_KEYMGR_AES_CTRL_RESET_MASK;
	reg_aes->ctrl =
		(CTRL_NO_SOFT_RESET << GC_KEYMGR_AES_CTRL_RESET_LSB) |
		(key_size << GC_KEYMGR_AES_CTRL_KEYSIZE_LSB) |
		(mode << GC_KEYMGR_AES_CTRL_CIPHER_MODE_LSB) |
		(encrypt << GC_KEYMGR_AES_CTRL_ENC_MODE_LSB) |
		(CTRL_CTR_BIG_ENDIAN << GC_KEYMGR_AES_CTRL_CTR_ENDIAN_LSB) |
		(CTRL_ENABLE << GC_KEYMGR_AES_CTRL_ENABLE_LSB);

	/* Turn off random nops (which are enabled by default). */
	reg_aes->rand_stall = 0;
	/* Configure random nop percentage at 25%, turn on random nops.  */
	reg_aes->rand_stall = (1 << GC_KEYMGR_AES_RAND_STALL_CTL_FREQ_LSB) |
			      GC_KEYMGR_AES_RAND_STALL_CTL_STALL_EN_MASK;
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

enum dcrypto_result dcrypto_aes_init(const uint8_t *key, size_t key_len,
				     const uint8_t *iv, enum cipher_mode c_mode,
				     enum encrypt_mode e_mode)
{
	size_t i;
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
		return DCRYPTO_FAIL;
	}

	switch (c_mode) {
	case CIPHER_MODE_ECB:
	case CIPHER_MODE_CTR:
	case CIPHER_MODE_CBC:
		set_control_register(c_mode, key_mode, e_mode);
		break;
	default:
		/* Invalid mode specified. */
		return DCRYPTO_FAIL;
	}

	/* Initialize hardware with AES key */
	p = (struct access_helper *) key;
	for (i = 0; i < (key_len >> 5); i++)
		reg_aes->key[i] = p[i].udata;
	/* Trigger key expansion. */
	reg_aes->key_start = 1;

	/* Wait for key expansion. */
	if (!wait_read_data(&reg_aes->key_start)) {
		/* Should not happen. */
		return DCRYPTO_FAIL;
	}

	/* Initialize IV for modes that require it. */
	if (iv)
		DCRYPTO_aes_write_iv(iv);

	return DCRYPTO_OK;
}

enum dcrypto_result DCRYPTO_aes_init(const uint8_t *key, size_t key_len,
				     const uint8_t *iv, enum cipher_mode c_mode,
				     enum encrypt_mode e_mode)
{
	if (!fips_crypto_allowed())
		return DCRYPTO_FAIL;
	return dcrypto_aes_init(key, key_len, iv, c_mode, e_mode);
}

enum dcrypto_result DCRYPTO_aes_block(const uint8_t *in, uint8_t *out)
{
	uint32_t buf[4];
	const uint32_t *inw;
	uint32_t *outw, *outw2;

	if (is_not_aligned(in)) {
		memcpy(buf, in, sizeof(buf));
		inw = buf;
	} else
		inw = (const uint32_t *)in;

	if (is_not_aligned(out))
		outw = buf;
	else
		outw = (uint32_t *)out;

	outw2 = outw;
	dcrypto_aes_process(&outw, &inw, 16);

	if (out != (uint8_t *)outw2)
		memcpy(out, outw2, sizeof(buf));

	return DCRYPTO_OK;
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
		reg_aes->counter[i] = ivw[i];
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
		ivw[i] = reg_aes->counter[i];

	if (iv != (uint8_t *)ivw)
		memcpy(iv, ivw, sizeof(buf));
}

enum dcrypto_result DCRYPTO_aes_ctr(uint8_t *out, const uint8_t *key,
				    uint32_t key_bits, const uint8_t *iv,
				    const uint8_t *in, size_t in_len)
{
	/* Initialize AES hardware. */
	if (DCRYPTO_aes_init(key, key_bits, iv, CIPHER_MODE_CTR,
			     ENCRYPT_MODE) != DCRYPTO_OK)
		return DCRYPTO_FAIL;

	while (in_len > 0) {
		uint32_t tmpin[4];
		uint32_t tmpout[4];
		const uint8_t *inp;
		uint8_t *outp;
		const size_t count = MIN(in_len, 16U);

		if (count < 16) {
			memcpy(tmpin, in, count);
			inp = (uint8_t *)tmpin;
			outp = (uint8_t *)tmpout;
		} else {
			inp = in;
			outp = out;
		}
		if (DCRYPTO_aes_block(inp, outp) != DCRYPTO_OK)
			return DCRYPTO_FAIL;
		if (outp != out)
			memcpy(out, outp, count);

		in += count;
		out += count;
		in_len -= count;
	}
	return DCRYPTO_OK;
}
