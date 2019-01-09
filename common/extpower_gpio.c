/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Pure GPIO-based external power detection */

#include "common.h"
#include "extpower.h"
#include "gpio.h"
#include "hooks.h"
#include "host_command.h"
#include "timer.h"
#include "charger.h"

static int debounced_extpower_presence;

int extpower_is_present(void)
{
	return debounced_extpower_presence;
}

/**
 * Deferred function to handle external power change
 */
static void extpower_deferred(void)
{
	int extpower_presence = gpio_get_level(GPIO_AC_PRESENT);

	if (extpower_presence == debounced_extpower_presence)
		return;

	debounced_extpower_presence = extpower_presence;
	hook_notify(HOOK_AC_CHANGE);

	/* Forward notification to host */
	if (extpower_presence)
		host_set_single_event(EC_HOST_EVENT_AC_CONNECTED);
	else
		host_set_single_event(EC_HOST_EVENT_AC_DISCONNECTED);
}
DECLARE_DEFERRED(extpower_deferred);

#ifdef CONFIG_CHARGER_BD9995X
static void extpower_deferred_set_input_current(void)
{
	if (gpio_get_level(GPIO_AC_PRESENT))
		charger_set_input_current(CONFIG_CHARGER_INPUT_CURRENT);

}
DECLARE_DEFERRED(extpower_deferred_set_input_current);
#endif

void extpower_interrupt(enum gpio_signal signal)
{
	/* Trigger deferred notification of external power change */
	hook_call_deferred(&extpower_deferred_data,
			CONFIG_EXTPOWER_DEBOUNCE_MS * MSEC);

#ifdef CONFIG_CHARGER_BD9995X
	/* As Rohm's recommendation, EC should set input current limit within 100ms
	 * once VCC&VBUS reset if doesn't enable charger's hardware IADP.
	 */
	hook_call_deferred(&extpower_deferred_set_input_current_data,
			30 * MSEC);
#endif
}

static void extpower_init(void)
{
	uint8_t *memmap_batt_flags = host_get_memmap(EC_MEMMAP_BATT_FLAG);

	debounced_extpower_presence = gpio_get_level(GPIO_AC_PRESENT);

	/* Initialize the memory-mapped AC_PRESENT flag */
	if (debounced_extpower_presence)
		*memmap_batt_flags |= EC_BATT_FLAG_AC_PRESENT;
	else
		*memmap_batt_flags &= ~EC_BATT_FLAG_AC_PRESENT;

	/* Enable interrupts, now that we've initialized */
	gpio_enable_interrupt(GPIO_AC_PRESENT);
}
DECLARE_HOOK(HOOK_INIT, extpower_init, HOOK_PRIO_INIT_EXTPOWER);
