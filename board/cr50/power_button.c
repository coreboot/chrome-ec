/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "ap_ro_integrity_check.h"
#include "console.h"
#include "extension.h"
#include "gpio.h"
#include "hooks.h"
#include "physical_presence.h"
#include "rbox.h"
#include "registers.h"
#include "system.h"
#include "system_chip.h"
#include "task.h"
#include "timer.h"
#include "u2f_impl.h"

#define CPRINTS(format, args...) cprints(CC_RBOX, format, ## args)
#define CPRINTF(format, args...) cprintf(CC_RBOX, format, ## args)

DECLARE_DEFERRED(deassert_ec_rst);

void power_button_release_enable_interrupt(int enable)
{
	/* Clear any leftover power button rising edge detection interrupts */
	GWRITE_FIELD(RBOX, INT_STATE, INTR_PWRB_IN_RED, 1);

	if (enable) {
		/* Enable power button rising edge detection interrupt */
		GWRITE_FIELD(RBOX, INT_ENABLE, INTR_PWRB_IN_RED, 1);
		task_enable_irq(GC_IRQNUM_RBOX0_INTR_PWRB_IN_RED_INT);
	} else {
		GWRITE_FIELD(RBOX, INT_ENABLE, INTR_PWRB_IN_RED, 0);
		task_disable_irq(GC_IRQNUM_RBOX0_INTR_PWRB_IN_RED_INT);
	}
}

/**
 * Enable/disable power button interrupt.
 *
 * @param enable	Enable (!=0) or disable (==0)
 */
static void power_button_press_enable_interrupt(int enable)
{
	if (enable) {
		/* Clear any leftover power button interrupts */
		GWRITE_FIELD(RBOX, INT_STATE, INTR_PWRB_IN_FED, 1);

		/* Enable power button interrupt */
		GWRITE_FIELD(RBOX, INT_ENABLE, INTR_PWRB_IN_FED, 1);
		task_enable_irq(GC_IRQNUM_RBOX0_INTR_PWRB_IN_FED_INT);
	} else {
		GWRITE_FIELD(RBOX, INT_ENABLE, INTR_PWRB_IN_FED, 0);
		task_disable_irq(GC_IRQNUM_RBOX0_INTR_PWRB_IN_FED_INT);
	}
}

#ifdef CONFIG_AP_RO_VERIFICATION

/*
 * Implement sequence detecting trigger for starting AP RO verification.
 *
 * 'RCTD' is short for 'RO check trigger detection'.
 *
 * Start the detection sequence each time power button is pressed. While it is
 * kept pressed, count number of presses of the refresh key. If the refresh
 * key is pressed PRESS_COUNT times within RCTD_CUTOFF_TIME, consider the RO
 * verification process triggered.
 *
 * If power button is released before the refresh key is pressed the required
 * number of times, or the cutoff time expires, stop the sequence until the
 * next power button press.
 */
static void rctd_poll(void);
DECLARE_DEFERRED(rctd_poll);

#define RCTD_CUTOFF_TIME (3 * SECOND)
#define RCTD_POLL_INTERVAL (20 * MSEC) /* Poll buttons this often.*/
#define DEBOUNCE_COUNT 2  /* Keyboard keys are pretty noisy, need debouncing. */
#define PRESS_COUNT 3

/*
 * Lower 32 bit of the timestamp when the sequence started. Is zero if the
 * sequence is not running.
 */
static uint32_t rctd_start_time;

/*
 * rctd_poll_handler - periodically check states of power button and refresh
 * key.
 *
 * Returns non-zero value if the polling needs to continue, or zero, if the
 * polling should stop.
 */
static int rctd_poll_handler(void)
{
	uint32_t dior_state;
	uint8_t ref_curr_state;
	static uint8_t ref_press_count;
	static uint8_t ref_last_state;
	static uint8_t ref_debounced_state;
	static uint8_t ref_debounce_counter;

	/*
	 * H1 DIORx pins provide current state of both the power button and
	 * escape key.
	 */
	dior_state = GREG32(RBOX, CHECK_INPUT);
	ref_curr_state = !!(dior_state & GC_RBOX_CHECK_INPUT_KEY0_IN_MASK);

	if (rctd_start_time == 0) {
		/* Start the new sequence. */
		rctd_start_time = get_time().le.lo;
		ref_debounced_state = ref_last_state = ref_curr_state;
		ref_debounce_counter = DEBOUNCE_COUNT;
		ref_press_count = 0;
	} else {
		/* Have this been running longer than the timeout? */
		if ((get_time().le.lo - rctd_start_time) > RCTD_CUTOFF_TIME) {
			if (ref_press_count) {
				/*
				 * Report timeout only in case the process
				 * started.
				 */
				ap_ro_add_flash_event(APROF_CHECK_TIMED_OUT);
				CPRINTS("Timeout, no RO check triggered");
			}
			return 0;
		}
	}


	if ((dior_state & GC_RBOX_CHECK_INPUT_PWRB_IN_MASK) != 0) {
		if (ref_press_count) {
			/*
			 * Report interruption only in case the process
			 * started.
			 */
			CPRINTS("Power button released, "
				"RO Check Detection stopped");
			ap_ro_add_flash_event(APROF_CHECK_STOPPED);
		}
		return 0;
	}

	if (ref_curr_state != ref_debounced_state) {
		ref_debounced_state = ref_curr_state;
		ref_debounce_counter = 0;
		return 1;
	}

	if (ref_debounce_counter >= DEBOUNCE_COUNT)
		return 1;

	if (++ref_debounce_counter != DEBOUNCE_COUNT)
		return 1;

	ref_last_state = ref_debounced_state;
	if (!ref_last_state)
		return 1;

	if (++ref_press_count != PRESS_COUNT) {
		ap_ro_add_flash_event(APROF_REFRESH_PRESSED);
		CPRINTS("Refresh press registered");
		return 1;
	}

	CPRINTS("RO Validation triggered");
	ap_ro_add_flash_event(APROF_CHECK_TRIGGERED);
	validate_ap_ro();
	return 0;
}

static void rctd_poll(void)
{
	if (rctd_poll_handler())
		hook_call_deferred(&rctd_poll_data, RCTD_POLL_INTERVAL);
	else
		rctd_start_time = 0;
}
#endif

static void power_button_handler(void)
{
	CPRINTS("power button pressed");

#ifdef CONFIG_AP_RO_VERIFICATION
	if (rctd_start_time == 0)
		hook_call_deferred(&rctd_poll_data, 0);
#endif

	if (physical_detect_press() != EC_SUCCESS) {
		/* Not consumed by physical detect */
#ifdef CONFIG_U2F
		/* Track last power button press for U2F */
		power_button_record();
#endif
	}

	GWRITE_FIELD(RBOX, INT_STATE, INTR_PWRB_IN_FED, 1);
}
DECLARE_IRQ(GC_IRQNUM_RBOX0_INTR_PWRB_IN_FED_INT, power_button_handler, 1);

static void power_button_release_handler(void)
{
#ifdef CR50_DEV
	CPRINTS("power button released");
#endif

	/*
	 * Let deassert_ec_rst be called deferred rather than
	 * by interrupt handler.
	 */
	hook_call_deferred(&deassert_ec_rst_data, 0);

	/* Note that this is for one-time use through the current power on. */
	power_button_release_enable_interrupt(0);
}
DECLARE_IRQ(GC_IRQNUM_RBOX0_INTR_PWRB_IN_RED_INT, power_button_release_handler,
	1);

#ifdef CONFIG_U2F
static void power_button_init(void)
{
	/*
	 * Enable power button interrupts all the time for U2F.
	 *
	 * Ideally U2F should only enable physical presence after the start of
	 * a U2F request (using atomic operations for the PP enable mask so it
	 * plays nicely with CCD config), but that doesn't happen yet.
	 */
	power_button_press_enable_interrupt(1);
}
DECLARE_HOOK(HOOK_INIT, power_button_init, HOOK_PRIO_DEFAULT);
#endif  /* CONFIG_U2F */

void board_physical_presence_enable(int enable)
{
#ifndef CONFIG_U2F
	/* Enable/disable power button interrupts */
	power_button_press_enable_interrupt(enable);
#endif

	/* Stay awake while we're doing this, just in case. */
	if (enable)
		disable_sleep(SLEEP_MASK_PHYSICAL_PRESENCE);
	else
		enable_sleep(SLEEP_MASK_PHYSICAL_PRESENCE);
}

static int command_powerbtn(int argc, char **argv)
{
	ccprintf("powerbtn: %s\n",
		 rbox_powerbtn_is_pressed() ? "pressed" : "released");

#ifdef CR50_DEV
	pop_check_presence(1);
#endif
	return EC_SUCCESS;
}
DECLARE_CONSOLE_COMMAND(powerbtn, command_powerbtn, "",
			"get the state of the power button");

/*
 * Perform a user presence check using the power button.
 */
static enum vendor_cmd_rc vc_get_pwr_btn(enum vendor_cmd_cc code,
					 void *buf,
					 size_t input_size,
					 size_t *response_size)
{
	/*
	 * The AP uses VENDOR_CC_GET_PWR_BTN to poll both for the press and
	 * release of the power button.
	 *
	 * pop_check_presence(1) returns true if a new power button press was
	 * recorded in the last 10 seconds.
	 *
	 * Indicate button release if no new presses have been recorded and the
	 * current button state is not pressed.
	 */
	if (pop_check_presence(1) == POP_TOUCH_YES ||
		rbox_powerbtn_is_pressed())
		*(uint8_t *)buf = 1;
	else
		*(uint8_t *)buf = 0;
	*response_size = 1;

	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_GET_PWR_BTN, vc_get_pwr_btn);

