/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.h"
#include "console.h"
#include "fpsensor/fpsensor_detect.h"
#include "gpio.h"
#include "registers.h"
#include "spi.h"
#include "task.h"
/*#include "usart_host_command.h"*/
#include "util.h"

#ifndef SECTION_IS_RW
#error "This file should only be built for RW."
#endif

/* TODO(b/279096907): Investigate de-duping with other FPMCU boards*/

/* SPI devices */
const struct spi_device_t spi_devices[] = {
	/* Fingerprint sensor (SCLK at 4Mhz)
	 * The SPI clock uses the APB2_CLK. The current setting of APB2_CLK is
	 * 15MHz (90MHz/6). Thus, the divider must be set to 4 as close as 4MHz
	 * SCLK. So actually the SCLK will be 3.75MHz (15MHz/4).
	 */
	{ .port = CONFIG_SPI_FP_PORT, .div = 4, .gpio_cs = GPIO_FP_SPI_CS }
};
const unsigned int spi_devices_used = ARRAY_SIZE(spi_devices);

static void configure_fp_sensor_spi(void)
{
	/* Configure SPI GPIOs */
	gpio_config_module(MODULE_SPI_CONTROLLER, 1);

	spi_enable(&spi_devices[0], 1);
}

void board_init_rw(void)
{
	/* Configure and enable SPI as master for FP sensor */
	configure_fp_sensor_spi();
}

#ifndef HAS_TASK_FPSENSOR
void fps_event(enum gpio_signal signal)
{
}
#endif
