/* Copyright 2023 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * TODO(b/272518464): Work around coreboot GCC preprocessor bug.
 * #line marks the *next* line, so it is off by one.
 */
#line 11

#include "charge_state.h"
#include "console.h"
#include "cros_board_info.h"
#include "cros_cbi.h"
#include "driver/retimer/anx7483_public.h"
#include "hooks.h"
#include "ioexpander.h"
#include "usb_mux.h"
#include "usbc/usb_muxes.h"

#include <zephyr/drivers/gpio.h>

#define CPRINTSUSB(format, args...) cprints(CC_USBCHARGE, format, ##args)
#define CPRINTFUSB(format, args...) cprintf(CC_USBCHARGE, format, ##args)

/*
 * USB C0 (general) and C1 (just ANX DB) use IOEX pins to
 * indicate flipped polarity to a protection switch.
 */

int board_anx7483_c1_mux_set(const struct usb_mux *me, mux_state_t mux_state)
{
	bool flipped = mux_state & USB_PD_MUX_POLARITY_INVERTED;

	/* Remove flipped from the state for easier compraisons */
	mux_state = mux_state & ~USB_PD_MUX_POLARITY_INVERTED;

	RETURN_ERROR(anx7483_set_default_tuning(me, mux_state));

	if (mux_state == USB_PD_MUX_USB_ENABLED) {
		RETURN_ERROR(anx7483_set_eq(me, ANX7483_PIN_DRX1,
					    ANX7483_EQ_SETTING_12_5DB));
		RETURN_ERROR(anx7483_set_eq(me, ANX7483_PIN_DRX2,
					    ANX7483_EQ_SETTING_12_5DB));
	} else if (mux_state == USB_PD_MUX_DOCK && !flipped) {
		RETURN_ERROR(anx7483_set_eq(me, ANX7483_PIN_DRX1,
					    ANX7483_EQ_SETTING_12_5DB));
	} else if (mux_state == USB_PD_MUX_DOCK && flipped) {
		RETURN_ERROR(anx7483_set_eq(me, ANX7483_PIN_DRX2,
					    ANX7483_EQ_SETTING_12_5DB));
	}

	/*
	 * Those registers all need to be set no matter what state the mux is
	 * in it needs to be set.
	 */
	RETURN_ERROR(
		anx7483_set_eq(me, ANX7483_PIN_URX1, ANX7483_EQ_SETTING_8_4DB));
	RETURN_ERROR(
		anx7483_set_eq(me, ANX7483_PIN_URX2, ANX7483_EQ_SETTING_8_4DB));
	RETURN_ERROR(
		anx7483_set_eq(me, ANX7483_PIN_UTX1, ANX7483_EQ_SETTING_8_4DB));
	RETURN_ERROR(
		anx7483_set_eq(me, ANX7483_PIN_UTX2, ANX7483_EQ_SETTING_8_4DB));

	RETURN_ERROR(
		anx7483_set_fg(me, ANX7483_PIN_URX1, ANX7483_FG_SETTING_1_2DB));
	RETURN_ERROR(
		anx7483_set_fg(me, ANX7483_PIN_URX2, ANX7483_FG_SETTING_1_2DB));
	RETURN_ERROR(
		anx7483_set_fg(me, ANX7483_PIN_UTX1, ANX7483_FG_SETTING_1_2DB));
	RETURN_ERROR(
		anx7483_set_fg(me, ANX7483_PIN_UTX2, ANX7483_FG_SETTING_1_2DB));

	return EC_SUCCESS;
}
