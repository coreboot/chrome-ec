/* Copyright 2016 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "internal.h"

/**
 * CRYPTO_FAST_COMPARE = 1 will enable machine word reads if performance
 * is important, but will increase code size by ~100 bytes.
 */
#define CRYPTO_FAST_COMPARE 0

/* Constant time, hardened comparator. */
enum dcrypto_result __optimize("O1") DCRYPTO_equals(
	const void *a, const void *b, size_t len)
{
	uintptr_t a_addr = (uintptr_t)a;
	uintptr_t b_addr = (uintptr_t)b;
	uintptr_t tail = a_addr + len;
	uintptr_t tail_copy = value_barrier(tail);
	uintptr_t diff = 0;

#if CRYPTO_FAST_COMPARE
	/* Set 'body' to the last word boundary. */
	uintptr_t body = tail & ~WORD_MASK;

	/* End of head is the tail if data is not aligned. */
	uintptr_t head = tail;

	/* Compute starting address if src and dest can be aligned. */
	if (((a_addr & WORD_MASK) == (b_addr & WORD_MASK)) &&
	    (len >= sizeof(uintptr_t)))
		/* Set 'head' to the first word boundary. */
		head = ((a_addr + WORD_MASK) & ~WORD_MASK);

	/* Process misaligned head. */
	while (a_addr < head) {
		diff |= *((volatile uint8_t *)a_addr) ^
			*((volatile uint8_t *)b_addr);
		a_addr++;
		b_addr++;
	}

	/* Process aligned body (if any). */
	while (a_addr < body) {
		diff |= *((volatile uintptr_t *)a_addr) ^
			*((volatile uintptr_t *)b_addr);
		a_addr += sizeof(uintptr_t);
		b_addr += sizeof(uintptr_t);
	}
#endif
	/* Process remaining part. Also serves as fault resistance. */
	while (a_addr < tail) {
		diff |= *((volatile uint8_t *)a_addr) ^
			*((volatile uint8_t *)b_addr);
		a_addr++;
		b_addr++;
	}

	/**
	 *   b_addr = src2 + len
	 *   tail, a_addr = src1 + len
	 *   (src2 + len) - (src1 + len) + src1 - src2 = 0
	 *   Any other result of expression will result in wrong value.
	 *   Don't use 'src2_addr' as it is possible to make
	 *   b_addr == a_addr
	 */
	return dcrypto_ok_if_zero((value_barrier(b_addr) - tail_copy +
				   (uintptr_t)a - (uintptr_t)b) |
				  diff);
}
