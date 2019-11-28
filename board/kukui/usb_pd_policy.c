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

int pd_set_power_supply_ready(int port)
{

	pd_set_vbus_discharge(port, 0);
	/* Provide VBUS */
	vbus_en = 1;
	charger_enable_otg_power(1);

#if BOARD_REV >= 2
	/* TODO(b:123268580): Implement POGO discharge logic. */
	gpio_set_level(GPIO_EN_USBC_CHARGE_L, 1);
	gpio_set_level(GPIO_EN_PP5000_USBC, 1);
#endif

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

#if BOARD_REV >= 2
	/*
	 * TODO(b:123268580): Implement POGO discharge logic.
	 *
	 * Turn off source path and POGO path before asserting
	 * EN_USB_CHARGE_L.
	 */
	gpio_set_level(GPIO_EN_PP5000_USBC, 0);
	gpio_set_level(GPIO_EN_POGO_CHARGE_L, 1);
	gpio_set_level(GPIO_EN_USBC_CHARGE_L, 0);
#endif

	/* notify host of power info change */
	pd_send_host_event(PD_EVENT_POWER_CHANGE);
}

int pd_check_vconn_swap(int port)
{
	/*
	 * VCONN is provided directly by the battery (PPVAR_SYS)
	 * but use the same rules as power swap.
	 */
	return pd_get_dual_role(port) == PD_DRP_TOGGLE_ON ? 1 : 0;
}

/* ----------------- Vendor Defined Messages ------------------ */
#ifdef CONFIG_USB_PD_ALT_MODE_DFP

__override void svdm_safe_dp_mode(int port)
{
	/* make DP interface safe until configure */
	dp_flags[port] = 0;
	dp_status[port] = 0;
	usb_mux_set(port, TYPEC_MUX_NONE,
		    USB_SWITCH_CONNECT, pd_get_polarity(port));
}

__override int svdm_enter_dp_mode(int port, uint32_t mode_caps)
{
	/* Only enter mode if device is DFP_D capable */
	if (mode_caps & MODE_DP_SNK) {
		svdm_safe_dp_mode(port);
		return 0;
	}

	return -1;
}

__override int svdm_dp_config(int port, uint32_t *payload)
{
	int opos = pd_alt_mode(port, USB_SID_DISPLAYPORT);
	int pin_mode = pd_dfp_dp_get_pin_mode(port, dp_status[port]);

	if (!pin_mode)
		return 0;

	payload[0] = VDO(USB_SID_DISPLAYPORT, 1,
			 CMD_DP_CONFIG | VDO_OPOS(opos));
	payload[1] = VDO_DP_CFG(pin_mode,      /* pin mode */
				1,             /* DPv1.3 signaling */
				2);            /* UFP connected */
	return 2;
};

/*
 * timestamp of the next possible toggle to ensure the 2-ms spacing
 * between IRQ_HPD.
 */
static uint64_t hpd_deadline[CONFIG_USB_PD_PORT_COUNT];

__override void svdm_dp_post_config(int port)
{
	const struct usb_mux * const mux = &usb_muxes[port];

	dp_flags[port] |= DP_FLAGS_DP_ON;
	if (!(dp_flags[port] & DP_FLAGS_HPD_HI_PENDING))
		return;

	gpio_set_level(GPIO_USB_C0_HPD_OD, 1);
	gpio_set_level(GPIO_USB_C0_DP_OE_L, 0);
	gpio_set_level(GPIO_USB_C0_DP_POLARITY, pd_get_polarity(port));

	/* set the minimum time delay (2ms) for the next HPD IRQ */
	hpd_deadline[port] = get_time().val + HPD_USTREAM_DEBOUNCE_LVL;
	mux->hpd_update(port, 1, 0);
}

__override int svdm_dp_attention(int port, uint32_t *payload)
{
	int cur_lvl = gpio_get_level(GPIO_USB_C0_HPD_OD);
	int lvl = PD_VDO_DPSTS_HPD_LVL(payload[1]);
	int irq = PD_VDO_DPSTS_HPD_IRQ(payload[1]);
	const struct usb_mux * const mux = &usb_muxes[port];

	dp_status[port] = payload[1];

	/* Its initial DP status message prior to config */
	if (!(dp_flags[port] & DP_FLAGS_DP_ON)) {
		if (lvl)
			dp_flags[port] |= DP_FLAGS_HPD_HI_PENDING;
		return 1;
	}

	if (irq & cur_lvl) {
		uint64_t now = get_time().val;
		/* wait for the minimum spacing between IRQ_HPD if needed */
		if (now < hpd_deadline[port])
			usleep(hpd_deadline[port] - now);

		/* generate IRQ_HPD pulse */
		gpio_set_level(GPIO_USB_C0_HPD_OD, 0);
		usleep(HPD_DSTREAM_DEBOUNCE_IRQ);
		gpio_set_level(GPIO_USB_C0_HPD_OD, 1);

		gpio_set_level(GPIO_USB_C0_DP_OE_L, 0);
		gpio_set_level(GPIO_USB_C0_DP_POLARITY, pd_get_polarity(port));

		/* set the minimum time delay (2ms) for the next HPD IRQ */
		hpd_deadline[port] = get_time().val + HPD_USTREAM_DEBOUNCE_LVL;
	} else if (irq & !cur_lvl) {
		CPRINTF("ERR:HPD:IRQ&LOW\n");
		return 0; /* nak */
	} else {
		gpio_set_level(GPIO_USB_C0_HPD_OD, lvl);
		gpio_set_level(GPIO_USB_C0_DP_OE_L, !lvl);
		gpio_set_level(GPIO_USB_C0_DP_POLARITY, pd_get_polarity(port));
		/* set the minimum time delay (2ms) for the next HPD IRQ */
		hpd_deadline[port] = get_time().val + HPD_USTREAM_DEBOUNCE_LVL;
	}

	mux->hpd_update(port, lvl, irq);
	/* ack */
	return 1;
}

__override void svdm_exit_dp_mode(int port)
{
	const struct usb_mux * const mux = &usb_muxes[port];

	svdm_safe_dp_mode(port);
	gpio_set_level(GPIO_USB_C0_HPD_OD, 0);
	gpio_set_level(GPIO_USB_C0_DP_OE_L, 1);
	mux->hpd_update(port, 0, 0);
}
#endif /* CONFIG_USB_PD_ALT_MODE_DFP */
