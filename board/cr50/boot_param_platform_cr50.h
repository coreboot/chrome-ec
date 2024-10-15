/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __EC_BOARD_CR50_BOOT_PARAM_PLATFORM_CR50_H
#define __EC_BOARD_CR50_BOOT_PARAM_PLATFORM_CR50_H

/* Handler for Owner Clear event.
 * Called by _plat__OwnerClearCallback.
 */
void boot_param_handle_owner_clear(void);

/* Handler for TPM Startup event.
 * Called by _plat__StartupCallback.
 */
void boot_param_handle_tpm_startup(void);

#endif /* __EC_BOARD_CR50_BOOT_PARAM_PLATFORM_CR50_H */
