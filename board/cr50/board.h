/* Copyright 2014 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_BOARD_H
#define __CROS_EC_BOARD_H

#define CONFIG_LTO

/*
 * The default watchdog timeout is 1.6 seconds, but there are some legitimate
 * flash-intensive TPM operations that actually take close to that long to
 * complete. Make sure we don't trigger the watchdog accidentally if the timing
 * is just a little off.
 */
#undef CONFIG_WATCHDOG_PERIOD_MS
#define CONFIG_WATCHDOG_PERIOD_MS 5000

/* Features that we don't want */
#undef CONFIG_CMD_LID_ANGLE
#undef CONFIG_CMD_POWERINDEBUG
#undef CONFIG_DMA_DEFAULT_HANDLERS
#undef CONFIG_FMAP
#undef CONFIG_HIBERNATE
#undef CONFIG_LID_SWITCH
#undef CONFIG_CMD_RW
#undef CONFIG_CMD_SYSINFO
#undef CONFIG_CMD_SYSJUMP
#undef CONFIG_CMD_SYSLOCK

#define CONFIG_CUSTOMIZED_RO
/* TODO: find a way to drop RO build. */
#define CONFIG_FW_INCLUDE_RO

#ifndef CR50_DEV
/* Disable stuff that should only be in debug builds */
#undef CONFIG_CMD_CRASH
#undef CONFIG_CMD_MD
#undef CONFIG_CMD_SLEEPMASK_SET
#undef CONFIG_CMD_WAITMS
#undef CONFIG_FLASH
#endif

#if defined(H1_RED_BOARD) || defined(CR50_DEV)
#define CONFIG_USB_SELECT_PHY
#endif

/* Enable getting gpio flags to tell if open drain pins are asserted */
#define CONFIG_GPIO_GET_EXTENDED
/* Disable sleep when gpios with GPIO_SLEEP_DIS flags are asserted. */
#define CONFIG_GPIO_DISABLE_SLEEP

/* Flash configuration */
#undef CONFIG_FLASH_PSTATE
#define CONFIG_WP_ALWAYS
#define CONFIG_CMD_FLASH

#define CONFIG_CRC8

/* We're using TOP_A for partition 0, TOP_B for partition 1 */
#define CONFIG_FLASH_NVMEM
/* Offset to start of NvMem area from base of flash */
#define CONFIG_FLASH_NVMEM_OFFSET_A (CFG_TOP_A_OFF)
#define CONFIG_FLASH_NVMEM_OFFSET_B (CFG_TOP_B_OFF)
/* Address of start of Nvmem area */
#define CONFIG_FLASH_NVMEM_BASE_A                                              \
	(CONFIG_PROGRAM_MEMORY_BASE + CONFIG_FLASH_NVMEM_OFFSET_A)
#define CONFIG_FLASH_NVMEM_BASE_B                                              \
	(CONFIG_PROGRAM_MEMORY_BASE + CONFIG_FLASH_NVMEM_OFFSET_B)
#define CONFIG_FLASH_NEW_NVMEM_BASE_A                                          \
	(CONFIG_FLASH_NVMEM_BASE_A + CONFIG_FLASH_BANK_SIZE)
#define CONFIG_FLASH_NEW_NVMEM_BASE_B                                          \
	(CONFIG_FLASH_NVMEM_BASE_B + CONFIG_FLASH_BANK_SIZE)

/* Size partition in NvMem */
#define NVMEM_PARTITION_SIZE (CFG_TOP_SIZE)
#define NEW_NVMEM_PARTITION_SIZE (NVMEM_PARTITION_SIZE - CONFIG_FLASH_BANK_SIZE)
#define NEW_NVMEM_TOTAL_PAGES                                                  \
	(2 * NEW_NVMEM_PARTITION_SIZE / CONFIG_FLASH_BANK_SIZE)
/* Size in bytes of NvMem area */
#define CONFIG_FLASH_LOG
#define CONFIG_FLASH_NVMEM_SIZE (NVMEM_PARTITION_SIZE * NVMEM_NUM_PARTITIONS)
/* Enable <key, value> variable support. */
#define CONFIG_FLASH_NVMEM_VARS
#define NVMEM_CR50_SIZE 272
#define CONFIG_FLASH_NVMEM_VARS_USER_SIZE NVMEM_CR50_SIZE


/* Go to sleep when nothing else is happening */
#define CONFIG_LOW_POWER_IDLE

/* Allow multiple concurrent memory allocations. */
#define CONFIG_MALLOC

/* Enable debug cable detection */
#define CONFIG_RDD

/* Also use the cr50 as a second factor authentication */
#define CONFIG_U2F

/* Additional FIPS KAT tests. */
#define CONFIG_FIPS_RSA2048
#define CONFIG_FIPS_SW_HMAC_DRBG
#define CONFIG_FIPS_AES_CBC_256

/* USB configuration */
#define CONFIG_USB
#define CONFIG_USB_CONSOLE_STREAM
#undef CONFIG_USB_CONSOLE_TX_BUF_SIZE
#define CONFIG_USB_CONSOLE_TX_BUF_SIZE		4096
#define CONFIG_USB_I2C
#define CONFIG_USB_INHIBIT_INIT
#define CONFIG_USB_SPI_V2
#define CONFIG_USB_SERIALNO
#define DEFAULT_SERIALNO "0"

#define CONFIG_STREAM_USART
#define CONFIG_STREAM_USB
#define CONFIG_STREAM_USART1
#define CONFIG_STREAM_USART2

/* Enable Case Closed Debugging */
#define CONFIG_CASE_CLOSED_DEBUG_V1
#define CONFIG_PHYSICAL_PRESENCE
/* Loosen CCD open requirements. Only allowed in prePVT images */
#define CONFIG_CCD_OPEN_PREPVT

/* (b/262324344): Enable debugging of EPS state in NVMEM */
#define CONFIG_NVMEM_DEBUG_EPS

/* Enable read-only `flog` command for Cr50 */
#define CONFIG_CMD_FLASH_LOG

#ifdef CR50_DEV
/* Remove console commands to save space. */
#undef CONFIG_CMD_SLEEPMASK
#undef CONFIG_CMD_TIMERINFO
#undef CONFIG_CONSOLE_HISTORY
#undef CONFIG_CMD_I2C_SCAN
#undef CONFIG_CMD_I2C_XFER
#undef CONFIG_FLASH
/* Enable unsafe dev features for CCD in dev builds */
#define CONFIG_CASE_CLOSED_DEBUG_V1_UNSAFE
#define CONFIG_CMD_FLASH_LOG_UNSAFE
#define CONFIG_PHYSICAL_PRESENCE_DEBUG_UNSAFE
#define CONFIG_CMD_ROLLBACK
#endif

#define CONFIG_USB_PID 0x5014
#define CONFIG_USB_SELF_POWERED

#undef CONFIG_USB_MAXPOWER_MA
#define CONFIG_USB_MAXPOWER_MA 0

/* Need to be able to bitbang the EC UART for updates through CCD. */
#define CONFIG_UART_BITBANG

/* Enable SPI controller (SPI) module */
#define CONFIG_SPI_CONTROLLER
#define CONFIG_SPI_CONTROLLER_CONFIGURE_GPIOS
#define CONFIG_SPI_FLASH_PORT 0

/* Enable SPI peripheral (SPP) module */
#define CONFIG_SPP
#define CONFIG_TPM_SPP

#define CONFIG_RBOX
#define CONFIG_RBOX_WAKEUP

/* We don't need to send events to the AP */
#undef  CONFIG_HOSTCMD_EVENTS

/* Make most commands restricted */
#define CONFIG_CONSOLE_COMMAND_FLAGS
#define CONFIG_RESTRICTED_CONSOLE_COMMANDS
#define CONFIG_CONSOLE_COMMAND_FLAGS_DEFAULT CMD_FLAG_RESTRICTED

/* Inject the fips checksum into the image. */
#define CONFIG_FIPS_CHECKSUM
/* Include crypto stuff, both software and hardware. Enable optimizations. */
/* Use board specific version of dcrypto */
#define CONFIG_FIPS_UTIL
#define CONFIG_DCRYPTO_BOARD
#define CONFIG_UPTO_SHA512
#define CONFIG_DCRYPTO_RSA_SPEEDUP

/**
 * Make sw version equal to hw. Unlike SHA2-256, dcrypto implementation
 * of SHA2-512/384 allows to save context, so can fully replace software
 * implementation.
 */
#define CONFIG_SHA512_HW_EQ_SW

/* Don't link with third_party/cryptoc. */
#undef CONFIG_LIBCRYPTOC

/* Don't use DCRYPTO code from chip/g. */
#undef CONFIG_DCRYPTO

/*
 * This is pretty arbitrary, a rough estimate of what's required for smooth
 * Cr50 operation.
 */
#ifndef CRYPTO_TEST_SETUP
#define CONFIG_SHAREDMEM_MINIMUM_SIZE 5500
#else
/* Crypto tests require more statically allocated memory. */
#define CONFIG_SHAREDMEM_MINIMUM_SIZE 5000
#endif

/* Implement custom udelay, due to usec hwtimer imprecision. */
#define CONFIG_HW_SPECIFIC_UDELAY

#ifndef __ASSEMBLER__
#include "common.h"
#include "gpio_signal.h"

/* USB string indexes */
enum usb_strings {
	USB_STR_DESC = 0,
	USB_STR_VENDOR,
	USB_STR_PRODUCT,
	USB_STR_VERSION,
	USB_STR_CONSOLE_NAME,
	USB_STR_BLOB_NAME,
	USB_STR_HID_KEYBOARD_NAME,
	USB_STR_AP_NAME,
	USB_STR_EC_NAME,
	USB_STR_UPGRADE_NAME,
	USB_STR_SPI_NAME,
	USB_STR_SERIALNO,
	USB_STR_I2C_NAME,

	USB_STR_COUNT
};

/*
 * Device states
 *
 * Note that not all states are used by all devices.
 */
enum device_state {
	/* Initial state at boot */
	DEVICE_STATE_INIT = 0,

	/*
	 * Detect was not asserted at boot, but we're not willing to give up on
	 * the device right away so we're debouncing to see if it shows up.
	 */
	DEVICE_STATE_INIT_DEBOUNCING,

	/*
	 * Device was detected at boot, but we can't enable transmit yet
	 * because that would interfere with detection of another device.
	 */
	DEVICE_STATE_INIT_RX_ONLY,

	/* Disconnected or off, because detect is deasserted */
	DEVICE_STATE_DISCONNECTED,
	DEVICE_STATE_OFF,

	/* Device state is not knowable because we're driving detect */
	DEVICE_STATE_UNDETECTABLE,

	/* Connected or on, because detect is asserted */
	DEVICE_STATE_CONNECTED,
	DEVICE_STATE_ON,

	/*
	 * Device was connected, but we saw detect deasserted and are
	 * debouncing to see if it stays deasserted - at which point we'll
	 * decide that it's disconnected.
	 */
	DEVICE_STATE_DEBOUNCING,

	/* Device state is unknown.  Used only by legacy device_state code. */
	DEVICE_STATE_UNKNOWN,

	/* The state is being ignored. */
	DEVICE_STATE_IGNORED,

	/* Number of device states */
	DEVICE_STATE_COUNT
};

/**
 * Return the name of the device state as as string.
 *
 * @param state		State to look up
 * @return Name of the state, or "?" if no match.
 */
const char *device_state_name(enum device_state state);

/* NVMem variables. */
enum nvmem_vars {
	NVMEM_VAR_CONSOLE_LOCKED = 0,
	NVMEM_VAR_TEST_VAR,
	NVMEM_VAR_U2F_SALT,
	NVMEM_VAR_CCD_CONFIG,
	NVMEM_VAR_G2F_SALT,

	NVMEM_VARS_COUNT
};

void board_configure_deep_sleep_wakepins(void);
void ap_detect_asserted(enum gpio_signal signal);
void ec_detect_asserted(enum gpio_signal signal);
void servo_detect_asserted(enum gpio_signal signal);
void tpm_rst_deasserted(enum gpio_signal signal);
void tpm_rst_asserted(enum gpio_signal signal);
void diom4_deasserted(enum gpio_signal signal);

void post_reboot_request(void);

/* Special controls over EC and AP */
void assert_sys_rst(void);
void deassert_sys_rst(void);
void assert_ec_rst(void);
void deassert_ec_rst(void);
int is_ec_rst_asserted(void);
/* Ignore the servo state. */
void servo_ignore(int enable);

/**
 * Set up a deferred call to update CCD state.
 *
 * This will enable/disable UARTs, SPI, I2C, etc. as needed.
 */
void ccd_update_state(void);

/**
 * Return the state of the BOARD_USE_PLT_RST board strap option.
 *
 * @return 0 if option is not set, !=0 if option set.
 */
int board_use_plt_rst(void);
/**
 * Return the state of the BOARD_NEEDS_SYS_RST_PULL_UP board strap option.
 *
 * @return 0 if option is not set, !=0 if option set.
 */
int board_rst_pullup_needed(void);
/**
 * Return the state of the BOARD_PERIPH_CONFIG_I2C board strap option.
 *
 * @return 0 if option is not set, !=0 if option set.
 */
int board_tpm_uses_i2c(void);
/**
 * Return the state of the BOARD_PERIPH_CONFIG_SPI board strap option.
 *
 * @return 0 if option is not set, !=0 if option set.
 */
int board_tpm_uses_spi(void);
/**
 * Return the state of the BOARD_CLOSED_SOURCE_SET1 board strap option.
 *
 * @return 0 if option is not set, !=0 if option set.
 */
int board_uses_closed_source_set1(void);
/**
 * The board needs to wait until TPM_RST_L is asserted before deasserting
 * system reset signals.
 *
 * @return 0 if option is not set, !=0 if option set.
 */
int board_uses_closed_loop_reset(void);
/**
 * The board has all necessary I2C pins connected for INA support.
 *
 * @return 0 if option is not set, !=0 if option set.
 */
int board_has_ina_support(void);
/* The board supports EC-CR50 communication. */
int board_has_ec_cr50_comm_support(void);
int board_id_is_mismatched(void);
/* Allow for deep sleep to be enabled on AP shutdown */
int board_deep_sleep_allowed(void);
/* The board uses DIOM4 for user_pres_l */
int board_use_diom4(void);

/* Set or clear a board property flag in long life scratch. */
void board_write_prop(uint32_t flag, uint8_t enable);

void power_button_record(void);

/**
 * Enable/disable power button release interrupt.
 *
 * @param enable	Enable (!=0) or disable (==0)
 */
void power_button_release_enable_interrupt(int enable);

/* Functions needed by CCD config */
int board_battery_is_present(void);
int board_fwmp_allows_boot_policy_update(void);
int board_fwmp_allows_unlock(void);
void board_fwmp_update_policies(void);
int board_vboot_dev_mode_enabled(void);
void board_reboot_ap(void);
void board_reboot_ec(void);
/**
 * Reboot the EC
 * @param usec_delay  microseconds to delay in rebooting EC.
 *                    negative input shall be disregarded.
 */
void board_reboot_ec_deferred(int usec_delay);
void board_closed_loop_reset(void);
int board_wipe_tpm(int reset_required);
int board_is_first_factory_boot(void);

int usb_i2c_board_enable(void);
void usb_i2c_board_disable(void);

void print_ap_state(void);
void print_ap_uart_state(void);
void print_ec_state(void);
void print_pcr0(void);
void print_servo_state(void);

void pmu_check_tpm_rst(void);
int ap_is_on(void);
int ap_uart_is_on(void);
int ec_is_on(void);
int ec_is_rx_allowed(void);
int servo_is_connected(void);

/*
 * Returns nonzero value if EC reset line is taken over and should not be
 * touched by the 'standard' EC reset functions.
 */
int ec_rst_override(void);

/*
 * Assert INT_AP_L to acknowledge AP that cr50 is ready for next TPM command.
 * NOTE: must be called by ISR only.
 *
 * Returns 1 if it successfully asserted (or scheduled to assert), or
 *         0 if the extended long pulse was disabled.
 */
int assert_int_ap(void);

/*
 * Deassert INT_AP_L immediately.
 * NOTE: must be called by ISR only.
 */
void deassert_int_ap(void);

/* Register a function that should be called when INT_AP_L extension starts. */
void int_ap_register(void (*func_enable)(void));

void int_ap_extension_enable(void);
void int_ap_extension_stop_pulse(void);

/* Moving from legacy versions might require NVMEM transition. */
int board_nvmem_legacy_check_needed(void);

void set_ap_on(void);

/*
 * Trigger generation of the ITE SYNC sequence on the way up after next
 * reboot.
 */
void board_start_ite_sync(void);

/*
 * Board specific function (needs information about pinmux settings) which
 * allows to take the i2cp driver out of the 'wedged' state where the controller
 * stopped i2c access mid transaction and the periph is holding SDA low.
 */
void board_unwedge_i2cp(void);

int board_in_prod_mode(void);

/* Bit masks for each bit in TPM_BOARD_CFG register */
enum board_cfg_reg_bitmask {
	BOARD_CFG_LONG_INT_AP_BIT = BIT(0),

	BOARD_CFG_LOCKED_BIT = BIT(31),
};

/* Disable write on TPM_BOARD_CFG register. */
void board_cfg_reg_write_disable(void);

/*
 * Write on TPM_BOARD_CFG register if BOARD_CFG_LOCKED_BIT is clear.
 *
 * @param value: value to write on TPM_BOARD_CFG
 */
void board_cfg_reg_write(unsigned int value);

/*
 * Read TPM_BOARD_CFG register.
 *
 * @param TPM_BOARD_CFG register value in uint32_t type.
 */
unsigned int board_cfg_reg_read(void);

#endif /* !__ASSEMBLER__ */

/* USB interface indexes (use define rather than enum to expand them) */
#define USB_IFACE_CONSOLE 0
#define USB_IFACE_AP      1
#define USB_IFACE_EC      2
#define USB_IFACE_UPGRADE 3
#define USB_IFACE_SPI     4
#define USB_IFACE_I2C     5
#define USB_IFACE_COUNT   6

/* USB endpoint indexes (use define rather than enum to expand them) */
#define USB_EP_CONTROL   0
#define USB_EP_CONSOLE   1
#define USB_EP_AP        2
#define USB_EP_EC        3
#define USB_EP_UPGRADE   4
#define USB_EP_SPI       5
#define USB_EP_I2C       6
#define USB_EP_COUNT     7

/* UART indexes (use define rather than enum to expand them) */
#define UART_CR50	0
#define UART_AP		1
#define UART_EC		2
#define UART_NULL	0xff

#define UARTN UART_CR50

/* By default disable TPM and TPM register channels. */
#define CC_DEFAULT (CC_ALL & ~(CC_MASK(CC_TPM) | CC_MASK(CC_TPM_REG)))

/* Nv Memory users */
#ifndef __ASSEMBLER__
enum nvmem_users {
	NVMEM_TPM = 0,
	NVMEM_CR50,
	NVMEM_NUM_USERS
};
#endif

#define CONFIG_FLASH_NVMEM_VARS_USER_NUM NVMEM_CR50
#define CONFIG_RW_B

#define CONFIG_AP_RO_VERIFICATION
#define CONFIG_SPI_HASH

/* Firmware upgrade options. */
#define CONFIG_NON_HC_FW_UPDATE
#define CONFIG_USB_FW_UPDATE

#define CONFIG_I2C
#define CONFIG_I2C_CONTROLLER
#define CONFIG_I2C_PERIPH
#define CONFIG_TPM_I2CP

#define CONFIG_BOARD_ID_SUPPORT
#define CONFIG_SN_BITS_SUPPORT
#define CONFIG_EXTENDED_VERSION_INFO

#define I2C_PORT_CONTROLLER 0

#define CONFIG_BASE32
#define CONFIG_RMA_AUTH
#define CONFIG_FACTORY_MODE
#define CONFIG_RNG

#define CONFIG_EC_EFS_SUPPORT
#define CONFIG_EC_EFS2_VERSION 0

#define CONFIG_ENABLE_H1_ALERTS

/* Enable hardware backed brute force resistance feature */
#define CONFIG_PLATFORM_PINWEAVER

/*
 * Disabling p256 will result in RMA Auth falling back to the x25519 curve
 * which in turn would require extra 5328 bytes of flash space.
 */
#define CONFIG_RMA_AUTH_USE_P256
#ifndef CONFIG_RMA_AUTH_USE_P256
#define CONFIG_CURVE25519
#endif

#define CONFIG_CCD_ITE_PROGRAMMING

/*
 * Increase sizes of USB over I2C read and write queues. Sizes are are such
 * that when appropriate overheads are included, total buffer sizes are powers
 * of 2 (2^9 in both cases below).
 */
#undef CONFIG_USB_I2C_MAX_WRITE_COUNT
#undef CONFIG_USB_I2C_MAX_READ_COUNT
#define CONFIG_USB_I2C_MAX_WRITE_COUNT 508
#define CONFIG_USB_I2C_MAX_READ_COUNT 506

/* The below time constants are way longer than should be required in practice:
 *
 * Time it takes to finish processing TPM command
 */
#define TPM_PROCESSING_TIME (1 * SECOND)

/*
 * Time it takse TPM reset function to wipe out the NVMEM and reboot the
 * device.
 */
#define TPM_RESET_TIME (10 * SECOND)

/* Total time deep sleep should not be allowed while wiping the TPM. */
#define DISABLE_SLEEP_TIME_TPM_WIPE (TPM_PROCESSING_TIME + TPM_RESET_TIME)

/* Enable dump of NV cache */
#define CONFIG_CMD_DUMP_NVCACHE

/*****************************************************************************/
/*
 * Options for CRYPTO_TEST=1 images. Crypto test support takes up more space
 * than the standard image has available. Use this section to add crypto test
 * features and remove things to free up enough space to build them.
 */
#ifdef CRYPTO_TEST_SETUP
/* Enable unsafe dev features for CCD in crypto test builds */
#define CONFIG_CMD_ROLLBACK

/* Remove console commands to save space */
#undef CONFIG_CMD_ECRST
#undef CONFIG_CMD_SYSRST
#undef CONFIG_CMD_WP
#undef CONFIG_CMD_DUMP_NVMEM
#undef CONFIG_CMD_PINMUX
#undef CONFIG_CMD_GPIOCFG
#undef CONFIG_CMD_SLEEPMASK
#undef CONFIG_CMD_TIMERINFO
#undef CONFIG_CONSOLE_HISTORY
#undef CONFIG_I2C_XFER
#undef CONFIG_I2C_SCAN
#undef CONFIG_CONSOLE_CMDHELP

/* Remove features crypto test doesn't use to save space */
#undef CONFIG_AP_RO_VERIFICATION
#undef CONFIG_SPI_HASH
#endif /* CRYPTO_TEST_SETUP */
#endif /* __CROS_EC_BOARD_H */
