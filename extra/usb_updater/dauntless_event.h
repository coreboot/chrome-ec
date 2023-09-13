/*
 * Copyright 2022 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef __EXTRA_USB_UPDATER_DAUNTLESS_EVENT_H
#define __EXTRA_USB_UPDATER_DAUNTLESS_EVENT_H

#include <stdint.h>

typedef struct {
	uint64_t time;
	uint32_t size;
	uint32_t event_type;
	uint8_t payload[0];
} dt_event_t;

union dt_entry_u {
	uint8_t raw[256];
	dt_event_t evt;
};

#endif // __EXTRA_USB_UPDATER_DAUNTLESS_EVENT_H
