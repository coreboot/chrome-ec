/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "dcrypto.h"

int usb_spi_sha256_start(struct sha256_ctx *ctx);
int usb_spi_sha256_update(struct sha256_ctx *ctx, uint32_t offset,
			  uint32_t size);
void usb_spi_sha256_final(struct sha256_ctx *ctx, void *digest,
			  size_t digest_size);
