/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Clocks and power management settings */

#ifndef __CROS_EC_CLOCK_H
#define __CROS_EC_CLOCK_H

#include "common.h"

/**
 * Set the CPU clocks and PLLs.
 */
int clock_init(void);

/**
 * Return the current clock frequency in Hz.
 */
int clock_get_freq(void);

/**
 * Enable or disable the PLL.
 *
 * @param enable	Enable PLL if non-zero; disable if zero.
 * @param notify	Notify other modules of the PLL change.  This should
 *			be 1 unless you're briefly turning on the PLL to work
 *			around a chip errata at init time.
 */
int clock_enable_pll(int enable, int notify);

/**
 * Wait for a number of clock cycles.
 *
 * Simple busy waiting for use before clocks/timers are initialized.
 *
 * @param cycles	Number of cycles to wait.
 */
void clock_wait_cycles(uint32_t cycles);

#endif  /* __CROS_EC_CLOCK_H */
