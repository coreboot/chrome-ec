/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef __CHIP_G_I2CP_H
#define __CHIP_G_I2CP_H

#include <stddef.h>

/*
 * Write complete interrupt callback function prototype. This function expects
 * two parameters: the address of the buffer containing received data and
 * number of bytes in the buffer.
 */
typedef void (*wr_complete_handler_f)(void *i2cp_data, size_t i2cp_data_size);

/* Register the write complete interrupt handler. */
int i2cp_register_write_complete_handler(wr_complete_handler_f wc_handler);

/*
 * Post a byte for the controller to read. Blend the byte into the appropriate
 * 4byte register of the controller read register file.
 */
void i2cp_post_read_data(uint8_t byte_to_read);

/*
 * Configure the pinmux registers required to connect the I2CP driver. This
 * function is board specific and so it exists in the associated board.c file.
 */
void i2cp_set_pinmux(void);

/*
 * Ensure no bytes are currently buffered in the I2CP READ fifo. This
 * value is calculated by finding the difference between read pointer that's
 * used by FW to add bytes to the HW fifo and the current value of the
 * I2CS_READ_PTR register.
 *
 * @returns: the number of bytes buffered when the function is called
 */
size_t i2cp_zero_read_fifo_buffer_depth(void);

/*
 * Write buffer of data into the I2CP HW read fifo. The function will operate a
 * byte at a time until the fifo write pointer is word aligned. Then it will
 * consume all remaining words of input data. There is another stage to handle
 * any excess bytes. The efficiency benefits relative the byte at a time
 * function diminish as the buffer size gets smaller and therefore not intended
 * to be used for <= 4 byte buffers.
 */
void i2cp_post_read_fill_fifo(uint8_t *buffer, size_t len);

/*
 * Provide upper layers with information with the I2CP driver
 * status/statistics. The only piece of information currently provided is the
 * counter of "hosed" i2c driver occurences, where i2c clocking stopped
 * while periph was transmitting a zero.
 */
struct i2cp_status {
	uint16_t read_recovery_count;
};
void i2cp_get_status(struct i2cp_status *status);

#endif /* ! __CHIP_G_I2CP_H */
