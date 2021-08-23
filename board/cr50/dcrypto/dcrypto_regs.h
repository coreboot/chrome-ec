/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef __EC_FIPS_MODULE_REGS_H
#define __EC_FIPS_MODULE_REGS_H

/**
 * This header file contains H1 crypto device register tables defined
 * as structs. This allows more efficient code generation compared to
 * using GREG() macro. The root cause is that with GREG compiler can't
 * always deduce that next register address can be calculated from
 * previous one already loaded in register by adding a small constant,
 * thus producing inefficient code to load address first and spill
 * registers. Access made as struct like:
 *
 * static volatile struct keymgr_sha *reg_keymgr_sha =
 *      (void *)(GC_KEYMGR_BASE_ADDR + GC_KEYMGR_SHA_CFG_MSGLEN_LO_OFFSET);
 *
 * reg_keymgr_sha->itop = 0;
 * reg_keymgr_sha->trig = GC_KEYMGR_SHA_TRIG_TRIG_RESET_MASK;
 *
 * becomes more compact and efficient.
 */
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "registers.h"



/**
 * SHA/HMAC part of KEYMGR starting offset 0x400
 */
struct keymgr_sha {
	uint32_t msglen_lo; /* KEYMGR_SHA_CFG_MSGLEN_LO 0x400 */
	uint32_t msglen_hi; /* KEYMGR_SHA_CFG_MSGLEN_HI 0x404 */
	uint32_t cfg_en; /* KEYMGR_SHA_CFG_EN 0x408 */
	uint32_t wr_en; /* KEYMGR_SHA_CFG_WR_EN 0x40c */
	uint32_t trig; /* KEYMGR_SHA_TRIG 0x410 */
	const uint32_t _pad1[11];
	union {
		uint32_t fifo_u32; /* KEYMGR_SHA_INPUT_FIFO 0x440 */
		uint8_t fifo_u8; /* KEYMGR_SHA_INPUT_FIFO 0x440 */
	};

	uint32_t h[8]; /* KEYMGR_SHA_STS_H0 .. H7 */
	uint32_t key[8]; /* KEYMGR_SHA_KEY_W0 .. W7 */
	const uint32_t sts; /* KEYMGR_SHA_STS */
	const uint32_t itcr; /* KEYMGR_SHA_ITCR */
	uint32_t itop; /* KEYMGR_SHA_ITOP */
	uint32_t use_hidden_key; /* KEYMGR_SHA_USE_HIDDEN_KEY */
	uint32_t use_cert; /* KEYMGR_SHA_USE_CERT */
	uint32_t cert_override; /* KEYMGR_SHA_CERT_OVERRIDE */
	uint32_t rand_stall; /* KEYMGR_SHA_RAND_STALL_CTL */
	uint32_t count_state; /* KEYMGR_SHA_EXECUTE_COUNT_STATE */
	uint32_t count_max; /* KEYMGR_SHA_EXECUTE_COUNT_MAX */
	uint32_t revoke_ctrl[3]; /* KEYMGR_CERT_REVOKE_CTRL0 .. CTRL3 */
};

BUILD_ASSERT(offsetof(struct keymgr_sha, trig) ==
	     GC_KEYMGR_SHA_TRIG_OFFSET - GC_KEYMGR_SHA_CFG_MSGLEN_LO_OFFSET);

BUILD_ASSERT(offsetof(struct keymgr_sha, fifo_u32) ==
	     GC_KEYMGR_SHA_INPUT_FIFO_OFFSET -
		     GC_KEYMGR_SHA_CFG_MSGLEN_LO_OFFSET);

BUILD_ASSERT(offsetof(struct keymgr_sha, h) ==
	     GC_KEYMGR_SHA_STS_H0_OFFSET - GC_KEYMGR_SHA_CFG_MSGLEN_LO_OFFSET);

BUILD_ASSERT(offsetof(struct keymgr_sha, rand_stall) ==
	     GC_KEYMGR_SHA_RAND_STALL_CTL_OFFSET -
		     GC_KEYMGR_SHA_CFG_MSGLEN_LO_OFFSET);

#ifdef __cplusplus
}
#endif

#endif /* __EC_FIPS_MODULE_REGS_H */

