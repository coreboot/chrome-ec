/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * MKBP keyboard protocol
 */

#ifndef __CROS_EC_KEYBOARD_MKBP_H
#define __CROS_EC_KEYBOARD_MKBP_H

#include "common.h"
#include "keyboard_config.h"

#define MKBP_BUTTON_COOKIE "bTnZ" /* Buttons! */
#define MKBP_BUTTON_COOKIE_LEN 4
#define MKBP_SWITCH_COOKIE "SwTz" /* Switches! */
#define MKBP_SWITCH_COOKIE_LEN 4
/*
 * These cookies contain enough 1s in the first 4 columns so that they shouldn't
 * be a valid keypress combination.
 */

struct mkbp_btn_data {
	char cookie[MKBP_BUTTON_COOKIE_LEN]; /* Must be MKBP_BUTTON_COOKIE */

	/* State of non-matrixed buttons. */
	uint32_t state;
} __packed;

struct mkbp_sw_data {
	char cookie[MKBP_SWITCH_COOKIE_LEN]; /* Must be MKBP_SWITCH_COOKIE */

	/* State of switches. */
	uint32_t state;
} __packed;

union kb_fifo_data {
	uint8_t key_matrix[KEYBOARD_COLS];

	struct mkbp_btn_data b_data;

	struct mkbp_sw_data sw_data;
} __packed;

#define POWER_BUTTON	0
#define VOL_UP		1
#define VOL_DOWN	2

/**
 * Add keyboard state into FIFO
 *
 * @return EC_SUCCESS if entry added, EC_ERROR_OVERFLOW if FIFO is full
 */
int keyboard_fifo_add(const uint8_t *buffp);

/**
 * Send KEY_BATTERY keystroke.
 */
#ifdef CONFIG_KEYBOARD_PROTOCOL_MKBP
void keyboard_send_battery_key(void);
#else
static inline void keyboard_send_battery_key(void) { }
#endif

#endif  /* __CROS_EC_KEYBOARD_MKBP_H */
