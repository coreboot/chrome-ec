#ifndef __CR50_INCLUDE_B64_H
#define __CR50_INCLUDE_B64_H
/* Copyright 2022 The Chromium OS Authors.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include "util.h"

/*
 * A function to print base64 encoding of a binary blob on the console.
 *
 * For testing purposed an alternative output function could be supplied.
 *
 * @param data - pointer to data to encode.
 * @param size - size of the data area
 * @param func - optional, function to pass to the characters to print. If
 *               NULL the output is printed on the console.
 */
void base64_encode_to_console(const uint8_t *data, size_t size,
			      void (*func)(char c));

#endif /* ! __CR50_INCLUDE_B64_H */
