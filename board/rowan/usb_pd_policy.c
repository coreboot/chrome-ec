/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "atomic.h"
#include "charge_manager.h"
#include "common.h"
#include "console.h"
#include "driver/tcpm/anx7688.h"
#include "driver/tcpm/tcpci.h"
#include "driver/tcpm/tcpm.h"
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

#define CPRINTF(format, args...) cprintf(CC_USBPD, format, ## args)
#define CPRINTS(format, args...) cprints(CC_USBPD, format, ## args)

int pd_set_power_supply_ready(int port)
{
	/* Disable charging */
	gpio_set_level(GPIO_USB_C0_CHARGE_L, 1);
	/* Provide VBUS */
	gpio_set_level(GPIO_USB_C0_5V_EN, 1);

	anx7688_set_power_supply_ready(port);

	/* notify host of power info change */
	pd_send_host_event(PD_EVENT_POWER_CHANGE);

	return EC_SUCCESS;
}

void pd_power_supply_reset(int port)
{
	/* Disable VBUS */
	gpio_set_level(GPIO_USB_C0_5V_EN, 0);

	anx7688_power_supply_reset(port);

	/* notify host of power info change */
	pd_send_host_event(PD_EVENT_POWER_CHANGE);
}

int pd_check_vconn_swap(int port)
{
	/* in G3, do not allow vconn swap since 5V power source is off */
	return gpio_get_level(GPIO_5V_POWER_GOOD);
}

__override void typec_set_source_current_limit(int port, enum tcpc_rp_value rp)
{
