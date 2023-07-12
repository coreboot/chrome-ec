/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* AP hang detect logic */

#include "ap_hang_detect.h"
#include "chipset.h"
#include "common.h"
#include "console.h"
#include "hooks.h"
#include "host_command.h"
#ifdef CONFIG_LID_SWITCH
#include "lid_switch.h"
#endif
#include "power_button.h"
#include "timer.h"
#include "util.h"

/* Console output macros */
#define CPUTS(outstr) cputs(CC_CHIPSET, outstr)
#define CPRINTS(format, args...) cprints(CC_CHIPSET, format, ## args)

static struct ec_params_hang_detect hdparams;

static unsigned int bootstatus = 0;

/**
 * hang detect handlers for reboot.
 */
static void hang_detect_deferred_for_reboot(void)
{
	/* If we're rebooting the AP, stop hang detection */
	CPRINTS("hang detect triggering warm reboot");
	host_set_single_event(EC_HOST_EVENT_HANG_REBOOT);
	chipset_reset(0);
	/* if EC rebooted AP due to watchdog timeout set bootstatus to 1 */
	bootstatus = 1;
	return;
}
DECLARE_DEFERRED(hang_detect_deferred_for_reboot);

static void hang_detect_start_for_reboot(const char *why)
{
	if (hdparams.warm_reboot_timeout_msec) {
		CPRINTS("hang detect started on %s (for reboot)", why);

		hook_call_deferred(&hang_detect_deferred_for_reboot_data,
				   hdparams.warm_reboot_timeout_msec * MSEC);
	}
}

static void hang_detect_stop_for_reboot(const char *why)
{
	CPRINTS("hang detect stop on %s (for reboot)", why);
	hook_call_deferred(&hang_detect_deferred_for_reboot_data, -1);
}

#ifdef CONFIG_AP_HANG_DETECT_FOR_EVENT
/**
 * hang detect handlers for event.
 */
static void hang_detect_deferred_for_event(void)
{
	/* Otherwise, we're starting with the host event */
	CPRINTS("hang detect sending host event");
	host_set_single_event(EC_HOST_EVENT_HANG_DETECT);
}
DECLARE_DEFERRED(hang_detect_deferred_for_event);

static void hang_detect_start_for_event(const char *why)
{
	if (hdparams.host_event_timeout_msec && !hook_call_is_active(&hang_detect_deferred_for_event_data)) {
		CPRINTS("hang detect started on %s (for event)", why);

		hook_call_deferred(&hang_detect_deferred_for_event_data,
				   hdparams.host_event_timeout_msec * MSEC);
	}
}

static void hang_detect_stop_for_event(const char *why)
{
	CPRINTS("hang detect stop on %s (for event)", why);
	hook_call_deferred(&hang_detect_deferred_for_event_data, -1);
}

void hang_detect_stop_on_host_command(void)
{
	if (hdparams.flags & EC_HANG_STOP_ON_HOST_COMMAND) {
		hang_detect_stop_for_event("host cmd");
	}
}
#endif

/*****************************************************************************/
/* Hooks */

#ifdef CONFIG_AP_HANG_DETECT_FOR_EVENT
static void hang_detect_power_button(void)
{
	if (power_button_is_pressed()) {
		if (hdparams.flags & EC_HANG_START_ON_POWER_PRESS)
			hang_detect_start_for_event("power button");
	} else {
		if (hdparams.flags & EC_HANG_STOP_ON_POWER_RELEASE)
			hang_detect_stop_for_event("power button");
	}
}
DECLARE_HOOK(HOOK_POWER_BUTTON_CHANGE, hang_detect_power_button,
	     HOOK_PRIO_DEFAULT);

#ifdef CONFIG_LID_SWITCH
static void hang_detect_lid(void)
{
	if (lid_is_open()) {
		if (hdparams.flags & EC_HANG_START_ON_LID_OPEN)
			hang_detect_start_for_event("lid open");
	} else {
		if (hdparams.flags & EC_HANG_START_ON_LID_CLOSE)
			hang_detect_start_for_event("lid close");
	}
}
DECLARE_HOOK(HOOK_LID_CHANGE, hang_detect_lid, HOOK_PRIO_DEFAULT);
#endif
#endif

static void hang_detect_resume(void)
{
	if (hdparams.flags & EC_HANG_START_ON_RESUME)
		hang_detect_start_for_reboot("resume");
}
DECLARE_HOOK(HOOK_CHIPSET_RESUME, hang_detect_resume, HOOK_PRIO_DEFAULT);

static void hang_detect_suspend(void)
{
	if (hdparams.flags & EC_HANG_STOP_ON_SUSPEND)
		hang_detect_stop_for_reboot("suspend");
}
DECLARE_HOOK(HOOK_CHIPSET_SUSPEND, hang_detect_suspend, HOOK_PRIO_DEFAULT);

static void hang_detect_shutdown(void)
{
	/* Stop the timers */
	hang_detect_stop_for_reboot("shutdown");

	/* Disable hang detection; it must be enabled every boot */
	memset(&hdparams, 0, sizeof(hdparams));
}
DECLARE_HOOK(HOOK_CHIPSET_SHUTDOWN, hang_detect_shutdown, HOOK_PRIO_DEFAULT);


/*****************************************************************************/
/* Host command */

static int hang_detect_host_command(struct host_cmd_handler_args *args)
{
	const struct ec_params_hang_detect *p = args->params;
	struct ec_params_hang_detect_resp *r = args->response;

	/* AP is asking if EC has rebooted it */
	if (p->flags & EC_GET_HANG_STATUS) {
		args->response_size = sizeof(*r);
		r->status = bootstatus;
		CPRINTS("EC Watchdog status %d", r->status);
		/* Ignore the other params */
		return EC_RES_SUCCESS;
	}

	if (p->flags & EC_CLEAR_HANG_STATUS) {
		CPRINTS("Clearing bootstatus - AP shutting down gracefully");
		bootstatus = 0;
		return EC_RES_SUCCESS;
	}

	/* Handle stopping hang timer on request */
	if (p->flags & EC_HANG_STOP_NOW) {
		hang_detect_stop_for_reboot("ap request");
#ifdef CONFIG_AP_HANG_DETECT_FOR_EVENT
		hang_detect_stop_for_event("ap request");
#endif
		/* Ignore the other params */
		return EC_RES_SUCCESS;
	}

	/* Handle starting hang timer on request */
	if (p->flags & EC_HANG_START_NOW) {
		hang_detect_start_for_reboot("ap request");
#ifdef CONFIG_AP_HANG_DETECT_FOR_EVENT
		hang_detect_start_for_event("ap request");
#endif
		/* Ignore the other params */
		return EC_RES_SUCCESS;
	}

	/* If hang detect transitioning to disabled, stop timers */
	if (hdparams.flags && !p->flags) {
		hang_detect_stop_for_reboot("ap flags=0");
#ifdef CONFIG_AP_HANG_DETECT_FOR_EVENT
		hang_detect_stop_for_event("ap flags=0");
#endif
	}

	/* Save new params */
	hdparams = *p;
	CPRINTS("hang detect flags=0x%x, event=%d ms, reboot=%d ms",
		hdparams.flags, hdparams.host_event_timeout_msec,
		hdparams.warm_reboot_timeout_msec);

	return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_HANG_DETECT,
		     hang_detect_host_command,
		     EC_VER_MASK(0));

/*****************************************************************************/
/* Console command */

static int command_hang_detect(int argc, char **argv)
{
	ccprintf("flags:  0x%x\n", hdparams.flags);

	ccputs("event:  ");
	if (hdparams.host_event_timeout_msec)
		ccprintf("%d ms\n", hdparams.host_event_timeout_msec);
	else
		ccputs("disabled\n");

	ccputs("reboot: ");
	if (hdparams.warm_reboot_timeout_msec)
		ccprintf("%d ms\n", hdparams.warm_reboot_timeout_msec);
	else
		ccputs("disabled\n");

#ifdef CONFIG_AP_HANG_DETECT_FOR_EVENT
	ccprintf("status for event: %s",
		(hook_call_is_active(&hang_detect_deferred_for_event_data) ? "active" : "inactive"));
#endif

	ccprintf("status for reboot: %s",
		(hook_call_is_active(&hang_detect_deferred_for_reboot_data) ? "active" : "inactive"));

	return EC_SUCCESS;
}
DECLARE_CONSOLE_COMMAND(hangdet, command_hang_detect,
			NULL,
			"Print hang detect state");
