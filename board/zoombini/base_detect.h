/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Meowth base detection code. */

#include "adc.h"
#include "common.h"
#include "gpio.h"

enum base_detect_state {
	BASE_DETACHED = 0,
	BASE_ATTACHED_DEBOUNCE,
	BASE_ATTACHED,
	BASE_DETACHED_DEBOUNCE,
};

struct base_det_cfg {
	enum adc_channel attach_pin;
	enum adc_channel detach_pin;
};

/**
 * Returns the current base detection state.
 */
enum base_detect_state base_get_detect_state(void);
