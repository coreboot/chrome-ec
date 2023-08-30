/* Copyright 2023 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef ZEPHYR_TEST_NISSA_INCLUDE_CRAASK_H_
#define ZEPHYR_TEST_NISSA_INCLUDE_CRAASK_H_

#include "ec_commands.h"

extern const struct ec_response_keybd_config craask_kb;
extern const struct ec_response_keybd_config craask_kb_w_kb_numpad;

void kb_init(void);
void buttons_init(void);

#endif /* ZEPHYR_TEST_NISSA_INCLUDE_CRAASK_H_ */
