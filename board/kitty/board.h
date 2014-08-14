/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* kitty board configuration */

#ifndef __BOARD_H
#define __BOARD_H

/* Optional features */
#define CONFIG_AP_HANG_DETECT
#define CONFIG_CHIPSET_TEGRA
#define CONFIG_POWER_COMMON
#define CONFIG_EXTPOWER_GPIO
#define CONFIG_HOST_COMMAND_STATUS
#define CONFIG_I2C
#define CONFIG_SPI
#define CONFIG_PWM
#define CONFIG_POWER_BUTTON
#define CONFIG_VBOOT_HASH
#define CONFIG_LED_COMMON

#ifndef __ASSEMBLER__



/* Single I2C port, where the EC is the master. */
#define I2C_PORT_MASTER 0

/* Timer selection */
#define TIM_CLOCK_MSB 3
#define TIM_CLOCK_LSB 9
#define TIM_POWER_LED 2
#define TIM_WATCHDOG  4

/* GPIO signal list */
enum gpio_signal {
	/* Inputs with interrupt handlers are first for efficiency */
	GPIO_POWER_BUTTON_L = 0,
	GPIO_SOC1V8_XPSHOLD,
	GPIO_LID_OPEN,
	GPIO_SUSPEND_L,
	GPIO_SPI1_NSS,
	GPIO_AC_PRESENT,
	GPIO_USB_CTL3,
	GPIO_USB_CTL2,
	GPIO_USB_CTL1,
	GPIO_WP_L,
	GPIO_AP_RESET_L,
	GPIO_EC_INT,
	GPIO_ENTERING_RW,
	GPIO_I2C1_SCL,
	GPIO_I2C1_SDA,
	GPIO_PMIC_PWRON_L,
	GPIO_PMIC_RESET,
	GPIO_PWR_LED0,
	GPIO_USB_ILIM_SEL,
	GPIO_USB_STATUS_L,
	GPIO_EC_BL_OVERRIDE,
	GPIO_PMIC_THERM_L,
	GPIO_PMIC_WARM_RESET_L,
	/* Number of GPIOs; not an actual GPIO */
	GPIO_COUNT
};

enum power_signal {
	TEGRA_XPSHOLD = 0,
	TEGRA_SUSPEND_ASSERTED,

	/* Number of power signals */
	POWER_SIGNAL_COUNT
};

enum pwm_channel {
	PWM_CH_POWER_LED = 0,
	/* Number of PWM channels */
	PWM_CH_COUNT
};



#endif /* !__ASSEMBLER__ */

#endif /* __BOARD_H */
