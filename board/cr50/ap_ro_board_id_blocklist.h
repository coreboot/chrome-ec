/*
 * Copyright 2023 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "compile_time_macros.h"
#include "stdint.h"

#ifndef __EC_BOARD_CR50_AP_RO_BOARD_ID_BLOCKLIST_H
#define __EC_BOARD_CR50_AP_RO_BOARD_ID_BLOCKLIST_H

/*
 * Certain boards need to skip AP RO verification even when the hash is saved.
 * Block AP RO verification based on the board id type.
 */
#define BLOCKED_BID_COUNT 7
/*
 * This contains the ap ro verification board id blocklist. Skip AP RO
 * verification if the board id is found in the blocklist.
 */
const uint32_t ap_ro_board_id_blocklist[] = {
	/* b/185783841 block verification on unsupported devices. */
	0x54514155, /* TQAU */
	0x524c4745, /* RLGE */
	0x56595243, /* VYRC */
	0x44554b49, /* DUKI */
	0x4346554c, /* CFUL */
	0x5248444e, /* RHDN */
	0x454b574c  /* EKWL */
};
BUILD_ASSERT(ARRAY_SIZE(ap_ro_board_id_blocklist) == BLOCKED_BID_COUNT);

#endif   /* ! __EC_BOARD_CR50_AP_RO_BOARD_ID_BLOCKLIST_H */
