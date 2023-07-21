/*
 * Copyright 2023 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Parade 8828 USB/DP Mux.
 */

#ifndef __CROS_EC_USB_MUX_PARADE8833_H
#define __CROS_EC_USB_MUX_PARADE8833_H

#include "usb_mux.h"

#define PS8833_ADDR0 0x10
#define PS8833_ADDR1 0x20
#define PS8833_ADDR3 0x30
#define PS8833_ADDR4 0x40
#define PS8833_ADDR5 0x50
#define PS8833_ADDR6 0x60
#define PS8833_ADDR7 0x70
#define PS8833_ADDR8 0x80

#define PS8833_REG_PAGE0 0x00

/* This register contains a mix of general bits a*/
#define PS8833_REG_MODE 0x00
#define PS8833_REG_MODE_USB_EN BIT(5)
#define PS8833_REG_MODE_FLIP BIT(1)
/* Used for both connected and safe-states. */
#define PS8833_REG_MODE_CONN BIT(0)

#define PS8833_REG_DP 0x01
#define PS8833_REG_DP_EN BIT(0)

extern const struct usb_mux_driver ps8833_usb_retimer_driver;

#endif /* __CROS_EC_USB_MUX_PARADE8833_H */
