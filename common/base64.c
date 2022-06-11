/* Copyright 2022 The Chromium OS Authors.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Yet another from scratch implementation of base64 encoding. */

#include "base64.h"
#include "common.h"
#include "console.h"

/*
 * Translation Table as described in RFC1113
 */
static const char cb64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Number of bytes converted into the output block of characters. */
#define BASE64_BYTE_SIZE 3

/* Number of characters in a single encoded block. */
#define BASE64_CHAR_SIZE 4

/*
 * encodeblock
 *
 * encode up to 3 bytes 6-bit characters, pad output with '=' if input is
 * shorter than 3 bytes.
 *
 * @param in: pointer to the input bytes
 * @param out: pointer to the output, guaranteed to be 4 bytes long.
 * @param len: number of input bytes to process, guaranteed to be in 1..3
 *              range.
 */
static void encodeblock(const uint8_t *in, uint8_t *out, size_t len)
{
	uint32_t input;
	int i;
	int limit;

	/* Place input bytes into a 32 bit value in big endian format. */
	input = 0;
	for (i = 0; i < len; i++)
		((uint8_t *)&input)[3 - i] = in[i];

	/*
	 * Map each 6 bit quantity of the input into an output character, make
	 * sure partial bit quantities are mapped too in case input is shorter
	 * than 3 bytes.
	 */
	limit = ((len * 8) + 7) / 6;
	for (i = 0; i < limit; i++) {
		int index = (input >> (26 - i * 6)) & 0x3f;

		out[i] = cb64[index];
	}

	while (i < BASE64_CHAR_SIZE)
		out[i++] = '=';
}

/* Pass one output character to the caller. */
static void printit(char c, void (*f)(char c))
{
	if (f)
		f(c);
	else
		ccprintf("%c", c);
}

#define LINE_SIZE 64
void base64_encode_to_console(const uint8_t *data, size_t size,
			      void (*func)(char c))
{
	uint8_t out[4];
	int blocksout;

	blocksout = 0;
	while (size) {
		int i;
		int in_length;

		if (size > BASE64_BYTE_SIZE)
			in_length = BASE64_BYTE_SIZE;
		else
			in_length = size;

		encodeblock(data, out, in_length);
		for (i = 0; i < sizeof(out); i++)
			printit(out[i], func);

		blocksout++;
		data += in_length;
		size -= in_length;
		if (blocksout >= (LINE_SIZE / BASE64_CHAR_SIZE)) {
			printit('\n', func);
			blocksout = 0;
			if (!func)
				cflush();
		}
	}
	if (blocksout)
		printit('\n', func);
}
