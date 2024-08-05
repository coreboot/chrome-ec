/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __USBD_INIT_H
#define __USBD_INIT_H

#include "hooks.h"

#include <stdint.h>
#include <stdlib.h>

#include <zephyr/drivers/usb/udc.h>
#include <zephyr/usb/usbd.h>

struct hid_dev_t {
	const struct device *dev;
	atomic_t state;
	uint8_t report_protocol;

	struct queue const report_queue;
	struct k_mutex *report_queue_mutex;
};

enum {
	HID_CLASS_IFACE_READY = 0,
	HID_CLASS_SUSPENDED,
	HID_EP_IN_BUSY,
};

struct usb_msg_manager {
	struct deferred_data *deferred;
	int callback_count;
	int max_callbacks;
};

extern struct usb_msg_manager msg_manager;
extern enum usbd_msg_type usb_message;

int request_usb_wake(void);
int usb_msg_deferred_register(const struct deferred_data *deferred);

#endif /* __USBD_INIT_H */
