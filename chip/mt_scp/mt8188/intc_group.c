/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "intc_group.h"

static struct intc_irq_group irqs[SCP_INTC_IRQ_COUNT] = {
/* 0 */
#ifdef BOARD_GERALT_SCP_CORE1
	[SCP_IRQ_GIPC_IN0] = { INTC_GRP_0 },
	[SCP_IRQ_GIPC_IN1] = { INTC_GRP_7 },
	[SCP_IRQ_GIPC_IN2] = { INTC_GRP_0 },
	[SCP_IRQ_GIPC_IN3] = { INTC_GRP_11 },
#else
	[SCP_IRQ_GIPC_IN0] = { INTC_GRP_7 },
	[SCP_IRQ_GIPC_IN1] = { INTC_GRP_0 },
	[SCP_IRQ_GIPC_IN2] = { INTC_GRP_11 },
	[SCP_IRQ_GIPC_IN3] = { INTC_GRP_0 },
#endif
	/* 4 */
	[SCP_IRQ_SPM] = { INTC_GRP_0 },
	[SCP_IRQ_AP_CIRQ] = { INTC_GRP_0 },
	[SCP_IRQ_EINT] = { INTC_GRP_0 },
	[SCP_IRQ_PMIC] = { INTC_GRP_0 },
	/* 8 */
	[SCP_IRQ_UART0_TX] = { INTC_GRP_12 },
	[SCP_IRQ_UART1_TX] = { INTC_GRP_12 },
	[SCP_IRQ_I3C0] = { INTC_GRP_0 },
	[SCP_IRQ_I2C0] = { INTC_GRP_0 },
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
	[SCP_IRQ_VDEC_INT_LINE_CNT] = { INTC_GRP_0 },
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
	[SCP_IRQ_VDEC] = { INTC_GRP_0 },
	[SCP_IRQ_WDT] = { INTC_GRP_0 },
	[SCP_IRQ_VDEC_LAT] = { INTC_GRP_0 },
	[SCP_IRQ_GCE_SECURE] = { INTC_GRP_0 },
	/* 40 */
	[SCP_IRQ_GCE1_SECURE] = { INTC_GRP_0 },
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
	[SCP_IRQ_CPU_TICK1] = { INTC_GRP_0 },
	[SCP_IRQ_VOW_DATAIN] = { INTC_GRP_0 },
	/* 56 */
	[SCP_IRQ_I3C0_IBI_WAKE] = { INTC_GRP_0 },
	[SCP_IRQ_I2C0_IBI_WAKE] = { INTC_GRP_0 },
	[SCP_IRQ_VENC] = { INTC_GRP_0 },
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
	[SCP_IRQ_CAMSYS_29] = { INTC_GRP_0 },
	[SCP_IRQ_CAMSYS_28] = { INTC_GRP_0 },
	/* 72 */
	[SCP_IRQ_CAMSYS_5] = { INTC_GRP_0 },
	[SCP_IRQ_CAMSYS_4] = { INTC_GRP_0 },
	[SCP_IRQ_CAMSYS_3] = { INTC_GRP_0 },
	[SCP_IRQ_CAMSYS_2] = { INTC_GRP_0 },
	/* 76 */
	[SCP_IRQ_SMI_LARB7] = { INTC_GRP_0 },
	[SCP_IRQ_WPE_VPP0] = { INTC_GRP_0 },
	[SCP_IRQ_DP_TX] = { INTC_GRP_0 },
	[SCP_IRQ_EDP_TX] = { INTC_GRP_0 },
	/* 80 */
	[SCP_IRQ_VPP0_0] = { INTC_GRP_0 },
	[SCP_IRQ_VPP0_12] = { INTC_GRP_0 },
	[SCP_IRQ_VPP0_14] = { INTC_GRP_0 },
	[SCP_IRQ_MSDC2] = { INTC_GRP_0 },
	/* 84 */
	[SCP_IRQ_JPEGENC] = { INTC_GRP_0 },
	[SCP_IRQ_JPEGDEC] = { INTC_GRP_0 },
	[SCP_IRQ_HDMITX] = { INTC_GRP_0 },
	[SCP_IRQ_CEC] = { INTC_GRP_0 },
	/* 88 */
	[SCP_IRQ_I2C_7] = { INTC_GRP_0 },
	[SCP_IRQ_I2C_8] = { INTC_GRP_0 },
	[SCP_IRQ_I2C_9] = { INTC_GRP_0 },
	[SCP_IRQ_I2C_10] = { INTC_GRP_0 },
	/* 92 */
	[SCP_IRQ_I2C_11] = { INTC_GRP_0 },
	[SCP_IRQ_I2C_12] = { INTC_GRP_0 },
	[SCP_IRQ_I2C_13] = { INTC_GRP_0 },
	[SCP_IRQ_IOMMU_SLOW_BANK0] = { INTC_GRP_0 },
	/* 96 */
	[SCP_IRQ_IOMMU_SLOW_BANK1] = { INTC_GRP_0 },
	[SCP_IRQ_IOMMU_SLOW_BANK2] = { INTC_GRP_0 },
	[SCP_IRQ_IOMMU_SLOW_BANK3] = { INTC_GRP_0 },
	[SCP_IRQ_IOMMU_SLOW_BANK4] = { INTC_GRP_0 },
	/* 100 */
	[SCP_IRQ_VDO0_OVL0] = { INTC_GRP_0 },
	[SCP_IRQ_VDO0_WDMA0] = { INTC_GRP_0 },
	[SCP_IRQ_VDO0_RDMA0] = { INTC_GRP_0 },
	[SCP_IRQ_VDO0_DSI0] = { INTC_GRP_0 },
	/* 104 */
	[SCP_IRQ_VDO0_DSC_CORE0] = { INTC_GRP_0 },
	[SCP_IRQ_VDO0_DSI1] = { INTC_GRP_0 },
	[SCP_IRQ_VDO0_DSC_CORE1] = { INTC_GRP_0 },
	[SCP_IRQ_VDO0_DPINTF0] = { INTC_GRP_0 },
	/* 108 */
	[SCP_IRQ_VDO0_MUTEX] = { INTC_GRP_0 },
	[SCP_IRQ_MMSYS_MUTEX] = { INTC_GRP_0 },
	[SCP_IRQ_MMSYS_RDMA0] = { INTC_GRP_0 },
	[SCP_IRQ_MMSYS_RDMA1] = { INTC_GRP_0 },
	/* 112 */
	[SCP_IRQ_MMSYS_RDMA4] = { INTC_GRP_0 },
	[SCP_IRQ_MMSYS_RDMA5] = { INTC_GRP_0 },
	[SCP_IRQ_MMSYS_MERGE4] = { INTC_GRP_0 },
	[SCP_IRQ_MMSYS_DPI0] = { INTC_GRP_0 },
	/* 116 */
	[SCP_IRQ_MMSYS_DPI1] = { INTC_GRP_0 },
	[SCP_IRQ_MMSYS_DPINTF] = { INTC_GRP_0 },
	[SCP_IRQ_VPPSYS_RDMA2] = { INTC_GRP_0 },
	[SCP_IRQ_VPPSYS_RDMA3] = { INTC_GRP_0 },
	/* 120 */
	[SCP_IRQ_VPPSYS_WROT2] = { INTC_GRP_0 },
	[SCP_IRQ_VPPSYS_WROT3] = { INTC_GRP_0 },
	[SCP_IRQ_VPPSYS_MUTEX] = { INTC_GRP_0 },
};
BUILD_ASSERT(ARRAY_SIZE(irqs) == SCP_INTC_IRQ_COUNT);

uint8_t intc_irq_group_get(int irq)
{
	return irqs[irq].group;
}
