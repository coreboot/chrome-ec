/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "console.h"
#include "gpio.h"
#include "hooks.h"
#include "host_command.h"
#include "lid_angle.h"
#include "stdbool.h"
#include "tablet_mode.h"
#include "timer.h"

#define CPRINTS(format, args...) cprints(CC_MOTION_LID, format, ## args)
#define CPRINTF(format, args...) cprintf(CC_MOTION_LID, format, ## args)

/*
 * Other code modules assume that notebook mode (i.e. tablet_mode = false) at
 * startup
 */
static bool tablet_mode;

/*
 * Console command can force the value of tablet_mode. If tablet_mode_force is
 * true, the all external set call for tablet_mode are ignored.
 */
static bool tablet_mode_forced;

/*
 * True: all calls to tablet_set_mode are ignored and tablet_mode if forced to 0
 * False: all calls to tablet_set_mode are honored
 */
static bool disabled;

int tablet_get_mode(void)
{
	return tablet_mode;
}

static void notify_tablet_mode_change(void)
{
	CPRINTS("tablet mode %sabled", tablet_mode ? "en" : "dis");
	hook_notify(HOOK_TABLET_MODE_CHANGE);

	/*
	 * When tablet mode changes, send an event to ACPI to retrieve
	 * tablet mode value and send an event to the kernel.
	 */
	if (IS_ENABLED(CONFIG_HOSTCMD_EVENTS))
		host_set_single_event(EC_HOST_EVENT_MODE_CHANGE);

}

void tablet_set_mode(int mode)
{
	/* If tablet_mode is forced via a console command, ignore set. */
	if (tablet_mode_forced)
		return;

	if (tablet_mode == !!mode)
		return;

	if (disabled) {
		CPRINTS("Tablet mode set while disabled (ignoring)!");
		return;
	}

	tablet_mode = !!mode;

	hook_notify(HOOK_TABLET_MODE_CHANGE);
}

void tablet_disable(void)
{
	tablet_mode = false;
	disabled = true;
}

/* This ifdef can be removed once we clean up past projects which do own init */
#ifdef CONFIG_HALL_SENSOR
#ifndef HALL_SENSOR_GPIO_L
#error  HALL_SENSOR_GPIO_L must be defined
#endif
static void hall_sensor_interrupt_debounce(void)
{
	int flipped_360_mode = !gpio_get_level(HALL_SENSOR_GPIO_L);

	/*
	 * 1. Peripherals are disabled only when lid reaches 360 position (It's
	 * probably already disabled by motion_sense task). We deliberately do
	 * not enable peripherals when the lid is leaving 360 position. Instead,
	 * we let motion sense task enable it once it is reaches laptop zone
	 * (180 or less).
	 * 2. Similarly, tablet mode is set here when lid reaches 360
	 * position. It should already be set by motion lid driver. We
	 * deliberately do not clear tablet mode when lid is leaving 360
	 * position(if motion lid driver is used). Instead, we let motion lid
	 * driver to clear it when lid goes into laptop zone.
	 */

#ifdef CONFIG_LID_ANGLE
	if (flipped_360_mode)
#endif /* CONFIG_LID_ANGLE */
		tablet_set_mode(flipped_360_mode);

#ifdef CONFIG_LID_ANGLE_UPDATE
	if (flipped_360_mode)
		lid_angle_peripheral_enable(0);
#endif /* CONFIG_LID_ANGLE_UPDATE */
}
DECLARE_DEFERRED(hall_sensor_interrupt_debounce);

/* Debounce time for hall sensor interrupt */
#define HALL_SENSOR_DEBOUNCE_US    (30 * MSEC)

void hall_sensor_isr(enum gpio_signal signal)
{
	hook_call_deferred(&hall_sensor_interrupt_debounce_data,
				HALL_SENSOR_DEBOUNCE_US);
}

static void hall_sensor_init(void)
{
	/* If this sub-system was disabled before initializing, honor that. */
	if (disabled)
		return;

	gpio_enable_interrupt(HALL_SENSOR_GPIO_L);
	/* Ensure tablet mode is initialized according to the hardware state
	 * so that the cached state reflects reality. */
	hall_sensor_interrupt_debounce();
}
DECLARE_HOOK(HOOK_INIT, hall_sensor_init, HOOK_PRIO_DEFAULT);

void hall_sensor_disable(void)
{
	gpio_disable_interrupt(HALL_SENSOR_GPIO_L);
	/* Cancel any pending debounce calls */
	hook_call_deferred(&hall_sensor_interrupt_debounce_data, -1);
	tablet_disable();
}
#endif

static int command_settabletmode(int argc, char **argv)
{
	if (argc != 2)
		return EC_ERROR_PARAM_COUNT;

	if (argv[1][0] == 'o' && argv[1][1] == 'n') {
		tablet_mode = true;
		tablet_mode_forced = true;
	} else if (argv[1][0] == 'o' && argv[1][1] == 'f') {
		tablet_mode = false;
		tablet_mode_forced = true;
	} else if (argv[1][0] == 'r') {
		tablet_mode_forced = false;
	} else {
		return EC_ERROR_PARAM1;
	}

	notify_tablet_mode_change();
	return EC_SUCCESS;
}
DECLARE_CONSOLE_COMMAND(tabletmode, command_settabletmode,
	"[on | off | reset]",
	"Manually force tablet mode to on, off or reset.");
