/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "ppm_common.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/logging/log.h>

#include <usbc/ppm.h>

LOG_MODULE_REGISTER(ppm_common, LOG_LEVEL_INF);

enum last_error_type {
	/* Error came from LPM; GET_ERROR_STATUS should query the LPM for a
	 * value.
	 */
	ERROR_LPM,

	/* Error came from PPM; GET_ERROR_STATUS should return directly from
	 * PPM.
	 */
	ERROR_PPM,
};

/* Internal data for ppm common implementation.  Exposed for test purposes. */
struct ppm_common_device {
	/* Parent PD driver instance. Not OWNED. */
	const struct ucsi_pd_driver *pd;

	/* Zephyr device instance for this driver. */
	const struct device *device;

	/* Doorbell notification callback (and context). */
	ucsi_ppm_notify *opm_notify;
	void *opm_context;

	/* PPM task */
	k_tid_t ppm_task_id;
	struct k_thread ppm_task_data;
	struct k_mutex ppm_lock;
	struct k_condvar ppm_condvar;

	/* PPM state */
	bool cleaning_up;
	enum ppm_states ppm_state;
	struct ppm_pending_data pending;

	/* Port and status information */
	uint8_t num_ports;
	struct ucsiv3_get_connector_status_data *per_port_status;

	/* Port number is 7 bits. */
	uint8_t last_connector_changed;
	uint8_t last_connector_alerted;

	/* Data dedicated to UCSI operation. */
	struct ucsi_memory_region ucsi_data;

	/* Last error status info. */
	enum last_error_type last_error;
	struct ucsiv3_get_error_status_data ppm_error_result;

	/** Notification mask */
	union notification_enable_t notif_mask;
};

const char *ppm_state_strings[PPM_STATE_MAX] = {
	"PPM_STATE_NOT_READY",	    "PPM_STATE_IDLE",
	"PPM_STATE_IDLE_NOTIFY",    "PPM_STATE_PROCESSING_COMMAND",
	"PPM_STATE_WAITING_CC_ACK", "PPM_STATE_WAITING_ASYNC_EV_ACK",
};

const char *ppm_state_to_string(int state)
{
	if (state < PPM_STATE_NOT_READY || state >= PPM_STATE_MAX) {
		return "PPM_STATE_Outside_valid_range";
	}

	return ppm_state_strings[state];
}

const char *ucsi_cmd_strings[UCSI_CMD_MAX] = {
	"UCSI_CMD_RESERVED",
	"UCSI_CMD_PPM_RESET",
	"UCSI_CMD_CANCEL",
	"UCSI_CMD_CONNECTOR_RESET",
	"UCSI_CMD_ACK_CC_CI",
	"UCSI_CMD_SET_NOTIFICATION_ENABLE",
	"UCSI_CMD_GET_CAPABILITY",
	"UCSI_CMD_GET_CONNECTOR_CAPABILITY",
	"UCSI_CMD_SET_CCOM",
	"UCSI_CMD_SET_UOR",
	"obsolete_UCSI_CMD_SET_PDM",
	"UCSI_CMD_SET_PDR",
	"UCSI_CMD_GET_ALTERNATE_MODES",
	"UCSI_CMD_GET_CAM_SUPPORTED",
	"UCSI_CMD_GET_CURRENT_CAM",
	"UCSI_CMD_SET_NEW_CAM",
	"UCSI_CMD_GET_PDOS",
	"UCSI_CMD_GET_CABLE_PROPERTY",
	"UCSI_CMD_GET_CONNECTOR_STATUS",
	"UCSI_CMD_GET_ERROR_STATUS",
	"UCSI_CMD_SET_POWER_LEVEL",
	"UCSI_CMD_GET_PD_MESSAGE",
	"UCSI_CMD_GET_ATTENTION_VDO",
	"UCSI_CMD_reserved_0x17",
	"UCSI_CMD_GET_CAM_CS",
	"UCSI_CMD_LPM_FW_UPDATE_REQUEST",
	"UCSI_CMD_SECURITY_REQUEST",
	"UCSI_CMD_SET_RETIMER_MODE",
	"UCSI_CMD_SET_SINK_PATH",
	"UCSI_CMD_SET_PDOS",
	"UCSI_CMD_READ_POWER_LEVEL",
	"UCSI_CMD_CHUNKING_SUPPORT",
	"UCSI_CMD_VENDOR_CMD",
};

const char *ucsi_command_to_string(uint8_t command)
{
	if (command >= UCSI_CMD_MAX) {
		return "UCSI_CMD_Outside_valid_range";
	}

	return ucsi_cmd_strings[command];
}

static void clear_cci(struct ppm_common_device *dev)
{
	memset(&dev->ucsi_data.cci, 0, sizeof(struct ucsi_cci));
}

static void clear_last_error(struct ppm_common_device *dev)
{
	dev->last_error = ERROR_LPM;
	memset(&dev->ppm_error_result, 0,
	       sizeof(struct ucsiv3_get_error_status_data));
}

inline static void set_cci_error(struct ppm_common_device *dev)
{
	clear_cci(dev);
	dev->ucsi_data.cci.error = 1;
	dev->ucsi_data.cci.cmd_complete = 1;
}

static bool is_pending_async_event(struct ppm_common_device *dev)
{
	return dev->pending.async_event;
}

static int ppm_common_opm_notify(struct ppm_common_device *dev)
{
	uint32_t cci;

	if (!dev->opm_notify) {
		LOG_ERR("User error: No notifier!");
		return -1;
	}

	memcpy(&cci, &dev->ucsi_data.cci, sizeof(cci));
	LOG_DBG("Notifying with CCI = 0x%08x", cci);
	dev->opm_notify(dev->opm_context);
	return 0;
}

static void clear_pending_command(struct ppm_common_device *dev)
{
	if (dev->pending.command) {
		LOG_DBG("Cleared pending command[0x%x]",
			dev->ucsi_data.control.command);
	}
	dev->pending.command = 0;
}

static void ppm_common_handle_async_event(struct ppm_common_device *dev)
{
	uint8_t port = 0;
	struct ucsiv3_get_connector_status_data *port_status;
	bool alert_port = false;

	/* Handle any smbus alert. */
	if (dev->pending.async_event) {
		LOG_DBG("PPM: Saw async event and processing.");

		/* If we are in the not ready or IDLE (no notifications) state,
		 * we do not bother updating OPM with status. Just clear the
		 * async event and move on.
		 */
		if (dev->ppm_state == PPM_STATE_NOT_READY ||
		    dev->ppm_state == PPM_STATE_IDLE) {
			dev->pending.async_event = 0;
			return;
		}

		/* Read per-port status if this is a fresh async event from an
		 * LPM alert.
		 */
		if (dev->last_connector_alerted) {
			const struct device *ppm = dev->device;

			LOG_DBG("Calling GET_CONNECTOR_STATUS on port %d",
				dev->last_connector_alerted);

			struct ucsi_control get_cs_cmd;
			memset((void *)&get_cs_cmd, 0,
			       sizeof(struct ucsi_control));

			get_cs_cmd.command = UCSI_CMD_GET_CONNECTOR_STATUS;
			get_cs_cmd.data_length = 0x0;
			get_cs_cmd.command_specific[0] =
				dev->last_connector_alerted;

			/* Clear port status before reading. */
			port = dev->last_connector_alerted - 1;
			port_status = &dev->per_port_status[port];
			memset(port_status, 0,
			       sizeof(struct ucsiv3_get_connector_status_data));

			if (dev->pd->execute_cmd(ppm, &get_cs_cmd,
						 (uint8_t *)port_status) < 0) {
				LOG_ERR("Failed to read port %d status. No recovery.",
					port + 1);
			} else {
				LOG_DBG("Port status change on %d: 0x%x",
					port + 1,
					(uint16_t)port_status
						->connector_status_change);
			}

			/* We got alerted with a change for a port we already
			 * sent notifications for but which has not yet acked.
			 * Resend the notification.
			 */
			if (port + 1 == dev->last_connector_changed) {
				alert_port = true;
			}

			dev->last_connector_alerted = 0;
		}

		/* If we are not already acting on an existing connector change,
		 * notify the OS if there are any other connector changes.
		 */
		if (!dev->last_connector_changed) {
			/* Find the first port with any pending change. */
			for (port = 0; port < dev->num_ports; ++port) {
				if (dev->per_port_status[port]
					    .connector_status_change != 0) {
					break;
				}
			}

			/* Handle events in order by setting CCI and notifying
			 * OPM.
			 */
			if (port < dev->num_ports) {
				/* Let through only enabled notifications. */
				port_status = &dev->per_port_status[port];
				if (dev->notif_mask.raw_value &
				    port_status->connector_status_change)
					alert_port = true;
			} else {
				LOG_DBG("No more ports needing OPM alerting");
			}
		}

		/* Should we alert? */
		if (alert_port) {
			LOG_DBG("Notifying async event for connector %d "
				"and changing state from %d (%s)",
				port + 1, dev->ppm_state,
				ppm_state_to_string(dev->ppm_state));
			/* Notify the OPM that we have data for it to read. */
			clear_cci(dev);
			dev->last_connector_changed = port + 1;
			dev->ucsi_data.cci.connector_changed = port + 1;
			ppm_common_opm_notify(dev);

			/* Set PPM state to waiting for async event ack */
			dev->ppm_state = PPM_STATE_WAITING_ASYNC_EV_ACK;
		}

		/* Clear the pending bit. */
		dev->pending.async_event = 0;
	}
}

static bool is_pending_command(struct ppm_common_device *dev)
{
	return dev->pending.command;
}

static bool match_pending_command(struct ppm_common_device *dev,
				  uint8_t command)
{
	return dev->pending.command &&
	       dev->ucsi_data.control.command == command;
}

static void ppm_common_reset_data(struct ppm_common_device *dev)
{
	clear_last_error(dev);
	dev->last_connector_changed = 0;
	dev->last_connector_alerted = 0;
	dev->notif_mask.raw_value = 0;
	memset(&dev->pending, 0, sizeof(dev->pending));
	memset(dev->per_port_status, 0,
	       sizeof(struct ucsiv3_get_connector_status_data) *
		       dev->num_ports);
}

static int ppm_common_execute_pending_cmd(struct ppm_common_device *dev)
{
	struct ucsi_control *control = &dev->ucsi_data.control;
	struct ucsi_cci *cci = &dev->ucsi_data.cci;
	uint8_t *message_in = (uint8_t *)&dev->ucsi_data.message_in;
	uint8_t ucsi_command = control->command;
	struct ucsiv3_ack_cc_ci_cmd *ack_cmd;
	int ret = -1;
	bool ack_ci = false;
	const struct device *ppm = dev->device;

	if (control->command == 0 || control->command >= UCSI_CMD_MAX) {
		LOG_ERR("Invalid command 0x%x", control->command);

		/* Set error condition to invalid command. */
		clear_last_error(dev);
		dev->last_error = ERROR_PPM;
		dev->ppm_error_result.error_information.unrecognized_command =
			1;
		set_cci_error(dev);
		return -1;
	}

	switch (ucsi_command) {
	case UCSI_CMD_ACK_CC_CI:
		ack_cmd = (struct ucsiv3_ack_cc_ci_cmd *)
				  control->command_specific;
		/* The ack should already validated before we reach here. */
		ack_ci = ack_cmd->connector_change_ack;
		break;

	case UCSI_CMD_GET_ERROR_STATUS:
		/* If the error status came from the PPM, return the cached
		 * value and skip the |execute_cmd| in the pd_driver.
		 */
		if (dev->last_error == ERROR_PPM) {
			ret = sizeof(struct ucsiv3_get_error_status_data);
			memcpy(message_in, &dev->ppm_error_result, ret);
			goto success;
		}
		break;
	case UCSI_CMD_PPM_RESET:
		ppm_common_reset_data(dev);
		ret = 0;
		goto success;
	case UCSI_CMD_SET_NOTIFICATION_ENABLE:
		/* Save the notification mask. */
		memcpy(&dev->notif_mask, control->command_specific,
		       sizeof(dev->notif_mask));
		ret = 0;
		goto success;
	default:
		break;
	}

	/* Do driver specific execute command. */
	ret = dev->pd->execute_cmd(ppm, control, message_in);

	/* Clear command since we just executed it. */
	memset(control, 0, sizeof(struct ucsi_control));

	if (ret < 0) {
		LOG_ERR("Error with UCSI command 0x%x. Return was %d",
			ucsi_command, ret);
		clear_last_error(dev);
		if (ret == -ENOTSUP) {
			dev->last_error = ERROR_PPM;
			dev->ppm_error_result.error_information
				.unrecognized_command = 1;
		} else if (ret == -EBUSY || ret == -ETIMEDOUT) {
			dev->last_error = ERROR_PPM;
			dev->ppm_error_result.error_information
				.ppm_policy_conflict = 1;
		} else {
			dev->last_error = ERROR_LPM;
		}
		set_cci_error(dev);
		return ret;
	}

success:
	LOG_DBG("Completed UCSI command 0x%x (%s). Read %d bytes.",
		ucsi_command, ucsi_command_to_string(ucsi_command), ret);
	clear_cci(dev);

	if (ret > 0) {
		LOG_DBG("Command 0x%x (%s) response", ucsi_command,
			ucsi_command_to_string(ucsi_command));
		LOG_HEXDUMP_DBG(message_in, ret, "");
	}

	/* Post-success command handling */
	if (ack_ci) {
		struct ucsiv3_get_connector_status_data *port_status =
			&dev->per_port_status[dev->last_connector_changed - 1];
		/* Clear port status for acked connector. */
		port_status->connector_status_change = 0;
		dev->last_connector_changed = 0;
		/* Flag a pending async event to process next event if it
		 * exists.
		 */
		dev->pending.async_event = 1;
	}

	/* If we reset, we only surface up the reset completed event after busy.
	 */
	if (ucsi_command == UCSI_CMD_PPM_RESET) {
		cci->reset_completed = 1;
	} else {
		cci->data_length = ret & 0xFF;
		cci->cmd_complete = 1;
	}
	return 0;
}

inline static bool check_ack_has_valid_bits(struct ucsiv3_ack_cc_ci_cmd *cmd)
{
	return cmd->command_complete_ack || cmd->connector_change_ack;
}

inline static bool check_ack_has_valid_ci(struct ucsiv3_ack_cc_ci_cmd *cmd,
					  struct ppm_common_device *dev)
{
	return cmd->connector_change_ack ? dev->last_connector_changed != 0 : 1;
}

inline static bool check_ack_has_valid_cc(struct ucsiv3_ack_cc_ci_cmd *cmd,
					  struct ppm_common_device *dev)
{
	return cmd->command_complete_ack ?
		       dev->ppm_state == PPM_STATE_WAITING_CC_ACK :
		       1;
}

inline static bool is_invalid_ack(struct ppm_common_device *dev)
{
	struct ucsiv3_ack_cc_ci_cmd *cmd =
		(struct ucsiv3_ack_cc_ci_cmd *)
			dev->ucsi_data.control.command_specific;
	return (!(check_ack_has_valid_bits(cmd) &&
		  check_ack_has_valid_ci(cmd, dev) &&
		  check_ack_has_valid_cc(cmd, dev)));
}

static void invalid_ack_notify(struct ppm_common_device *dev)
{
	struct ucsiv3_ack_cc_ci_cmd *cmd =
		(struct ucsiv3_ack_cc_ci_cmd *)
			dev->ucsi_data.control.command_specific;
	LOG_ERR("Invalid ack usage (CI=%d CC=%d last_connector_changed=%d) in "
		"state %d",
		cmd->connector_change_ack, cmd->command_complete_ack,
		dev->last_connector_changed, dev->ppm_state);

	clear_last_error(dev);
	dev->last_error = ERROR_PPM;
	dev->ppm_error_result.error_information.invalid_cmd_specific_params = 1;

	set_cci_error(dev);
	/* TODO(UCSI WG): Clarify pending clear behavior in case of PPM error */
	clear_pending_command(dev);
	ppm_common_opm_notify(dev);
}

/* Handle pending command. When handling pending commands, it is recommended
 * that dev->ppm_state changes or notifications are made only in  this function.
 * Error bits may be set by other functions.
 */
static void ppm_common_handle_pending_command(struct ppm_common_device *dev)
{
	uint8_t next_command = 0;
	int ret;

	if (dev->pending.command) {
		/* Check what command is currently pending. */
		next_command = dev->ucsi_data.control.command;

		LOG_DBG("PEND_CMD: Started command processing in "
			"state %d (%s), cmd 0x%x (%s)",
			dev->ppm_state, ppm_state_to_string(dev->ppm_state),
			next_command, ucsi_command_to_string(next_command));
		switch (dev->ppm_state) {
		case PPM_STATE_IDLE:
		case PPM_STATE_IDLE_NOTIFY:
			/* We are now processing the command. Change state,
			 * notify OPM and then continue.
			 */
			dev->ppm_state = PPM_STATE_PROCESSING_COMMAND;
			clear_cci(dev);
			dev->ucsi_data.cci.busy = 1;
			ppm_common_opm_notify(dev);
			/* Intentional fallthrough since we are now processing.
			 */
			__attribute__((fallthrough));
		case PPM_STATE_PROCESSING_COMMAND:
			/* TODO - Handle the case where we have a command that
			 * takes multiple smbus calls to process (i.e. firmware
			 * update). If we were handling something that requires
			 * processing (i.e. firmware update), we would not
			 * update to WAITING_CC_ACK until it was completed.
			 */
			ret = ppm_common_execute_pending_cmd(dev);
			if (ret < 0) {
				/* CCI error bits are handled by
				 * execute_pending_command
				 */
				dev->ppm_state = PPM_STATE_IDLE_NOTIFY;
				ppm_common_opm_notify(dev);
				break;
			}

			/* If we were handling a PPM Reset, we go straight back
			 * to idle and clear any error indicators.
			 */
			if (next_command == UCSI_CMD_PPM_RESET) {
				dev->ppm_state = PPM_STATE_IDLE;
				clear_last_error(dev);
			} else if (next_command == UCSI_CMD_ACK_CC_CI) {
				/* We've received a standalone CI ack after
				 * completing command loop(s).
				 */
				dev->ppm_state = PPM_STATE_IDLE_NOTIFY;

				clear_cci(dev);
				dev->ucsi_data.cci.ack_command = 1;
			} else {
				dev->ppm_state = PPM_STATE_WAITING_CC_ACK;
			}

			/* Notify OPM to handle result and wait for ack if we're
			 * not still processing.
			 */
			if (dev->ppm_state != PPM_STATE_PROCESSING_COMMAND) {
				ppm_common_opm_notify(dev);
			}
			break;
		case PPM_STATE_WAITING_CC_ACK:
		case PPM_STATE_WAITING_ASYNC_EV_ACK:
			/* If we successfully ACK, update CCI and notify. On
			 * error, the CCI will already be set by
			 * |ppm_common_execute_pending_cmd|.
			 */
			ret = ppm_common_execute_pending_cmd(dev);
			if (ret >= 0 && next_command == UCSI_CMD_PPM_RESET) {
				dev->ppm_state = PPM_STATE_IDLE;
			} else if (ret >= 0) {
				dev->ppm_state = PPM_STATE_IDLE_NOTIFY;

				clear_cci(dev);
				dev->ucsi_data.cci.ack_command = 1;
			}

			ppm_common_opm_notify(dev);
			break;
		default:
			LOG_ERR("Unhandled ppm state (%d) when handling pending command",
				dev->ppm_state);
			break;
		}

		LOG_DBG("PEND_CMD: Ended command processing in state %d (%s)",
			dev->ppm_state, ppm_state_to_string(dev->ppm_state));

		/* Last thing is to clear the pending command bit before
		 * executing the command.
		 */
		if (dev->ppm_state != PPM_STATE_PROCESSING_COMMAND) {
			clear_pending_command(dev);
		}
	}
}

static void ppm_common_taskloop(struct ppm_common_device *dev)
{
	/* We will handle async events only in idle state if there is
	 * one pending.
	 */
	bool handle_async_event = (dev->ppm_state <= PPM_STATE_IDLE_NOTIFY) &&
				  is_pending_async_event(dev);
	/* Wait for a task from OPM unless we are already processing a
	 * command or we need to fall through for a pending command or
	 * handleable async event.
	 */
	if (dev->ppm_state != PPM_STATE_PROCESSING_COMMAND &&
	    !is_pending_command(dev) && !handle_async_event) {
		LOG_DBG("Waiting for next command at state %d (%s)...",
			dev->ppm_state, ppm_state_to_string(dev->ppm_state));
		k_condvar_wait(&dev->ppm_condvar, &dev->ppm_lock, K_FOREVER);
	}

	LOG_DBG("Handling next task at state %d (%s)", dev->ppm_state,
		ppm_state_to_string(dev->ppm_state));

	bool is_ppm_reset = match_pending_command(dev, UCSI_CMD_PPM_RESET);

	switch (dev->ppm_state) {
	/* Idle with notifications enabled. */
	case PPM_STATE_IDLE:
		if (is_pending_command(dev)) {
			/* Only handle SET_NOTIFICATION_ENABLE or
			 * PPM_RESET. Otherwise clear the pending
			 * command.
			 */
			if (match_pending_command(
				    dev, UCSI_CMD_SET_NOTIFICATION_ENABLE) ||
			    is_ppm_reset) {
				ppm_common_handle_pending_command(dev);
			} else {
				clear_pending_command(dev);
			}
		} else if (is_pending_async_event(dev)) {
			ppm_common_handle_async_event(dev);
		}
		break;

	/* Idle and waiting for a command or event. */
	case PPM_STATE_IDLE_NOTIFY:
		/* Check if you're acking in the right state for
		 * ACK_CC_CI. Only CI acks are allowed here. i.e. we are
		 * still waiting for a CI ack after a command loop was
		 * completed.
		 */
		if (is_pending_command(dev) &&
		    match_pending_command(dev, UCSI_CMD_ACK_CC_CI) &&
		    is_invalid_ack(dev)) {
			invalid_ack_notify(dev);
			break;
		}

		if (is_pending_command(dev)) {
			ppm_common_handle_pending_command(dev);
		} else if (is_pending_async_event(dev)) {
			ppm_common_handle_async_event(dev);
		}
		break;

	/* Processing a command. We only ever enter this state for
	 * firmware update (for example if we're breaking up a chunk of
	 * firmware into multiple transactions).
	 */
	case PPM_STATE_PROCESSING_COMMAND:
		ppm_common_handle_pending_command(dev);
		break;

	/* Waiting for a command completion acknowledge. */
	case PPM_STATE_WAITING_CC_ACK:
		if (is_pending_command(dev)) {
			if (!is_ppm_reset &&
			    (!match_pending_command(dev, UCSI_CMD_ACK_CC_CI) ||
			     is_invalid_ack(dev))) {
				invalid_ack_notify(dev);
				break;
			}
			ppm_common_handle_pending_command(dev);
		}
		break;

	/* Waiting for async event ack. */
	case PPM_STATE_WAITING_ASYNC_EV_ACK:
		if (is_pending_command(dev)) {
			bool is_ack =
				match_pending_command(dev, UCSI_CMD_ACK_CC_CI);
			if (!is_ppm_reset && is_ack && is_invalid_ack(dev)) {
				invalid_ack_notify(dev);
				break;
			}
			/* Waiting ASYNC_EV_ACK is a weird state. It can
			 * directly ACK the CI or it can go into a
			 * PROCESSING_COMMAND state (in which case it
			 * should be treated as a IDLE_NOTIFY).
			 *
			 * Thus, if we don't get UCSI_CMD_ACK_CC_CI
			 * here, we just treat this as IDLE_NOTIFY
			 * state.
			 */
			if (!is_ack) {
				LOG_DBG("ASYNC EV ACK state turned into IDLE_NOTIFY state");
				dev->ppm_state = PPM_STATE_IDLE_NOTIFY;
			}
			ppm_common_handle_pending_command(dev);
		}
		break;

	default:
		break;
	}
}

static void ppm_common_task(void *context)
{
	struct ppm_common_device *dev = (struct ppm_common_device *)context;
	const struct device *ppm = dev->device;

	LOG_DBG("PPM: Starting the ppm task");

	k_mutex_lock(&dev->ppm_lock, K_FOREVER);

	/* Initialize the system state. */
	dev->ppm_state = PPM_STATE_NOT_READY;

	/* Send PPM reset and set state to IDLE if successful. */
	memset(&dev->ucsi_data.control, 0, sizeof(struct ucsi_control));
	dev->ucsi_data.control.command = UCSI_CMD_PPM_RESET;
	if (dev->pd->execute_cmd(ppm, &dev->ucsi_data.control,
				 dev->ucsi_data.message_in) >= 0) {
		dev->ppm_state = PPM_STATE_IDLE;
		memset(&dev->ucsi_data.cci, 0, sizeof(struct ucsi_cci));
	}

	/* TODO - Note to self:  Smbus function calls are currently done with
	 * PPM lock; may need to  fix that.
	 */
	do {
		ppm_common_taskloop(dev);
	} while (!dev->cleaning_up);

	k_mutex_unlock(&dev->ppm_lock);
	LOG_DBG("Exiting ppm common task");

	k_thread_abort(k_current_get());
}

K_THREAD_STACK_DEFINE(ppm_stack, CONFIG_UCSI_PPM_STACK_SIZE);

static void ppm_common_thread_init(struct ppm_common_device *dev)
{
	dev->ppm_task_id = k_thread_create(
		&dev->ppm_task_data, ppm_stack, CONFIG_UCSI_PPM_STACK_SIZE,
		(void *)ppm_common_task, (void *)dev, 0, 0,
		CONFIG_UCSI_PPM_THREAD_PRIORITY, 0, K_NO_WAIT);
}

static int ppm_common_init_and_wait(struct ucsi_ppm_device *device)
{
#define MAX_TIMEOUT_MS 1000
#define POLL_EVERY_MS 10
	struct ppm_common_device *dev = (struct ppm_common_device *)device;
	struct ucsi_memory_region *ucsi_data = &dev->ucsi_data;
	bool ready_to_exit = false;

	/* First clear the PPM shared memory region. */
	memset(ucsi_data, 0, sizeof(*ucsi_data));

	/* Initialize to UCSI version 3.0 */
	ucsi_data->version.version = 0x0300;
	/* TODO - Set real lpm address based on smbus driver. */
	ucsi_data->version.lpm_address = 0x0;

	/* Reset state. */
	dev->cleaning_up = false;
	dev->ppm_state = PPM_STATE_NOT_READY;
	memset(&dev->pending, 0, sizeof(dev->pending));

	/* Clear port status and state. */
	memset(dev->per_port_status, 0,
	       dev->num_ports *
		       sizeof(struct ucsiv3_get_connector_status_data));
	dev->last_connector_changed = 0;
	dev->last_connector_alerted = 0;

	LOG_DBG("Ready to initialize PPM task!");

	/* Initialize the PPM task. */
	ppm_common_thread_init(dev);

	LOG_DBG("PPM is waiting for task to run.");

	for (int count = 0; count * POLL_EVERY_MS < MAX_TIMEOUT_MS; count++) {
		k_mutex_lock(&dev->ppm_lock, K_FOREVER);
		ready_to_exit = dev->ppm_state != PPM_STATE_NOT_READY;
		k_mutex_unlock(&dev->ppm_lock);

		if (ready_to_exit) {
			break;
		}

		k_usleep(POLL_EVERY_MS * 1000);
	}

	LOG_DBG("PPM initialized result: Success=%d", (ready_to_exit ? 1 : 0));

	return (ready_to_exit ? 0 : -1);
}

bool ppm_common_get_next_connector_status(
	struct ucsi_ppm_device *device, uint8_t *out_port_num,
	struct ucsiv3_get_connector_status_data **out_connector_status)
{
	struct ppm_common_device *dev = (struct ppm_common_device *)device;

	if (dev->last_connector_changed) {
		*out_port_num = (uint8_t)dev->last_connector_changed;
		*out_connector_status =
			&dev->per_port_status[dev->last_connector_changed - 1];
		return true;
	}

	return false;
}

static int ppm_common_read(struct ucsi_ppm_device *device, unsigned int offset,
			   void *buf, size_t length)
{
	struct ppm_common_device *dev = (struct ppm_common_device *)device;

	/* Validate memory to read and allow any offset for reading. */
	if (offset + length >= sizeof(struct ucsi_memory_region)) {
		LOG_ERR("UCSI read exceeds bounds of memory: offset(0x%x), length(0x%x)",
			offset, length);
		return -EINVAL;
	}

	memcpy(buf, (const void *)((uint8_t *)(&dev->ucsi_data) + offset),
	       length);
	return length;
}

static int ppm_common_handle_control_message(struct ppm_common_device *dev,
					     const void *buf, size_t length)
{
	const uint8_t *cmd = (const uint8_t *)buf;
	uint8_t prev_cmd;
	uint8_t busy = 0;

	if (length > sizeof(struct ucsi_control)) {
		LOG_ERR("Tried to send control message with invalid size (%d)",
			(int)length);
		return -EINVAL;
	}

	/* If we're currently sending a command, we should immediately discard
	 * this call.
	 */
	{
		k_mutex_lock(&dev->ppm_lock, K_FOREVER);
		busy = dev->pending.command || dev->ucsi_data.cci.busy;
		prev_cmd = dev->ucsi_data.control.command;
		k_mutex_unlock(&dev->ppm_lock);
	}
	if (busy) {
		LOG_ERR("Tried to send control message (cmd=0x%x) when one "
			"is already pending (cmd=0x%x).",
			cmd[0], prev_cmd);
		return -EBUSY;
	}

	/* If we didn't get a full CONTROL message, zero the region before
	 * copying.
	 */
	if (length != sizeof(struct ucsi_control)) {
		memset(&dev->ucsi_data.control, 0, sizeof(struct ucsi_control));
	}
	memcpy(&dev->ucsi_data.control, cmd, length);

	LOG_DBG("Got valid control message: 0x%x (%s)", cmd[0],
		ucsi_command_to_string(cmd[0]));

	/* Schedule command send. */
	{
		k_mutex_lock(&dev->ppm_lock, K_FOREVER);

		/* Mark command pending. */
		dev->pending.command = 1;
		k_condvar_signal(&dev->ppm_condvar);

		LOG_DBG("Signaled pending command");

		k_mutex_unlock(&dev->ppm_lock);
	}

	return 0;
}

/*
 * Only allow writes into two regions:
 * - Control (to send commands)
 * - Message Out (to prepare data to send commands)
 *
 * A control message will result in an actual UCSI command being called if the
 * data is valid.
 *
 * A write into message in doesn't modify the PPM state but is often
 * a precursor to actually sending a control message. This will be used for fw
 * updates.
 *
 * Any writes into non-aligned offsets (except Message IN) will be discarded.
 */
static int ppm_common_write(struct ucsi_ppm_device *device, unsigned int offset,
			    const void *buf, size_t length)
{
	struct ppm_common_device *dev = (struct ppm_common_device *)device;
	bool valid_fixed_offset;

	if (!buf || length == 0) {
		LOG_ERR("Invalid buffer (%p) or length (%x)", buf, length);
		return -EINVAL;
	}

	/* OPM can only write to CONTROL and MESSAGE_OUT. */
	valid_fixed_offset = (offset == UCSI_CONTROL_OFFSET);
	if (!valid_fixed_offset &&
	    !(offset >= UCSI_MESSAGE_OUT_OFFSET &&
	      offset < UCSI_MESSAGE_OUT_OFFSET + MESSAGE_OUT_SIZE)) {
		LOG_ERR("UCSI can't write to invalid offset: 0x%x", offset);
		return -EINVAL;
	}

	/* Handle control messages */
	if (offset == UCSI_CONTROL_OFFSET) {
		return ppm_common_handle_control_message(dev, buf, length);
	}

	if (offset >= UCSI_MESSAGE_OUT_OFFSET &&
	    offset + length > UCSI_MESSAGE_OUT_OFFSET + MESSAGE_OUT_SIZE) {
		LOG_ERR("UCSI write [0x%x ~ 0x%x] exceeds the "
			"MESSAGE_OUT range [0x%x ~ 0x%x]",
			offset, offset + length - 1, UCSI_MESSAGE_OUT_OFFSET,
			UCSI_MESSAGE_OUT_OFFSET + MESSAGE_OUT_SIZE - 1);

		return -EINVAL;
	}

	/* Copy from input buffer to offset within MESSAGE_OUT. */
	memcpy(dev->ucsi_data.message_out + (offset - UCSI_MESSAGE_OUT_OFFSET),
	       buf, length);
	return 0;
}

static int ppm_common_register_notify(struct ucsi_ppm_device *device,
				      ucsi_ppm_notify *callback, void *context)
{
	struct ppm_common_device *dev = (struct ppm_common_device *)device;
	int ret = 0;

	/* Are we replacing the notify? */
	if (dev->opm_notify) {
		LOG_DBG("Replacing existing notify function!");
		ret = 1;
	}

	dev->opm_notify = callback;
	dev->opm_context = context;

	return ret;
}

static void ppm_common_lpm_alert(struct ucsi_ppm_device *device, uint8_t lpm_id)
{
	struct ppm_common_device *dev = (struct ppm_common_device *)device;

	LOG_DBG("LPM alert seen on connector %d!", lpm_id);

	k_mutex_lock(&dev->ppm_lock, K_FOREVER);

	if (lpm_id <= dev->num_ports) {
		/* Set async event and mark port status as not read. */
		dev->pending.async_event = 1;
		dev->last_connector_alerted = lpm_id;

		k_condvar_signal(&dev->ppm_condvar);
	} else {
		LOG_ERR("Alert id out of range: %d (num_ports = %d)", lpm_id,
			dev->num_ports);
	}

	k_mutex_unlock(&dev->ppm_lock);
}

static void ppm_common_cleanup(struct ucsi_ppm_driver *driver)
{
	if (driver->dev) {
		struct ppm_common_device *dev =
			(struct ppm_common_device *)driver->dev;

		LOG_DBG("Cleaning up.");
		/* Signal clean up to waiting thread. */
		k_mutex_lock(&dev->ppm_lock, K_FOREVER);
		dev->cleaning_up = true;
		k_condvar_signal(&dev->ppm_condvar);
		k_mutex_unlock(&dev->ppm_lock);

		/* Wait for task to complete. */
		k_thread_join(&dev->ppm_task_data, K_FOREVER);

		driver->dev = NULL;
	}
}

struct ucsi_ppm_driver *ppm_open(const struct ucsi_pd_driver *pd_driver,
				 struct ucsiv3_get_connector_status_data *data,
				 const struct device *device)
{
	/* These are all set to 0 by default. */
	static struct ppm_common_device dev;
	static struct ucsi_ppm_driver drv;

	drv.dev = (struct ucsi_ppm_device *)&dev;

	/* Initialize mutexes, condvars, etc. */
	k_mutex_init(&dev.ppm_lock);
	k_condvar_init(&dev.ppm_condvar);

	dev.pd = pd_driver;
	dev.num_ports = pd_driver->get_active_port_count(NULL);
	dev.per_port_status = data;
	dev.device = device;

	drv.init_and_wait = ppm_common_init_and_wait;
	drv.get_next_connector_status = ppm_common_get_next_connector_status;
	drv.read = ppm_common_read;
	drv.write = ppm_common_write;
	drv.register_notify = ppm_common_register_notify;
	drv.lpm_alert = ppm_common_lpm_alert;
	drv.cleanup = ppm_common_cleanup;

	return &drv;
}

#ifdef CONFIG_TEST_SUITE_PPM

enum ppm_states ppm_test_get_state(const struct ppm_common_device *dev)
{
	return dev->ppm_state;
}

bool ppm_test_is_async_pending(struct ppm_common_device *dev)
{
	bool pending;

	k_mutex_lock(&dev->ppm_lock, K_FOREVER);
	pending = is_pending_async_event(dev);
	k_mutex_unlock(&dev->ppm_lock);

	return pending;
}

bool ppm_test_is_cmd_pending(struct ppm_common_device *dev)
{
	bool pending;

	k_mutex_lock(&dev->ppm_lock, K_FOREVER);
	pending = is_pending_command(dev);
	k_mutex_unlock(&dev->ppm_lock);

	return pending;
}

#endif
