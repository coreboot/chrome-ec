/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "chip/npcx/trng_hw.h"
#include "chipset.h"
#include "clock.h"
#include "common.h"
#include "console.h"
#include "fpsensor/fpsensor_detect.h"
#include "gpio.h"
#include "hooks.h"
#include "registers.h"
#include "shi_chip.h"
#include "spi.h"
#include "switch.h"
#include "system.h"
#include "task.h"
#include "uart.h"
#include "uart_host_command.h"
#include "uartn.h"

/* TODO(b/279096907): Investigate de-duping with other FPMCU boards*/

/**
 * Disable restricted commands when the system is locked.
 *
 * @see console.h system.c
 */
int console_is_restricted(void)
{
	return system_is_locked();
}

/* Must come after other header files. */
#include "gpio_list.h"

static void ap_deferred(void)
{
	/*
	 * Behavior:
	 * AP Active  (ex. Intel S0):   SLP_L is 1
	 * AP Suspend (ex. Intel S0ix): SLP_L is 0
	 */
	int running = gpio_get_level(GPIO_SLP_L);

	if (running) { /* S0 */
		disable_sleep(SLEEP_MASK_AP_RUN);
		hook_notify(HOOK_CHIPSET_RESUME);
	} else { /* S0ix */
		hook_notify(HOOK_CHIPSET_SUSPEND);
		enable_sleep(SLEEP_MASK_AP_RUN);
	}
}
DECLARE_DEFERRED(ap_deferred);

static void board_init_transport(void)
{
	enum fp_transport_type ret_transport = get_fp_transport_type();

	ccprints("TRANSPORT_SEL: %s", fp_transport_type_to_str(ret_transport));

	/* Initialize transport based on bootstrap */
	switch (ret_transport) {
	case FP_TRANSPORT_TYPE_UART:

		/* Check if CONFIG_USART_HOST_COMMAND is enabled. */
		if (IS_ENABLED(CONFIG_USART_HOST_COMMAND))
			uart_host_command_init();
		else
			ccprints("ERROR: UART not supported in fw build.");

		/* Disable SPI interrupt to disable SPI transport layer */
		gpio_disable_interrupt(GPIO_SHI_CS_L);
		break;

	case FP_TRANSPORT_TYPE_SPI:
		/* SPI transport is enabled. */
		break;
	default:
		ccprints("ERROR: Selected transport is not valid.");
	}

	ccprints("TRANSPORT_SEL: %s",
		 fp_transport_type_to_str(get_fp_transport_type()));
}

static void board_init(void)
{
	/* Run until the first S3 entry */
	disable_sleep(SLEEP_MASK_AP_RUN);

	/* TOOD(b/291273378): Depending on the outcome of b/291273378, we may
	 * want to change the method of speeding up CPU.
	 */
	/* Turn on FAST_CPU mode */
	clock_enable_module(MODULE_FAST_CPU, 1);

	board_init_transport();

	/* Enable interrupt on PCH power signals */
	gpio_enable_interrupt(GPIO_SLP_L);

	/* Initialize trng peripheral before kicking off the application to
	 * avoid incurring that cost when generating random numbers
	 */
	npcx_trng_hw_init();

	if (IS_ENABLED(SECTION_IS_RW)) {
		board_init_rw();
	}

	/*
	 * Enable the SPI slave interface if the PCH is up.
	 * Do not use hook_call_deferred(), because ap_deferred() will be
	 * called after tasks with priority higher than HOOK task (very late).
	 */
	ap_deferred();
}
DECLARE_HOOK(HOOK_INIT, board_init, HOOK_PRIO_DEFAULT);

/* PCH power state changes */
void slp_event(enum gpio_signal signal)
{
	hook_call_deferred(&ap_deferred_data, 0);
}
