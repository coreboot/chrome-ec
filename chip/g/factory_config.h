/*
 * Copyright 2023 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __EC_CHIP_G_FACTORY_CONFIG_H
#define __EC_CHIP_G_FACTORY_CONFIG_H

#include "board_space.h"
/**
 * Print the factory config value.
 */
void print_factory_config(void);

/**
 * Read the INFO1 factory config value into fc.
 *
 * @return EC_SUCCESS or an error code in cases of various failures to read the
 *		      flash space.
 */
int read_factory_config(uint64_t *fc);
#endif  /* ! __EC_CHIP_G_FACTORY_CONFIG_H */
