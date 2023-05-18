/* Copyright 2023 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Screebo board-specific USB-C configuration */

#include "driver/tcpm/ps8xxx_public.h"
#include "ppc/syv682x_public.h"
#include "system.h"
#include "usbc_config.h"
#include "usbc_ppc.h"

#include <zephyr/drivers/gpio.h>

void screebo_ppc_interrupt(enum gpio_signal signal)
{
	switch (signal) {
	case GPIO_USB_C0_PPC_INT_ODL:
		syv682x_interrupt(USBC_PORT_C0);
		break;
	case GPIO_USB_C1_PPC_INT_ODL:
		syv682x_interrupt(USBC_PORT_C1);
		break;
	default:
		break;
	}
}

void board_reset_pd_mcu(void)
{
	/* Reset TCPC0 */
	reset_nct38xx_port(USBC_PORT_C0);

	/* Reset TCPC1 */
	if (tcpc_config[1].rst_gpio.port) {
		gpio_pin_set_dt(&tcpc_config[1].rst_gpio, 1);
		msleep(PS8XXX_RESET_DELAY_MS);
		gpio_pin_set_dt(&tcpc_config[1].rst_gpio, 0);
		msleep(PS8815_FW_INIT_DELAY_MS);
	}
}
