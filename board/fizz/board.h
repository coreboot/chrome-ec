/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Eve board configuration */

#ifndef __CROS_EC_BOARD_H
#define __CROS_EC_BOARD_H

/*
 * Allow dangerous commands.
 * TODO: Remove this config before production.
 */
#undef  CONFIG_SYSTEM_UNLOCKED
#define CONFIG_USB_PD_COMM_LOCKED

/* EC */
#define CONFIG_ADC
#define CONFIG_BOARD_VERSION
#define CONFIG_DEDICATED_RECOVERY_BUTTON
#define CONFIG_EMULATED_SYSRQ
#define CONFIG_LED_COMMON
#define CONFIG_KEYBOARD_PROTOCOL_MKBP
#define CONFIG_MKBP_USE_HOST_EVENT
#define CONFIG_DPTF
#define CONFIG_FLASH_SIZE 0x80000
#define CONFIG_FPU
#define CONFIG_I2C
#define CONFIG_I2C_MASTER
#undef  CONFIG_LID_SWITCH
#define CONFIG_POWER_BUTTON_IGNORE_LID
#define CONFIG_PWM
#define CONFIG_LTO
#define CONFIG_CHIP_PANIC_BACKUP
#define CONFIG_SPI_FLASH_REGS
#define CONFIG_SPI_FLASH_W25X40
#define CONFIG_UART_HOST 0
#define CONFIG_WATCHDOG_HELP
#define CONFIG_WIRELESS
#define CONFIG_WIRELESS_SUSPEND \
	(EC_WIRELESS_SWITCH_WLAN | EC_WIRELESS_SWITCH_WLAN_POWER)
#define WIRELESS_GPIO_WLAN GPIO_WLAN_OFF_L
#define WIRELESS_GPIO_WLAN_POWER GPIO_PP3300_DX_WLAN
#define WIRELESS_GPIO_WWAN GPIO_PP3300_DX_LTE
#define CONFIG_FANS 1
#define CONFIG_FAN_RPM_CUSTOM
#define CONFIG_THROTTLE_AP
#define CONFIG_CHIPSET_CAN_THROTTLE
#define CONFIG_PWM

/* EC console commands */
#define CONFIG_CMD_BUTTON

/* SOC */
#define CONFIG_CHIPSET_SKYLAKE
#define CONFIG_CHIPSET_HAS_PLATFORM_PMIC_RESET
#define CONFIG_CHIPSET_RESET_HOOK
#undef  CONFIG_PECI
#define CONFIG_ESPI
/* Eve and Poppy all have wires from GPIO to PCH but CONFIG_ESPI_VW_SIGNALS
 * is defined. So, those GPIOs are not used by EC. */
#define CONFIG_ESPI_VW_SIGNALS
#define CONFIG_LPC

/* Charger */
#define CONFIG_CHARGE_MANAGER

#define CONFIG_CHARGER_LIMIT_POWER_THRESH_CHG_MW 50000

#define CONFIG_CMD_PD_CONTROL
#define CONFIG_EXTPOWER_GPIO
#undef  CONFIG_EXTPOWER_DEBOUNCE_MS
#define CONFIG_EXTPOWER_DEBOUNCE_MS 1000
#define CONFIG_POWER_BUTTON
#define CONFIG_POWER_BUTTON_X86
#define CONFIG_POWER_BUTTON_INIT_IDLE
#define CONFIG_POWER_COMMON
#define CONFIG_POWER_SIGNAL_INTERRUPT_STORM_DETECT_THRESHOLD 30
#define CONFIG_DELAY_DSW_PWROK_TO_PWRBTN

/* Sensor */
#define CONFIG_TEMP_SENSOR
#define CONFIG_TEMP_SENSOR_TMP432
/* TODO(crosbug.com/p/61098): Is this the correct thermistor? */
#define CONFIG_THERMISTOR_NCP15WB

/* USB */
#undef  CONFIG_USB_CHARGER		/* dnojiri: verify */
#define CONFIG_USB_PD_ALT_MODE
#define CONFIG_USB_PD_ALT_MODE_DFP
#define CONFIG_USB_PD_CUSTOM_VDM
#define CONFIG_USB_PD_DISCHARGE
#define CONFIG_USB_PD_DISCHARGE_TCPC
#define CONFIG_USB_PD_DUAL_ROLE
#define CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE
#define CONFIG_USB_PD_LOGGING
#define CONFIG_USB_PD_PORT_COUNT 1
#define CONFIG_USB_PD_VBUS_DETECT_GPIO
#define CONFIG_USB_PD_TCPC_LOW_POWER
#define CONFIG_USB_PD_TCPM_MUX
#define CONFIG_USB_PD_TCPM_TCPCI
#define CONFIG_USB_PD_TCPM_PS8751
#define CONFIG_USB_PD_TRY_SRC
#define CONFIG_USB_POWER_DELIVERY
#define CONFIG_USBC_SS_MUX
#define CONFIG_USBC_SS_MUX_DFP_ONLY
#define CONFIG_USBC_VCONN
#define CONFIG_USBC_VCONN_SWAP

/* Charge ports */
#undef  CONFIG_DEDICATED_CHARGE_PORT_COUNT
#define CONFIG_DEDICATED_CHARGE_PORT_COUNT 1
#define DEDICATED_CHARGE_PORT 1

/* USB-A config */
#define CONFIG_USB_PORT_POWER_DUMB
#define USB_PORT_COUNT 5

/* Optional feature to configure npcx chip */
#define NPCX_UART_MODULE2	1 /* 1:GPIO64/65 as UART */
#define NPCX_JTAG_MODULE2	0 /* 0:GPIO21/17/16/20 as JTAG */
#define NPCX_TACH_SEL2		1 /* 0:GPIO40/73 1:GPIO93/A6 as TACH */

/* I2C ports */
#define I2C_PORT_TCPC0		NPCX_I2C_PORT0_0
#define I2C_PORT_EEPROM		NPCX_I2C_PORT0_1
#define I2C_PORT_BATTERY	NPCX_I2C_PORT1
#define I2C_PORT_CHARGER	NPCX_I2C_PORT1
#define I2C_PORT_PMIC		NPCX_I2C_PORT2
#define I2C_PORT_THERMAL	NPCX_I2C_PORT3

/* I2C addresses */
#define I2C_ADDR_TCPC0		0x16

/* Verify and jump to RW image on boot */
#define CONFIG_VBOOT_EFS
#define CONFIG_VBOOT_HASH
#define CONFIG_VSTORE
#define CONFIG_VSTORE_SLOT_COUNT 1

/*
 * Flash layout. Since config_flash_layout.h is included before board.h,
 * we can only overwrite (=undef/define) these parameters here.
 *
 * Flash stores 3 images: RO, RW_A, RW_B. We divide the flash by 4.
 * A public key is stored at the end of RO. Signatures are stored at the
 * end of RW_A and RW_B, respectively.
 */
#define CONFIG_RW_B
#define CONFIG_RW_B_MEM_OFF		CONFIG_RO_MEM_OFF
#undef  CONFIG_RO_SIZE
#define CONFIG_RO_SIZE			(CONFIG_FLASH_SIZE / 4)
#undef  CONFIG_RW_SIZE
#define CONFIG_RW_SIZE			CONFIG_RO_SIZE
#define CONFIG_RW_A_STORAGE_OFF		CONFIG_RW_STORAGE_OFF
#define CONFIG_RW_B_STORAGE_OFF		(CONFIG_RW_A_STORAGE_OFF + \
					 CONFIG_RW_SIZE)
#define CONFIG_RW_A_SIGN_STORAGE_OFF	(CONFIG_RW_A_STORAGE_OFF + \
					 CONFIG_RW_SIZE - CONFIG_RW_SIG_SIZE)
#define CONFIG_RW_B_SIGN_STORAGE_OFF	(CONFIG_RW_B_STORAGE_OFF + \
					 CONFIG_RW_SIZE - CONFIG_RW_SIG_SIZE)

#define CONFIG_RWSIG
#define CONFIG_RWSIG_TYPE_RWSIG
#define CONFIG_RSA
#define CONFIG_SHA256
#define CONFIG_RSA_KEY_SIZE 3072
#define CONFIG_RSA_EXPONENT_3

#ifndef __ASSEMBLER__

#include "gpio_signal.h"
#include "registers.h"

enum charge_port {
	CHARGE_PORT_TYPEC0,
	CHARGE_PORT_BARRELJACK,
};

enum power_signal {
	X86_SLP_S0_DEASSERTED,
	X86_SLP_S3_DEASSERTED,
	X86_SLP_S4_DEASSERTED,
	X86_SLP_SUS_DEASSERTED,
	X86_RSMRST_L_PGOOD,
	X86_PMIC_DPWROK,
	POWER_SIGNAL_COUNT
};

enum temp_sensor_id {
	TEMP_SENSOR_CHARGER,	/* BD99992GW SYSTHERM1 */
	TEMP_SENSOR_DRAM,	/* BD99992GW SYSTHERM2 */
	TEMP_SENSOR_EMMC,	/* BD99992GW SYSTHERM3 */
	TEMP_SENSOR_COUNT
};

enum adc_channel {
	ADC_VBUS,
	ADC_CH_COUNT
};

enum pwm_channel {
	PWM_CH_LED_RED,
	PWM_CH_LED_GREEN,
	PWM_CH_FAN,
	/* Number of PWM channels */
	PWM_CH_COUNT
};

enum fan_channel {
	FAN_CH_0,
	/* Number of FAN channels */
	FAN_CH_COUNT
};

enum mft_channel {
	MFT_CH_0,
	/* Number of MFT channels */
	MFT_CH_COUNT
};

/* TODO(crosbug.com/p/61098): Verify the numbers below. */
/*
 * delay to turn on the power supply max is ~16ms.
 * delay to turn off the power supply max is about ~180ms.
 */
#define PD_POWER_SUPPLY_TURN_ON_DELAY	30000  /* us */
#define PD_POWER_SUPPLY_TURN_OFF_DELAY	250000 /* us */

/* delay to turn on/off vconn */
#define PD_VCONN_SWAP_DELAY		5000   /* us */

/* Define typical operating power. Since Fizz doesn't have a battery to charge,
 * we're not interested in any power lower than the AP power-on threshold. */
#define PD_OPERATING_POWER_MW	CONFIG_CHARGER_LIMIT_POWER_THRESH_CHG_MW
#define PD_MAX_POWER_MW		100000
#define PD_MAX_CURRENT_MA	5000
#define PD_MAX_VOLTAGE_MV	20000

/* Board specific handlers */
int board_get_version(void);
void board_reset_pd_mcu(void);
void board_set_tcpc_power_mode(int port, int mode);
int board_get_battery_soc(void);
void led_alert(int enable);
void led_critical(void);

#endif /* !__ASSEMBLER__ */

#endif /* __CROS_EC_BOARD_H */
