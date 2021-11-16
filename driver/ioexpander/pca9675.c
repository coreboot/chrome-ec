/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * NXP PCA9675PW I/O Port expander driver source
 */

#include "i2c.h"
#include "ioexpander.h"
#include "pca9675.h"

#ifdef CONFIG_IO_EXPANDER_SUPPORT_GET_PORT
#error "This driver doesn't support get_port function"
#endif

struct pca9675_ioexpander {
	/* I/O port direction (1 = input, 0 = output) */
	uint16_t io_direction;
	uint16_t cache_out_pins;
};

static struct pca9675_ioexpander pca9675_iox[CONFIG_IO_EXPANDER_PORT_COUNT];

static int pca9675_read16(int ioex, uint16_t *data)
{
	return i2c_xfer(ioex_config[ioex].i2c_host_port,
			ioex_config[ioex].i2c_addr_flags,
			NULL, 0, (uint8_t *)data, 2);
}

static int pca9675_write16(int ioex, uint16_t data)
{
	/*
	 * PCA9675 is Quasi-bidirectional I/O architecture hence
	 * append the direction (1 = input, 0 = output) to prevent
	 * overwriting I/O pins inadvertently.
	 */
	data |= pca9675_iox[ioex].io_direction;

	return i2c_xfer(ioex_config[ioex].i2c_host_port,
			ioex_config[ioex].i2c_addr_flags,
			(uint8_t *)&data, 2, NULL, 0);
}

static int pca9675_reset(int ioex)
{
	uint8_t reset = PCA9675_RESET_SEQ_DATA;

	return i2c_xfer(ioex_config[ioex].i2c_host_port,
			0, &reset, 1, NULL, 0);
}

static int pca9675_get_flags_by_mask(int ioex, int port, int mask, int *flags)
{
	*flags = mask & pca9675_iox[ioex].io_direction ?
				GPIO_INPUT : GPIO_OUTPUT;

	return EC_SUCCESS;
}

static int pca9675_get_level(int ioex, int port, int mask, int *val)
{
	int rv = EC_SUCCESS;
	uint16_t data_read;

	/* Read from IO-expander only if the pin is input */
	if (pca9675_iox[ioex].io_direction & mask) {
		rv = pca9675_read16(ioex, &data_read);
		if (!rv)
			*val = !!(data_read & mask);
	} else {
		*val = !!(pca9675_iox[ioex].cache_out_pins & mask);
	}

	return rv;
}

static int pca9675_set_level(int ioex, int port, int mask, int val)
{
	/* Update the output pins */
	if (val)
		pca9675_iox[ioex].cache_out_pins |= mask;
	else
		pca9675_iox[ioex].cache_out_pins &= ~mask;

	return pca9675_write16(ioex, pca9675_iox[ioex].cache_out_pins);
}

static int pca9675_set_flags_by_mask(int ioex, int port, int mask, int flags)
{
	int rv = EC_SUCCESS;

	/* Initialize the I/O directions */
	if (flags & GPIO_INPUT) {
		pca9675_iox[ioex].io_direction |= mask;
	} else {
		pca9675_iox[ioex].io_direction &= ~mask;

		/* Initialize the pin */
		rv = pca9675_set_level(ioex, port, mask, flags & GPIO_HIGH);
	}

	return rv;
}

static int pca9675_enable_interrupt(int ioex, int port, int mask, int enable)
{
	/*
	 * Nothing to do as an interrupt is generated by any rising or
	 * falling edge of the port inputs.
	 */
	return EC_SUCCESS;
}

int pca9675_init(int ioex)
{
	pca9675_iox[ioex].io_direction = PCA9675_DEFAULT_IO_DIRECTION;
	pca9675_iox[ioex].cache_out_pins = 0;

	/* Set pca9675 to Power-on reset */
	return pca9675_reset(ioex);
}

const struct ioexpander_drv pca9675_ioexpander_drv = {
	.init			= &pca9675_init,
	.get_level		= &pca9675_get_level,
	.set_level		= &pca9675_set_level,
	.get_flags_by_mask	= &pca9675_get_flags_by_mask,
	.set_flags_by_mask	= &pca9675_set_flags_by_mask,
	.enable_interrupt	= &pca9675_enable_interrupt,
};
