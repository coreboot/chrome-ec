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
#define BLOCKED_BID_COUNT 70
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
	0x454b574c, /* EKWL */
	/* b/283786298 */
	0x41475659, /* AGVY */
	0x414c4454, /* ALDT */
	0x41544645, /* ATFE */
	0x4244584a, /* BDXJ */
	0x424a4a4a, /* BJJJ */
	0x434c5159, /* CLQY */
	0x434d4c50, /* CMLP */
	0x44434643, /* DCFC */
	0x444a4449, /* DJDI */
	0x45415349, /* EASI */
	0x454a574c, /* EJWL */
	0x46514d4f, /* FQMO */
	0x46584d47, /* FXMG */
	0x47484244, /* GHBD */
	0x47485851, /* GHXQ */
	0x474f4b45, /* GOKE */
	0x484b5653, /* HKVS */
	0x48555547, /* HUUG */
	0x49535653, /* ISVS */
	0x4a415851, /* JAXQ */
	0x4b414345, /* KACE */
	0x4b4e5549, /* KNUI */
	0x4b4f4b46, /* KOKF */
	0x4b575648, /* KWVH */
	0x4b594c41, /* KYLA */
	0x4c42594b, /* LBYK */
	0x4c47455a, /* LGEZ */
	0x4c504444, /* LPDD */
	0x4c505850, /* LPXP */
	0x4e4a4f53, /* NJOS */
	0x4f574445, /* OWDE */
	0x50475255, /* PGRU */
	0x5047584d, /* PGXM */
	0x50534855, /* PSHU */
	0x50564849, /* PVHI */
	0x50584c53, /* PXLS */
	0x505a4e46, /* PZNF */
	0x514b474c, /* QKGL */
	0x51534851, /* QSHQ */
	0x51594642, /* QYFB */
	0x52545457, /* RTTW */
	0x5256504a, /* RVPJ */
	0x5356475a, /* SVGZ */
	0x544a4654, /* TJFT */
	0x54555250, /* TURP */
	0x54594f4f, /* TYOO */
	0x5541545a, /* UATZ */
	0x55484d54, /* UHMT */
	0x55494c43, /* UILC */
	0x554c4541, /* ULEA */
	0x55524946, /* URIF */
	0x555a5451, /* UZTQ */
	0x564a5855, /* VJXU */
	0x56504e44, /* VPND */
	0x574f414a, /* WOAJ */
	0x584c5446, /* XLTF */
	0x58585044, /* XXPD */
	0x58585a42, /* XXZB */
	0x59445648, /* YDVH */
	0x59534a53, /* YSJS */
	0x59564a42, /* YVJB */
	0x59594a50, /* YYJP */
	0x5a504953  /* ZPIS */
};
BUILD_ASSERT(ARRAY_SIZE(ap_ro_board_id_blocklist) == BLOCKED_BID_COUNT);

#endif   /* ! __EC_BOARD_CR50_AP_RO_BOARD_ID_BLOCKLIST_H */
