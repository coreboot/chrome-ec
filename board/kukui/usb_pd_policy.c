/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "atomic.h"
#include "charger.h"
#include "charge_manager.h"
#include "common.h"
#include "console.h"
#include "driver/charger/rt946x.h"
#include "gpio.h"
#include "hooks.h"
#include "host_command.h"
#include "registers.h"
#include "system.h"
#include "task.h"
#include "timer.h"
#include "util.h"
#include "usb_mux.h"
#include "usb_pd.h"
#include "usb_pd_tcpm.h"

#define CPRINTF(format, args...) cprintf(CC_USBPD, format, ## args)
#define CPRINTS(format, args...) cprints(CC_USBPD, format, ## args)

static uint8_t vbus_en;

int board_vbus_source_enabled(int port)
{
	return vbus_en;
}

int pd_check_vconn_swap(int port)
{
	/*
	 * VCONN is provided directly by the battery (PPVAR_SYS)
	 * but use the same rules as power swap.
	 */
	return pd_get_dual_role(port) == PD_DRP_TOGGLE_ON ? 1 : 0;
}

int pd_set_power_supply_ready(int port)
{

	pd_set_vbus_discharge(port, 0);
	/* Provide VBUS */
	vbus_en = 1;
	charger_enable_otg_power(1);

	/* notify host of power info change */
	pd_send_host_event(PD_EVENT_POWER_CHANGE);

	return EC_SUCCESS; /* we are ready */
}

void pd_power_supply_reset(int port)
{
	int prev_en;

	prev_en = vbus_en;
	/* Disable VBUS */
	vbus_en = 0;
	charger_enable_otg_power(0);
	/* Enable discharge if we were previously sourcing 5V */
	if (prev_en)
		pd_set_vbus_discharge(port, 1);

	/* notify host of power info change */
	pd_send_host_event(PD_EVENT_POWER_CHANGE);
}

/* ----------------- Vendor Defined Messages ------------------ */
#ifdef CONFIG_USB_PD_ALT_MODE_DFP
__override void svdm_dp_post_config(int port)
{
	dp_flags[port] |= DP_FLAGS_DP_ON;
}

__override int svdm_dp_attention(int port, uint32_t *payload)
{
	const struct usb_mux *mux = &usb_muxes[port];
	int lvl = PD_VDO_DPSTS_HPD_LVL(payload[1]);
	int irq = PD_VDO_DPSTS_HPD_IRQ(payload[1]);
	int mf_pref = PD_VDO_DPSTS_MF_PREF(payload[1]);

	dp_status[port] = payload[1];

	mux->hpd_update(port, lvl, irq);

	if (lvl)
		usb_mux_set(port, mf_pref ? TYPEC_MUX_DOCK : TYPEC_MUX_DP,
			    USB_SWITCH_CONNECT, pd_get_polarity(port));
	else
		usb_mux_set(port, mf_pref ? TYPEC_MUX_USB : TYPEC_MUX_NONE,
			    USB_SWITCH_CONNECT, pd_get_polarity(port));

	return 1;
}

__override void svdm_exit_dp_mode(int port)
{
	const struct usb_mux *mux = &usb_muxes[port];

	svdm_safe_dp_mode(port);
	mux->hpd_update(port, 0, 0);
}
#endif /* CONFIG_USB_PD_ALT_MODE_DFP */

