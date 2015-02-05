/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_CONFIG_CHIP_H
#define __CROS_EC_CONFIG_CHIP_H

/* CPU core BFD configuration */
#include "core/cortex-m/config_core.h"

/* Number of IRQ vectors on the NVIC */
#define CONFIG_IRQ_COUNT 93

/* Use a bigger console output buffer */
#undef CONFIG_UART_TX_BUF_SIZE
#define CONFIG_UART_TX_BUF_SIZE 2048

/* Interval between HOOK_TICK notifications */
#define HOOK_TICK_INTERVAL_MS 250
#define HOOK_TICK_INTERVAL    (HOOK_TICK_INTERVAL_MS * MSEC)

/* Maximum number of deferrable functions */
#define DEFERRABLE_MAX_COUNT 8

/* Number of I2C ports */
#define I2C_PORT_COUNT 4

/****************************************************************************/
/* Memory mapping */

/*
 * The memory region for RAM is actually 0x00100000-0x00120000. The lower 96K
 * stores code and the higher 32K stores data. To reflect this, let's say
 * the lower 96K is flash, and the higher 32K is RAM.
 */

/*
 * The memory region for RAM is actually 0x00100000-0x00120000.
 * RAM for Loader = 2k
 * RAM for RO/RW = 24k
 * CODE size of the Loader is 4k
 * As per the above configuartion  the upper 26k
 * is used to store data.The rest is for code.
 * the lower 100K is flash[ 4k Loader and 96k RO/RW],
 * and the higher 26K is RAM shared by loader and RO/RW.
 */

/****************************************************************************/
/* Define our RAM layout. */

#define CONFIG_MEC_SRAM_BASE_START	 0x00100000
#define CONFIG_MEC_SRAM_BASE_END	 0x00120000
#define CONFIG_MEC_SRAM_SIZE		 (CONFIG_MEC_SRAM_BASE_END - \
						CONFIG_MEC_SRAM_BASE_START)

/* 2k RAM for Loader */
#define CONFIG_RAM_SIZE_LOADER           0x0000800
/* 24k RAM for RO /RW */
#define CONFIG_RAM_SIZE_RORW             0x00006000

#define CONFIG_RAM_SIZE_TOTAL		 (CONFIG_RAM_SIZE_LOADER + \
						CONFIG_RAM_SIZE_RORW)
#define CONFIG_RAM_BASE_RORW    	 (CONFIG_MEC_SRAM_BASE_END - \
						CONFIG_RAM_SIZE_TOTAL)
#define CONFIG_RAM_BASE             	  CONFIG_RAM_BASE_RORW
#define CONFIG_RAM_SIZE             	  CONFIG_RAM_SIZE_TOTAL

/* System stack size */
#define CONFIG_STACK_SIZE           	  4096

/* non-standard task stack sizes */
#define IDLE_TASK_STACK_SIZE        	  512
#define LARGER_TASK_STACK_SIZE      	  640

/* Default task stack size */
#define TASK_STACK_SIZE             	  512

/****************************************************************************/
/* Define our flash layout. */

/* Loader code Image size 4k*/
#define CONFIG_LOADE_IMAGE_SIZE   	0x00001000

/* Memory Lcation shared between Loader and RO/RWimage */
#define SHARED_RAM_LOADER_RORW		(CONFIG_MEC_SRAM_BASE_START + \
						(CONFIG_LOADE_IMAGE_SIZE - 4))

#define CONFIG_FLASH_PHYSICAL_SIZE  	0x00040000
#define CONFIG_FLASH_BASE           	(CONFIG_MEC_SRAM_BASE_START)
#define CONFIG_FLASH_SIZE		CONFIG_FLASH_PHYSICAL_SIZE

#define CONFIG_FW_LOADER_OFF        	0
#define CONFIG_FW_LOADER_SIZE       	CONFIG_LOADE_IMAGE_SIZE
/* Size of one firmware image in flash */
#ifndef CONFIG_FW_IMAGE_SIZE
/* 128 SRAM - ( 2k LoRAM + 24k RO/RW RAM + 4k Loader code) */
#define CONFIG_FW_IMAGE_SIZE        	(96 * 1024)
#endif

/* RO/RW firmware must after Loader code */
#define CONFIG_FW_RO_OFF            	CONFIG_FW_LOADER_SIZE
#define CONFIG_FW_RO_SIZE           	CONFIG_FW_IMAGE_SIZE


#define CONFIG_FW_RW_OFF            	CONFIG_FW_LOADER_SIZE
#define CONFIG_FW_RW_SIZE           	CONFIG_FW_RO_SIZE


/* TODO(crosbug.com/p/23796): why 2 sets of configs with the same numbers? */
#define CONFIG_FW_WP_RO_OFF         	CONFIG_FW_LOADER_OFF
#define CONFIG_FW_WP_RO_SIZE        	(CONFIG_FW_LOADER_SIZE + \
						CONFIG_FW_RO_SIZE)
#define CONFIG_FW_PSTATE_SIZE   CONFIG_FW_RO_SIZE
#define CONFIG_FW_PSTATE_OFF    (CONFIG_FW_RO_OFF + CONFIG_FW_RO_SIZE)

/****************************************************************************/
/* SPI Flash Memory Mapping */

/* Size of tolal image used ( 256k = loader + RSA Keys + RO + RW images)
     located at the end of the flash */
#define CONFIG_FLASH_BASE_SPI	CONFIG_SPI_FLASH_SIZE - (0x40000)

#define CONFIG_RO_WP_SPI_OFF		0x20000
#define CONFIG_RO_SPI_OFF		0x20000
#define CONFIG_RW_SPI_OFF		0
#define CONFIG_RO_IMAGE_FLASHADDR 	(CONFIG_FLASH_BASE_SPI +	\
						CONFIG_RO_SPI_OFF)
#define CONFIG_RW_IMAGE_FLASHADDR 	(CONFIG_FLASH_BASE_SPI +	\
						CONFIG_RW_SPI_OFF)

/****************************************************************************/
/* Customize the build */
/* Optional features present on this chip */
#if 0
#define CONFIG_ADC
#define CONFIG_PECI
#define CONFIG_SWITCH
#define CONFIG_MPU
#endif
#define CONFIG_I2C
#define CONFIG_LPC
#define CONFIG_FPU
#define CONFIG_SPI
#define CONFIG_DMA
#define CONFIG_FLASH_SPI
#undef CONFIG_FLASH

#endif  /* __CROS_EC_CONFIG_CHIP_H */
