/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef __CROS_EC_COMM_H
#define __CROS_EC_COMM_H
#include <stdint.h>
#include "common.h"
/* GPIO Interrupt handler for GPIO_EC_PACKET_MODE_EN rising edge */
void ec_comm_packet_mode_en(enum gpio_signal unsed);
/* GPIO Interrupt handler for GPIO_EC_PACKET_MODE_DIS falling edge */
void ec_comm_packet_mode_dis(enum gpio_signal unsed);
/*
 * Return True if the given UART is in packet mode, in which EC-CR50
 * communication is on-going.
 */
int ec_comm_is_uart_in_packet_mode(int uart);
/*
 * Try to process the given char as a EC-CR50 communication packet.
 * If EC-CR50 communication is broken or uninitiated yet, then
 * it does not process it.
 *
 * @return 1 if the given char was detected and processed as a part of packet.
 *         0 otherwise.
 */
int ec_comm_process_packet(uint8_t ch);
/*
 * Block or unblock EC-CR50 communication.
 * @param block   non-zero value blocks EC-CR50 communication.
 *                Zero value unblocks it.
 */
void ec_comm_block(int block);
/* Reset EC EFS context */
void ec_efs_reset(void);
/* Set EC-EFS boot_mode */
uint16_t ec_efs_set_boot_mode(const char *data, const uint8_t size);
/* Verify the given hash data against the EC-FW hash from kernel secdata */
uint16_t ec_efs_verify_hash(const char *hash_data, const uint8_t size);

/* Re-load EC Hash code from TPM Kernel Secdata */
void ec_efs_refresh(void);
/* print EC-EFS status */
void ec_efs_print_status(void);

#endif /* __CROS_EC_COMM_H */
