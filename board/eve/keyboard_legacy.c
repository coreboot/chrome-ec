/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Eve board-specific keyboard configuration for legacy mode */

#include "board_config.h"
#include "chipset.h"
#include "keyboard_8042.h"
#include "keyboard_protocol.h"
#include "util.h"

/**
 * Special make codes for keys to handle directly. Make code in all code sets
 * should not start with 0xf0 so we can use that to encode special values.
 */
#define MK_PAUSE	0xf001
#define MK_CBREAK	0xf002
#define MK_DIM		0xf003
#define MK_BRIGHT	0xf004

/* Scan codes in set2 */

/* Use SEARCH (before translate) as Fn key. */
#define SCANCODE_FN	0xe01f

static const struct makecode_translate_entry legacy_mapping[] = {
	{ 0xe007, 0xe020 },  /* ASSIST => SEARCH(Win) */
	{ 0x002f, 0xe02f },  /* MENU => APP */
};

/* Alternate mapping when Fn is pressed. */
static const struct makecode_translate_entry legacy_fn_mapping[] = {
	{ 0x0005, 0xe038 },  /* F1 => Browser Back */
	{ 0x0006, 0xe020 },  /* F2 => Browser Refresh */
	{ 0x0004, 0x0078 },  /* F3 => Full Screen */
	{ 0x000c, 0xe07c },  /* F4 => Print Screen */
	{ 0x0003, MK_DIM },  /* F5 => Dim Screen */
	{ 0x000b, MK_BRIGHT },  /* F6 => Brighten Screen */
	{ 0x0083, 0xe034 },  /* F7 => Play/Pause */
	{ 0x000a, 0xe023 },  /* F8 => Mute */
	{ 0x0001, 0xe021 },  /* F9 => Vol Down */
	{ 0x0009, 0xe032 },  /* F10 => Vol Up */

	{ 0x0011, 0x0058 },  /* LAlt => Caps Lock */
	{ 0x0049, 0xe070 },  /* Dot(.) => Insert */
	{ 0x0066, 0xe071 },  /* BackSpace => Delete */

	{ 0x004d, MK_PAUSE },  /* P => Pause */
	{ 0x0032, MK_CBREAK },  /* B => Ctrl-Break */

	{ 0xe075, 0xe07d },  /* Up => Page Up */
	{ 0xe072, 0xe07a },  /* Down => Page Down */
	{ 0xe06b, 0xe06c },  /* Left => Home */
	{ 0xe074, 0xe069 },  /* Right => End */
};

static const char pause_key_scancode_set1[] = {
	0xe1, 0x1d, 0x45, 0xe1, 0x9d, 0xc5
};

static const char pause_key_scancode_set2[] = {
	0xe1, 0x14, 0x77, 0xe1, 0xf0, 0x14, 0xf0, 0x77
};

static const char cbreak_key_scancode_set1[] = {
	0xe0, 0x46, 0xe0, 0xc6
};

static const char cbreak_key_scancode_set2[] = {
	0xe0, 0x7e, 0xe0, 0xf0, 0x7e
};

struct mk_entry {
	int len;
	const char *data;
};

struct mk_data {
	struct mk_entry make_set1, make_set2, break_set1, break_set2;
};

/* PAUSE and CBREAK do not have 8042 break code. */
static const struct mk_data mk_pause = {
	{ARRAY_SIZE(pause_key_scancode_set1),
	 ARRAY_BEGIN(pause_key_scancode_set1)},
	{ARRAY_SIZE(pause_key_scancode_set2),
	 ARRAY_BEGIN(pause_key_scancode_set2)},
};

static const const struct mk_data mk_cbreak = {
	{ARRAY_SIZE(cbreak_key_scancode_set1),
	 ARRAY_BEGIN(cbreak_key_scancode_set1)},
	{ARRAY_SIZE(cbreak_key_scancode_set2),
	 ARRAY_BEGIN(cbreak_key_scancode_set2)},
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

static void process_mk_data(const struct mk_entry *make_code,
			    const struct mk_entry *break_code,
			    int8_t pressed)
{
	if (pressed) {
		if (make_code->len)
			i8042_send_to_host(make_code->len, make_code->data);
	} else {
		if (break_code->len)
			i8042_send_to_host(break_code->len, break_code->data);
	}
}


static void process_mk(const struct mk_data *data, int8_t pressed,
		       enum scancode_set_list code_set)
{
	if (code_set == SCANCODE_SET_1) {
		process_mk_data(&data->make_set1, &data->break_set1, pressed);
	} else if (code_set == SCANCODE_SET_2) {
		process_mk_data(&data->make_set2, &data->break_set2, pressed);
	}
}

/* Translate legacy keys */
uint16_t keyboard_board_translate(uint16_t make_code, int8_t pressed,
				  enum scancode_set_list code_set)
{
	if (!is_legacy_mapping)
		return make_code;

	/* Fn must be processed because Fn makecode conflicts with Win. */
	if (make_code == SCANCODE_FN) {
		fn_pressed = pressed;
		/**
		 * TODO(hungte): If we press Fn, X, (triggers an Fn+X make) then
		 * release Fn, X, then it'll trigger a X break instead of Fn+X
		 * break. This is a known issue and should be fixed.
		 */
		return 0;
	}

	make_code = makecode_translate(
			make_code, ARRAY_BEGIN(legacy_mapping),
			ARRAY_SIZE(legacy_mapping));
	if (!fn_pressed)
		return make_code;

	make_code = makecode_translate(
			make_code, ARRAY_BEGIN(legacy_fn_mapping),
			ARRAY_SIZE(legacy_fn_mapping));
	switch (make_code) {
	case MK_PAUSE:
		process_mk(&mk_pause, pressed, code_set);
		return 0;

	case MK_CBREAK:
		process_mk(&mk_cbreak, pressed, code_set);
		return 0;

	case MK_DIM:
		/* TODO(hungte): Complete how to dim screen. */
		return 0;
	case MK_BRIGHT:
		/* TODO(hungte): Complete how to brighten screen. */
		return 0;
	}
	return make_code;
}
