/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Type-C port manager for Parade PS8XXX with integrated superspeed muxes.
 *
 * Supported TCPCs:
 * - PS8705
 * - PS8751
 * - PS8755
 * - PS8805
 * - PS8815
 */

#include "common.h"
#include "console.h"
#include "ps8xxx.h"
#include "tcpci.h"
#include "tcpm.h"
#include "timer.h"
#include "usb_pd.h"

#if !defined(CONFIG_USB_PD_TCPM_PS8705) && \
	!defined(CONFIG_USB_PD_TCPM_PS8751) && \
	!defined(CONFIG_USB_PD_TCPM_PS8755) && \
	!defined(CONFIG_USB_PD_TCPM_PS8805) && \
	!defined(CONFIG_USB_PD_TCPM_PS8815)
#error "Unsupported PS8xxx TCPC."
#endif

#if !defined(CONFIG_USB_PD_TCPM_TCPCI) || \
	!defined(CONFIG_USB_PD_TCPM_MUX) || \
	!defined(CONFIG_USBC_SS_MUX)

#error "PS8XXX is using a standard TCPCI interface with integrated mux control"
#error "Please upgrade your board configuration"

#endif

#define CPRINTF(format, args...) cprintf(CC_USBPD, format, ## args)
#define CPRINTS(format, args...) cprints(CC_USBPD, format, ## args)

/*
 * The product_id per ports here is expected to be set in callback function -
 * .init of tcpm_drv by calling board_get_ps8xxx_product_id().
 *
 * In case of CONFIG_USB_PD_TCPM_MULTI_PS8XXX enabled, board code should
 * override the board_get_ps8xxx_product_id() for getting the correct id.
 */
static uint16_t product_id[CONFIG_USB_PD_PORT_MAX_COUNT];

/*
 * timestamp of the next possible toggle to ensure the 2-ms spacing
 * between IRQ_HPD.
 */
static uint64_t hpd_deadline[CONFIG_USB_PD_PORT_MAX_COUNT];

#if defined(CONFIG_USB_PD_TCPM_PS8705) || \
	defined(CONFIG_USB_PD_TCPM_PS8751) || \
	defined(CONFIG_USB_PD_TCPM_PS8755) || \
	defined(CONFIG_USB_PD_TCPM_PS8805)
/*
 * DCI is enabled by default and burns about 40 mW when the port is in
 * USB2 mode or when a C-to-A dongle is attached, so force it off.
 */

static int ps8xxx_addr_dci_disable(int port, int i2c_addr, int i2c_reg)
{
	int status;
	int dci;

	status = tcpc_addr_read(port, i2c_addr, i2c_reg, &dci);
	if (status != EC_SUCCESS)
		return status;
	if ((dci & PS8XXX_REG_MUX_USB_DCI_CFG_MODE_MASK) !=
	    PS8XXX_REG_MUX_USB_DCI_CFG_MODE_OFF) {
		dci &= ~PS8XXX_REG_MUX_USB_DCI_CFG_MODE_MASK;
		dci |= PS8XXX_REG_MUX_USB_DCI_CFG_MODE_OFF;
		if (tcpc_addr_write(port, i2c_addr, i2c_reg, dci) != EC_SUCCESS)
			return status;
	}
	return EC_SUCCESS;
}
#endif /* CONFIG_USB_PD_TCPM_PS875[15] || CONFIG_USB_PD_TCPM_PS8[78]05 */

#if defined(CONFIG_USB_PD_TCPM_PS8705) || \
	defined(CONFIG_USB_PD_TCPM_PS8755) || \
	defined(CONFIG_USB_PD_TCPM_PS8805)
static int ps8705_dci_disable(int port)
{
	int p1_addr;
	int p3_addr;
	int regval;
	int rv;

	/* Enable access to debug pages. */
	p3_addr = tcpc_config[port].i2c_info.addr;
	rv = tcpc_addr_read(port, p3_addr, PS8XXX_REG_I2C_DEBUGGING_ENABLE,
			    &regval);
	if (rv)
		return rv;

	rv = tcpc_addr_write(port, p3_addr, PS8XXX_REG_I2C_DEBUGGING_ENABLE,
			     PS8XXX_REG_I2C_DEBUGGING_ENABLE_ON);

	/* Disable Auto DCI */
	p1_addr = PS8751_P3_TO_P1(p3_addr);
	rv = ps8xxx_addr_dci_disable(port, p1_addr,
				     PS8XXX_P1_REG_MUX_USB_DCI_CFG);

	/*
	 * PS8705/PS8755/PS8805 will automatically re-assert bit:0 on the
	 * PS8XXX_REG_I2C_DEBUGGING_ENABLE register.
	 */
	return rv;
}
#endif /* CONFIG_USB_PD_TCPM_PS8755 || CONFIG_USB_PD_TCPM_PS8[78]05 */

#ifdef CONFIG_USB_PD_TCPM_PS8751
static int ps8751_dci_disable(int port)
{
	int p3_addr;

	p3_addr = tcpc_config[port].i2c_info.addr;
	return ps8xxx_addr_dci_disable(port, p3_addr,
				       PS8751_REG_MUX_USB_DCI_CFG);
}
#endif /* CONFIG_USB_PD_TCPM_PS8751 */

#ifdef CONFIG_USB_PD_TCPM_PS8815
static int ps8815_dci_disable(int port)
{
	/* DCI is disabled on the ps8815 */
	return EC_SUCCESS;
}
#endif /* CONFIG_USB_PD_TCPM_PS8815 */

enum ps8xxx_variant_regs {
	REG_FIRST_INDEX = 0,
	/* NOTE: The rev will read as 0x00 if the FW has malfunctioned. */
	REG_FW_VER = REG_FIRST_INDEX,
	REG_MAX_COUNT,
};

struct ps8xxx_variant_map {
	int product_id;
	int (*dci_disable_ptr)(int port);
	int reg_map[REG_MAX_COUNT];
};

/*
 * variant_map here is leveraged to lookup from ps8xxx_variant_regs to i2c
 * register and corresponding dci_disable function based on product_id.
 */
static struct ps8xxx_variant_map variant_map[] = {
#ifdef CONFIG_USB_PD_TCPM_PS8705
	{
		PS8705_PRODUCT_ID,
		ps8705_dci_disable,
		{
		 [REG_FW_VER] = 0x82,
		}
	},
#endif
#ifdef CONFIG_USB_PD_TCPM_PS8751
	{
		PS8751_PRODUCT_ID,
		ps8751_dci_disable,
		{
			[REG_FW_VER] = 0x90,
		}
	},
#endif
#ifdef CONFIG_USB_PD_TCPM_PS8755
	{
		PS8755_PRODUCT_ID,
		ps8705_dci_disable,
		{
			[REG_FW_VER] = 0x82,
		}
	},
#endif
#ifdef CONFIG_USB_PD_TCPM_PS8805
	{
		PS8805_PRODUCT_ID,
		ps8705_dci_disable,
		{
			[REG_FW_VER] = 0x82,
		}
	},
#endif
#ifdef CONFIG_USB_PD_TCPM_PS8815
	{
		PS8815_PRODUCT_ID,
		ps8815_dci_disable,
		{
			[REG_FW_VER] = 0x82,
		}
	},
#endif
};

static int get_reg_by_product(const int port,
				const enum ps8xxx_variant_regs reg)
{
	int i;

	if (reg < REG_FIRST_INDEX || reg >= REG_MAX_COUNT)
		return INT32_MAX;

	for (i = 0; i < ARRAY_SIZE(variant_map); i++) {
		if (product_id[port] ==
		      variant_map[i].product_id) {
			return variant_map[i].reg_map[reg];
		}
	}

	CPRINTS("%s: failed to get register number by product_id.", __func__);
	return INT32_MAX;
}

static int dp_set_hpd(int port, int enable)
{
	int reg;
	int rv;

	rv = mux_read(port, MUX_IN_HPD_ASSERTION_REG, &reg);
	if (rv)
		return rv;
	if (enable)
		reg |= IN_HPD;
	else
		reg &= ~IN_HPD;
	return mux_write(port, MUX_IN_HPD_ASSERTION_REG, reg);
}

static int dp_set_irq(int port, int enable)
{

	int reg;
	int rv;

	rv = mux_read(port, MUX_IN_HPD_ASSERTION_REG, &reg);
	if (rv)
		return rv;
	if (enable)
		reg |= HPD_IRQ;
	else
		reg &= ~HPD_IRQ;
	return mux_write(port, MUX_IN_HPD_ASSERTION_REG, reg);
}

__overridable
uint16_t board_get_ps8xxx_product_id(int port)
{
	/* Board supporting multiple chip sources in ps8xxx.c MUST override this
	 * function to judge the real chip source for this board. For example,
	 * SKU ID / strappings / provisioning in the factory can be the ways.
	 */

	if (IS_ENABLED(CONFIG_USB_PD_TCPM_MULTI_PS8XXX)) {
		CPRINTS("%s: board should override this function.", __func__);
		return 0;
	} else if (IS_ENABLED(CONFIG_USB_PD_TCPM_PS8705)) {
		return PS8705_PRODUCT_ID;
	} else if (IS_ENABLED(CONFIG_USB_PD_TCPM_PS8751)) {
		return PS8751_PRODUCT_ID;
	} else if (IS_ENABLED(CONFIG_USB_PD_TCPM_PS8755)) {
		return PS8755_PRODUCT_ID;
	} else if (IS_ENABLED(CONFIG_USB_PD_TCPM_PS8805)) {
		return PS8805_PRODUCT_ID;
	} else if (IS_ENABLED(CONFIG_USB_PD_TCPM_PS8815)) {
		return PS8815_PRODUCT_ID;
	}

	CPRINTS("%s: Any new product id is not defined here?", __func__);
	return 0;
}

void ps8xxx_tcpc_update_hpd_status(int port, int hpd_lvl, int hpd_irq)
{
	dp_set_hpd(port, hpd_lvl);

	if (hpd_irq) {
		uint64_t now = get_time().val;
		/* wait for the minimum spacing between IRQ_HPD if needed */
		if (now < hpd_deadline[port])
			usleep(hpd_deadline[port] - now);

		dp_set_irq(port, 0);
		usleep(HPD_DSTREAM_DEBOUNCE_IRQ);
		dp_set_irq(port, hpd_irq);
	}
	/* enforce 2-ms delay between HPD pulses */
	hpd_deadline[port] = get_time().val + HPD_USTREAM_DEBOUNCE_LVL;
}

static int ps8xxx_tcpc_bist_mode_2(int port)
{
	int rv;

	/* Generate BIST for 50ms. */
	rv = tcpc_write(port,
		PS8XXX_REG_BIST_CONT_MODE_BYTE0, PS8751_BIST_COUNTER_BYTE0);
	rv |= tcpc_write(port,
		PS8XXX_REG_BIST_CONT_MODE_BYTE1, PS8751_BIST_COUNTER_BYTE1);
	rv |= tcpc_write(port,
		PS8XXX_REG_BIST_CONT_MODE_BYTE2, PS8751_BIST_COUNTER_BYTE2);

	/* Auto stop */
	rv |= tcpc_write(port, PS8XXX_REG_BIST_CONT_MODE_CTR, 0);

	/* Start BIST MODE 2 */
	rv |= tcpc_write(port, TCPC_REG_TRANSMIT, TCPC_TX_BIST_MODE_2);

	return rv;
}

static int ps8xxx_tcpm_transmit(int port, enum tcpm_transmit_type type,
			uint16_t header, const uint32_t *data)
{
	if (type == TCPC_TX_BIST_MODE_2)
		return ps8xxx_tcpc_bist_mode_2(port);
	else
		return tcpci_tcpm_transmit(port, type, header, data);
}

static int ps8xxx_tcpm_release(int port)
{
	int version;
	int status;
	int reg = get_reg_by_product(port, REG_FW_VER);

	status = tcpc_read(port, reg, &version);
	if (status != 0) {
		/* wait for chip to wake up */
		msleep(10);
	}

	return tcpci_tcpm_release(port);
}

static int ps8xxx_get_chip_info(int port, int live,
			struct ec_response_pd_chip_info_v1 **chip_info)
{
	int val;
	int reg;
	int rv = tcpci_get_chip_info(port, live, chip_info);

	if (rv)
		return rv;

	if (!live) {
		product_id[port] = board_get_ps8xxx_product_id(port);
		(*chip_info)->vendor_id = PS8XXX_VENDOR_ID;
		(*chip_info)->product_id = product_id[port];
	}

	if ((*chip_info)->fw_version_number == 0 ||
	    (*chip_info)->fw_version_number == -1 || live) {
		reg = get_reg_by_product(port, REG_FW_VER);
		rv = tcpc_read(port, reg, &val);
		if (rv != EC_SUCCESS)
			return rv;

		(*chip_info)->fw_version_number = val;
	}

	/* Treat unexpected values as error (FW not initiated from reset) */
	if (live && (
	    (*chip_info)->vendor_id != PS8XXX_VENDOR_ID ||
	    (*chip_info)->product_id != board_get_ps8xxx_product_id(port) ||
	    (*chip_info)->fw_version_number == 0))
		return EC_ERROR_UNKNOWN;

#if defined(CONFIG_USB_PD_TCPM_PS8751) && \
	defined(CONFIG_USB_PD_VBUS_DETECT_TCPC)
	/*
	 * Min firmware version of PS8751 to ensure that it can detect Vbus
	 * properly. See b/109769787#comment7
	 */
	(*chip_info)->min_req_fw_version_number = 0x39;
#endif

	return rv;
}

static int ps8xxx_dci_disable(int port)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(variant_map); i++) {
		if (product_id[port] == variant_map[i].product_id)
			return variant_map[i].dci_disable_ptr(port);
	}

	CPRINTS("%s: failed to get dci_disable function pointers.", __func__);
	return EC_ERROR_INVAL;
}

static int ps8xxx_tcpm_init(int port)
{
	int status;

	product_id[port] = board_get_ps8xxx_product_id(port);

	status = tcpci_tcpm_init(port);
	if (status != EC_SUCCESS)
		return status;

	return ps8xxx_dci_disable(port);
}

const struct tcpm_drv ps8xxx_tcpm_drv = {
	.init			= &ps8xxx_tcpm_init,
	.release		= &ps8xxx_tcpm_release,
	.get_cc			= &tcpci_tcpm_get_cc,
#ifdef CONFIG_USB_PD_VBUS_DETECT_TCPC
	.get_vbus_level		= &tcpci_tcpm_get_vbus_level,
#endif
	.select_rp_value	= &tcpci_tcpm_select_rp_value,
	.set_cc			= &tcpci_tcpm_set_cc,
	.set_polarity		= &tcpci_tcpm_set_polarity,
	.set_vconn		= &tcpci_tcpm_set_vconn,
	.set_msg_header		= &tcpci_tcpm_set_msg_header,
	.set_rx_enable		= &tcpci_tcpm_set_rx_enable,
	.get_message_raw	= &tcpci_tcpm_get_message_raw,
	.transmit		= &ps8xxx_tcpm_transmit,
	.tcpc_alert		= &tcpci_tcpc_alert,
#ifdef CONFIG_USB_PD_DISCHARGE_TCPC
	.tcpc_discharge_vbus	= &tcpci_tcpc_discharge_vbus,
#endif
#ifdef CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE
	.drp_toggle		= &tcpci_tcpc_drp_toggle,
#endif
#ifdef CONFIG_USBC_PPC
	.set_snk_ctrl		= &tcpci_tcpm_set_snk_ctrl,
	.set_src_ctrl		= &tcpci_tcpm_set_src_ctrl,
#endif
	.get_chip_info		= &ps8xxx_get_chip_info,
#ifdef CONFIG_USB_PD_TCPC_LOW_POWER
	.enter_low_power_mode	= &tcpci_enter_low_power_mode,
#endif
};

#ifdef CONFIG_CMD_I2C_STRESS_TEST_TCPC
struct i2c_stress_test_dev ps8xxx_i2c_stress_test_dev = {
	.reg_info = {
		.read_reg = PS8XXX_REG_VENDOR_ID_L,
		.read_val = PS8XXX_VENDOR_ID & 0xFF,
		.write_reg = MUX_IN_HPD_ASSERTION_REG,
	},
	.i2c_read = &tcpc_i2c_read,
	.i2c_write = &tcpc_i2c_write,
};
#endif /* CONFIG_CMD_I2C_STRESS_TEST_TCPC */

static int ps8xxx_mux_init(int port)
{
	tcpci_tcpm_mux_init(port);

	/* If this MUX is also the TCPC, then skip init */
	if (!(usb_muxes[port].flags & USB_MUX_FLAG_NOT_TCPC))
		return EC_SUCCESS;

	/* We always want to be a sink when this device is only being used as a mux
	 * to support external peripherals better.
	 */
	return mux_write(port, TCPC_REG_ROLE_CTRL,
		TCPC_REG_ROLE_CTRL_SET(0, 1, TYPEC_CC_RD, TYPEC_CC_RD));
}

static int ps8xxx_mux_enter_low_power_mode(int port)
{
	mux_write(port, TCPC_REG_ROLE_CTRL,
		TCPC_REG_ROLE_CTRL_SET(0, 0, TYPEC_CC_RP, TYPEC_CC_RP));
	return tcpci_tcpm_mux_enter_low_power(port);
}

/* This is meant for mux-only applications */
const struct usb_mux_driver ps8xxx_usb_mux_driver = {
	.init = &ps8xxx_mux_init,
	.set = &tcpci_tcpm_mux_set,
	.get = &tcpci_tcpm_mux_get,
	.enter_low_power_mode = &ps8xxx_mux_enter_low_power_mode,
};
