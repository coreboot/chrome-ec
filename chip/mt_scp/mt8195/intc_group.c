/* Copyright 2022 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "intc_group.h"

static struct intc_irq_group irqs[SCP_INTC_IRQ_COUNT] = {
	/* 0 */
	[SCP_IRQ_GIPC_IN0] = { INTC_GRP_7 },
	[SCP_IRQ_GIPC_IN1] = { INTC_GRP_0 },
	[SCP_IRQ_GIPC_IN2] = { INTC_GRP_0 },
	[SCP_IRQ_GIPC_IN3] = { INTC_GRP_0 },
	/* 4 */
	[SCP_IRQ_SPM] = { INTC_GRP_0 },
	[SCP_IRQ_AP_CIRQ] = { INTC_GRP_0 },
	[SCP_IRQ_EINT] = { INTC_GRP_0 },
	[SCP_IRQ_PMIC] = { INTC_GRP_0 },
	/* 8 */
	[SCP_IRQ_UART0_TX] = { INTC_GRP_12 },
	[SCP_IRQ_UART1_TX] = { INTC_GRP_12 },
	[SCP_IRQ_I2C0] = { INTC_GRP_0 },
	[SCP_IRQ_I2C1_0] = { INTC_GRP_0 },
	/* 12 */
	[SCP_IRQ_BUS_DBG_TRACKER] = { INTC_GRP_0 },
	[SCP_IRQ_CLK_CTRL] = { INTC_GRP_0 },
	[SCP_IRQ_VOW] = { INTC_GRP_0 },
	[SCP_IRQ_TIMER0] = { INTC_GRP_6 },
	/* 16 */
	[SCP_IRQ_TIMER1] = { INTC_GRP_6 },
	[SCP_IRQ_TIMER2] = { INTC_GRP_6 },
	[SCP_IRQ_TIMER3] = { INTC_GRP_6 },
	[SCP_IRQ_TIMER4] = { INTC_GRP_6 },
	/* 20 */
	[SCP_IRQ_TIMER5] = { INTC_GRP_6 },
	[SCP_IRQ_OS_TIMER] = { INTC_GRP_0 },
	[SCP_IRQ_UART0_RX] = { INTC_GRP_12 },
	[SCP_IRQ_UART1_RX] = { INTC_GRP_12 },
	/* 24 */
	[SCP_IRQ_GDMA] = { INTC_GRP_0 },
	[SCP_IRQ_AUDIO] = { INTC_GRP_0 },
	[SCP_IRQ_MD_DSP] = { INTC_GRP_0 },
	[SCP_IRQ_ADSP] = { INTC_GRP_0 },
	/* 28 */
	[SCP_IRQ_CPU_TICK] = { INTC_GRP_0 },
	[SCP_IRQ_SPI0] = { INTC_GRP_0 },
	[SCP_IRQ_SPI1] = { INTC_GRP_0 },
	[SCP_IRQ_SPI2] = { INTC_GRP_0 },
	/* 32 */
	[SCP_IRQ_NEW_INFRA_SYS_CIRQ] = { INTC_GRP_0 },
	[SCP_IRQ_DBG] = { INTC_GRP_0 },
	[SCP_IRQ_GCE] = { INTC_GRP_0 },
	[SCP_IRQ_MDP_GCE] = { INTC_GRP_0 },
	/* 36 */
	[SCP_IRQ_VDEC] = { INTC_GRP_8 },
	[SCP_IRQ_WDT] = { INTC_GRP_0 },
	[SCP_IRQ_VDEC_LAT] = { INTC_GRP_8 },
	[SCP_IRQ_VDEC1] = { INTC_GRP_8 },
	/* 40 */
	[SCP_IRQ_VDEC1_LAT] = { INTC_GRP_8 },
	[SCP_IRQ_INFRA] = { INTC_GRP_0 },
	[SCP_IRQ_CLK_CTRL_CORE] = { INTC_GRP_0 },
	[SCP_IRQ_CLK_CTRL2_CORE] = { INTC_GRP_0 },
	/* 44 */
	[SCP_IRQ_CLK_CTRL2] = { INTC_GRP_0 },
	[SCP_IRQ_GIPC_IN4] = { INTC_GRP_0 },
	[SCP_IRQ_PERIBUS_TIMEOUT] = { INTC_GRP_0 },
	[SCP_IRQ_INFRABUS_TIMEOUT] = { INTC_GRP_0 },
	/* 48 */
	[SCP_IRQ_MET0] = { INTC_GRP_0 },
	[SCP_IRQ_MET1] = { INTC_GRP_0 },
	[SCP_IRQ_MET2] = { INTC_GRP_0 },
	[SCP_IRQ_MET3] = { INTC_GRP_0 },
	/* 52 */
	[SCP_IRQ_AP_WDT] = { INTC_GRP_0 },
	[SCP_IRQ_L2TCM_SEC_VIO] = { INTC_GRP_0 },
	[SCP_IRQ_VDEC_INT_LINE_CNT] = { INTC_GRP_0 },
	[SCP_IRQ_VOW_DATAIN] = { INTC_GRP_0 },
	/* 56 */
	[SCP_IRQ_I3C0_IBI_WAKE] = { INTC_GRP_0 },
	[SCP_IRQ_I3C1_IBI_WAKE] = { INTC_GRP_0 },
	[SCP_IRQ_VENC] = { INTC_GRP_8 },
	[SCP_IRQ_APU_ENGINE] = { INTC_GRP_0 },
	/* 60 */
	[SCP_IRQ_MBOX0] = { INTC_GRP_0 },
	[SCP_IRQ_MBOX1] = { INTC_GRP_0 },
	[SCP_IRQ_MBOX2] = { INTC_GRP_0 },
	[SCP_IRQ_MBOX3] = { INTC_GRP_0 },
	/* 64 */
	[SCP_IRQ_MBOX4] = { INTC_GRP_0 },
	[SCP_IRQ_SYS_CLK_REQ] = { INTC_GRP_0 },
	[SCP_IRQ_BUS_REQ] = { INTC_GRP_0 },
	[SCP_IRQ_APSRC_REQ] = { INTC_GRP_0 },
	/* 68 */
	[SCP_IRQ_APU_MBOX] = { INTC_GRP_0 },
	[SCP_IRQ_DEVAPC_SECURE_VIO] = { INTC_GRP_0 },
	[SCP_IRQ_APDMA0] = { INTC_GRP_0 },
	[SCP_IRQ_APDMA1] = { INTC_GRP_0 },
	/* 72 */
	[SCP_IRQ_APDMA2] = { INTC_GRP_0 },
	[SCP_IRQ_APDMA3] = { INTC_GRP_0 },
	[SCP_IRQ_APDMA4] = { INTC_GRP_0 },
	[SCP_IRQ_APDMA5] = { INTC_GRP_0 },
	/* 76 */
	[SCP_IRQ_HDMIRX_PM_DVI_SQH] = { INTC_GRP_0 },
	[SCP_IRQ_HDMIRX_RESERVED] = { INTC_GRP_0 },
	[SCP_IRQ_NNA0_0] = { INTC_GRP_0 },
	[SCP_IRQ_NNA0_1] = { INTC_GRP_0 },
	/* 80 */
	[SCP_IRQ_NNA0_2] = { INTC_GRP_0 },
	[SCP_IRQ_NNA1_0] = { INTC_GRP_0 },
	[SCP_IRQ_NNA1_1] = { INTC_GRP_0 },
	[SCP_IRQ_NNA1_2] = { INTC_GRP_0 },
	/* 84 */
	[SCP_IRQ_JPEGENC] = { INTC_GRP_0 },
	[SCP_IRQ_JPEGDEC] = { INTC_GRP_0 },
	[SCP_IRQ_JPEGDEC_C2] = { INTC_GRP_0 },
	[SCP_IRQ_VENC_C1] = { INTC_GRP_8 },
	/* 88 */
	[SCP_IRQ_JPEGENC_C1] = { INTC_GRP_0 },
	[SCP_IRQ_JPEGDEC_C1] = { INTC_GRP_0 },
	[SCP_IRQ_HDMITX] = { INTC_GRP_0 },
	[SCP_IRQ_HDMI2] = { INTC_GRP_0 },
	/* 92 */
	[SCP_IRQ_EARC] = { INTC_GRP_0 },
	[SCP_IRQ_CEC] = { INTC_GRP_0 },
	[SCP_IRQ_HDMI_DEV_DET] = { INTC_GRP_0 },
	[SCP_IRQ_HDMIRX_OUT_ARM_PHY] = { INTC_GRP_0 },
	/* 96 */
	[SCP_IRQ_I2C2] = { INTC_GRP_0 },
	[SCP_IRQ_I2C3] = { INTC_GRP_0 },
	[SCP_IRQ_I3C2_IBI_WAKE] = { INTC_GRP_0 },
	[SCP_IRQ_I3C3_IBI_WAKE] = { INTC_GRP_0 },
	/* 100 */
	[SCP_IRQ_SYS_I2C_0] = { INTC_GRP_0 },
	[SCP_IRQ_SYS_I2C_1] = { INTC_GRP_0 },
	[SCP_IRQ_SYS_I2C_2] = { INTC_GRP_0 },
	[SCP_IRQ_SYS_I2C_3] = { INTC_GRP_0 },
	/* 104 */
	[SCP_IRQ_SYS_I2C_4] = { INTC_GRP_0 },
	[SCP_IRQ_SYS_I2C_5] = { INTC_GRP_0 },
	[SCP_IRQ_SYS_I2C_6] = { INTC_GRP_0 },
	[SCP_IRQ_SYS_I2C_7] = { INTC_GRP_0 },
	/* 108 */
	[SCP_IRQ_DISP2ADSP_0] = { INTC_GRP_0 },
	[SCP_IRQ_DISP2ADSP_1] = { INTC_GRP_0 },
	[SCP_IRQ_DISP2ADSP_2] = { INTC_GRP_0 },
	[SCP_IRQ_DISP2ADSP_3] = { INTC_GRP_0 },
	/* 112 */
	[SCP_IRQ_DISP2ADSP_4] = { INTC_GRP_0 },
	[SCP_IRQ_VDO1_DISP_MON2ADSP_0] = { INTC_GRP_0 },
	[SCP_IRQ_VDO1_DISP_MON2ADSP_1] = { INTC_GRP_0 },
	[SCP_IRQ_VDO1_DISP_MON2ADSP_2] = { INTC_GRP_0 },
	/* 116 */
	[SCP_IRQ_GCE1_SECURE] = { INTC_GRP_0 },
	[SCP_IRQ_GCE_SECURE] = { INTC_GRP_0 },
};
BUILD_ASSERT(ARRAY_SIZE(irqs) == SCP_INTC_IRQ_COUNT);

uint8_t intc_irq_group_get(int irq)
{
	return irqs[irq].group;
}
