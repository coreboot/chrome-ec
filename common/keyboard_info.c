/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "button.h"
#include "console.h"
#include "ec_commands.h"
#include "host_command.h"
#include "lid_switch.h"
#include "keyboard_config.h"
#include "keyboard_protocol.h"
#include "keyboard_scan.h"
#include "system.h"
#include "tablet_mode.h"
#include "util.h"

static uint32_t get_supported_buttons(void)
{
	uint32_t val = 0;
#ifdef CONFIG_BUTTON_COUNT
	int i;

	for (i = 0; i < CONFIG_BUTTON_COUNT; i++) {
		if (buttons[i].type == KEYBOARD_BUTTON_VOLUME_UP)
			val |= (1 << EC_MKBP_VOL_UP);
		if (buttons[i].type == KEYBOARD_BUTTON_VOLUME_DOWN)
			val |= (1 << EC_MKBP_VOL_DOWN);
	}
#endif
#ifdef CONFIG_POWER_BUTTON
	val |= (1 << EC_MKBP_POWER_BUTTON);
#endif
	return val;
}

static uint32_t get_supported_switches(void)
{
	uint32_t val = 0;

#ifdef CONFIG_LID_SWITCH
	val |= (1 << EC_MKBP_LID_OPEN);
#endif
#ifdef CONFIG_TABLET_MODE_SWITCH
	val |= (1 << EC_MKBP_TABLET_MODE);
#endif
	return val;
}

static uint32_t get_switch_state(void)
{
	uint32_t val = 0;

#ifdef CONFIG_LID_SWITCH
	if (lid_is_open())
		val |= (1 << EC_MKBP_LID_OPEN);
#endif
#ifdef CONFIG_TABLET_MODE_SWITCH
	if (tablet_get_mode())
		val |= (1 << EC_MKBP_TABLET_MODE);
#endif
	return val;
}

static int mkbp_get_info(struct host_cmd_handler_args *args)
{
	const struct ec_params_mkbp_info *p = args->params;

	if (args->params_size == 0 || p->info_type == EC_MKBP_INFO_KBD) {
		struct ec_response_mkbp_info *r = args->response;

		/* Version 0 just returns info about the keyboard. */
		r->rows = KEYBOARD_ROWS;
		r->cols = KEYBOARD_COLS;
		/* This used to be "switches" which was previously 0. */
		r->reserved = 0;

		args->response_size = sizeof(struct ec_response_mkbp_info);
	} else {
		union ec_response_get_next_data *r = args->response;

		/* Version 1 (other than EC_MKBP_INFO_KBD) */
		switch (p->info_type) {
		case EC_MKBP_INFO_SUPPORTED:
			switch (p->event_type) {
			case EC_MKBP_EVENT_BUTTON:
				r->buttons = get_supported_buttons();
				args->response_size = sizeof(r->buttons);
				break;

			case EC_MKBP_EVENT_SWITCH:
				r->switches = get_supported_switches();
				args->response_size = sizeof(r->switches);
				break;

			default:
				/* Don't care for now for other types. */
				return EC_RES_INVALID_PARAM;
			}
			break;

		case EC_MKBP_INFO_CURRENT:
			switch (p->event_type) {
#ifdef HAS_TASK_KEYSCAN
			case EC_MKBP_EVENT_KEY_MATRIX:
				memcpy(r->key_matrix, keyboard_scan_get_state(),
				       sizeof(r->key_matrix));
				args->response_size = sizeof(r->key_matrix);
				break;
#endif
			case EC_MKBP_EVENT_HOST_EVENT:
				r->host_event = host_get_events();
				args->response_size = sizeof(r->host_event);
				break;

			case EC_MKBP_EVENT_BUTTON:
				r->buttons = keyboard_get_button_state();
				args->response_size = sizeof(r->buttons);
				break;

			case EC_MKBP_EVENT_SWITCH:
				r->switches = get_switch_state();
				args->response_size = sizeof(r->switches);
				break;

			default:
				/* Doesn't make sense for other event types. */
				return EC_RES_INVALID_PARAM;
			}
			break;

		case EC_MKBP_INFO_GET_KB_AT_BOOT:
			if (keyboard_get_matrix_at_boot(r->key_matrix,
							KEYBOARD_COLS))
				return EC_RES_RESPONSE_TOO_BIG;
			args->response_size = KEYBOARD_COLS;
			break;

		case EC_MKBP_INFO_CLEAR_KB_AT_BOOT:
			keyboard_clear_matrix_at_boot();
			args->response_size = 0;
			break;

		case EC_MKBP_INFO_SIMULATE_KB_AT_BOOT:
			if (system_is_locked())
				return EC_RES_ACCESS_DENIED;
			keyboard_simulate_matrix_at_boot(
					p->simulate_kb_at_boot.key_matrix,
					KEYBOARD_COLS);
			args->response_size = 0;
			break;

		default:
			/* Unsupported query. */
			return EC_RES_ERROR;
		}
	}
	return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_MKBP_INFO, mkbp_get_info,
		     EC_VER_MASK(0) | EC_VER_MASK(1));

