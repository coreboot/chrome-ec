/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Event handling in MKBP keyboard protocol
 */

#include "atomic.h"
#include "chipset.h"
#include "gpio.h"
#include "host_command.h"
#include "link_defs.h"
#include "mkbp_event.h"
#include "power.h"
#include "util.h"

#define CPRINTF(format, args...) cprintf(CC_SYSTEM, format, ## args)

static uint32_t events;

#ifdef CONFIG_MKBP_EVENT_WAKEUP_MASK
static uint32_t mkbp_event_wake_mask = CONFIG_MKBP_EVENT_WAKEUP_MASK;
#endif /* CONFIG_MKBP_EVENT_WAKEUP_MASK */

#ifdef CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK
static uint32_t mkbp_host_event_wake_mask = CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK;
#endif /* CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK */

static void set_event(uint8_t event_type)
{
	atomic_or(&events, 1 << event_type);
}

static void clear_event(uint8_t event_type)
{
	atomic_clear(&events, 1 << event_type);
}

static int event_is_set(uint8_t event_type)
{
	return events & (1 << event_type);
}

/**
 * Assert host keyboard interrupt line.
 */
static void set_host_interrupt(int active)
{
	/* interrupt host by using active low EC_INT signal */
#ifdef CONFIG_MKBP_USE_HOST_EVENT
	if (active)
		host_set_single_event(EC_HOST_EVENT_MKBP);
#else
	gpio_set_level(GPIO_EC_INT_L, !active);
#endif
}

#if defined(CONFIG_MKBP_EVENT_WAKEUP_MASK) || \
	defined(CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK)
/**
 * Check if the host is sleeping. Check our power state in addition to the
 * self-reported sleep state of host (CONFIG_POWER_TRACK_HOST_SLEEP_STATE).
 */
static inline int host_is_sleeping(void)
{
	int is_sleeping = !chipset_in_state(CHIPSET_STATE_ON);

#ifdef CONFIG_POWER_TRACK_HOST_SLEEP_STATE
	is_sleeping |=
		(power_get_host_sleep_state() == HOST_SLEEP_EVENT_S3_SUSPEND);
#endif
	return is_sleeping;
}
#endif /* CONFIG_MKBP_(HOST_EVENT_)?WAKEUP_MASK */

int mkbp_send_event(uint8_t event_type)
{
	int skip_interrupt = 0;

	set_event(event_type);

#ifdef CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK
	/* Check to see if this host event should wake the system. */
	skip_interrupt = host_is_sleeping() &&
			 !(host_get_events() &
			   mkbp_host_event_wake_mask);
#endif /* CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK */

#ifdef CONFIG_MKBP_EVENT_WAKEUP_MASK
	/* Check to see if this MKBP event should wake the system. */
	if (!skip_interrupt)
		skip_interrupt = host_is_sleeping() &&
			!((1 << event_type) & mkbp_event_wake_mask);
#endif /* CONFIG_MKBP_EVENT_WAKEUP_MASK */

	/* To skip the interrupt, we cannot have the EC_MKBP_EVENT_KEY_MATRIX */
	skip_interrupt = skip_interrupt &&
			 (event_type != EC_MKBP_EVENT_KEY_MATRIX);

	if (skip_interrupt)
		return 0;

	set_host_interrupt(1);
	return 1;
}

static int mkbp_get_next_event(struct host_cmd_handler_args *args)
{
	static int last;
	int i, data_size, evt;
	uint8_t *resp = args->response;
	const struct mkbp_event_source *src;

	do {
		/*
		 * Find the next event to service.  We do this in a round-robin
		 * way to make sure no event gets starved.
		 */
		for (i = 0; i < EC_MKBP_EVENT_COUNT; ++i)
			if (event_is_set((last + i) % EC_MKBP_EVENT_COUNT))
				break;

		if (i == EC_MKBP_EVENT_COUNT) {
			set_host_interrupt(0);
			return EC_RES_UNAVAILABLE;
		}

		evt = (i + last) % EC_MKBP_EVENT_COUNT;
		last = evt + 1;

		/*
		 * Clear the event before retrieving the event data in case the
		 * event source wants to send the same event.
		 */
		clear_event(evt);

		for (src = __mkbp_evt_srcs; src < __mkbp_evt_srcs_end; ++src)
			if (src->event_type == evt)
				break;

		if (src == __mkbp_evt_srcs_end)
			return EC_RES_ERROR;

		resp[0] = evt; /* Event type */

		/*
		 * get_data() can return -EC_ERROR_BUSY which indicates that the
		 * next element in the keyboard FIFO does not match what we were
		 * called with.  For example, get_data is expecting a keyboard
		 * matrix, however the next element in the FIFO is a button
		 * event instead.  Therefore, we have to service that button
		 * event first.
		 */
		data_size = src->get_data(resp + 1);
		if (data_size == -EC_ERROR_BUSY)
			set_event(evt);
	} while (data_size == -EC_ERROR_BUSY);

	if (data_size < 0)
		return EC_RES_ERROR;
	args->response_size = 1 + data_size;

	if (!events)
		set_host_interrupt(0);

	return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_GET_NEXT_EVENT,
		     mkbp_get_next_event,
		     EC_VER_MASK(0));

#ifdef CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK
#ifdef CONFIG_MKBP_USE_HOST_EVENT
static int mkbp_get_host_event_wake_mask(struct host_cmd_handler_args *args)
{
	struct ec_response_host_event_mask *r = args->response;

	r->mask = CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK;
	args->response_size = sizeof(*r);

	return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_HOST_EVENT_GET_WAKE_MASK,
		     mkbp_get_host_event_wake_mask,
		     EC_VER_MASK(0));
#endif /* CONFIG_MKBP_USE_HOST_EVENT */
#endif /* CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK */

static int hc_mkbp_wake_mask(struct host_cmd_handler_args *args)
{
	struct ec_response_mkbp_event_wake_mask *r = args->response;
	const struct ec_params_mkbp_event_wake_mask *p = args->params;
	enum ec_mkbp_event_mask_action action = p->action;

	switch (action) {
	case GET_WAKE_MASK:
		switch (p->mask_type) {
#ifdef CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK
		case EC_MKBP_HOST_EVENT_WAKE_MASK:
			r->wake_mask = mkbp_host_event_wake_mask;
			break;
#endif /* CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK */

#ifdef CONFIG_MKBP_EVENT_WAKEUP_MASK
		case EC_MKBP_EVENT_WAKE_MASK:
			r->wake_mask = mkbp_event_wake_mask;
			break;
#endif /* CONFIG_MKBP_EVENT_WAKEUP_MASK */

		default:
			/* Unknown mask, or mask is not in use. */
			CPRINTF("%s: mask_type=%d is unknown or not used.\n",
				__func__, p->mask_type);
			return EC_RES_INVALID_PARAM;
		}

		args->response_size = sizeof(*r);
		break;

	case SET_WAKE_MASK:
		args->response_size = 0;

		switch (p->mask_type) {
#ifdef CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK
		case EC_MKBP_HOST_EVENT_WAKE_MASK:
			CPRINTF("MKBP hostevent mask updated to: 0x%08x "
				"(was 0x%08x)\n",
				p->new_wake_mask,
				mkbp_host_event_wake_mask);
			mkbp_host_event_wake_mask = p->new_wake_mask;
			break;
#endif /* CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK */

#ifdef CONFIG_MKBP_EVENT_WAKEUP_MASK
		case EC_MKBP_EVENT_WAKE_MASK:
			mkbp_event_wake_mask = p->new_wake_mask;
			CPRINTF("MKBP event mask updated to: 0x%08x\n",
				mkbp_event_wake_mask);
			break;
#endif /* CONFIG_MKBP_EVENT_WAKEUP_MASK */

		default:
			/* Unknown mask, or mask is not in use. */
			CPRINTF("%s: mask_type=%d is unknown or not used.\n",
				__func__, p->mask_type);
			return EC_RES_INVALID_PARAM;
		}
		break;

	default:
		return EC_RES_INVALID_PARAM;
	}

	return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_MKBP_WAKE_MASK,
		     hc_mkbp_wake_mask,
		     EC_VER_MASK(0));

#if defined(CONFIG_MKBP_EVENT_WAKEUP_MASK) ||	\
	defined(CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK)
static int command_mkbp_wake_mask(int argc, char **argv)
{
	if (argc == 3) {
		char *e;
		uint32_t new_mask = (uint32_t)strtoi(argv[2], &e, 0);

		if (*e)
			return EC_ERROR_PARAM2;

#ifdef CONFIG_MKBP_EVENT_WAKEUP_MASK
		if (strncmp(argv[1], "event", 5) == 0)
			mkbp_event_wake_mask = new_mask;
#endif /* CONFIG_MKBP_EVENT_WAKEUP_MASK */

#ifdef CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK
		if (strncmp(argv[1], "hostevent", 9) == 0)
			mkbp_host_event_wake_mask = new_mask;
#endif /* CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK */
	} else if (argc != 1) {
		return EC_ERROR_PARAM_COUNT;
	}

#ifdef CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK
	ccprintf("MKBP host event wake mask: 0x%08x\n",
		 mkbp_host_event_wake_mask);
#endif /* CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK */
#ifdef CONFIG_MKBP_EVENT_WAKEUP_MASK
	ccprintf("MKBP event wake mask: 0x%08x\n", mkbp_event_wake_mask);
#endif /* CONFIG_MKBP_EVENT_WAKEUP_MASK */
	return EC_SUCCESS;
}
DECLARE_CONSOLE_COMMAND(mkbpwakemask, command_mkbp_wake_mask,
			"[event | hostevent] [new_mask]",
			"Show or set MKBP event/hostevent wake mask");
#endif /* CONFIG_MKBP_(HOST)?EVENT_WAKEUP_MASK */
