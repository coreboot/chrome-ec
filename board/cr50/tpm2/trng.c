/* Copyright 2016 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "CryptoEngine.h"

CRYPT_RESULT _cpri__StirRandom(int32_t num, uint8_t *entropy)
{
	return CRYPT_SUCCESS;     /* NO-OP on CR50. */
}
