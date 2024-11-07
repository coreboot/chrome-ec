/* Copyright 2022 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_POWER_MT8186_H_
#define __CROS_EC_POWER_MT8186_H_

#include "common.h"
#include "ec_commands.h"

__override_proto void
board_process_host_sleep_event(enum host_sleep_event state);

#endif /* __CROS_EC_POWER_MT8186_H_ */
