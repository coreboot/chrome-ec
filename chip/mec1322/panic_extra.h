/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_PANIC_EXTRA_H
#define __CROS_EC_PANIC_EXTRA_H

/*
* The backup address for the panic data.
* Set it to CONFIG_RAM_BASE because the panic_backup is put
* on this address by specifying section .bss.panic_extra
*/

#define PANIC_DATA_BACKUP_ADDRESS CONFIG_RAM_BASE

/**
 * Save the panic data to the panic_backup
 */
static inline void panic_data_backup(void)
{
	uint8_t *src_ptr = (uint8_t *)panic_get_data();
	uint8_t *dest_ptr;
	int num_bytes = sizeof(struct panic_data);

	dest_ptr = (uint8_t *)PANIC_DATA_BACKUP_ADDRESS;
	if (src_ptr) {
		while (num_bytes--)
			*dest_ptr++ = *src_ptr++;
	}
}

/**
 * Restore the panic data from the panic_backup
 */
static inline void panic_data_restore(void)
{
	uint8_t *src_ptr = (uint8_t *)PANIC_DATA_BACKUP_ADDRESS;
	uint8_t *dest_ptr = (uint8_t *)PANIC_DATA_PTR;
	int num_bytes = sizeof(struct panic_data);

	while (num_bytes--)
		*dest_ptr++ = *src_ptr++;
}

#endif /* __CROS_EC_PANIC_EXTRA_H */
