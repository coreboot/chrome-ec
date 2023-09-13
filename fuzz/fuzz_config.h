/* Copyright 2018 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Fuzzer target config flags */

#ifndef __FUZZ_FUZZ_CONFIG_H
#define __FUZZ_FUZZ_CONFIG_H
#ifdef TEST_FUZZ

/* Disable hibernate: We never want to exit while fuzzing. */
#undef CONFIG_HIBERNATE

#ifdef TEST_PINWEAVER_FUZZ
#define CONFIG_DCRYPTO
#define CONFIG_PINWEAVER
#define CONFIG_UPTO_SHA512
#define SHA512_SUPPORT
#define CONFIG_MALLOC

/******************************************************************************/
/* From chip/g/config_chip.h */

#define CFG_FLASH_HALF (CONFIG_FLASH_SIZE >> 1)
#define CFG_TOP_SIZE   0x3000
#define CFG_TOP_A_OFF  (CFG_FLASH_HALF - CFG_TOP_SIZE)
#define CFG_TOP_B_OFF  (CONFIG_FLASH_SIZE - CFG_TOP_SIZE)

/******************************************************************************/
/* From board/cr50/board.h */
/* Non-volatile counter storage for U2F */
#define CONFIG_CRC8
#define CONFIG_FLASH_ERASED_VALUE32 (-1U)
#define CONFIG_FLASH_LOG
#define CONFIG_FLASH_LOG_BASE  CONFIG_PROGRAM_MEMORY_BASE
#define CONFIG_FLASH_LOG_SPACE 0x800

/* We're using TOP_A for partition 0, TOP_B for partition 1 */
#define CONFIG_FLASH_NVMEM
/* Offset to start of NvMem area from base of flash */
#define CONFIG_FLASH_NVMEM_OFFSET_A (CFG_TOP_A_OFF)
#define CONFIG_FLASH_NVMEM_OFFSET_B (CFG_TOP_B_OFF)
/* Address of start of Nvmem area */
#define CONFIG_FLASH_NVMEM_BASE_A \
	(CONFIG_PROGRAM_MEMORY_BASE + CONFIG_FLASH_NVMEM_OFFSET_A)
#define CONFIG_FLASH_NVMEM_BASE_B \
	(CONFIG_PROGRAM_MEMORY_BASE + CONFIG_FLASH_NVMEM_OFFSET_B)
#define CONFIG_FLASH_NEW_NVMEM_BASE_A \
	(CONFIG_FLASH_NVMEM_BASE_A + CONFIG_FLASH_BANK_SIZE)
#define CONFIG_FLASH_NEW_NVMEM_BASE_B \
	(CONFIG_FLASH_NVMEM_BASE_B + CONFIG_FLASH_BANK_SIZE)
/* Size partition in NvMem */
#define NVMEM_PARTITION_SIZE (CFG_TOP_SIZE)
/* Size in bytes of NvMem area */
#define CONFIG_FLASH_NVMEM_SIZE (NVMEM_PARTITION_SIZE * NVMEM_NUM_PARTITIONS)

#define NEW_NVMEM_PARTITION_SIZE (NVMEM_PARTITION_SIZE - CONFIG_FLASH_BANK_SIZE)
#define NEW_NVMEM_TOTAL_PAGES \
	(2 * NEW_NVMEM_PARTITION_SIZE / CONFIG_FLASH_BANK_SIZE)

/* Enable <key, value> variable support. */
#define CONFIG_FLASH_NVMEM_VARS
#define NVMEM_CR50_SIZE			  272
#define CONFIG_FLASH_NVMEM_VARS_USER_SIZE NVMEM_CR50_SIZE

#ifndef __ASSEMBLER__
enum nvmem_users { NVMEM_TPM = 0, NVMEM_CR50, NVMEM_NUM_USERS };
#endif

#define NVMEM_TPM_SIZE \
	(sizeof(((nvmem_partition *)(0))->buffer) - NVMEM_CR50_SIZE)

#define CONFIG_FLASH_NVMEM_VARS_USER_NUM NVMEM_CR50

/******************************************************************************/
#define CONFIG_SW_CRC

#endif /* TEST_PINWEAVER_FUZZ */

#ifdef TEST_HOST_COMMAND_FUZZ
#undef CONFIG_HOSTCMD_DEBUG_MODE

/* Defining this makes fuzzing slower, but exercises additional code paths. */
#define FUZZ_HOSTCMD_VERBOSE

#ifdef FUZZ_HOSTCMD_VERBOSE
#define CONFIG_HOSTCMD_DEBUG_MODE HCDEBUG_PARAMS
#else
#define CONFIG_HOSTCMD_DEBUG_MODE HCDEBUG_OFF
#endif /* ! FUZZ_HOSTCMD_VERBOSE */

/* The following are for fpsensor host commands. */
#define CONFIG_AES
#define CONFIG_AES_GCM
#define CONFIG_ROLLBACK_SECRET_SIZE 32
#define CONFIG_SHA256

#endif /* TEST_HOST_COMMAND_FUZZ */

#ifdef TEST_U2F_FUZZ
#define CONFIG_DCRYPTO
#define CONFIG_U2F
#define CC_EXTENSION CC_COMMAND
#endif /* TEST_U2F_FUZZ */

#endif /* TEST_FUZZ */
#endif /* __FUZZ_FUZZ_CONFIG_H */
