/* Copyright 2022 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Track USER_PRES_L
 */
#include "extension.h"
#include "gpio.h"
#include "hooks.h"
#include "registers.h"
#include "scratch_reg1.h"
#include "timer.h"

static uint64_t last_press;
static uint8_t asserted;

/* Save the timestamp when DIOM4 is deasserted. */
void diom4_deasserted(enum gpio_signal unused)
{
	if (!board_use_diom4())
		return;

	asserted = 1;
	last_press = get_time().val;
}

void disable_user_pres(void)
{
	asserted = 0;
	gpio_set_wakepin(GPIO_DIOM4, 0);
	/* Disable the pullup and input on DIOM4 */
	GWRITE_FIELD(PINMUX, DIOM4_CTL, PU, 0);
	GWRITE_FIELD(PINMUX, DIOM4_CTL, IE, 0);
	gpio_disable_interrupt(GPIO_DIOM4);
}

void enable_user_pres(void)
{
	/*
	 * The interrupt happens on the rising edge. Wake on the falling edge
	 * to ensure cr50 sees the rising edge interrupt.
	 */
	gpio_set_wakepin(GPIO_DIOM4, GPIO_HIB_WAKE_FALLING);
	/*
	 * Some boards don't have an external pull up. Add an internal one to
	 * make sure the signal isn't floating.
	 * Enable the pullup and input on DIOM4
	 */
	GWRITE_FIELD(PINMUX, DIOM4_CTL, PU, 1);
	GWRITE_FIELD(PINMUX, DIOM4_CTL, IE, 1);
	/* Enable interrupts for user_pres_l detection */
	gpio_enable_interrupt(GPIO_DIOM4);
}

/*
 * Vendor command to for tracking USER_PRES_L
 *
 * This vendor command can be used to enable tracking USER_PRES_L on DIOM4 and
 * get the time since DIOM4 was deasserted.
 *
 * checks:
 * - batt_is_present - Factory reset can only be done if HW write protect is
 *              removed.
 * - FWMP disables ccd -  If FWMP has disabled ccd, then we can't bypass it with
 *              a factory reset.
 * - CCD password is set - If there is a password, someone will have to use that
 *              to open ccd and enable ccd manually. A factory reset cannot be
 *              used to get around the password.
 */
static enum vendor_cmd_rc vc_user_pres(enum vendor_cmd_cc code,
				       void *buf,
				       size_t input_size,
				       size_t *response_size)
{
	struct user_pres_response response;
	uint8_t *buffer = buf;

	*response_size = 0;

	if (input_size == 1) {
		uint8_t *cmd = buffer;

		if (*cmd == USER_PRES_DISABLE) {
			board_write_prop(BOARD_USE_DIOM4, 0);
			disable_user_pres();
		} else if (*cmd == USER_PRES_ENABLE) {
			board_write_prop(BOARD_USE_DIOM4, 1);
			enable_user_pres();
		} else {
			return VENDOR_RC_BOGUS_ARGS;
		}
		return VENDOR_RC_SUCCESS;
	} else if (input_size) {
		return VENDOR_RC_BOGUS_ARGS;
	}
	*response_size = sizeof(response);
	response.state = board_use_diom4() ? USER_PRES_ENABLE :
			USER_PRES_DISABLE;
	if (board_use_diom4() && asserted) {
		response.state |= USER_PRES_PRESSED;
		response.last_press = get_time().val - last_press;
	}
	memcpy(buffer, &response, sizeof(response));

	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_USER_PRES, vc_user_pres);

/**
 * Setup DIOM4 interrupts if the board uses them.
 */
static void init_user_pres(void)
{
	if (!board_use_diom4()) {
		disable_user_pres();
		return;
	}
	enable_user_pres();
}
DECLARE_HOOK(HOOK_INIT, init_user_pres, HOOK_PRIO_DEFAULT);
