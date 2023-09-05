/* Copyright 2022 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Richtek RT1739 Type-C Power Path Controller */

#ifndef __CROS_EC_PPC_RT1739_H
#define __CROS_EC_PPC_RT1739_H

#include "usb_charge.h"
#include "usbc_ppc.h"

#define RT1739_ADDR1_FLAGS 0x70
#define RT1739_ADDR2_FLAGS 0x71
#define RT1739_ADDR3_FLAGS 0x72
#define RT1739_ADDR4_FLAGS 0x73

#define RT1739_REG_DEVICE_ID0 0x02
#define RT1739_DEVICE_ID_ES1 0x11
#define RT1739_DEVICE_ID_ES2 0x12
#define RT1739_DEVICE_ID_ES4 0x14

#define RT1739_REG_SW_RESET 0x04
#define RT1739_SW_RESET BIT(0)

#define RT1739_REG_INT_MASK4 0x0C
#define RT1739_FRS_RX_MASK BIT(4)

#define RT1739_REG_INT_MASK5 0x0D
#define RT1739_BC12_SNK_DONE_MASK BIT(0)

#define RT1739_REG_INT_EVENT4 0x14

#define RT1739_REG_INT_EVENT5 0x15
#define RT1739_BC12_SNK_DONE_INT BIT(0)

#define RT1739_REG_INT_STS4 0x1C
#define RT1739_VBUS_VALID BIT(2)
#define RT1739_VBUS_PRESENT BIT(0)

#define RT1739_REG_SYS_CTRL 0x20
#define RT1739_OT_EN BIT(4)
#define RT1739_DEAD_BATTERY BIT(1)
#define RT1739_SHUTDOWN_OFF BIT(0)

#define RT1739_REG_VBUS_SWITCH_CTRL 0x21
#define RT1739_LV_SRC_EN BIT(2)
#define RT1739_HV_SRC_EN BIT(1)
#define RT1739_HV_SNK_EN BIT(0)

#define RT1739_REG_VBUS_CTRL1 0x23
#define RT1739_HVLV_SCP_EN BIT(1)
#define RT1739_HVLV_OCRC_EN BIT(0)

#define RT1739_REG_VBUS_OV_SETTING 0x24

#define RT1739_VBUS_OVP_SEL_SHIFT 0
#define RT1739_VIN_HV_OVP_SEL_SHIFT 4
#define RT1739_OVP_SEL_6_0V 0
#define RT1739_OVP_SEL_6_8V 1
#define RT1739_OVP_SEL_10_0V 2
#define RT1739_OVP_SEL_11_5V 3
#define RT1739_OVP_SEL_14_0V 4
#define RT1739_OVP_SEL_17_0V 5
#define RT1739_OVP_SEL_23_0V 6

#define RT1739_REG_VBUS_OC_SETTING 0x25

#define RT1739_OCP_TIMEOUT_SEL_SHIFT 6
#define RT1739_OCP_TIMEOUT_SEL_0_5MS (0 << RT1739_OCP_TIMEOUT_SEL_SHIFT)
#define RT1739_OCP_TIMEOUT_SEL_1MS (1 << RT1739_OCP_TIMEOUT_SEL_SHIFT)
#define RT1739_OCP_TIMEOUT_SEL_8MS (2 << RT1739_OCP_TIMEOUT_SEL_SHIFT)
#define RT1739_OCP_TIMEOUT_SEL_16MS (3 << RT1739_OCP_TIMEOUT_SEL_SHIFT)

#define RT1739_LV_SRC_OCP_SHIFT 4
#define RT1739_LV_SRC_OCP_MASK (3 << RT1739_LV_SRC_OCP_SHIFT)
#define RT1739_LV_SRC_OCP_SEL_1_25A (0 << RT1739_LV_SRC_OCP_SHIFT)
#define RT1739_LV_SRC_OCP_SEL_1_75A (1 << RT1739_LV_SRC_OCP_SHIFT)
#define RT1739_LV_SRC_OCP_SEL_2_25A (2 << RT1739_LV_SRC_OCP_SHIFT)
#define RT1739_LV_SRC_OCP_SEL_3_3A (3 << RT1739_LV_SRC_OCP_SHIFT)

#define RT1739_HV_SINK_OCP_SHIFT 0
#define RT1739_HV_SINK_OCP_SEL_1_25A (0 << RT1739_HV_SINK_OCP_SHIFT)
#define RT1739_HV_SINK_OCP_SEL_1_75A (1 << RT1739_HV_SINK_OCP_SHIFT)
#define RT1739_HV_SINK_OCP_SEL_3_3A (2 << RT1739_HV_SINK_OCP_SHIFT)
#define RT1739_HV_SINK_OCP_SEL_5_5A (3 << RT1739_HV_SINK_OCP_SHIFT)

#define RT1739_VBUS_FAULT_DIS 0x26
#define RT1739_OVP_DISVBUS_EN BIT(6)
#define RT1739_UVLO_DISVBUS_EN BIT(5)
#define RT1739_SRCP_DISVBUS_EN BIT(4)
#define RT1739_RCP_DISVBUS_EN BIT(3)
#define RT1739_SCP_DISVBUS_EN BIT(2)
#define RT1739_OCPS_DISVBUS_EN BIT(1)
#define RT1739_OCP_DISVBUS_EN BIT(0)

#define RT1739_REG_VBUS_DET_EN 0x27
#define RT1739_VBUS_SAFE5V_EN BIT(2)
#define RT1739_VBUS_SAFE0V_EN BIT(1)
#define RT1739_VBUS_PRESENT_EN BIT(0)

#define RT1739_REG_VBUS_DEG_TIME 0x2B
#define RT1739_FRS_SRCP_MASK BIT(7)
#define RT1739_FRS_OSCS_MASK BIT(6)

#define RT1739_REG_CC_FRS_CTRL1 0x2D
#define RT1739_FRS_RX_EN BIT(1)

#define RT1739_REG_VCONN_CTRL1 0x31
#define RT1739_VCONN_ORIENT BIT(1)
#define RT1739_VCONN_EN BIT(0)

#define RT1739_VCONN_ORIENT_CC1 MASK_SET
#define RT1739_VCONN_ORIENT_CC2 MASK_CLR

#define RT1739_REG_VCONN_CTRL3 0x33
#define RT1739_VCONN_CLIMIT_EN BIT(0)

#define RT1739_REG_VCONN_CTRL4 0x34
#define RT1739_VCONN_OCP_SEL_SHIFT 2
#define RT1739_VCONN_OCP_SEL_MASK (7 << RT1739_VCONN_OCP_SEL_SHIFT)
#define RT1739_VCONN_OCP_SEL_200MA (1 << RT1739_VCONN_OCP_SEL_SHIFT)
#define RT1739_VCONN_OCP_SEL_300MA (2 << RT1739_VCONN_OCP_SEL_SHIFT)
#define RT1739_VCONN_OCP_SEL_400MA (3 << RT1739_VCONN_OCP_SEL_SHIFT)
#define RT1739_VCONN_OCP_SEL_500MA (4 << RT1739_VCONN_OCP_SEL_SHIFT)
#define RT1739_VCONN_OCP_SEL_600MA (5 << RT1739_VCONN_OCP_SEL_SHIFT)
#define RT1739_VCONN_OCP_SEL_700MA (6 << RT1739_VCONN_OCP_SEL_SHIFT)
#define RT1739_VCONN_OCP_SEL_800MA (7 << RT1739_VCONN_OCP_SEL_SHIFT)

#define RT1739_REG_LVHVSW_OV_CTRL 0x36
#define RT1739_OT_SEL_LVL BIT(1)

#define RT1739_REG_SBU_CTRL_01 0x38
#define RT1739_SBUSW_MUX_SEL BIT(4)
#define RT1739_SBU2_SWEN BIT(3)
#define RT1739_SBU1_SWEN BIT(2)
#define RT1739_DM_SWEN BIT(1)
#define RT1739_DP_SWEN BIT(0)

#define RT1739_REG_BC12_SNK_FUNC 0x40
#define RT1739_BC12_SNK_EN BIT(7)

#define RT1739_REG_BC12_STAT 0x41
#define RT1739_PORT_STAT_MASK 0x0F
#define RT1739_PORT_STAT_SDP 0x0D
#define RT1739_PORT_STAT_CDP 0x0E
#define RT1739_PORT_STAT_DCP 0x0F

#define RT1739_REG_SYS_CTRL1 0x60
#define RT1739_OSC640K_FORCE_EN BIT(3)

extern const struct ppc_drv rt1739_ppc_drv;
extern const struct bc12_drv rt1739_bc12_drv;

void rt1739_interrupt(int port);

int rt1739_init(int port);

#endif /* defined(__CROS_EC_PPC_RT1739_H) */
