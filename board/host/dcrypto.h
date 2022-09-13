/* Copyright 2018 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Provides the minimal declarations needed by pinweaver and u2f to build on
 * CHIP_HOST. While it might be preferable to simply use the original dcrypto.h,
 * That would require incorporating additional headers / dependencies such as
 * cryptoc.
 */

#ifndef __CROS_EC_DCRYPTO_HOST_H
#define __CROS_EC_DCRYPTO_HOST_H
#include <stdint.h>
#include <string.h>

/* Allow tests to return a faked result for the purpose of testing. If
 * this is not set, a combination of cryptoc and openssl are used for the
 * dcrypto implementation.
 */

#ifndef CONFIG_DCRYPTO_MOCK
#include "board/cr50/dcrypto/dcrypto.h"
#else
#include "board/cr50/dcrypto/internal.h"
#endif


#endif  /* __CROS_EC_HOST_DCRYPTO_H */
