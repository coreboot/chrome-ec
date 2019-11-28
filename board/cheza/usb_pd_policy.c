/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "charge_manager.h"
#include "console.h"
#include "gpio.h"
#include "system.h"
#include "usb_mux.h"
#include "usbc_ppc.h"
#include "util.h"

#define CPRINTS(format, args...) cprints(CC_USBCHARGE, format, ## args)
#define CPRINTF(format, args...) cprintf(CC_USBCHARGE, format, ## args)

int pd_check_vconn_swap(int port)
{
	/* TODO(waihong): Check any case we do not allow. */
	return 1;
}

__override void pd_execute_data_swap(int port, int data_role)
{
	/* Do nothing */
}

static uint8_t vbus_en[CONFIG_USB_PD_PORT_COUNT];
static uint8_t vbus_rp[CONFIG_USB_PD_PORT_COUNT] = {TYPEC_RP_1A5, TYPEC_RP_1A5};

static void board_vbus_update_source_current(int port)
{
	if (port == 0) {
		/*
		 * Port 0 is controlled by a USB-C PPC SN5S330.
		 */
		ppc_set_vbus_source_current_limit(port, vbus_rp[port]);
		ppc_vbus_source_enable(port, vbus_en[port]);
	} else if (port == 1) {
		/*
		 * Port 1 is controlled by a USB-C current-limited power
		 * switch, NX5P3290.   Change the GPIO driving the load switch.
		 *
		 * 1.5 vs 3.0 A limit is controlled by a dedicated gpio.
		 * If the GPIO is asserted, it shorts a n-MOSFET to put a
		 * 16.5k resistance (2x 33k in parallel) on the NX5P3290 load
		 * switch ILIM pin, setting a minimum OCP current of 3100 mA.
		 * If the GPIO is deasserted, the n-MOSFET is open that makes
		 * a single 33k resistor on ILIM, setting a minimum OCP
		 * current of 1505 mA.
		 */
		gpio_set_level(GPIO_EN_USB_C1_3A,
			       vbus_rp[port] == TYPEC_RP_3A0 ? 1 : 0);
		gpio_set_level(GPIO_EN_USB_C1_5V_OUT, vbus_en[port]);
	}
}

void pd_power_supply_reset(int port)
{
	int prev_en;

	prev_en = vbus_en[port];

	/* Disable VBUS */
	vbus_en[port] = 0;
	board_vbus_update_source_current(port);

	/* Enable discharge if we were previously sourcing 5V */
	if (prev_en)
		pd_set_vbus_discharge(port, 1);

#ifdef CONFIG_USB_PD_MAX_SINGLE_SOURCE_CURRENT
	/* Give back the current quota we are no longer using */
	charge_manager_source_port(port, 0);
#endif /* defined(CONFIG_USB_PD_MAX_SINGLE_SOURCE_CURRENT) */

	/* notify host of power info change */
	pd_send_host_event(PD_EVENT_POWER_CHANGE);
}

int pd_set_power_supply_ready(int port)
{
	/* Disable charging */
	board_vbus_sink_enable(port, 0);

	pd_set_vbus_discharge(port, 0);

	/* Provide VBUS */
	vbus_en[port] = 1;
	board_vbus_update_source_current(port);

	/* Ensure we advertise the proper available current quota */
	charge_manager_source_port(port, 1);

	/* notify host of power info change */
	pd_send_host_event(PD_EVENT_POWER_CHANGE);

	return EC_SUCCESS; /* we are ready */
}

int board_vbus_source_enabled(int port)
{
	return vbus_en[port];
}

__override void typec_set_source_current_limit(int port, enum tcpc_rp_value rp)
{
	vbus_rp[port] = rp;
	board_vbus_update_source_current(port);
}

int pd_snk_is_vbus_provided(int port)
{
	return !gpio_get_level(port ? GPIO_USB_C1_VBUS_DET_L :
				      GPIO_USB_C0_VBUS_DET_L);
}

/* ----------------- Vendor Defined Messages ------------------ */


extern uint32_t dp_status[CONFIG_USB_PD_PORT_COUNT];
__override int svdm_dp_attention(int port, uint32_t *payload)
{
	int lvl = PD_VDO_DPSTS_HPD_LVL(payload[1]);
	int irq = PD_VDO_DPSTS_HPD_IRQ(payload[1]);
	const struct usb_mux *mux = &usb_muxes[port];

	dp_status[port] = payload[1];
	if (!(dp_flags[port] & DP_FLAGS_DP_ON)) {
		if (lvl)
			dp_flags[port] |= DP_FLAGS_HPD_HI_PENDING;
		return 1;
	}
	mux->hpd_update(port, lvl, irq);

	/* ack */
	return 1;
}
