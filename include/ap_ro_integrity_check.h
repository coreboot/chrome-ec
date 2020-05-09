/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */
#ifndef __CR50_INCLUDE_AP_RO_INTEGRITY_CHECK_H
#define __CR50_INCLUDE_AP_RO_INTEGRITY_CHECK_H

#include "flash_log.h"

/*
 * validate_ap_ro: based on information saved in an H1 RO flash page verify
 * contents of the AP flash.
 *
 * Returns:
 *
 *  EC_SUCCESS if valid integrity check information was found and the AP flash
 *             check succeeded.
 *  EC_ERROR_INVAL in valid integrity check information was not found.
 *  EC_ERROR_CRC if information was found, but the AP flash check failed.
 */
int validate_ap_ro(void);

/*
 * ap_ro_add_flash_event: add a flash log event to keep track of AP RO
 *       verification attempt progress.
 */
void ap_ro_add_flash_event(enum ap_ro_verification_ev event);

#endif /* ! __CR50_INCLUDE_AP_RO_INTEGRITY_CHECK_H */
