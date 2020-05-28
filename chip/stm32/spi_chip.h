/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* STM32-specific SPI module for Chrome EC */

#ifndef __CROS_EC_SPI_CHIP_H
#define __CROS_EC_SPI_CHIP_H

/*
 * Configures the SPI GPIO pins to high speed. If the GPIO frequency is not
 * high enough, we might end up getting bad packets.
 */
__override_proto void board_set_stm32_spi_pin_speed(void);

#endif /* __CROS_EC_SPI_CHIP_H */

