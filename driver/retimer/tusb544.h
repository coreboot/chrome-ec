/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * TI TUSB544 USB Type-C Multi-Protocol Linear Redriver
 */
#include "usb_mux.h"

#ifndef __CROS_EC_USB_REDRIVER_TUSB544_H
#define __CROS_EC_USB_REDRIVER_TUSB544_H


#define TUSB544_I2C_ADDR_FLAGS0 0x44

#define TUSB544_REG_GENERAL4	0x0A
#define TUSB544_GEN4_CTL_SEL		GENMASK(1, 0)
#define TUSB544_GEN4_FLIP_SEL		BIT(2)
#define TUSB544_GEN4_HPDIN		BIT(3)
#define TUSB544_GEN4_EQ_OVRD		BIT(4)
#define TUSB544_GEN4_SWAP_SEL		BIT(5)

#define TUSB544_REG_DISPLAYPORT_1	0x10
#define TUSB544_REG_DISPLAYPORT_2	0x11
#define TUSB544_REG_USB3_1_1	0x20
#define TUSB544_REG_USB3_1_2	0x21
#define TUSB544_EQ_RX_DFP_MINUS14_UFP_MINUS33		(0)
#define TUSB544_EQ_RX_DFP_04_UFP_MINUS15		(1)
#define TUSB544_EQ_RX_DFP_17_UFP_0		(2)
#define TUSB544_EQ_RX_DFP_32_UFP_14		(3)
#define TUSB544_EQ_RX_DFP_41_UFP_24		(4)
#define TUSB544_EQ_RX_DFP_52_UFP_35		(5)
#define TUSB544_EQ_RX_DFP_61_UFP_43		(6)
#define TUSB544_EQ_RX_DFP_69_UFP_52		(7)
#define TUSB544_EQ_RX_DFP_77_UFP_60		(8)
#define TUSB544_EQ_RX_DFP_83_UFP_66		(9)
#define TUSB544_EQ_RX_DFP_88_UFP_72		(10)
#define TUSB544_EQ_RX_DFP_94_UFP_77		(11)
#define TUSB544_EQ_RX_DFP_98_UFP_81		(12)
#define TUSB544_EQ_RX_DFP_103_UFP_86		(13)
#define TUSB544_EQ_RX_DFP_106_UFP_90		(14)
#define TUSB544_EQ_RX_DFP_110_UFP_94		(15)
#define TUSB544_EQ_RX_MASK		(0x0F)

#define TUSB544_EQ_TX_DFP_MINUS14_UFP_MINUS33		(0 << 4)
#define TUSB544_EQ_TX_DFP_04_UFP_MINUS15		(1 << 4)
#define TUSB544_EQ_TX_DFP_17_UFP_0		(2 << 4)
#define TUSB544_EQ_TX_DFP_32_UFP_14		(3 << 4)
#define TUSB544_EQ_TX_DFP_41_UFP_24		(4 << 4)
#define TUSB544_EQ_TX_DFP_52_UFP_35		(5 << 4)
#define TUSB544_EQ_TX_DFP_61_UFP_43		(6 << 4)
#define TUSB544_EQ_TX_DFP_69_UFP_52		(7 << 4)
#define TUSB544_EQ_TX_DFP_77_UFP_60		(8 << 4)
#define TUSB544_EQ_TX_DFP_83_UFP_66		(9 << 4)
#define TUSB544_EQ_TX_DFP_88_UFP_72		(10 << 4)
#define TUSB544_EQ_TX_DFP_94_UFP_77		(11 << 4)
#define TUSB544_EQ_TX_DFP_98_UFP_81		(12 << 4)
#define TUSB544_EQ_TX_DFP_103_UFP_86		(13 << 4)
#define TUSB544_EQ_TX_DFP_106_UFP_90		(14 << 4)
#define TUSB544_EQ_TX_DFP_110_UFP_94		(15 << 4)
#define TUSB544_EQ_TX_MASK		(0xF0)

enum tusb544_ct_sel {
	TUSB544_CTL_SEL_DISABLED,
	TUSB544_CTL_SEL_USB_ONLY,
	TUSB544_CTL_SEL_DP_ONLY,
	TUSB544_CTL_SEL_DP_USB,
};

#define TUSB544_REG_GENERAL6	0x0C
#define TUSB544_GEN6_DIR_SEL	GENMASK(1, 0)

enum tusb544_dir_sel {
	TUSB544_DIR_SEL_USB_DP_SRC,
	TUSB544_DIR_SEL_USB_DP_SNK,
	TUSB544_DIR_SEL_CUSTOM_SRC,
	TUSB544_DIS_SEL_CUSTOM_SNK,
};

/*
 * Note: TUSB544 automatically snoops DP lanes to enable, but may be manually
 * directed which lanes to turn on when snoop is disabled
 */
#define TUSB544_REG_DP4			0x13
#define TUSB544_DP4_DP0_DISABLE		BIT(0)
#define TUSB544_DP4_DP1_DISABLE		BIT(1)
#define TUSB544_DP4_DP2_DISABLE		BIT(2)
#define TUSB544_DP4_DP3_DISABLE		BIT(3)
#define TUSB544_DP4_AUX_SBU_OVR		GENMASK(5, 4)
#define TUSB544_DP4_AUX_SNOOP_DISABLE	BIT(7)

extern const struct usb_mux_driver tusb544_drv;

int tusb544_i2c_field_update8(const struct usb_mux *me, int offset,
			     uint8_t field_mask, uint8_t set_value);

#endif
