/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * INT_AP_L extension
 */

#include "config.h"
#include "console.h"
#include "gpio.h"
#include "registers.h"
#include "stdbool.h"
#include "task.h"
#include "timer.h"

/*
 * Minimum time length (in microseconds) of INT_AP_L pulse required by CPU.
 * It is set to be 100 microseconds, which a requirement of INTL TGL, GPIO
 * in low power mode.
 */
#define PULSE_LENGTH                100ul

#define USEC_TO_TIMEHS_TICKS(usec)  ((usec) * (PCLK_FREQ / SEC_UL))

/* INT_AP_L pulse length in TIMEHS ticks. 0 means the extension is disabled. */
static uint32_t pulse_length;

/* true if INT_AP_L is asserted, or false if it is deasserted. */
static bool int_ap_asserted_;

/*
 * true if INT_AP assertion was requested but delayed for a process, or
 * false if there is no delayed INT_AP assertion request.
 */
static bool assertion_delayed_;

/*
 * true if the high-speed timer for int_ap pulse (timehs0_timer1) is enabled, or
 * false otherwise.
 */
static bool timer_running_;

/* function that should be called when INT_AP_L extension is enabled. */
static void (*interface_func_enable_)(void);

/*
 * Change INT_AP_L level, and set a timer for PULSE_LENGTH
 * @param do_assert  true asserts INT_AP_L (LOW)
 *                   false deasserts INT_AP_L (HIGH)
 */
static void set_int_ap_(bool do_assert)
{
	/* Set INT_AP_L level. */
	int_ap_asserted_ = do_assert;
	/* It is asserted when low. */
	gpio_set_level(GPIO_INT_AP_L, !int_ap_asserted_);

	/* Schedule to toggle INT_AP_L. */
	GR_TIMEHS_LOAD(0, 1) = pulse_length;
	GR_TIMEHS_CONTROL(0, 1) = GC_TIMEHS_TIMER1CONTROL_ONESHOT_MASK |
				  GC_TIMEHS_TIMER1CONTROL_SIZE_MASK |
				  GC_TIMEHS_TIMER1CONTROL_INTENABLE_MASK |
				  GC_TIMEHS_TIMER1CONTROL_ENABLE_MASK;
	timer_running_ = true;
}

static void disable_timer_(void)
{
	/* Disable Timer. */
	GR_TIMEHS_CONTROL(0, 1) = 0;

	/* Clear interrupt status of TIMEHS0 TIMER1. */
	GR_TIMEHS_INTCLR(0, 1) = 1;

	timer_running_ = false;
}

/* Interrupt handler of timehs0_timint1, a timer for INT_AP_L extension. */
void timer_int_ap_irq_handler(void)
{
	disable_timer_();

	if (!int_ap_asserted_) {
		/*
		 * While INT_AP_L is being deasserted, if assertion was
		 * requested (and delayed), then toggle the signal.
		 * Otherwise, just return without changing INT_AP_L level.
		 */
		if (!assertion_delayed_)
			return;

		assertion_delayed_ = false;
	}

	/* Toggle the INT_AP_L level. */
	set_int_ap_(!int_ap_asserted_);
}
DECLARE_IRQ(GC_IRQNUM_TIMEHS0_TIMINT1, timer_int_ap_irq_handler, 1);

int assert_int_ap(void)
{
#ifdef CR50_DEV
	if (!in_interrupt_context())
		ccprintf("WARN: %s in non-ISR ctx.", __func__);
#endif
	/* If the INT_AP_L extension is not enabled, then just return 0. */
	if (!pulse_length)
		return 0;

	if (int_ap_asserted_) {
#ifdef CR50_DEV
		ccprintf("WARN: INT_AP_L assertion request is duplicated.");
#endif
		return 1;
	}

	/*
	 * If the timer is running, it means INT_AP_L deassertion pulse isn't
	 * long enough yet. If so, let's delay to assert.
	 * Otherwise, assert INT_AP_L immediately.
	 */
	if (timer_running_)
		assertion_delayed_ = true;
	else
		set_int_ap_(true);

	return 1;
}

void deassert_int_ap(void)
{
#ifdef CR50_DEV
	if (!in_interrupt_context())
		ccprintf("WARN: %s in non-ISR ctx", __func__);
#endif
	/* If INT_AP_L is deasserted already, do nothing. */
	if (!int_ap_asserted_) {
		assertion_delayed_ = false;
		return;
	}
	timer_int_ap_irq_handler();
}

void int_ap_register(void (*func_enable)(void))
{
	interface_func_enable_ = func_enable;
}

void int_ap_extension_enable(void)
{
	int_ap_extension_stop_pulse();

	pulse_length = USEC_TO_TIMEHS_TICKS(PULSE_LENGTH);

	task_enable_irq(GC_IRQNUM_TIMEHS0_TIMINT1);

	if (interface_func_enable_)
		interface_func_enable_();
}

void int_ap_extension_stop_pulse(void)
{
	disable_timer_();

	/* Initialize INT_AP_L status. */
	int_ap_asserted_ = false;
	gpio_set_level(GPIO_INT_AP_L, 1);

	assertion_delayed_ = false;
}
