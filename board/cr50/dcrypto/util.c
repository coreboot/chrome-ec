/* Copyright 2021 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Utility functions for Chrome EC within the FIPS boundary */

#include "common.h"
#include "console.h"
#include "util.h"

__stdlib_compat size_t strlen(const char *s)
{
	int len = 0;

	while (*s++)
		len++;

	return len;
}


__stdlib_compat int memcmp(const void *s1, const void *s2, size_t len)
{
	const char *sa = s1;
	const char *sb = s2;

	int diff = 0;

	while (len-- > 0) {
		diff = *(sa++) - *(sb++);
		if (diff)
			return diff;
	}

	return 0;
}


__stdlib_compat void *memcpy(void *dest, const void *src, size_t len)
{
	char *d = (char *)dest;
	const char *s = (const char *)src;
	uint32_t *dw;
	const uint32_t *sw;
	char *head;
	char * const tail = (char *)dest + len;
	/* Set 'body' to the last word boundary */
	uint32_t * const body = (uint32_t *)((uintptr_t)tail & ~3);

	if (((uintptr_t)dest & 3) != ((uintptr_t)src & 3)) {
		/* Misaligned. no body, no tail. */
		head = tail;
	} else {
		/* Aligned */
		if ((uintptr_t)tail < (((uintptr_t)d + 3) & ~3))
			/* len is shorter than the first word boundary */
			head = tail;
		else
			/* Set 'head' to the first word boundary */
			head = (char *)(((uintptr_t)d + 3) & ~3);
	}

	/* Copy head */
	while (d < head)
		*(d++) = *(s++);

	/* Copy body */
	dw = (uint32_t *)d;
	sw = (uint32_t *)s;
	while (dw < body)
		*(dw++) = *(sw++);

	/* Copy tail */
	d = (char *)dw;
	s = (const char *)sw;
	while (d < tail)
		*(d++) = *(s++);

	return dest;
}

static void *memset_core(volatile void *dest, int c, size_t len)
{
	volatile char *d = (volatile char *)dest;
	uint32_t cccc;
	volatile uint32_t *dw;
	char *head;
	char * const tail = (char *)dest + len;
	/* Set 'body' to the last word boundary */
	uint32_t * const body = (uint32_t *)((uintptr_t)tail & ~3);

	c &= 0xff;	/* Clear upper bits before ORing below */
	cccc = c | (c << 8) | (c << 16) | (c << 24);

	if ((uintptr_t)tail < (((uintptr_t)d + 3) & ~3))
		/* len is shorter than the first word boundary */
		head = tail;
	else
		/* Set 'head' to the first word boundary */
		head = (char *)(((uintptr_t)d + 3) & ~3);

	/* Copy head */
	while (d < head)
		*(d++) = c;

	/* Copy body */
	dw = (volatile uint32_t *)d;
	while (dw < body)
		*(dw++) = cccc;

	/* Copy tail */
	d = (volatile char *)dw;
	while (d < tail)
		*(d++) = c;

	return (void *)dest;
}

__stdlib_compat __visible void *memset(void *dest, int c, size_t len)
{
	return memset_core(dest, c, len);
}

__stdlib_compat void *memmove(void *dest, const void *src, size_t len)
{
	if ((uintptr_t)dest <= (uintptr_t)src ||
	    (uintptr_t)dest >= (uintptr_t)src + len) {
		/*
		 * Start of destination doesn't overlap source, so just use
		 * memcpy().
		 */
		return memcpy(dest, src, len);
	}

	{
		/* Need to copy from tail because there is overlap. */
		char *d = (char *)dest + len;
		const char *s = (const char *)src + len;
		uint32_t *dw;
		const uint32_t *sw;
		char *head;
		char * const tail = (char *)dest;
		/* Set 'body' to the last word boundary */
		uint32_t * const body = (uint32_t *)(((uintptr_t)tail+3) & ~3);

		if (((uintptr_t)dest & 3) != ((uintptr_t)src & 3)) {
			/* Misaligned. no body, no tail. */
			head = tail;
		} else {
			/* Aligned */
			if ((uintptr_t)tail > ((uintptr_t)d & ~3))
				/* Shorter than the first word boundary */
				head = tail;
			else
				/* Set 'head' to the first word boundary */
				head = (char *)((uintptr_t)d & ~3);
		}

		/* Copy head */
		while (d > head)
			*(--d) = *(--s);

		/* Copy body */
		dw = (uint32_t *)d;
		sw = (uint32_t *)s;
		while (dw > body)
			*(--dw) = *(--sw);

		/* Copy tail */
		d = (char *)dw;
		s = (const char *)sw;
		while (d > tail)
			*(--d) = *(--s);

		return dest;
	}
}


void reverse(void *dest, size_t len)
{
	size_t i;
	uint8_t *start = dest;
	uint8_t *end = start + len;

	for (i = 0; i < len / 2; ++i) {
		uint8_t tmp = *start;

		*start++ = *--end;
		*end = tmp;
	}
}


__stdlib_compat int strncmp(const char *s1, const char *s2, size_t n)
{
	while (n--) {
		if (*s1 != *s2)
			return *s1 - *s2;
		if (!*s1)
			break;
		s1++;
		s2++;

	}
	return 0;
}

void *always_memset(void *s, int c, size_t n)
{
	return memset_core(s, c, n);
}
