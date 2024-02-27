/* Copyright 2017 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * AP state machine
 */
#include "ap_ro_integrity_check.h"
#include "ec_commands.h"
#include "extension.h"
#include "gpio.h"
#include "hooks.h"
#include "nvmem.h"
#include "registers.h"
#include "system.h"
#include "timer.h"
#include "tpm_registers.h"

#define CPRINTS(format, args...) cprints(CC_SYSTEM, format, ## args)

static enum device_state state = DEVICE_STATE_INIT;
static bool disable_ds_temp;
static bool inkernel;

void print_ap_state(void)
{
	ccprintf("DS Dis:  %s\n", disable_ds_temp ? "on" : "off");
	ccprintf("AP:      %s (%s)\n", device_state_name(state),
		 !ap_is_on() ? "" : inkernel ? "K" : "F");
}

int ap_is_on(void)
{
	return state == DEVICE_STATE_ON;
}

void system_notify_ap_boot(void)
{
	inkernel = 1;
}

void pmu_check_tpm_rst(void)
{
	/*
	 * disable_ds_temp modifies the GPIO_TPM_RST_L wake settings to
	 * WAKE_HIGH. With that setting it's possible to miss a rising edge.
	 * If TPM_RST_L is high, but the ap state isn't on, cr50 missed the
	 * edge. Trigger the interrupt manually.
	 */
	if (disable_ds_temp && !ap_is_on() &&
	    gpio_get_level(GPIO_TPM_RST_L)) {
		tpm_rst_deasserted(GPIO_TPM_RST_L);
		CPRINTS("Missed edge");
	}
}

void clear_ds_disable(void)
{
	if (!disable_ds_temp)
		return;
	CPRINTS("DDS: clear");
	disable_ds_temp = false;
}
DECLARE_DEFERRED(clear_ds_disable);

/*
 * Disable deep sleep during the next TPM_RST_L pulse
 *
 * Normally cr50 enters deep sleep while TPM_RST_L is asserted. This vendor
 * command can be used to disable deep sleep the next time TPM_RST_L is
 * asserted.
 *
 * This can be run on any board with any CCD state.
 */
static enum vendor_cmd_rc vc_ds_disable_temp(enum vendor_cmd_cc code,
					     void *buf,
					     size_t input_size,
					     size_t *response_size)
{
	*response_size = 0;
	if (input_size)
		return VENDOR_RC_BOGUS_ARGS;

	/*
	 * Clear this if the AP doesn't turn off in 10 seconds, since
	 * that probably means suspend was aborted.
	 */
	hook_call_deferred(&clear_ds_disable_data, 10 * SECOND);
	disable_ds_temp = true;
	CPRINTS("dis DS");

	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_DS_DIS_TEMP, vc_ds_disable_temp);

/**
 * Set the AP state.
 *
 * Done as a function to make it easier to debug state transitions.  Note that
 * this ONLY sets the state (and possibly prints debug info), and doesn't do
 * all the additional transition work that set_ap_on(), etc. do.
 *
 * @param new_state	State to set.
 */
static void set_state(enum device_state new_state)
{
#ifdef CR50_DEBUG_AP_STATE
	/* Print all state transitions.  May spam the console. */
	if (state != new_state)
		CPRINTS("AP %s -> %s",
			device_state_name(state), device_state_name(new_state));
#endif
	state = new_state;
}

/**
 * Set AP to the off state. Disable functionality that should only be available
 * when the AP is on.
 */
static void deferred_set_ap_off(void)
{
	/*
	 * It's important cr50 keeps INT_AP_L enabled whenever the AP is on.
	 * Check if TPM_RST_L is still asserted before touching the AP state or
	 * the INT_AP_L signal. It shouldn't be possible for TPM_RST_L to be
	 * deasserted in set_ap_off. Check the level just to be safe.
	 * Block the AP state change if it's deasserted.
	 */
	if (gpio_get_level(GPIO_TPM_RST_L)) {
		CPRINTS("AP: ovrd");
		return;
	}
	CPRINTS("AP off");
	set_state(DEVICE_STATE_OFF);

	/*
	 * If TPM is configured then the INT_AP_L signal is used as a low pulse
	 * trigger to sync transactions with the host. By default Cr50 is
	 * driving this line high, but when the AP powers off, the 1.8V rail
	 * that it's pulled up to will be off and cause excessive power to be
	 * consumed by the Cr50. Set INT_AP_L as an input while the AP is
	 * powered off.
	 */
	gpio_set_flags(GPIO_INT_AP_L, GPIO_INPUT);

	ccd_update_state();

	/*
	 * If AP reset w/o TPM2_Shutdown, save NV on flash before deep sleep.
	 * Note, for most cases this is a no-op as commits happens after each
	 * TPM2 command. However, if AP is off in the first 3 seconds after
	 * it was on, so commits are disabled and next AP on doesn't happen
	 * before cr50 goes to deep sleep, we can loose some data in NV.
	 */
	nvmem_enable_commits();

	/*
	 * We don't enable deep sleep on ARM devices yet, as its processing
	 * there will require more support on the AP side than is available
	 * now.
	 *
	 * Note: Presence of platform reset is a poor indicator of deep sleep
	 * support.  It happens to be correlated with ARM vs x86 at present.
	 */
	if (disable_ds_temp) {
		/* Cancel the clear ds disable call since the AP turned off. */
		hook_call_deferred(&clear_ds_disable_data, -1);

		CPRINTS("Block DS");
		disable_deep_sleep();
		/*
		 * TPM_RST_L will stay asserted until the system resumes.
		 * Wake from sleep when it goes high.
		 */
		gpio_set_wakepin(GPIO_TPM_RST_L, GPIO_HIB_WAKE_HIGH);
	} else if (board_deep_sleep_allowed()) {
		enable_deep_sleep();
	}
}
DECLARE_DEFERRED(deferred_set_ap_off);

/**
 * Move the AP to the ON state
 */
void set_ap_on(void)
{
	hook_call_deferred(&deferred_set_ap_off_data, -1);
	CPRINTS("AP on");
	set_state(DEVICE_STATE_ON);

	/*
	 * AP is powering up, set the host sync signal to output and set it
	 * high which is the default level.
	 */
	gpio_set_flags(GPIO_INT_AP_L, GPIO_OUT_HIGH);
	gpio_set_level(GPIO_INT_AP_L, 1);

	ccd_update_state();

	if (board_deep_sleep_allowed())
		disable_deep_sleep();


	/*
	 * Restore TPM_RST_L wake low now that TPM_RST_L has been
	 * deasserted. Only change the TPM_RST_L wake setting if cr50 blocked
	 * deep sleep.
	 */
	if (disable_ds_temp) {
		CPRINTS("set TPM wake");
		gpio_set_wakepin(GPIO_TPM_RST_L, GPIO_HIB_WAKE_LOW);
	}

	/*
	 * disable_ds_temp only survives one TPM_RST_L pulse. Clear it when
	 * the AP turns back on.
	 */
	hook_call_deferred(&clear_ds_disable_data, 0);
}

static uint8_t waiting_for_ap_reset;
/*
 * If TPM_RST_L is asserted, the AP is in reset. Disable all AP functionality
 * in 1 second if it remains asserted.
 */
void tpm_rst_asserted(enum gpio_signal unused)
{
	CPRINTS("%s", __func__);

#ifdef CONFIG_AP_RO_VERIFICATION
	/* Clear AP RO verification state since this is a new boot. */
	if (!tpm_reset_in_progress())
		ap_ro_device_reset();
#endif
	inkernel = 0;
	/*
	 * It's possible the signal is being pulsed. Wait 3 seconds to disable
	 * functionality, so it's more likely the AP is fully off and not being
	 * reset.
	 */
	hook_call_deferred(&deferred_set_ap_off_data, 3 * SECOND);

	set_state(DEVICE_STATE_DEBOUNCING);

	if (waiting_for_ap_reset) {
		CPRINTS("CL: done");
		waiting_for_ap_reset = 0;
		deassert_ec_rst();
		enable_sleep(SLEEP_MASK_AP_RUN);
	}
}

void board_closed_loop_reset(void)
{
	CPRINTS("CL: start");
	/* Disable sleep while waiting for the reset */
	disable_sleep(SLEEP_MASK_AP_RUN);

	/* Until the AP resets, we can't trust it's state */
	set_state(DEVICE_STATE_UNKNOWN);

	waiting_for_ap_reset = 1;

	/* Disable AP communications with the TPM until cr50 sees the reset */
	tpm_stop();

	/* Use EC_RST_L to reset the system */
	assert_ec_rst();

	/*
	 * DETECT_TPM_RST_L_ASSERTED is edge triggered. If TPM_RST_L is already
	 * low, tpm_rst_asserted wont get called. Alert tpm_rst_asserted
	 * manually if the signal is already low.
	 */
	if (!gpio_get_level(GPIO_DETECT_TPM_RST_L_ASSERTED))
		tpm_rst_asserted(GPIO_TPM_RST_L);
}

static void poll_ap_state(void)
{
	/*
	 * High means the AP is on. If cr50 thinks the AP is off, it missed an
	 * interrupt. Trigger the deasserted interrupt.
	 */
	if (gpio_get_level(GPIO_TPM_RST_L)) {
		/*
		 * If cr50 thinks the AP is off while TPM_RST_L is high, it
		 * missed a falling edge interrupt. Trigger it.
		 */
		if (!ap_is_on()) {
			CPRINTS("appoll: R-");
			tpm_rst_deasserted(GPIO_TPM_RST_L);
		}
	} else if (ap_is_on()) {
		/*
		 * If cr50 thinks the AP is on while TPM_RST_L is low, it missed
		 * a falling edge interrupt. Trigger it.
		 */
		CPRINTS("appoll: -F");
		tpm_rst_asserted(GPIO_TPM_RST_L);
	}
}
DECLARE_HOOK(HOOK_SECOND, poll_ap_state, HOOK_PRIO_DEFAULT);
/**
 * Check the initial AP state.
 */
static void init_ap_detect(void)
{
	/* Enable interrupts for AP state detection */
	gpio_enable_interrupt(GPIO_TPM_RST_L);
	gpio_enable_interrupt(GPIO_DETECT_TPM_RST_L_ASSERTED);
	/*
	 * After resuming from any reset other than deep sleep, cr50 needs to
	 * make sure the rest of the system has reset. If cr50 needs a closed
	 * loop reset to reset the system, it can't rely on the short EC_RST
	 * pulse from RO. Use the closed loop reset to ensure the system has
	 * actually been reset.
	 *
	 * During this reset, the ap state will not be set to 'on' until the AP
	 * enters and then leaves reset. The tpm waits until the ap is on before
	 * allowing any tpm activity, so it wont do anything until the reset is
	 * complete.
	 */
	if (board_uses_closed_loop_reset() &&
	    !(system_get_reset_flags() & EC_RESET_FLAG_HIBERNATE)) {
		board_closed_loop_reset();
	} else {
		/*
		 * If the TPM_RST_L signal is already high when cr50 wakes up or
		 * transitions to high before we are able to configure the gpio
		 * then we will have missed the edge and the tpm reset isr will
		 * not get called. Check that we haven't already missed the
		 * rising edge. If we have alert tpm_rst_isr.
		 *
		 * DONT alert tpm_rst_isr if the board is waiting for the closed
		 * loop reset to finish. The isr is edge triggered, so
		 * tpm_rst_deasserted wont be called until the AP enters and
		 * exits reset. That is what we want. The TPM and other
		 * peripherals check ap_is_on before enabling interactions with
		 * the AP, and we want these to be disabled until the closed
		 * loop reset is complete.
		 */
		if (gpio_get_level(GPIO_TPM_RST_L))
			tpm_rst_deasserted(GPIO_TPM_RST_L);
		else
			tpm_rst_asserted(GPIO_TPM_RST_L);
	}
}
/*
 * TPM_RST_L isn't setup until board_init. Make sure init_ap_detect happens
 * after that.
 */
DECLARE_HOOK(HOOK_INIT, init_ap_detect, HOOK_PRIO_DEFAULT + 1);
