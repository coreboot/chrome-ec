/* Copyright 2021 The ChromiumOS Authors
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
 * AES/GCM part of KEYMGR starting offset 0x000
 */
struct keymgr_aes {
	uint32_t ctrl; /* KEYMGR_AES_CTRL */
	const uint32_t _pad0;
	uint32_t wfifo_data; /*KEYMGR_AES_WFIFO_DATA */
	const uint32_t rfifo_data; /* KEYMGR_AES_RFIFO_DATA */
	const uint32_t _pad1[7];
	uint32_t key[8]; /* KEYMGR_AES_KEY0 .. 7 */
	uint32_t key_start; /* KEYMGR_AES_KEY_START */
	uint32_t counter[4]; /* KEYMGR_AES_CTR0 .. 3 */
	uint32_t rand_stall; /* KEYMGR_AES_RAND_STALL_CTL */
	uint32_t wfifo_level; /* KEYMGR_AES_WFIFO_LEVEL */
	uint32_t wfifo_full; /* KEYMGR_AES_WFIFO_FULL */
	uint32_t rfifo_level; /* KEYMGR_AES_RFIFO_LEVEL */
	uint32_t rfifo_empty; /* KEYMGR_AES_RFIFO_EMPTY */
	const uint32_t execute_count_state; /* KEYMGR_AES_EXECUTE_COUNT_STATE */
	uint32_t execute_count_max; /* KEYMGR_AES_EXECUTE_COUNT_MAX */
	uint32_t gcm_do_acc; /* KEYMGR_GCM_DO_ACC */
	uint32_t gcm_h[4]; /* KEYMGR_GCM_H0 .. 3 */
	uint32_t gcm_mac[4]; /* KEYMGR_GCM_MAC0 .. 3 */
	uint32_t gcm_hash_in[4]; /* KEYMGR_GCM_HASH_IN0 .. 3 */
	uint32_t wipe_secrets; /* KEYMGR_AES_WIPE_SECRETS */
	uint32_t int_enable; /* KEYMGR_AES_INT_ENABLE */
	uint32_t int_state; /* KEYMGR_AES_INT_STATE */
	uint32_t int_test; /* KEYMGR_AES_INT_TEST */
	uint32_t use_hidden_key; /* KEYMGR_AES_USE_HIDDEN_KEY */
};
BUILD_ASSERT(offsetof(struct keymgr_aes, wfifo_data) ==
	     GC_KEYMGR_AES_WFIFO_DATA_OFFSET);
BUILD_ASSERT(offsetof(struct keymgr_aes, key) == GC_KEYMGR_AES_KEY0_OFFSET);
BUILD_ASSERT(offsetof(struct keymgr_aes, counter) == GC_KEYMGR_AES_CTR0_OFFSET);
BUILD_ASSERT(offsetof(struct keymgr_aes, gcm_h) == GC_KEYMGR_GCM_H0_OFFSET);
BUILD_ASSERT(offsetof(struct keymgr_aes, use_hidden_key) ==
	     GC_KEYMGR_AES_USE_HIDDEN_KEY_OFFSET);

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

struct trng_reg {
	const uint32_t version; /* TRNG_VERSION = 0x2d013316 */
	uint32_t int_enable; /* TRNG_INT_ENABLE */
	uint32_t int_state; /* TRNG_INT_STATE */
	uint32_t int_test; /* TRNG_INT_TEST */
	uint32_t secure_post_processing; /* TRNG_SECURE_POST_PROCESSING_CTRL */
	uint32_t post_processing; /* TRNG_POST_PROCESSING_CTRL */
	uint32_t go_event; /* TRNG_GO_EVENT */
	uint32_t timeout_counter; /* TRNG_TIMEOUT_COUNTER */
	uint32_t timeout_max_try; /* TRNG_TIMEOUT_MAX_TRY_NUM */
	uint32_t output_time; /* TRNG_OUTPUT_TIME_COUNTER */
	uint32_t stop_work; /* TRNG_STOP_WORK */
	const uint32_t fsm_state; /* TRNG_FSM_STATE */
	uint32_t allowed_values; /* TRNG_ALLOWED_VALUES */
	const uint32_t timer_counter; /* TRNG_TIMER_COUNTER */
	uint32_t slice_max_upper_limit; /* TRNG_SLICE_MAX_UPPER_LIMIT */
	uint32_t slice_min_lower_limit; /* TRNG_SLICE_MIN_LOWER_LIMIT */
	const uint32_t max_value; /* TRNG_MAX_VALUE */
	const uint32_t min_value; /* TRNG_MIN_VALUE */
	uint32_t ldo_ctrl; /* TRNG_LDO_CTRL */
	uint32_t power_down_b; /* TRNG_POWER_DOWN_B */
	uint32_t proc_lock_power_down_b; /* TRNG_PROC_LOCK_POWER_DOWN_B */
	uint32_t antest; /* TRNG_ANTEST */
	uint32_t analog_sen_lsr_in; /* TRNG_ANALOG_SEN_LSR_INPUT */
	const uint32_t analog_sen_lsr_out; /* TRNG_ANALOG_SEN_LSR_OUTPUT */
	uint32_t analog_test; /* TRNG_ANALOG_TEST */
	uint32_t analog_control; /* TRNG_ANALOG_CTRL */
	uint32_t one_shot_mode; /* TRNG_ONE_SHOT_MODE */
	uint32_t one_shot_reg; /* TRNG_ONE_SHOT_REG */
	const uint32_t read_data; /* TRNG_READ_DATA */
	const uint32_t frequency_calls; /* TRNG_FREQUENCY_CALLS */
	const uint32_t num_ones; /* TRNG_CUR_NUM_ONES */
	const uint32_t empty; /* TRNG_EMPTY */
};

BUILD_ASSERT(offsetof(struct trng_reg, go_event) ==
	     GC_TRNG_GO_EVENT_OFFSET - GC_TRNG_VERSION_CHANGE_OFFSET);

BUILD_ASSERT(offsetof(struct trng_reg, read_data) ==
	     GC_TRNG_READ_DATA_OFFSET - GC_TRNG_VERSION_CHANGE_OFFSET);

#ifdef __cplusplus
}
#endif

#endif /* __EC_FIPS_MODULE_REGS_H */
