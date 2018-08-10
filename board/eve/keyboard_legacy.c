/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Eve board-specific keyboard configuration for legacy mode */

#include "board_config.h"
#include "chipset.h"
#include "keyboard_8042.h"
#include "keyboard_8042_sharedlib.h"
#include "keyboard_protocol.h"
#include "util.h"

/* Scan codes in set2 */

/* Use LWIN=SEARCH (before translate) as Fn key. */
#define SCANCODE_FN	SCANCODE_LEFT_WIN

struct makecode_translate_entry {
	uint32_t from, to;
};

static const struct makecode_translate_entry legacy_mapping[] = {
	{ 0xe007, SCANCODE_LEFT_WIN },  /* ASSIST => SEARCH(Win) */
	{ 0x002f, SCANCODE_APP },  /* MENU => APP */
};

/* Alternate mapping when Fn is pressed. */
static const struct makecode_translate_entry legacy_fn_mapping[] = {
	{ SCANCODE_F1, 0xe038 },  /* F1 => Browser Back */
	{ SCANCODE_F2, 0xe020 },  /* F2 => Browser Refresh */
	{ SCANCODE_F3, 0x0078 },  /* F3 => Full Screen */
	{ SCANCODE_F4, 0xe07c },  /* F4 => Print Screen */
	/* TODO(hungte) Add F5 and F6 for DIM/BRIGHT. */
	{ SCANCODE_F7, 0xe034 },  /* F7 => Play/Pause */
	{ SCANCODE_F8, 0xe023 },  /* F8 => Mute */

	{ 0x0001, 0xe021 },  /* F9 => Vol Down */
	{ 0x0009, 0xe032 },  /* F10 => Vol Up */

	{ SCANCODE_LEFT_ALT, SCANCODE_CAPSLOCK },
	{ 0x0049, 0xe070 },  /* Dot(.) => Insert */
	{ 0x0066, 0xe071 },  /* BackSpace => Delete */

	{ 0x004d, SCANCODE_PAUSE },  /* P => Pause */
	{ 0x0032, SCANCODE_CTRL_BREAK },  /* B => Ctrl-Break */

	{ SCANCODE_UP, 0xe07d },  /* Up => Page Up */
	{ SCANCODE_DOWN, 0xe07a },  /* Down => Page Down */
	{ SCANCODE_LEFT, 0xe06c },  /* Left => Home */
	{ SCANCODE_RIGHT, 0xe069 },  /* Right => End */
};

/* Indicate if the mapping is KEYBOARD_MAPPING_LEGACY. */
static int is_legacy_mapping;

/* Indicate if Fn key is already pressed. */
static int fn_pressed;

/* Override the board_init_keyboard_mapping in board.c */
void board_init_keyboard_mapping(int reset)
{
	if (reset && chipset_in_state(CHIPSET_STATE_SUSPEND))
		return;
	keyboard_select_mapping(KEYBOARD_MAPPING_DEFAULT);
}

/* Callback when the mapping is changed (keyboard_select_mapping called). */
void keyboard_board_mapping_changed(enum keyboard_mapping_type new_mapping)
{
	is_legacy_mapping = (new_mapping == KEYBOARD_MAPPING_LEGACY);
}

enum ec_error_list keyboard_scancode_callback(uint32_t *make_code,
					      int8_t pressed,
					      int32_t *oneshot)
{
	int i;
	uint32_t m = *make_code;
	if (!is_legacy_mapping)
		return EC_SUCCESS;

	/* Fn must be processed because Fn makecode conflicts with Win. */
	if (m == SCANCODE_FN) {
		fn_pressed = pressed;
		*oneshot = 1;
		*make_code = 0;
		/**
		 * TODO(hungte): If we press Fn, X, (triggers an Fn+X make) then
		 * release Fn, X, then it'll trigger a X break instead of Fn+X
		 * break. This is a known issue and should be fixed.
		 */
		return EC_SUCCESS;
	}

	for (i = 0; i < ARRAY_SIZE(legacy_mapping); i++) {
		if (m == legacy_mapping[i].from) {
			m = legacy_mapping[i].to;
			/* TODO(hungte): Should we also do one-shot here? */
			*make_code = m;
			break;
		}

	}

	if (!fn_pressed)
		return EC_SUCCESS;

	for (i = 0; i < ARRAY_SIZE(legacy_fn_mapping); i++) {
		if (m == legacy_fn_mapping[i].from) {
			m = legacy_fn_mapping[i].to;
			*make_code = m;
			break;
		}

	}
	if (m == SCANCODE_PAUSE || m == SCANCODE_CTRL_BREAK) {
		*oneshot = 1;
		if (!pressed)
			*make_code = 0;
	}

	return EC_SUCCESS;
}
