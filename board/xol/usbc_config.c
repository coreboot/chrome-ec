/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cbi.h"
#include "charge_ramp.h"
#include "charger.h"
#include "common.h"
#include "compile_time_macros.h"
#include "console.h"
#include "driver/ppc/nx20p348x.h"
#include "driver/tcpm/nct38xx.h"
#include "driver/tcpm/tcpci.h"
#include "ec_commands.h"
#include "gpio.h"
#include "gpio_signal.h"
#include "hooks.h"
#include "ioexpander.h"
#include "system.h"
#include "task.h"
#include "task_id.h"
#include "timer.h"
#include "usb_charge.h"
#include "usb_mux.h"
#include "usb_pd.h"
#include "usb_pd_tcpm.h"
#include "usbc_config.h"
#include "usbc_ppc.h"

#include <stdbool.h>
#include <stdint.h>

#define CPRINTF(format, args...) cprintf(CC_USBPD, format, ##args)
#define CPRINTS(format, args...) cprints(CC_USBPD, format, ##args)

#ifdef CONFIG_ZEPHYR
enum ioex_port { IOEX_C0_NCT38XX = 0, IOEX_C2_NCT38XX, IOEX_PORT_COUNT };
#endif /* CONFIG_ZEPHYR */

#ifndef CONFIG_ZEPHYR
/* USBC TCPC configuration */
const struct tcpc_config_t tcpc_config[] = {
	[USBC_PORT_C0] = {
		.bus_type = EC_BUS_TYPE_I2C,
		.i2c_info = {
			.port = I2C_PORT_USB_C0_C2_TCPC,
			.addr_flags = NCT38XX_I2C_ADDR1_1_FLAGS,
		},
		.drv = &nct38xx_tcpm_drv,
		.flags = TCPC_FLAGS_TCPCI_REV2_0,
	},
	[USBC_PORT_C2] = {
		.bus_type = EC_BUS_TYPE_I2C,
		.i2c_info = {
			.port = I2C_PORT_USB_C0_C2_TCPC,
			.addr_flags = NCT38XX_I2C_ADDR2_1_FLAGS,
		},
		.drv = &nct38xx_tcpm_drv,
		.flags = TCPC_FLAGS_TCPCI_REV2_0,
	},
};
BUILD_ASSERT(ARRAY_SIZE(tcpc_config) == USBC_PORT_COUNT);
BUILD_ASSERT(CONFIG_USB_PD_PORT_MAX_COUNT == USBC_PORT_COUNT);
#endif /* !CONFIG_ZEPHYR */

/******************************************************************************/
/* USB-A charging control */

#ifndef CONFIG_ZEPHYR
const int usb_port_enable[USB_PORT_COUNT] = {
	GPIO_EN_PP5000_USBA_R,
};
#endif
BUILD_ASSERT(ARRAY_SIZE(usb_port_enable) == USB_PORT_COUNT);

/******************************************************************************/

#ifndef CONFIG_ZEPHYR
/* USBC PPC configuration */
struct ppc_config_t ppc_chips[] = {
	[USBC_PORT_C0] = {
		.i2c_port = I2C_PORT_USB_C0_C2_PPC,
		.i2c_addr_flags = NX20P3483_ADDR2_FLAGS,
		.drv = &nx20p348x_drv,
	},
	[USBC_PORT_C2] = {
		.i2c_port = I2C_PORT_USB_C0_C2_PPC,
		.i2c_addr_flags = NX20P3483_ADDR3_FLAGS,
		.drv = &nx20p348x_drv,
	},
};
BUILD_ASSERT(ARRAY_SIZE(ppc_chips) == USBC_PORT_COUNT);

unsigned int ppc_cnt = ARRAY_SIZE(ppc_chips);

const struct usb_mux_chain usb_muxes[] = {
	[USBC_PORT_C0] = {
		.mux = &(const struct usb_mux){
			.usb_port = USBC_PORT_C0,
			.driver = &virtual_usb_mux_driver,
			.hpd_update = &virtual_hpd_update,
		},
	},
	[USBC_PORT_C2] = {
		.mux = &(const struct usb_mux){
			.usb_port = USBC_PORT_C2,
			.driver = &virtual_usb_mux_driver,
			.hpd_update = &virtual_hpd_update,
		},
	},
};
BUILD_ASSERT(ARRAY_SIZE(usb_muxes) == USBC_PORT_COUNT);

/*
 * USB C0 and C2 uses burnside bridge chips and have their reset
 * controlled by their respective TCPC chips acting as GPIO expanders.
 *
 * ioex_init() is normally called before we take the TCPCs out of
 * reset, so we need to start in disabled mode, then explicitly
 * call ioex_init().
 */

struct ioexpander_config_t ioex_config[] = {
	[IOEX_C0_NCT38XX] = {
		.i2c_host_port = I2C_PORT_USB_C0_C2_TCPC,
		.i2c_addr_flags = NCT38XX_I2C_ADDR1_1_FLAGS,
		.drv = &nct38xx_ioexpander_drv,
		.flags = IOEX_FLAGS_DEFAULT_INIT_DISABLED,
	},
	[IOEX_C2_NCT38XX] = {
		.i2c_host_port = I2C_PORT_USB_C0_C2_TCPC,
		.i2c_addr_flags = NCT38XX_I2C_ADDR2_1_FLAGS,
		.drv = &nct38xx_ioexpander_drv,
		.flags = IOEX_FLAGS_DEFAULT_INIT_DISABLED,
	},
};
BUILD_ASSERT(ARRAY_SIZE(ioex_config) == CONFIG_IO_EXPANDER_PORT_COUNT);
#endif /* !CONFIG_ZEPHYR */

void board_reset_pd_mcu(void)
{
}

static void board_tcpc_init(void)
{
	/*
	 * These IO expander pins are implemented using the
	 * C0/C2 TCPC, so they must be set up after the TCPC has
	 * been taken out of reset.
	 */
#ifndef CONFIG_ZEPHYR
	ioex_init(IOEX_C0_NCT38XX);
	ioex_init(IOEX_C2_NCT38XX);
#else
	gpio_reset_port(DEVICE_DT_GET(DT_NODELABEL(ioex_port1)));
	gpio_reset_port(DEVICE_DT_GET(DT_NODELABEL(ioex_port2)));
#endif

	/* Enable PPC interrupts. */
	gpio_enable_interrupt(GPIO_USB_C0_PPC_INT_ODL);
	gpio_enable_interrupt(GPIO_USB_C2_PPC_INT_ODL);

#ifndef CONFIG_ZEPHYR
	/* Enable TCPC interrupts. */
	gpio_enable_interrupt(GPIO_USB_C0_C2_TCPC_INT_ODL);
#endif /* !CONFIG_ZEPHYR */
}
DECLARE_HOOK(HOOK_INIT, board_tcpc_init, HOOK_PRIO_INIT_CHIPSET);

#ifndef CONFIG_ZEPHYR
uint16_t tcpc_get_alert_status(void)
{
	uint16_t status = 0;

	if (gpio_get_level(GPIO_USB_C0_C2_TCPC_INT_ODL) == 0)
		status |= PD_STATUS_TCPC_ALERT_0 | PD_STATUS_TCPC_ALERT_1;

	return status;
}
#endif

int ppc_get_alert_status(int port)
{
	if (port == USBC_PORT_C0)
		return gpio_get_level(GPIO_USB_C0_PPC_INT_ODL) == 0;
	else if (port == USBC_PORT_C2)
		return gpio_get_level(GPIO_USB_C2_PPC_INT_ODL) == 0;
	return 0;
}

#ifndef CONFIG_ZEPHYR
void tcpc_alert_event(enum gpio_signal signal)
{
	switch (signal) {
	case GPIO_USB_C0_C2_TCPC_INT_ODL:
		schedule_deferred_pd_interrupt(USBC_PORT_C0);
		break;
	default:
		break;
	}
}
#endif

void ppc_interrupt(enum gpio_signal signal)
{
	switch (signal) {
	case GPIO_USB_C0_PPC_INT_ODL:
		nx20p348x_interrupt(USBC_PORT_C0);
		break;

	case GPIO_USB_C2_PPC_INT_ODL:
		nx20p348x_interrupt(USBC_PORT_C2);
		break;

	default:
		break;
	}
}

__override bool board_is_dts_port(int port)
{
	return port == USBC_PORT_C0;
}
