/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Silergy SYV682x Type-C Power Path Controller */

#ifndef __CROS_EC_SYV682X_H
#define __CROS_EC_SYV682X_H

/* 8 bit I2C addresses */
#define SYV682X_ADDR0			0x80
#define SYV682X_ADDR1			0x82
#define SYV682X_ADDR2			0x84
#define SYV682x_ADDR3			0x86

/* SYV682x register addresses */
#define SYV682X_STATUS_REG		0x00
#define SYV682X_CONTROL_1_REG		0x01
#define SYV682X_CONTROL_2_REG		0x02
#define SYV682X_CONTROL_3_REG		0x03
#define SYV682X_CONTROL_4_REG		0x04

/* Status Register */
#define SYV682X_STATUS_OC_HV		BIT(7)
#define SYV682X_STATUS_RVS		BIT(6)
#define SYV682X_STATUS_OC_5V		BIT(5)
#define SYV682X_STATUS_OVP		BIT(4)
#define SYV682X_STATUS_FRS		BIT(3)
#define SYV682X_STATUS_TSD		BIT(2)
#define SYV682X_STATUS_VSAFE_5V		(1 << 1)
#define SYV682X_STATUS_VSAFE_0V		(1 << 0)
#define SYV682X_STATUS_INT_MASK		0xfc

/* Control Register 1 */
#define SYV682X_CONTROL_1_CH_SEL	(1 << 1)
#define SYV682X_CONTROL_1_HV_DR		(1 << 2)
#define SYV682X_CONTROL_1_PWR_ENB	(1 << 7)

#define SYV682X_5V_ILIM_MASK		0x18
#define SYV682X_5V_ILIM_BIT_SHIFT	3
#define SYV682X_5V_ILIM_1_25		0
#define SYV682X_5V_ILIM_1_75		1
#define SYV682X_5V_ILIM_2_25		2
#define SYV682X_5V_ILIM_3_30		3

#define SYV682X_HV_ILIM_MASK		0x60
#define SYV682X_HV_ILIM_BIT_SHIFT	5
#define SYV682X_HV_ILIM_1_25		0
#define SYV682X_HV_ILIM_1_75		1
#define SYV682X_HV_ILIM_3_30		2
#define SYV682X_HV_ILIM_5_50		3

/* Control Register 2 */
#define SYV682X_OC_DELAY_MASK		GENMASK(7, 6)
#define SYV682X_OC_DELAY_SHIFT		6
#define SYV682X_OC_DELAY_1MS		0
#define SYV682X_OC_DELAY_10MS		1
#define SYV682X_OC_DELAY_50MS		2
#define SYV682X_OC_DELAY_100MS		3
#define SYV682X_DSG_TIME_MASK		GENMASK(5, 4)
#define SYV682X_DSG_TIME_SHIFT		4
#define SYV682X_DSG_TIME_50MS		0
#define SYV682X_DSG_TIME_100MS		1
#define SYV682X_DSG_TIME_200MS		2
#define SYV682X_DSG_TIME_400MS		3
#define SYV682X_DSG_RON_MASK		GENMASK(3, 2)
#define SYV682X_DSG_RON_SHIFT		2
#define SYV682X_DSG_RON_200_OHM		0
#define SYV682X_DSG_RON_400_OHM		1
#define SYV682X_DSG_RON_800_OHM		2
#define SYV682X_DSG_RON_1600_OHM	3
#define SYV682X_CONTROL_2_SDSG		(1 << 1)
#define SYV682X_CONTROL_2_FDSG		(1 << 0)

/* Control Register 3 */
#define SYV682X_BUSY			BIT(7)
#define SYV682X_RST_REG			BIT(0)
#define SYV682X_OVP_MASK		0x70
#define SYV682X_OVP_BIT_SHIFT		4
#define SYV682X_OVP_06_0		0
#define SYV682X_OVP_08_0		1
#define SYV682X_OVP_11_1		2
#define SYV682X_OVP_12_1		3
#define SYV682X_OVP_14_2		4
#define SYV682X_OVP_17_9		5
#define SYV682X_OVP_21_6		6
#define SYV682X_OVP_23_7		7

/* Control Register 4 */
#define SYV682X_CONTROL_4_CC1_BPS	(1 << 7)
#define SYV682X_CONTROL_4_CC2_BPS	(1 << 6)
#define SYV682X_CONTROL_4_VCONN1	(1 << 5)
#define SYV682X_CONTROL_4_VCONN2	(1 << 4)
#define SYV682X_CONTROL_4_VBAT_OVP	(1 << 3)
#define SYV682X_CONTROL_4_VCONN_OCP	(1 << 2)
#define SYV682X_CONTROL_4_CC_FRS	(1 << 1)
#define SYV682X_CONTROL_4_INT_MASK	0x0c

struct ppc_drv;
extern const struct ppc_drv syv682x_drv;

void syv682x_interrupt(int port);

#endif /* defined(__CROS_EC_SYV682X_H) */
