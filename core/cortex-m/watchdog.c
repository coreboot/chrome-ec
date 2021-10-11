/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Watchdog common code */

#include "common.h"
#include "panic.h"
#include "system.h"
#include "task.h"
#include "timer.h"
#include "watchdog.h"

/* Panic data goes at the end of RAM. */
static struct panic_data * const pdata_ptr = PANIC_DATA_PTR;

void __keep watchdog_trace(uint32_t excep_lr, uint32_t excep_sp)
{
	uint32_t psp;
	uint32_t *stack;

	asm("mrs %0, psp" : "=r"(psp));
	if ((excep_lr & 0xf) == 1) {
		/* we were already in exception context */
		stack = (uint32_t *)excep_sp;
	} else {
		/* we were in task context */
		stack = (uint32_t *)psp;
	}

	panic_set_reason(PANIC_SW_WATCHDOG, stack[6],
			 (excep_lr & 0xf) == 1 ? 0xff : task_get_current());

	/* Copy task# to cm.hfsr[9:2]. */
	pdata_ptr->cm.hfsr = HFSR_FLAG_WATCHDOG << 26
			| (pdata_ptr->cm.regs[1] & 0xff) << 2;

	/*
	 * Store LR in cm.regs[1] (ipsr). IPSR holds exception# but we don't
	 * need it because the reason (=PANIC_SW_WATCHDOG) is already stored
	 * in cm.regs[3] (r4). Note this overwrites task# in cm.regs[1] stored
	 * by panic_set_reason.
	 */
	pdata_ptr->cm.regs[1] = stack[5];

	panic_printf("### WATCHDOG PC=%08x / LR=%08x / pSP=%08x ",
		     stack[6], stack[5], psp);
	if ((excep_lr & 0xf) == 1)
		panic_puts("(exc) ###\n");
	else
		panic_printf("(task %d) ###\n", task_get_current());

	/* If we are blocked in a high priority IT handler, the following debug
	 * messages might not appear but they are useless in that situation. */
	timer_print_info();
	task_print_list();
}
