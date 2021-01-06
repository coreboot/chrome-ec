/* Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_GPIO_SIGNAL_H
#define __CROS_EC_GPIO_SIGNAL_H

#include "compile_time_macros.h"

/*
 * There are 3 different IO signal types used by the EC.
 * Ensure they each use a unique range of values so we can tell them apart.
 * 1) Local GPIO => 0 to 0x0FFF
 * 2) IO expander GPIO => 0x1000 to 0x1FFF
 * 3) eSPI virtual wire signals (defined in include/espi.h) => 0x2000 to 0x2FFF
 */

#define GPIO(name, pin, flags) GPIO_##name,
#define UNIMPLEMENTED(name) GPIO_##name,
#define GPIO_INT(name, pin, flags, signal) GPIO_##name,

#define GPIO_SIGNAL_START 0 /* The first valid GPIO signal is 0 */

enum gpio_signal {
	#include "gpio.wrap"
	GPIO_COUNT,
	/* Ensure that sizeof gpio_signal is large enough for ioex_signal */
	GPIO_LIMIT = 0x0FFF
};
BUILD_ASSERT(GPIO_COUNT < GPIO_LIMIT);

#endif /* __CROS_EC_GPIO_SIGNAL_H */
