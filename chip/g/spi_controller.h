/*
 * Copyright 2015 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_INCLUDE_SPI_CONTROLLER_H
#define __CROS_EC_INCLUDE_SPI_CONTROLLER_H

#include "spi.h"

void configure_spi0_passthrough(int enable);
void set_spi_clock_mode(int port, enum spi_clock_mode mode);

/*
 * Not defined in the hardware register spec, the RX and TX buffers share the
 * same 128 bytes FIFO.
 */
#define SPI_BUF_SIZE 0x80

#endif
