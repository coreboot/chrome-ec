/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * The functions implemented by keyboard component of EC core.
 */

#ifndef __CROS_EC_KEYBOARD_8042_H
#define __CROS_EC_KEYBOARD_8042_H

#include <stddef.h>

#include "button.h"
#include "common.h"

/**
 * Called by power button handler and button interrupt handler.
 *
 * This function sends the corresponding make or break code to the host.
 */
void button_state_changed(enum keyboard_button_type button, int is_pressed);

/**
 * Notify the keyboard module when a byte is written by the host.
 *
 * Note: This is called in interrupt context by the LPC interrupt handler.
 *
 * @param data		Byte written by host
 * @param is_cmd        Is byte command (!=0) or data (0)
 */
void keyboard_host_write(int data, int is_cmd);


/* Utilities for scan code and make code. */

enum scancode_set_list {
	SCANCODE_GET_SET = 0,
	SCANCODE_SET_1,
	SCANCODE_SET_2,
	SCANCODE_SET_3,
	SCANCODE_MAX = SCANCODE_SET_3,
};

struct makecode_entry {
	uint16_t set1, set2;
};

struct makecode_translate_entry {
	struct makecode_entry from, to;
};

/**
 * Checks if a given make code matches given record.
 *
 * Returns 1 if matches.
 */
int makecode_match(uint16_t make_code, enum scancode_set_list code_set,
		   const struct makecode_entry *entry);

/**
 * Translate a make code to different value.
 *
 * @param make_code	The value of make_code.
 * @param code_set	The code set of make code.
 * @param entries	Pointer to array of struct makecode_translate_entry
 * @param count		Number of elements in entries
 */
uint16_t makecode_translate(
		uint16_t make_code, enum scancode_set_list code_set,
		const struct makecode_translate_entry *entries, size_t count);

/**
 * Returns a board-specific translated make code.
 *
 * @param make_code	8042 make code
 * @param pressed	Whether the key was pressed
 * @param code_set	8042 scan code set
 */
uint16_t keyboard_board_translate(
		uint16_t make_code, int8_t pressed,
		enum scancode_set_list code_set);

#endif  /* __CROS_EC_KEYBOARD_8042_H */
