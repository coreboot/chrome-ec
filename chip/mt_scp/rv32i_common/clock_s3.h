/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CLOCK_S3_H
#define __CLOCK_S3_H

enum scp_clock_source {
	SCP_CLK_SYSTEM,
	SCP_CLK_32K,
	SCP_CLK_ULPOSC1,
	SCP_CLK_ULPOSC2_LOW_SPEED,
	SCP_CLK_ULPOSC2_HIGH_SPEED,
};

void clock_select_clock(enum scp_clock_source src);

#define TASK_EVENT_SUSPEND TASK_EVENT_CUSTOM_BIT(4)
#define TASK_EVENT_RESUME TASK_EVENT_CUSTOM_BIT(5)
#define TASK_EVENT_C1_READY TASK_EVENT_CUSTOM_BIT(6)

/* core 0 -> core 1 */
#define S3_IPI_SUSPEND 12
#define S3_IPI_RESUME 13

/* core 1 -> core 0 */
#define S3_IPI_READY 8

#endif /* __CLOCK_S3_H */
