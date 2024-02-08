/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/pd_driver.h"
#include "include/platform.h"
#include "include/ppm.h"
#include "include/smbus.h"
#include "um_ppm_chardev.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

// Packed message skeleton for cdev communication.
struct um_message_skeleton {
	uint8_t type;
	uint32_t offset;
	uint32_t data_length;
	uint8_t data[];
} __attribute__((__packed__));

#define MAX_DATA_SIZE 256
#define MAX_MESSAGE_DATA_SIZE \
	(MAX_DATA_SIZE + sizeof(struct um_message_skeleton))

enum um_message_types {
	// Notify kernel
	UM_MSG_NOTIFY = 0x1,

	// Kernel reads from userspace
	UM_MSG_READ = 0x2,

	// Userspace responds to kernel read
	UM_MSG_READ_RSP = 0x3,

	// Kernel writes to userspace
	UM_MSG_WRITE = 0x4,

	// Ready for communication
	UM_MSG_USERSPACE_READY = 0x5,

	// Closing down
	UM_MSG_USERSPACE_CLOSING = 0x6,
};

const char *message_type_strings[UM_MSG_USERSPACE_CLOSING + 1] = {
	"MSGTYPE_Invalid",
	"MSGTYPE_Notify",
	"MSGTYPE_Read",
	"MSGTYPE_Read_Response",
	"MSGTYPE_Write",
	"MSGTYPE_Userspace_Ready",
	"MSGTYPE_Userspace_Closing",
};

const char *message_type_to_string(uint8_t type)
{
	if (type > UM_MSG_USERSPACE_CLOSING) {
		return "MSGTYPE_Out_Of_Bounds";
	}

	return message_type_strings[type];
}

struct um_ppm_cdev {
	int fd;

	struct ucsi_pd_driver *pd;
	struct ucsi_ppm_driver *ppm;
	struct smbus_driver *smbus;
	struct pd_driver_config *driver_config;
};

#define READ_NOINTR(fd, buf, size)                                  \
	do {                                                        \
		if (read(fd, buf, size) == -1 && errno == -EINTR) { \
			continue;                                   \
		}                                                   \
	} while (false)

#define WRITE_NOINTR(fd, buf, size)                                  \
	do {                                                         \
		if (write(fd, buf, size) == -1 && errno == -EINTR) { \
			continue;                                    \
		}                                                    \
	} while (false)

static void write_to_cdev(int fd, void *buf, size_t size)
{
	DLOG("Writing to cdev (%d total bytes)", size);
	WRITE_NOINTR(fd, buf, size);
}

static void pretty_print_message(const char *prefix,
				 const struct um_message_skeleton *msg)
{
	DLOG_START("%s: Type 0x%x (%s): ", prefix, msg->type,
		   message_type_to_string(msg->type));

	// All message types will have offset + data length.
	DLOG_LOOP("Offset = 0x%x, Data Length = 0x%x, ", msg->offset,
		  msg->data_length);

	// Only write and read responses will have valid data within.
	switch (msg->type) {
	case UM_MSG_WRITE:
	case UM_MSG_READ_RSP:
		DLOG_LOOP("[ ");
		for (int i = 0; i < msg->data_length; ++i) {
			DLOG_LOOP("0x%x, ", msg->data[i]);
		}
		DLOG_LOOP("]");
		break;
	}

	DLOG_END("");
}

static void um_ppm_notify(void *context)
{
	struct um_ppm_cdev *cdev = (struct um_ppm_cdev *)context;
	uint8_t data[sizeof(struct um_message_skeleton)];

	struct um_message_skeleton *msg = (struct um_message_skeleton *)data;
	msg->type = UM_MSG_NOTIFY;
	msg->offset = 0;
	msg->data_length = 0;

	pretty_print_message("Notify", msg);
	write_to_cdev(cdev->fd, msg, sizeof(struct um_message_skeleton));
}

struct um_ppm_cdev *um_ppm_cdev_open(char *devpath, struct ucsi_pd_driver *pd,
				     struct smbus_driver *smbus,
				     struct pd_driver_config *driver_config)
{
	struct um_ppm_cdev *cdev = NULL;
	int fd = -1;

	fd = open(devpath, O_RDWR);
	if (fd < 0) {
		ELOG("Could not open PPM char device.");
		return NULL;
	}

	cdev = calloc(1, sizeof(struct um_ppm_cdev));
	if (!cdev) {
		goto handle_error;
	}

	cdev->fd = fd;
	cdev->pd = pd;
	cdev->ppm = pd->get_ppm(pd->dev);
	cdev->smbus = smbus;
	cdev->driver_config = driver_config;

	cdev->ppm->register_notify(cdev->ppm->dev, um_ppm_notify, (void *)cdev);

	return cdev;

handle_error:
	close(fd);
	free(cdev);

	return NULL;
}

void um_ppm_cdev_cleanup(struct um_ppm_cdev *cdev)
{
	if (cdev) {
		// Clean up the notify task first.
		cdev->smbus->cleanup(cdev->smbus);

		// Now clean up the cdev file (stopping communication).
		if (cdev->fd >= 0) {
			close(cdev->fd);
			cdev->fd = -1;
		}

		// Finally, clean up the pd driver.
		cdev->pd->cleanup(cdev->pd);

		free(cdev);
	}
}

static void um_ppm_notify_ready(struct um_ppm_cdev *cdev)
{
	uint8_t data[sizeof(struct um_message_skeleton)];

	struct um_message_skeleton *msg = (struct um_message_skeleton *)data;
	msg->type = UM_MSG_USERSPACE_READY;
	msg->offset = 0;
	msg->data_length = 0;

	pretty_print_message("Ready", msg);
	write_to_cdev(cdev->fd, msg, sizeof(struct um_message_skeleton));
}

static int um_ppm_handle_message(struct um_ppm_cdev *cdev,
				 struct um_message_skeleton *msg)
{
	int ret = 0;
	struct ucsi_ppm_driver *ppm = cdev->ppm;

	switch (msg->type) {
	case UM_MSG_READ:
		// Read and write response on success.
		ret = ppm->read(ppm->dev, msg->offset, msg->data,
				msg->data_length);
		if (ret != -1) {
			msg->data_length = ret;
			msg->type = UM_MSG_READ_RSP;

			pretty_print_message("Read response", msg);
			write_to_cdev(cdev->fd, msg,
				      sizeof(struct um_message_skeleton) +
					      msg->data_length);
		} else {
			ELOG("Error on read (%d) at offset 0x%x, length 0x%x",
			     ret, msg->offset, msg->data_length);
		}
		break;
	case UM_MSG_WRITE:
		ret = ppm->write(ppm->dev, msg->offset, msg->data,
				 msg->data_length);
		if (ret == -1) {
			ELOG("Error on write (%d) at offset 0x%x, length 0x%x",
			     ret, msg->offset, msg->data_length);
		}
		break;

	default:
		ELOG("Unhandled um_ppm message of type (%d): offset(0x%x), data-len(0x%x)",
		     msg->type, msg->offset, msg->data_length);
		return -1;
	}

	return 0;
}

void um_ppm_cdev_mainloop(struct um_ppm_cdev *cdev)
{
	int bytes = 0;

	uint8_t data[MAX_MESSAGE_DATA_SIZE];
	platform_memset(data, 0, MAX_MESSAGE_DATA_SIZE);

	// Make sure IRQ is configured before continuing.
	if (cdev->pd->configure_lpm_irq(cdev->pd->dev) != 0) {
		ELOG("Failed to configure LPM IRQ!");
		return;
	}

	// Wait for ppm to be ready before starting.
	cdev->pd->init_ppm(cdev->pd->dev);

	// Let kernel know we're ready to handle events.
	um_ppm_notify_ready(cdev);

	do {
		// Clear read data and re-read
		platform_memset(data, 0, MAX_MESSAGE_DATA_SIZE);
		bytes = read(cdev->fd, data, MAX_MESSAGE_DATA_SIZE);

		// We got a valid message.
		if (bytes >= sizeof(struct um_message_skeleton)) {
			// If there's additional data left, read it out. Only
			// valid for writes.
			struct um_message_skeleton *msg =
				(struct um_message_skeleton *)data;
			pretty_print_message("Read from cdev", msg);

			// Handle the message.
			if (um_ppm_handle_message(cdev, msg) == -1) {
				break;
			}
		}
		// Nothing to read at this time (not sure why we woke up).
		else if (bytes == 0) {
			DLOG("Got back zero bytes from read. Shouldn't we block here?");
			break;
		} else if (bytes == -1) {
			if (errno == -EINTR) {
				continue;
			}
			DLOG("Failed to read from cdev due to errno=%d", errno);
			break;
		}
	} while (true);
}

void um_ppm_handle_signal(struct um_ppm_cdev *cdev, int signal)
{
	ELOG("Handling signal %d", signal);

	if (cdev) {
		um_ppm_cdev_cleanup(cdev);
	}
}
