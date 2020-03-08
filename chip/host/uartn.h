/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_UARTN_H
#define __CROS_EC_UARTN_H

#include "uart.h"

/**
 * Flush the transmit FIFO.
 */
void uartn_tx_flush(int uart);

/**
 * Send a character to the UART data register.
 *
 * If the transmit FIFO is full, blocks until there is space.
 *
 * @param c		Character to send.
 */
void uartn_write_char(int uart, char c);


#endif  /* __CROS_EC_UARTN_H */
