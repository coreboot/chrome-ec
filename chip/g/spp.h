/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_INCLUDE_SPP_H
#define __CROS_EC_INCLUDE_SPP_H

#include "spi.h"
#include "util.h"

/* SPP Control Mode */
enum spp_mode {
	SPP_GENERIC_MODE = 0,
	SPP_SWETLAND_MODE = 1,
	SPP_ROM_MODE = 2,
	SPP_UNDEF_MODE = 3,
};

/* Receive and transmit FIFO size and mask. */
#define SPP_FIFO_SIZE		BIT(10)
#define SPP_FIFO_MASK		(SPP_FIFO_SIZE - 1)

/*
 * Tx interrupt callback function prototype. This function returns a portion
 * of the received SPI data and current status of the CS line. When CS is
 * deasserted, this function is called with data_size of zero and a non-zero
 * cs_status. This allows the recipient to delineate the SPP frames.
 */
typedef void (*rx_handler_f)(uint8_t *data, size_t data_size, int cs_disabled);

/*
 * Push data to the SPP TX FIFO
 * @param data Pointer to 8-bit data
 * @param data_size Number of bytes to transmit
 * @return : actual number of bytes placed into tx fifo
 */
int spp_transmit(uint8_t *data, size_t data_size);

/*
 * These functions return zero on success or non-zero on failure (attempt to
 * register a callback on top of existing one, or attempt to unregister
 * non-existing callback.
 *
 * rx_fifo_threshold value of zero means 'default'.
 */
int spp_register_rx_handler(enum spp_mode mode,
			    rx_handler_f rx_handler,
			    unsigned rx_fifo_threshold);
int spp_unregister_rx_handler(void);
void spp_tx_status(uint8_t byte);

#endif
