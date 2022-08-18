/* Copyright 2021 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "dcrypto.h"

int usb_spi_sha256_start(struct sha256_ctx *ctx);
int usb_spi_sha256_update(struct sha256_ctx *ctx, uint32_t offset,
			  uint32_t size, bool print_range);
void usb_spi_sha256_final(struct sha256_ctx *ctx, void *digest,
			  size_t digest_size);

/**
 * Returns the content of SPI flash
 *
 * @param buf Buffer to write flash contents
 * @param offset Flash offset to start reading from
 * @param bytes Number of bytes to read.
 *
 * @return EC_SUCCESS, or non-zero if any error.
 */
int usb_spi_read_buffer(void *buf, unsigned int offset,  size_t bytes);
