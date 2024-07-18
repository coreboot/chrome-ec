/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "battery.h"

#include <zephyr/fff.h>

DECLARE_FAKE_VALUE_FUNC(enum battery_present, battery_is_present);
