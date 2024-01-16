/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * config.h - Top-level configuration Chrome EC
 *
 * All configuration settings (CONFIG_*) are defined in this file or in a
 * sub-configuration file (config_chip.h, board.h, etc.) included by this file.
 *
 * Note that this file is included by assembly (.S) files.  Any C-isms such as
 * struct definitions or enums in a sub-configuration file MUST be guarded with
 * #ifndef __ASSEMBLER__ to prevent those C-isms from being evaluated by the
 * assembler.
 */

#ifndef __CROS_EC_CONFIG_H
#define __CROS_EC_CONFIG_H

/*
 * All config options are listed alphabetically and described here.
 *
 * If you add a new config option somewhere in the code, you must add a
 * default value here and describe what it does.
 *
 * To get a list current list, run this command:
 *    git grep " CONFIG_" | grep -o "CONFIG_[A-Za-z0-9_]\+" | sort | uniq
 *
 * Some options are #defined here to enable them by default.  Chips or boards
 * may override this by #undef'ing them in config_chip.h or board.h,
 * respectively.
 *
 * TODO(crosbug.com/p/23758): Describe all of these.  Also describe the
 * HAS_TASK_* macro and how/when it should be used vs. a config define.  And
 * BOARD_*, CHIP_*, and CHIP_FAMILY_*.
 */

/*
 * Add support for sensor FIFO:
 * define the size of the global fifo, must be a power of 2.
 */
#undef CONFIG_ACCEL_FIFO

/* The amount of free entries that trigger an interrupt to the AP. */
#undef CONFIG_ACCEL_FIFO_THRES

/*
 * Sensors in this mask are in forced mode: they needed to be polled
 * at their data rate frequency.
 */
#undef CONFIG_ACCEL_FORCE_MODE_MASK

/* Enable accelerometer interrupts. */
#undef CONFIG_ACCEL_INTERRUPTS

/*
 * Support "spoof" mode for sensors.  This allows sensors to have their values
 * spoofed to any arbitrary value.  This is useful for testing.
 */
#define CONFIG_ACCEL_SPOOF_MODE

/* Specify type of accelerometers attached. */
#undef CONFIG_ACCEL_BMA255
#undef CONFIG_ACCEL_KXCJ9
#undef CONFIG_ACCEL_KX022
#undef CONFIG_ACCEL_LIS2DH
#undef CONFIG_ACCELGYRO_LSM6DS0
#undef CONFIG_ACCELGYRO_BMI160
#undef CONFIG_ACCELGYRO_LSM6DSM

/* Support for BMI160 hardware orientation sensor */
#undef CONFIG_BMI160_ORIENTATION_SENSOR

/* Support for KIONIX KX022 hardware orientation sensor */
#undef CONFIG_KX022_ORIENTATION_SENSOR

/*
 * Define if either CONFIG_BMI160_ORIENTATION_SUPPORT or
 * CONFIG_KX022_ORIENTATION_SUPPORT is set.
 */
#undef CONFIG_ORIENTATION_SENSOR

/* Support the orientation gesture */
#undef CONFIG_GESTURE_ORIENTATION

/* Specify barometer attached */
#undef CONFIG_BARO_BMP280

/*
 * Use the old standard reference frame for accelerometers. The old
 * reference frame is:
 * Z-axis: perpendicular to keyboard, pointing up, such that if the device
 *  is sitting flat on a table, the accel reads +G.
 * X-axis: in the plane of the keyboard, pointing from the front lip to the
 *  hinge, such that if the device is oriented with the front lip touching
 *  the table and the hinge directly above, the accel reads +G.
 * Y-axis: in the plane of the keyboard, pointing to the right, such that
 *  if the device is on it's left side, the accel reads +G.
 *
 * Also, in the old reference frame, the lid accel matches the base accel
 * readings when lid is closed.
 */
#undef CONFIG_ACCEL_STD_REF_FRAME_OLD

/*
 * Define the event to raise when BMI160 interrupt.
 * Must be within TASK_EVENT_MOTION_INTERRUPT_MASK.
 */
#undef CONFIG_ACCELGYRO_BMI160_INT_EVENT

/* Set when INT2 is an ouptut */
#undef CONFIG_ACCELGYRO_BMI160_INT2_OUTPUT

/* Specify type of Gyrometers attached. */
#undef CONFIG_GYRO_L3GD20H

/*
 * Define the event to raise when LIS2DH  interrupt.
 * Must be within TASK_EVENT_MOTION_INTERRUPT_MASK.
 */
#undef CONFIG_ACCEL_LIS2DH_INT_EVENT

/* Compile chip support for analog-to-digital convertor */
#undef CONFIG_ADC

/*
 * ADC sample time selection. The value is chip-dependent.
 * TODO: Replace this with CONFIG_ADC_PROFILE entries.
 */
#undef CONFIG_ADC_SAMPLE_TIME

/* Include the ADC analog watchdog feature in the ADC code */
#define CONFIG_ADC_WATCHDOG

/*
 * Chip-dependent ADC configuration - select one.
 * SINGLE - Sample all inputs once when requested.
 * FAST_CONTINUOUS - Sample all inputs continuously using DMA, with minimal
 *                   sample time.
 */
#define CONFIG_ADC_PROFILE_SINGLE
#undef CONFIG_ADC_PROFILE_FAST_CONTINUOUS

/*
 * Some ALS modules may be connected to the EC. We need the command, and
 * specific drivers for each module.
 */
#ifdef HAS_TASK_ALS
#define CONFIG_ALS
#else
#undef CONFIG_ALS
#endif
#undef CONFIG_ALS_AL3010
#undef CONFIG_ALS_BH1730
/*
 * If defined, BH1730 uses board specific lux calculation formula parameters.
 * If not defined, BH1730 uses default parameters to calculate lux.
 */
#undef CONFIG_ALS_BH1730_LUXTH_PARAMS
#undef CONFIG_ALS_ISL29035
#undef CONFIG_ALS_OPT3001
/* Define the exact model ID present on the board: SI1141 = 41, SI1142 = 42, */
#undef CONFIG_ALS_SI114X
/* Check if the device revision is supported */
#undef CONFIG_ALS_SI114X_CHECK_REVISION
/*
 * Define the event to raise when BMI160 interrupt.
 * Must be within TASK_EVENT_MOTION_INTERRUPT_MASK.
 */
#undef CONFIG_ALS_SI114X_INT_EVENT

/*
 * Enable Si114x to operate in polling mode. This config is used in conjunction
 * with CONFIG_ALS_SI114X_INT_EVENT. When polling is enabled, the read is
 * initiated in the same manner as when interrupts are used, but the event which
 * triggers the irq_handler is generated by deferred call using a fixed delay.
 */
#undef CONFIG_ALS_SI114X_POLLING

/* Define which ALS sensor is used for dimming the lightbar when dark */
#undef CONFIG_ALS_LIGHTBAR_DIMMING

/* Support AP hang detection host command and state machine */
#undef CONFIG_AP_HANG_DETECT

/* Support AP Warm reset Interrupt. */
#undef CONFIG_AP_WARM_RESET_INTERRUPT

/* Allow proprietary communication protocols' extensions. */
#undef CONFIG_EXTENSION_COMMAND

/*
 * Support controlling the display backlight based on the state of the lid
 * switch.  The EC will disable the backlight when the lid is closed.
 *
 * The GPIO should be named GPIO_BACKLIGHT_ENABLED if active high, or
 * GPIO_BACKLIGHT_ENABLED_L if active low. See CONFIG_BACKLIGHT_LID_ACTIVE_LOW.
 */
#undef CONFIG_BACKLIGHT_LID

/*
 * The backlight GPIO pin is active low and named GPIO_BACKLIGHT_ENABLED_L
 */
#undef CONFIG_BACKLIGHT_LID_ACTIVE_LOW

/*
 * If defined, EC will enable the backlight signal only if this GPIO is
 * asserted AND the lid is open.  This supports passing the backlight-enable
 * signal from the AP through EC.
 */
#undef CONFIG_BACKLIGHT_REQ_GPIO

/* Support base32 text encoding */
#undef CONFIG_BASE32

/*****************************************************************************/
/* Battery config */

/* Support a simple battery. */
#undef CONFIG_BATTERY

/*
 * Compile battery-specific code.
 *
 * Note that some boards have their own unique battery constants / functions.
 * In this case, those are provided in board/(boardname)/battery.c, and none of
 * these are defined.
 * Defining one of these will automatically define CONFIG_BATTERY near the end
 * of this file. If you add a new config here, you'll need to update that
 * check.
 */
#undef CONFIG_BATTERY_BQ20Z453
#undef CONFIG_BATTERY_BQ27541
#undef CONFIG_BATTERY_BQ27621
#undef CONFIG_BATTERY_MAX17055

/* Compile mock battery support; used by tests. */
#undef CONFIG_BATTERY_MOCK

/* Maximum time to wake a non-responsive battery, in second */
#define CONFIG_BATTERY_PRECHARGE_TIMEOUT 30

/*
 * If defined, the charger will check a board specific function for battery hw
 * presence as an additional condition to determine if power on is allowed for
 * factory override, where allowing booting of a bare board with no battery and
 * no power button press is required.
 */
#undef CONFIG_BATTERY_HW_PRESENT_CUSTOM

/*
 * If defined, the charger will check for battery presence before attempting
 * to communicate with it. This avoids the 30 second delay when booting
 * without a battery present. Do not use with CONFIG_BATTERY_PRESENT_GPIO.
 *
 * Replace the default battery_is_present() function with a board-specific
 * implementation in board.c
 */
#undef CONFIG_BATTERY_PRESENT_CUSTOM

/*
 * If defined, GPIO which is driven low when battery is present.
 * Charger will check for battery presence before attempting to communicate
 * with it. This avoids the 30 second delay when booting without a battery
 * present. Do not use with CONFIG_BATTERY_PRESENT_CUSTOM.
 */
#undef CONFIG_BATTERY_PRESENT_GPIO

/*
 * Compile smart battery support
 *
 * For batteries which support this specification:
 * http://sbs-forum.org/specs/sbdat110.pdf)
 */
#undef CONFIG_BATTERY_SMART

/* Chemistry of the battery device */
#undef CONFIG_BATTERY_DEVICE_CHEMISTRY

/*
 * Critical battery shutdown timeout (seconds)
 *
 * If the battery is at extremely low charge (and discharging) or extremely
 * high temperature, the EC will shut itself down. This defines the timeout
 * period in seconds between the critical condition being detected and the
 * EC shutting itself down. Note that if the critical condition is corrected
 * before the timeout expiration, the EC will not shut itself down.
 *
 */
#define CONFIG_BATTERY_CRITICAL_SHUTDOWN_TIMEOUT 30

/* Perform a battery cut-off when we reach the battery critical level */
#undef CONFIG_BATTERY_CRITICAL_SHUTDOWN_CUT_OFF

/*
 * Support battery cut-off as host command and console command.
 *
 * Once defined, you have to implement a board_cut_off_battery() function
 * in board/???/battery.c file.
 */
#undef CONFIG_BATTERY_CUT_OFF

/*
 * The default delay is 1 second. Define this if a board prefers
 * different delay.
 */
#undef CONFIG_BATTERY_CUTOFF_DELAY_US

/*
 * The board-specific battery.c implements get and set functions to read and
 * write arbirary vendor-specific parameters stored in the battery.
 * See include/battery.h for prototypes.
 */
#undef CONFIG_BATTERY_VENDOR_PARAM

/*
 * TODO(crosbug.com/p/29467): allows charging of a dead battery that
 * requests nil for current and voltage. Remove this workaround when
 * possible.
 */
#undef CONFIG_BATTERY_REQUESTS_NIL_WHEN_DEAD

/*
 * Check for battery in disconnect state (similar to cut-off state). If this
 * battery is found to be in disconnect state, take it out of this state by
 * force-applying a charge current.
 */
#undef CONFIG_BATTERY_REVIVE_DISCONNECT

/*
 * Specify the battery percentage at which the host is told it is full.
 * If this value is not specified the default is 97% set in battery.h.
 */
#undef CONFIG_BATTERY_LEVEL_NEAR_FULL

/*
 * Use an alternative method to store battery information: Instead of writing
 * directly to host memory mapped region, this keeps the battery information in
 * ec_response_battery_static/dynamic_info structures, that can then be fetched
 * using host commands, or via EC_ACPI_MEM_BATTERY_INDEX command, which tells
 * the EC to update the shared memory.
 *
 * This is required on dual-battery systems, and on on hostless bases with a
 * battery.
 */
#undef CONFIG_BATTERY_V2

/*
 * Number of batteries, only matters when CONFIG_BATTERY_V2 is used.
 */
#undef CONFIG_BATTERY_COUNT

/*
 * Expose some data when it is needed.
 * For example, battery disconnect state
 */
#undef CONFIG_CHARGE_STATE_DEBUG

/* Include support for Bluetooth LE */
#undef CONFIG_BLUETOOTH_LE

/* Include support for testing the radio for Bluetooth LE */
#undef CONFIG_BLUETOOTH_LE_RADIO_TEST

/* Include support for the HCI and link layers for Bluetooth LE */
#undef CONFIG_BLUETOOTH_LE_STACK

/* Include debugging support for the Bluetooth link layer */
#undef CONFIG_BLUETOOTH_LL_DEBUG

/* Include debugging support for Bluetooth HCI */
#undef CONFIG_BLUETOOTH_HCI_DEBUG

/* Boot header storage offset. */
#undef CONFIG_BOOT_HEADER_STORAGE_OFF

/* Size of boot header in storage. */
#undef CONFIG_BOOT_HEADER_STORAGE_SIZE

/*****************************************************************************/

/* EC has GPIOs to allow board to reset RTC */
#undef CONFIG_BOARD_HAS_RTC_RESET

/*
 * Call board_before_rsmrst(state) before passing RSMRST# to the AP.
 * This is for board workarounds that are required after rails are up
 * but before the AP is out of reset.
 */
#undef CONFIG_BOARD_HAS_BEFORE_RSMRST

/*
 * Call board_config_post_gpio_init() after GPIOs are initialized.  See
 * include/board_config.h for more information.
 */
#undef CONFIG_BOARD_POST_GPIO_INIT

/*
 * Call board_config_pre_init() before any inits are called.  See
 * include/board_config.h for more information.
 */
#undef CONFIG_BOARD_PRE_INIT

/* EC has GPIOs attached to board version stuffing resistors */
#undef CONFIG_BOARD_VERSION
/* The decoding of the GPIOs defining board version is defined in board code */
#undef CONFIG_BOARD_SPECIFIC_VERSION

/* EC responses to a board defined I2C slave address */
#undef CONFIG_BOARD_I2C_SLAVE_ADDR

/* Permanent LM4 boot configuration */
#undef CONFIG_BOOTCFG_VALUE

/*****************************************************************************/
/* Modify the default behavior to make system bringup easier. */
#undef CONFIG_BRINGUP

/*
 * Enable debug prints / asserts that may helpful for debugging board bring-up,
 * but probably shouldn't be enabled for production for performance reasons.
 */
#undef CONFIG_DEBUG_BRINGUP

/*****************************************************************************/

/*
 * Support for entering recovery mode using the volume buttons or a dedicated
 * recovery button.  Note that these are *buttons* and not keys in the keyboard
 * matrix.
 */
#undef CONFIG_BUTTON_TRIGGERED_RECOVERY

/*
 * Indicates there is a dedicated recovery button.  Note, that if there are
 * volume buttons, a dedicated recovery button is not needed.  This is intended
 * because if a board has volume buttons, they can do everything a dedicated
 * recovery button can do.
 */
#undef CONFIG_DEDICATED_RECOVERY_BUTTON

/*
 * The board has volume up and volume down buttons.  Note, these are *buttons*
 * and not keys in the keyboard matrix.
 */
#undef CONFIG_VOLUME_BUTTONS

/* Support V1 CCD configuration */
#undef CONFIG_CASE_CLOSED_DEBUG_V1
/* Allow unsafe debugging functionality in V1 configuration */
#undef CONFIG_CASE_CLOSED_DEBUG_V1_UNSAFE

/*
 * Capsense chip has buttons, too.
 */
#undef CONFIG_CAPSENSE

/*****************************************************************************/
/* Support CEC */
#undef CONFIG_CEC

/*****************************************************************************/

/* Compile charge manager */
#undef CONFIG_CHARGE_MANAGER

/* Number of charge ports excluding type-c ports */
#define CONFIG_DEDICATED_CHARGE_PORT_COUNT 0

/* Allow charge manager to default to charging from dual-role partners */
#undef CONFIG_CHARGE_MANAGER_DRP_CHARGING

/* Handle the external power limit host command in charge manager */
#undef CONFIG_CHARGE_MANAGER_EXTERNAL_POWER_LIMIT

/* Initially enter safe mode, with relaxed port / current selection rules */
#define CONFIG_CHARGE_MANAGER_SAFE_MODE

/* Leave safe mode when battery pct meets or exceeds this value */
#define CONFIG_CHARGE_MANAGER_BAT_PCT_SAFE_MODE_EXIT 2

/* The hardware has some input current ramping/back-off mechanism */
#undef CONFIG_CHARGE_RAMP_HW

/* Compile input current ramping support using software control */
#undef CONFIG_CHARGE_RAMP_SW

/*****************************************************************************/
/* Charger config */

/* Compile common charge state code. You must pick an implementation. */
#undef CONFIG_CHARGER
#undef CONFIG_CHARGER_V2

/* Compile charger-specific code for these chargers (pick at most one) */
#undef CONFIG_CHARGER_BD9995X
#undef CONFIG_CHARGER_BQ24707A
#undef CONFIG_CHARGER_BQ24715
#undef CONFIG_CHARGER_BQ24725
#undef CONFIG_CHARGER_BQ24735
#undef CONFIG_CHARGER_BQ24738
#undef CONFIG_CHARGER_BQ24770
#undef CONFIG_CHARGER_BQ24773
#undef CONFIG_CHARGER_BQ25890
#undef CONFIG_CHARGER_BQ25892
#undef CONFIG_CHARGER_BQ25895
#undef CONFIG_CHARGER_ISL9237
#undef CONFIG_CHARGER_ISL9238
#undef CONFIG_CHARGER_RT9466
#undef CONFIG_CHARGER_RT9467
#undef CONFIG_CHARGER_SY21612

/*
 * Enable the CHG_EN at initialization to turn-on the BGATE which allows voltage
 * to be applied to the battery PACK & wakes the battery if it is in shipmode.
 */
#undef CONFIG_CHARGER_BD9995X_CHGEN

/*
 * BD9995X Power Save Mode
 *
 * Which power save mode should the charger enter when VBUS is removed.  Check
 * driver/bd9995x.h for the power save settings.  By default, no power save mode
 * is enabled.
 */
#undef CONFIG_BD9995X_POWER_SAVE_MODE

/*
 * If the battery temperature sense pin is connected to charger,
 * get the battery temperature from the charger.
 */
#undef CONFIG_CHARGER_BATTERY_TSENSE

/*
 * BQ2589x IR Compensation settings.
 * Should be the combination of BQ2589X_IR_TREG_xxxC, BQ2589X_IR_VCLAMP_yyyMV
 * and  BQ2589X_IR_BAT_COMP_zzzMOHM.
 */
#undef CONFIG_CHARGER_BQ2589X_IR_COMP
/*
 * BQ2589x 5V boost current limit and voltage.
 * Should be the combination of BQ2589X_BOOSTV_MV(voltage) and
 * BQ2589X_BOOST_LIM_xxxMA.
 */
#undef CONFIG_CHARGER_BQ2589X_BOOST

/*
 * Board specific charging current limit, in mA.  If defined, the charge state
 * machine will not allow the battery to request more current than this.
 */
#undef CONFIG_CHARGER_CURRENT_LIMIT

/*
 * Enable/disable system power monitor PSYS function: this enables output
 * from charger chip to SoC.
 */
#undef CONFIG_CHARGER_PSYS

/*
 * Enable reading PSYS (system power) value, either via "psys" console command,
 * or via charger_get_system_power function.
 */
#undef CONFIG_CHARGER_PSYS_READ

/*
 * Board specific charging current termination limit, in mA.  If defined and
 * charger supports setting termination current it should be set during charger
 * init.
 *
 * TODO(tbroch): Only valid for bq2589x currently.  Configure defaults for other
 * charger ICs that support termination currents.
 */
#undef CONFIG_CHARGER_TERM_CURRENT_LIMIT

/*
 * Board supports discharge mode.  In this mode, the battery will discharge
 * even if AC is present.  Used for testing.
 */
#undef CONFIG_CHARGER_DISCHARGE_ON_AC

/* Board has a custom discharge mode. */
#undef CONFIG_CHARGER_DISCHARGE_ON_AC_CUSTOM

/*
 * Board specific flag used to disable external ILIM pin used to determine input
 * current limit. When defined, the input current limit is decided only by
 * the software register value.
 */
#undef CONFIG_CHARGER_ILIM_PIN_DISABLED

/*
 * Default input current for the board, in mA.
 *
 * This value should depend on external power adapter, designed charging
 * voltage, and the maximum power of the running system. For type-C chargers,
 * this should be set to 512 mA in order to not brown-out low-current USB
 * charge ports.
 */
#undef CONFIG_CHARGER_INPUT_CURRENT

/*
 * Board specific maximum input current limit, in mA.
 */
#undef CONFIG_CHARGER_MAX_INPUT_CURRENT

/*
 * Leave charger VBAT configured to battery-requested voltage under all
 * conditions, even when AC is not present. This may be necessary to work
 * around quirks of certain charger chips, such as the BD9995X.
 */
#undef CONFIG_CHARGER_MAINTAIN_VBAT

/* Minimum battery percentage for power on */
#undef CONFIG_CHARGER_MIN_BAT_PCT_FOR_POWER_ON

/* Set this option when using a Narrow VDC (NVDC) charger, such as ISL9237/8. */
#undef CONFIG_CHARGER_NARROW_VDC

/*
 * Low energy thresholds - when battery level is below BAT_PCT and an external
 * charger provides less than CHG_MW of power, inform the AP of the situation
 * through the LIMIT_POWER host event.
 */
#undef CONFIG_CHARGER_LIMIT_POWER_THRESH_BAT_PCT
#undef CONFIG_CHARGER_LIMIT_POWER_THRESH_CHG_MW

/*
 * Enable charger's OTG functions, i.e. make it possible to supply output power
 * from battery.
 */
#undef CONFIG_CHARGER_OTG

/*
 * Charger should call battery_override_params() to limit/correct the voltage
 * and current requested by the battery pack before acting on the request.
 *
 * This is valid with CONFIG_CHARGER_V2 only.
 */
#undef CONFIG_CHARGER_PROFILE_OVERRIDE

/*
 * Common code for charger profile override. Should be used with
 * CONFIG_CHARGER_PROFILE_OVERRIDE.
 */
#undef CONFIG_CHARGER_PROFILE_OVERRIDE_COMMON

/*
 * Battery voltage threshold ranges for charge profile override.
 * Override it in board.h if battery has multiple threshold ranges.
 */
#define CONFIG_CHARGER_PROFILE_VOLTAGE_RANGES 2

/* Value of the charge sense resistor, in mOhms */
#undef CONFIG_CHARGER_SENSE_RESISTOR

/* Value of the input current sense resistor, in mOhms */
#undef CONFIG_CHARGER_SENSE_RESISTOR_AC

/*
 * Board has an GPIO pin to enable or disable charging.
 *
 * This GPIO should be named GPIO_CHARGER_EN, if active high. Or
 * GPIO_CHARGER_EN_L if active low.
 */
#undef CONFIG_CHARGER_EN_GPIO

/* Charger enable GPIO is active low */
#undef CONFIG_CHARGER_EN_ACTIVE_LOW

/* Enable trickle charging */
#undef CONFIG_TRICKLE_CHARGING

/*****************************************************************************/

/*
 * Chip needs to do pre-init very early in main(), and provides chip_pre_init()
 * to do so.
 */
#undef CONFIG_CHIP_PRE_INIT

/*****************************************************************************/
/* Chipset config */

/* AP chipset support; pick at most one */
#undef CONFIG_CHIPSET_APOLLOLAKE/* Intel Apollolake (x86) */
#undef CONFIG_CHIPSET_BRASWELL  /* Intel Braswell (x86) */
#undef CONFIG_CHIPSET_CANNONLAKE /* Intel Cannonlake (x86) */
#undef CONFIG_CHIPSET_ECDRIVEN  /* Dummy power module */
#undef CONFIG_CHIPSET_MEDIATEK  /* MediaTek MT81xx */
#undef CONFIG_CHIPSET_RK3399    /* Rockchip rk3399 */
/* TODO: Rename below config to CONFIG_CHIPSET_RK32XX */
#undef CONFIG_CHIPSET_ROCKCHIP  /* Rockchip rk32xx */
#undef CONFIG_CHIPSET_SKYLAKE   /* Intel Skylake (x86) */
#undef CONFIG_CHIPSET_STONEY     /* AMD Stoney (x86)*/

/* Support chipset throttling */
#undef CONFIG_CHIPSET_CAN_THROTTLE

/* Enable additional chipset debugging */
#undef CONFIG_CHIPSET_DEBUG

/* Enable chipset reset hook, requires a deferrable function */
#undef CONFIG_CHIPSET_RESET_HOOK

/* Support power rail control */
#define CONFIG_CHIPSET_HAS_PP1350
#define CONFIG_CHIPSET_HAS_PP5000

/* Support PMIC reset(using LDO_EN) in chipset */
#undef CONFIG_CHIPSET_HAS_PLATFORM_PMIC_RESET

/* Redefine when we need a different power-on sequence on the same chipset. */
#define CONFIG_CHIPSET_POWER_SEQ_VERSION 0

/*****************************************************************************/
/*
 * Chip config for clock circuitry
 *	define = crystal / undef = oscillator
 */
#undef CONFIG_CLOCK_CRYSTAL

/* Indicate if a clock source is connected to stm32f4's "HSE" specific input */
#undef CONFIG_STM32_CLOCK_HSE_HZ

/* Indicate if a clock source is connected to "LSE" specific input */
#undef CONFIG_STM32_CLOCK_LSE

/*
 * Chip config for clock source
 *	 define = external crystal oscillator / undef = internal clock source
 */
#undef CONFIG_CLOCK_SRC_EXTERNAL

/*****************************************************************************/
/* Support curve25519 public key cryptography */
#undef CONFIG_CURVE25519

/*****************************************************************************/
/* PMIC config */

/* Support firmware long press power-off timer */
#undef CONFIG_PMIC_FW_LONG_PRESS_TIMER

/* Support PMIC power control */
#undef CONFIG_PMIC

/*****************************************************************************/
/*
 * Optional console commands
 *
 * Defining these options will enable the corresponding command on the EC
 * console.
 */

#undef  CONFIG_CMD_ACCELS
#undef  CONFIG_CMD_ACCEL_FIFO
#undef  CONFIG_CMD_ACCEL_INFO
#define CONFIG_CMD_ACCELSPOOF
#define CONFIG_CMD_ADC
#undef  CONFIG_CMD_ALS
#define CONFIG_CMD_APTHROTTLE
#undef  CONFIG_CMD_BATDEBUG
#define CONFIG_CMD_BATTFAKE
#undef  CONFIG_CMD_BATT_MFG_ACCESS
#undef  CONFIG_CMD_BUTTON
#undef  CONFIG_CMD_CCD_DISABLE  /* 'ccd disable' subcommand */
#define CONFIG_CMD_CHARGER
#undef  CONFIG_CMD_CHARGER_ADC_AMON_BMON
#undef  CONFIG_CMD_CHARGER_PROFILE_OVERRIDE
#undef  CONFIG_CMD_CHARGER_PROFILE_OVERRIDE_TEST
#define CONFIG_CMD_CHARGE_SUPPLIER_INFO
#undef  CONFIG_CMD_CHGRAMP
#undef  CONFIG_CMD_CLOCKGATES
#undef  CONFIG_CMD_COMXTEST
#define CONFIG_CMD_CRASH
#define CONFIG_CMD_DEVICE_EVENT
#undef  CONFIG_CMD_ECTEMP
#define CONFIG_CMD_FASTCHARGE
#undef  CONFIG_CMD_FLASH
#define CONFIG_CMD_FLASHINFO
#undef  CONFIG_CMD_FLASH_TRISTATE
#undef  CONFIG_CMD_FORCETIME
#undef  CONFIG_CMD_GPIO_EXTENDED
#undef  CONFIG_CMD_GSV
#define CONFIG_CMD_HASH
#define CONFIG_CMD_HCDEBUG
#undef  CONFIG_CMD_HOSTCMD
#undef  CONFIG_CMD_I2CWEDGE
#undef  CONFIG_CMD_I2C_PROTECT
#define CONFIG_CMD_I2C_SCAN
#undef  CONFIG_CMD_I2C_STRESS_TEST
#undef  CONFIG_CMD_I2C_STRESS_TEST_ACCEL
#undef  CONFIG_CMD_I2C_STRESS_TEST_ALS
#undef  CONFIG_CMD_I2C_STRESS_TEST_BATTERY
#undef  CONFIG_CMD_I2C_STRESS_TEST_CHARGER
#undef  CONFIG_CMD_I2C_STRESS_TEST_TCPC
#define CONFIG_CMD_I2C_XFER
#define CONFIG_CMD_IDLE_STATS
#undef  CONFIG_CMD_ILIM
#define CONFIG_CMD_INA
#undef  CONFIG_CMD_JUMPTAGS
#define CONFIG_CMD_KEYBOARD
#undef CONFIG_CMD_LEDTEST
#undef  CONFIG_CMD_LID_ANGLE
#undef  CONFIG_CMD_MCDP
#define CONFIG_CMD_MD
#define CONFIG_CMD_MEM
#define CONFIG_CMD_MMAPINFO
#define CONFIG_CMD_PD
#undef  CONFIG_CMD_PD_CONTROL
#undef  CONFIG_CMD_PD_DEV_DUMP_INFO
#undef  CONFIG_CMD_PD_FLASH
#undef  CONFIG_CMD_PLL
#undef  CONFIG_CMD_PMU
#define CONFIG_CMD_POWERINDEBUG
#undef  CONFIG_CMD_POWERLED
#define CONFIG_CMD_PWR_AVG
#define CONFIG_CMD_POWER_AP
#undef  CONFIG_CMD_PPC_DUMP
#define CONFIG_CMD_REGULATOR
#undef  CONFIG_CMD_RTC
#undef  CONFIG_CMD_RTC_ALARM
#define CONFIG_CMD_RW
#undef  CONFIG_CMD_SCRATCHPAD
#define CONFIG_CMD_SHMEM
#undef  CONFIG_CMD_SLEEP
#define CONFIG_CMD_SLEEPMASK
#undef  CONFIG_CMD_SPI_FLASH
#undef  CONFIG_CMD_SPI_NOR
#undef  CONFIG_CMD_SPI_XFER
#undef  CONFIG_CMD_STACKOVERFLOW
#define CONFIG_CMD_SYSINFO
#define CONFIG_CMD_SYSJUMP
#define CONFIG_CMD_SYSLOCK
#undef  CONFIG_CMD_TASKREADY
#define CONFIG_CMD_TEMP_SENSOR
#define CONFIG_CMD_TIMERINFO
#undef  CONFIG_CMD_TPM_LOG
#define CONFIG_CMD_TYPEC
#undef  CONFIG_CMD_USART_INFO
#define CONFIG_CMD_USBMUX
#undef  CONFIG_CMD_USB_PD_PE
#define CONFIG_CMD_WAITMS
#undef  CONFIG_CMD_AP_RESET_LOG

/*****************************************************************************/

/* Provide common core code to output panic information without interrupts. */
#define CONFIG_COMMON_PANIC_OUTPUT

/*
 * Store a panic log and halt the system for a software-related reasons, such as
 * stack overflow or assertion failure.
 */
#undef CONFIG_SOFTWARE_PANIC

/*
 * Certain platforms(e.g. eve, poppy) cannot retain panic info in data ram since
 * VCC is powered down on EC reset. On such platforms, panic data needs to be
 * saved/restored to persistent storage by using chip specific
 * implementations. This option can be enabled by those platforms that have and
 * wish to use chip-implemented panic backup/restore functions.
 */
#undef CONFIG_CHIP_PANIC_BACKUP

/*
 * Provide the default GPIO abstraction layer.
 * You want this unless you are doing a really tiny firmware.
 */
#define CONFIG_COMMON_GPIO

/*
 * Provides smaller GPIO names to reduce flash size.  Instead of the 'name'
 * field in GPIO macro it will concat 'port' and 'pin' to reduce flash size.
 */
#undef CONFIG_COMMON_GPIO_SHORTNAMES

/*
 * EC's supporting powering down GPIO pins.
 * Add flag GPIO_POWER_DOWN and additional API's.
 */
#undef CONFIG_GPIO_POWER_DOWN

/*
 * Provide common runtime layer code (tasks, hooks ...)
 * You want this unless you are doing a really tiny firmware.
 */
#define CONFIG_COMMON_RUNTIME

/* Provide common core code to handle the operating system timers. */
#define CONFIG_COMMON_TIMER

/*****************************************************************************/

/*
 * Provide additional help on console commands, such as the supported
 * options/usage.
 *
 * Boards may #undef this to reduce image size.
 */
#define CONFIG_CONSOLE_CMDHELP

/*
 * Add a .flags field to the console commands data structure, to distinguish
 * some commands from others. The available flags bits are defined in
 * include/console.h
 */
#undef CONFIG_CONSOLE_COMMAND_FLAGS

/*
 * One use of the .flags field is to make some console commands restricted, so
 * that they can be disabled or enabled at run time.
 */
#undef CONFIG_RESTRICTED_CONSOLE_COMMANDS

/* The default .flags field value is zero, unless overridden with this. */
#undef CONFIG_CONSOLE_COMMAND_FLAGS_DEFAULT

/*
 * Enable EC_CMD_CONSOLE_READ V1. One could disable this config to prevent
 * kernel from creating the `console_log` debugfs entry.
 */
#define CONFIG_CONSOLE_ENABLE_READ_V1

/*
 * Number of entries in console history buffer.
 *
 * Boards may #undef this to reduce memory usage.
 */
#define CONFIG_CONSOLE_HISTORY 8

/* Max length of a single line of input */
#define CONFIG_CONSOLE_INPUT_LINE_SIZE 80

/* Enable verbose output to UART console and extra timestamp print precision. */
#define CONFIG_CONSOLE_VERBOSE

/*****************************************************************************/
/* Support for EC-EC communication */

/*
 * Board is master or slave in EC-EC communication.
 */
#undef CONFIG_EC_EC_COMM_MASTER
#undef CONFIG_EC_EC_COMM_SLAVE

/*
 * Board support battery-related functions in EC-EC communication.
 */
#undef CONFIG_EC_EC_COMM_BATTERY

/*
 * Enable the experimental console.
 *
 * NOTE: If you enable this experimental console, you will need to run the
 * EC-3PO interactive console in the util directory!  Otherwise, you won't be
 * able to enter any commands.
 */
#undef CONFIG_EXPERIMENTAL_CONSOLE

/* Include CRC-8 utility function */
#undef CONFIG_CRC8

/*
 * When enabled, do not build RO image from the same set of files as the RW
 * image. Instead define a separate set of object files in the respective
 * build.mk files by adding the objects to the custom-ro_objs-y variable.
 */
#undef CONFIG_CUSTOMIZED_RO

/*
 * When enabled, build in support for software & hardware crypto;
 * only supported on CR50.
 */
#undef CONFIG_DCRYPTO

/*
 * When enabled, accelerate sha512 using the generic crypto engine;
 * only supported on CR50
 */
#undef CONFIG_DCRYPTO_SHA512

/*
 * When enabled build support for SHA-384/512, requires CONFIG_DCRYPTO.
 */
#undef CONFIG_UPTO_SHA512

/*
 * When enabled ignore version et al during fw upgrade for chip/g.
 */
#undef CONFIG_IGNORE_G_UPDATE_CHECKS

/*
 * When enabled hardware alerts statistics provided via VendorCommand extension.
 */
#undef CONFIG_ENABLE_H1_ALERTS

/*
 * Enable console shell command 'alerts' that prints chip alerts statistics.
 */
#undef CONFIG_ENABLE_H1_ALERTS_CONSOLE

/*****************************************************************************/
/*
 * Debugging config
 *
 * Note that these options are enabled by default, because they're really
 * handy for debugging systems during bringup and even at factory time.
 *
 * A board may undefine any or all of these to reduce image size and RAM usage,
 * at the cost of debuggability.
 */

/*
 * ASSERT() macros are checked at runtime.  See CONFIG_DEBUG_ASSERT_REBOOTS
 * to see what happens if one fails.
 *
 * Boards may #undef this to reduce image size.
 */
#define CONFIG_DEBUG_ASSERT

/*
 * Prints a message and reboots if an ASSERT() macro fails at runtime.  When
 * enabled, an ASSERT() which fails will produce a message of the form:
 *
 * ASSERTION FAILURE '<expr>' in function() at file:line
 *
 * If this is not defined, failing ASSERT() will trigger a BKPT instruction
 * instead.
 *
 * Ignored if CONFIG_DEBUG_ASSERT is not defined.
 *
 * Boards may #undef this to reduce image size.
 */
#define CONFIG_DEBUG_ASSERT_REBOOTS

/*
 * On assertion failure, prints only the file name and the line number.
 *
 * Ignored if CONFIG_DEBUG_ASSERT_REBOOTS is not defined.
 *
 * Boards may define this to reduce image size.
 */
#undef CONFIG_DEBUG_ASSERT_BRIEF

/*
 * Disable the write buffer used for default memory map accesses.
 * This turns "Imprecise data bus errors" into "Precise" errors
 * in exception traces at the cost of some performance.
 * This may help identify the offending instruction causing an
 * exception. Supported on cortex-m.
 */
#undef CONFIG_DEBUG_DISABLE_WRITE_BUFFER

/*
 * Print additional information when exceptions are triggered, such as the
 * fault address, here shown as bfar. This shows the reason for the fault
 * and may help to determine the cause.
 *
 *	=== EXCEPTION: 03 ====== xPSR: 01000000 ===========
 *	r0 :0000000b r1 :00000047 r2 :60000000 r3 :200013dd
 *	r4 :00000000 r5 :080053f4 r6 :200013d0 r7 :00000002
 *	r8 :00000000 r9 :200013de r10:00000000 r11:00000000
 *	r12:00000000 sp :200009a0 lr :08002b85 pc :08003a8a
 *	Precise data bus error, Forced hard fault, Vector catch, bfar = 60000000
 *	mmfs = 00008200, shcsr = 00000000, hfsr = 40000000, dfsr = 00000008
 *
 * If this is not defined, only a register dump will be printed.
 *
 * Boards may #undef this to reduce image size.
 */
#define CONFIG_DEBUG_EXCEPTIONS

/*
 * Print orientation when device orientation changes
 * (requires CONFIG_SENSOR_ORIENTATION)
 */
#undef CONFIG_DEBUG_ORIENTATION

/* Support Synchronous UART debug printf. */
#undef CONFIG_DEBUG_PRINTF

/* Check for stack overflows on every context switch */
#define CONFIG_DEBUG_STACK_OVERFLOW

/*****************************************************************************/

/* Support events from devices attached to the EC */
#undef CONFIG_DEVICE_EVENT

/* Monitor the states of other devices */
#undef CONFIG_DEVICE_STATE

/* Support DMA transfers inside the EC */
#undef CONFIG_DMA

/* Use the common interrupt handlers for DMA IRQs */
#define CONFIG_DMA_DEFAULT_HANDLERS

/* Compile extra debugging and tests for the DMA module */
#undef CONFIG_DMA_HELP

/* Support EC to Internal bus bridge. */
#undef CONFIG_EC2I

/* Usually, EC capable of sensor speeds up to 200000 mHz */
#define CONFIG_EC_MAX_SENSOR_FREQ_DEFAULT_MILLIHZ 200000

/* Maximal EC sampling rate */
#undef CONFIG_EC_MAX_SENSOR_FREQ_MILLIHZ

/*
 * Allow board to override the feature bitmap provided through host command
 * and ACPI.
 */
#undef CONFIG_EC_FEATURE_BOARD_OVERRIDE

/* Support EC chip internal data EEPROM */
#undef CONFIG_EEPROM

/*
 * Support for sending emulated sysrq events to AP (on designs with a keyboard,
 * sysrq is passed as normal key presses).
 */
#undef CONFIG_EMULATED_SYSRQ

/* Support for eSPI for host communication */
#undef CONFIG_ESPI

/* Use Virtual Wire signals instead of GPIO with eSPI interface */
#undef CONFIG_ESPI_VW_SIGNALS

/* MCHP next two items are EC eSPI slave configuration */
/* Maximum clock frequence eSPI EC slave advertises
 * Values in MHz are 20, 25, 33, 50, and 66
 */
#undef CONFIG_ESPI_EC_MAX_FREQ

/* EC eSPI slave advertises IO lanes
 * 0 = Single
 * 1 = Single and Dual
 * 2 = Single and Quad
 * 3 = Single, Dual, and Quad
 */
#undef CONFIG_ESPI_EC_MODE

/* Bit map of eSPI channels EC advertises
 * bit[0] = 1 Peripheral channel
 * bit[1] = 1 Virtual Wire channel
 * bit[2] = 1 OOB channel
 * bit[3] = 1 Flash channel
 */
#undef CONFIG_ESPI_EC_CHAN_BITMAP

/* Use Virtual Wire for Platform Reset instead of a sideband signal */
#undef CONFIG_ESPI_PLTRST_IS_VWIRE


/* Include code for handling external power */
#define CONFIG_EXTPOWER

/* Support detecting external power presence via a GPIO */
#undef CONFIG_EXTPOWER_GPIO

/* Default debounce time for external power signal */
#define CONFIG_EXTPOWER_DEBOUNCE_MS 30

/*****************************************************************************/
/* Number of cooling fans. Undef if none. */
#undef CONFIG_FANS

/* Percentage to which all fans are set at initiation */
#define CONFIG_FAN_INIT_SPEED 100

/* Support fan control while in low-power idle */
#undef CONFIG_FAN_DSLEEP

/*
 * Replace the default fan_percent_to_rpm() function with a board-specific
 * implementation in board.c
 */
#undef CONFIG_FAN_RPM_CUSTOM

/*
 * We normally check and update the fans once per second (HOOK_SECOND). If this
 * is #defined to a postive integer N, we will only update the fans every N
 * seconds instead.
 */
#undef CONFIG_FAN_UPDATE_PERIOD

/* Send event when mode change, host read acpi memory and select DPTF table */
#undef CONFIG_DPTF_DEVICE_ORIENTATION

/*****************************************************************************/
/* Flash configuration */

/* This enables console commands and higher-level features */
#define CONFIG_FLASH
/* This enables chip-specific access functions */
#define CONFIG_FLASH_PHYSICAL
#undef CONFIG_FLASH_BANK_SIZE
#undef CONFIG_FLASH_ERASED_VALUE32
#undef CONFIG_FLASH_ERASE_SIZE
#undef CONFIG_FLASH_ROW_SIZE
/* Allow deferred (async) flash erase */
#undef CONFIG_FLASH_DEFERRED_ERASE
/* Flash must be selected for write/erase operations to succeed. */
#undef CONFIG_FLASH_SELECT_REQUIRED

/* Base address of program memory */
#undef CONFIG_PROGRAM_MEMORY_BASE

/*
 * EC code can reside on internal or external storage. Only one of these
 * CONFIGs should be defined.
 */
#undef CONFIG_EXTERNAL_STORAGE
#undef CONFIG_INTERNAL_STORAGE

/*
 * Flash is directly mapped into the EC's address space.  If this is not
 * defined, the flash driver must implement flash_physical_read().
 */
#define CONFIG_MAPPED_STORAGE

/*
 * Base address of memory-mapped flash storage, for platforms which define
 * CONFIG_MAPPED_STORAGE.
 */
#undef CONFIG_MAPPED_STORAGE_BASE

#undef CONFIG_FLASH_PROTECT_NEXT_BOOT

/*
 * Some platforms need to write protect RW independently of all flash.
 */
#undef CONFIG_FLASH_PROTECT_RW

/*
 * Store persistent write protect for the flash inside the flash data itself.
 * This allows ECs with internal flash to emulate something closer to a SPI
 * flash write protect register.  If this is not defined, write protect state
 * is maintained solely by the physical flash driver.
 */
#define CONFIG_FLASH_PSTATE

/*
 * Store the pstate data in its own dedicated bank of flash.  This allows
 * disabling the protect-RO-at-boot flag without rewriting the RO firmware,
 * but costs a bank of flash.
 *
 * If this is not defined, the pstate data is stored inside the RO firmware
 * image itself.  This is more space-efficient, but the only way to clear the
 * flag once it's set is to rewrite the RO firmware (after removing the WP
 * screw, of course).
 */
#define CONFIG_FLASH_PSTATE_BANK

/*
 * Lock the PSTATE by default (currently only supported when
 * CONFIG_FLASH_PSTATE_BANK is not defined).
 */
#undef CONFIG_FLASH_PSTATE_LOCKED

/*
 * For flash that is segemented in different regions.
 */
#undef CONFIG_FLASH_MULTIPLE_REGION
/* Number of regions of different size/type */
#undef CONFIG_FLASH_REGION_TYPE_COUNT

/* Total size of writable flash */
#undef CONFIG_FLASH_SIZE

/* Minimum flash write size (in bytes) */
#undef CONFIG_FLASH_WRITE_SIZE
/* Most efficient flash write size (in bytes) */
#undef CONFIG_FLASH_WRITE_IDEAL_SIZE

/* Protected region of storage belonging to EC */
#undef CONFIG_EC_PROTECTED_STORAGE_OFF
#undef CONFIG_EC_PROTECTED_STORAGE_SIZE

/* Writable region of storage belonging to EC */
#undef CONFIG_EC_WRITABLE_STORAGE_OFF
#undef CONFIG_EC_WRITABLE_STORAGE_SIZE

/* Enable robust non-volatile counter in flash */
#undef CONFIG_FLASH_NVCOUNTER
/* Address of start of the NVcounter flash page */
#undef CONFIG_FLASH_NVCTR_BASE_A
#undef CONFIG_FLASH_NVCTR_BASE_B

/*****************************************************************************/
/* NvMem Configuration */
/* Enable NV Memory module within flash */
#undef CONFIG_FLASH_NVMEM
/* Offset to start of NvMem area from base of flash */
#undef CONFIG_FLASH_NVMEM_OFFSET_A
#undef CONFIG_FLASH_NVMEM_OFFSET_B
/* Address of start of Nvmem area */
#undef CONFIG_FLASH_NVMEM_BASE_A
#undef CONFIG_FLASH_NVMEM_BASE_B
/* Size in bytes of NvMem area */
#undef CONFIG_FLASH_NVMEM_SIZE

/* Enable <key,value> variable support (requires CONFIG_FLASH_NVMEM) */
#undef CONFIG_FLASH_NVMEM_VARS
/*
 * We already have to define nvmem_user_sizes[] to specify the order and size
 * of the user regions. CONFIG_FLASH_NVMEM_VARS looks for two symbols to
 * specify the region number and size for the variable region.
 */
#undef CONFIG_FLASH_NVMEM_VARS_USER_NUM
#undef CONFIG_FLASH_NVMEM_VARS_USER_SIZE

/*****************************************************************************/

/* Include a flashmap in the compiled firmware image */
#define CONFIG_FMAP

/* Allow EC serial console input to wake up the EC from STOP mode */
#undef CONFIG_FORCE_CONSOLE_RESUME

/* Enable support for floating point unit */
#undef CONFIG_FPU

/*****************************************************************************/
/* Firmware region configuration */

#undef CONFIG_FW_PSTATE_OFF
#undef CONFIG_FW_PSTATE_SIZE

/*
 * Reuse the space that was occupied in RAM by the little firmware (LFW) loader
 * with the section ".bss.slow" instead.
 */
#undef CONFIG_REPLACE_LOADER_WITH_BSS_SLOW

/*
 * Read-only / read-write image configuration.
 * Images may reside on storage (ex. external or internal SPI) at a different
 * offset than when copied to program memory. Hence, two sets of offsets,
 * for STORAGE and for MEMORY.
 */
#undef CONFIG_RO_MEM_OFF
/* Offset relative to CONFIG_EC_PROTECTED_STORAGE_OFF */
#undef CONFIG_RO_STORAGE_OFF
#undef CONFIG_RO_SIZE

#undef CONFIG_RW_MEM_OFF
/* Some targets include two RW sections in the image. */
#undef CONFIG_RW_B
/* This is the offset of the second RW section into the flash. */
#undef CONFIG_RW_B_MEM_OFF

/* Offset relative to CONFIG_EC_WRITABLE_STORAGE_OFF */
#undef CONFIG_RW_STORAGE_OFF
#undef CONFIG_RW_SIZE

/*
 * NPCX-specific bootheader geometry.
 * TODO(crosbug.com/p/23796): Factor these CONFIGs out.
 */
#undef CONFIG_RO_HDR_MEM_OFF
#undef CONFIG_RO_HDR_SIZE

/*
 * Write protect region offset / size. This region normally encompasses the
 * RO image, but may also contain additional images or data.
 */
#undef CONFIG_WP_STORAGE_OFF
#undef CONFIG_WP_STORAGE_SIZE

/*
 * Rollback protect region. If CONFIG_ROLLBACK is defined to enable the rollback
 * protect region, CONFIG_ROLLBACK_OFF and CONFIG_ROLLBACK_SIZE must be defined
 * too.
 */
#undef CONFIG_ROLLBACK
#undef CONFIG_ROLLBACK_OFF
#undef CONFIG_ROLLBACK_SIZE

/* If defined, add support for storing some entropy in the rollback region. */
#undef CONFIG_ROLLBACK_SECRET_SIZE

/*
 * If defined, inject some locally generated entropy when secret is updated,
 * using board_get_entropy function.
 * Large values may take a long time to generate.
 */
#undef CONFIG_ROLLBACK_SECRET_LOCAL_ENTROPY_SIZE

/* If defined, we can update rollback information (RW can unset this). */
#define CONFIG_ROLLBACK_UPDATE

/*
 * Current rollback version. Meaningless for RO (but provides the minimum value
 * that will be written to the rollback protection at flash time).
 *
 * For RW, rollback version included in version structure, used by RO to
 * determine if the RW image is recent enough and can be jumped to.
 *
 * Valid values are >= 0, <= INT32_MAX (positive, 32-bit signed integer).
 */
#define CONFIG_ROLLBACK_VERSION 0

/*
 * Board Image ec.bin contains a RO firmware.  If not defined, the image will
 * only contain the RW firmware. The RO firmware comes from another board.
 */
#define CONFIG_FW_INCLUDE_RO

/* If defined, another image (RW) exists with more features */
#undef CONFIG_FW_LIMITED_IMAGE

/*
 * If defined, we can use system_get_fw_reset_vector function to decide
 * reset vector of RO/RW firmware for sysjump.
 */
#undef CONFIG_FW_RESET_VECTOR

/*****************************************************************************/
/* Motion sensor based gesture recognition information */
/* These all require HAS_TASK_MOTIONSENSE to work */

/* Do we want to detect gestures? */
#undef CONFIG_GESTURE_DETECTION

/* Mask of all sensors used for gesture dectections */
#undef CONFIG_GESTURE_DETECTION_MASK

/* some gesture recognition done in software */
#undef CONFIG_GESTURE_SW_DETECTION

/* enable gesture host interface */
#undef CONFIG_GESTURE_HOST_DETECTION
/* Sensor sampling interval for gesture recognition */
#undef CONFIG_GESTURE_SAMPLING_INTERVAL_MS

/* Which sensor to look for battery tap recognition */
#undef CONFIG_GESTURE_SENSOR_BATTERY_TAP

/*
 * Double tap detection parameters
 * Double tap works by looking for two isolated Z-axis accelerometer impulses
 * preceded and followed by relatively calm periods of accelerometer motion.
 *
 * Define an outer and inner window. The inner window specifies how
 * long the tap impulse is expected to last. The outer window specifies the
 * period before the initial tap impluse and after the final tap impulse for
 * which to check for relatively calm periods. In between the two impulses
 * there is a minimum and maximum interstice time allowed.
 *
 * Define an acceleration threshold to dectect a tap, in mg.
 */
#undef CONFIG_GESTURE_TAP_OUTER_WINDOW_T
#undef CONFIG_GESTURE_TAP_INNER_WINDOW_T
#undef CONFIG_GESTURE_TAP_MIN_INTERSTICE_T
#undef CONFIG_GESTURE_TAP_MAX_INTERSTICE_T
#undef CONFIG_GESTURE_TAP_THRES_MG

/* Event generated when battery tap is detected */
#undef CONFIG_GESTURE_TAP_EVENT

/* Which sensor to look for significant motion activity */
#undef CONFIG_GESTURE_SIGMO

/*
 * Significant motion parameters
 * Sigmo state machine looks for movement, waits skip milli-seconds,
 * and check for movement again with proof milli-seconds.
 */
#undef CONFIG_GESTURE_SIGMO_PROOF_MS
#undef CONFIG_GESTURE_SIGMO_SKIP_MS
#undef CONFIG_GESTURE_SIGMO_THRES_MG
/* Event generated when significant motion is detected. */

#undef CONFIG_GESTURE_SIGMO_EVENT

/* Do we want to detect the lid angle? */
#undef CONFIG_LID_ANGLE

/*
 * Add code for preventing 0 and 360 degree transition. Needed when
 * Device supports tablet mode.
 */
#undef CONFIG_LID_ANGLE_INVALID_CHECK

/*
 * Use lid angle to detect tablet mode.
 */
#undef CONFIG_LID_ANGLE_TABLET_MODE

/* Which sensor is located on the base? */
#undef CONFIG_LID_ANGLE_SENSOR_BASE
/* Which sensor is located on the lid? */
#undef CONFIG_LID_ANGLE_SENSOR_LID
/*
 * Allows using the lid angle measurement to determine if peripheral devices
 * should be enabled or disabled, like key scanning, trackpad interrupt.
 */
#undef CONFIG_LID_ANGLE_UPDATE

/*
 * During shutdown sequence sensor rails can be powered down asynchronously
 * to the EC hence EC cannot interlock the sensor states with the power down
 * states. To avoid this issue, defer switching the sensors rate with a
 * configurable delay if in S3. By the time deferred function is serviced,
 * if the chipset is in S5 we can back out from switching the sensor rate.
 */
#define CONFIG_MOTION_SENSE_SUSPEND_DELAY_US 0

/* Define motion sensor count in board layer */
#undef CONFIG_DYNAMIC_MOTION_SENSOR_COUNT
/******************************************************************************/
/* Host to RAM (H2RAM) Memory Mapping */

/* H2RAM Base memory address */
#undef CONFIG_H2RAM_BASE

/* H2RAM Size */
#undef CONFIG_H2RAM_SIZE

/* H2RAM Host LPC I/O base memory address */
#undef CONFIG_H2RAM_HOST_LPC_IO_BASE

/* ISH boot start address */
#undef CONFIG_ISH_BOOT_START
/*
 * Define the minimal amount of time (in ms) betwen running motion sense task
 * loop.
 */
#define CONFIG_MOTION_MIN_SENSE_WAIT_TIME 3

/*****************************************************************************/
/*
 * Support the host asking the EC about the status of the most recent host
 * command.
 *
 * When the AP is attached to the EC via a serialized bus such as I2C or SPI,
 * it needs a way to minimize the length of time an EC command will tie up the
 * bus (and the kernel driver on the AP).  If this config is defined, the EC
 * may return an in-progress result code for slow commands such as flash
 * erase/write instead of stalling until the command finishes processing, and
 * the AP may then inquire the status of the current command and/or the result
 * of the previous command.
 */
#undef CONFIG_HOST_COMMAND_STATUS

/* clear bit(s) to mask reporting of an EC_HOST_EVENT_XXX event(s) */
#define CONFIG_HOST_EVENT_REPORT_MASK 0xffffffff
#define CONFIG_HOST_EVENT64_REPORT_MASK 0xffffffffffffffffULL

/* Config option to support 64-bit hostevents and wake-masks. */
#define CONFIG_HOST_EVENT64

/*
 * The host commands are sorted in the .rodata.hcmds section so use the binary
 * search algorithm to match a command to its handler
 */
#undef CONFIG_HOSTCMD_SECTION_SORTED

/*
 * Host command parameters and response are 32-bit aligned.  This generates
 * much more efficient code on ARM.
 */
#undef CONFIG_HOSTCMD_ALIGNED

/*
 * Include host commands to fetch battery information from
 * ec_response_battery_static/dynamic_info structures, only makes sense when
 * CONFIG_BATTERY_V2 is enabled.
 */
#undef CONFIG_HOSTCMD_BATTERY_V2

/* Default hcdebug mode, e.g. HCDEBUG_OFF or HCDEBUG_NORMAL */
#define CONFIG_HOSTCMD_DEBUG_MODE HCDEBUG_NORMAL

/* If we have host command task, assume we also are using host events. */
#ifdef HAS_TASK_HOSTCMD
#define CONFIG_HOSTCMD_EVENTS
#else
#undef  CONFIG_HOSTCMD_EVENTS
#endif

/*
 * Board supports host command to get EC SPI flash info.  This is typically
 * only needed if the factory needs to determine which of several possible SPI
 * flash chips is attached to the EC on a given board.
 */
#undef CONFIG_HOSTCMD_FLASH_SPI_INFO

/*
 * For ECs where the host command interface is I2C, slave
 * address which the EC will respond to.
 */
#undef CONFIG_HOSTCMD_I2C_SLAVE_ADDR

/*
 * Accept EC host commands over the SPI slave (SPS) interface.
 */
#undef CONFIG_HOSTCMD_SPS

/*
 * Host command rate limiting assures EC will have time to process lower
 * priority tasks even if the AP is hammering the EC with host commands.
 * If there is less than CONFIG_HOSTCMD_RATE_LIMITING_MIN_REST between
 * host commands for CONFIG_HOSTCMD_RATE_LIMITING_PERIOD, then a
 * recess period of CONFIG_HOSTCMD_RATE_LIMITING_RECESS will be
 * enforced.
 */
#define CONFIG_HOSTCMD_RATE_LIMITING_PERIOD   (500 * MSEC)
#define CONFIG_HOSTCMD_RATE_LIMITING_MIN_REST (3   * MSEC)
#define CONFIG_HOSTCMD_RATE_LIMITING_RECESS   (20  * MSEC)

/* PD MCU supports host commands */
#undef CONFIG_HOSTCMD_PD

/* EC supports EC_CMD_PD_CHIP_INFO */
#define CONFIG_EC_CMD_PD_CHIP_INFO

/*
 * Use if PD MCU controls charging (selecting charging port and input
 * current limit).
 */
#undef CONFIG_HOSTCMD_PD_CHG_CTRL

/* Panic when status of PD MCU reflects that it has crashed */
#undef CONFIG_HOSTCMD_PD_PANIC

/* Board supports RTC host commands */
#undef CONFIG_HOSTCMD_RTC

/* For access to VBNV on-EC battery-backed storage */
#undef CONFIG_HOSTCMD_VBNV_CONTEXT

/* EC controls the board's SKU ID and can report that to the AP */
#undef CONFIG_HOSTCMD_SKUID

/* Set SKU ID from AP */
#undef CONFIG_HOSTCMD_AP_SET_SKUID

/* List of host commands whose debug output will be suppressed */
#undef CONFIG_SUPPRESSED_HOST_COMMANDS

/*****************************************************************************/

/* Enable debugging and profiling statistics for hook functions */
#undef CONFIG_HOOK_DEBUG

/*****************************************************************************/
/* CRC configuration */

/* Enable the hardware accelerator for CRC computation */
#undef CONFIG_HW_CRC

/* Enable the software routine for CRC computation */
#undef CONFIG_SW_CRC

/*****************************************************************************/

/* Enable system hibernate */
#define CONFIG_HIBERNATE

/* Default delay after shutting down before hibernating */
#define CONFIG_HIBERNATE_DELAY_SEC 3600

/*
 * Use to define going in to hibernate early if low on battery.
 * CONFIG_HIBERNATE_BATT_PCT specifies the low battery threshold
 * for going into hibernate early, and CONFIG_HIBERNATE_BATT_SEC defines
 * the minimum amount of time to stay in G3 before checking for low
 * battery hibernate.
 */
#undef CONFIG_HIBERNATE_BATT_PCT
#undef CONFIG_HIBERNATE_BATT_SEC

/* For ECs with multiple wakeup pins, define enabled wakeup pins */
#undef CONFIG_HIBERNATE_WAKEUP_PINS

/*
 * If defined, chip hibernation is used. Your board needs to define wake-up
 * signals. Undefine this to use board hibernation capability.
 */
#define CONFIG_SUPPORT_CHIP_HIBERNATION

/*
 * Use PSL (Power Switch Logic) for hibernating. It turns off VCC power rail
 * for ultra-low power consumption and uses PSL inputs rely on VSBY power rail
 * to wake up ec and the whole system.
 */
#undef CONFIG_HIBERNATE_PSL

/* Use a hardware specific udelay(). */
#undef CONFIG_HW_SPECIFIC_UDELAY

/*****************************************************************************/
/* I2C configuration */

#undef CONFIG_I2C
#undef CONFIG_I2C_DEBUG
#undef CONFIG_I2C_DEBUG_PASSTHRU
#undef CONFIG_I2C_PASSTHROUGH
#undef CONFIG_I2C_PASSTHRU_RESTRICTED
#undef CONFIG_I2C_VIRTUAL_BATTERY

/*
 * Conservative I2C reading size per single transaction. For example, register
 * of stm32f0 and stm32l4 are limited to be 8 bits for this field.
 */
#define CONFIG_I2C_CHIP_MAX_READ_SIZE 255

/*
 * Enable i2c_xfer() for receiving request larger than
 * CONFIG_I2C_CHIP_MAX_READ_SIZE.
 */
#undef CONFIG_I2C_XFER_LARGE_READ

/* EC uses an I2C master interface */
#undef CONFIG_I2C_MASTER

/* EC uses an I2C slave interface */
#undef CONFIG_I2C_SLAVE

/* Defines I2C operation retry count when slave nack'd(EC_ERROR_BUSY) */
#define CONFIG_I2C_NACK_RETRY_COUNT 0
/*
 * I2C SCL gating.
 *
 * If CONFIG_I2C_SCL_GATE_ADDR/PORT is defined, whenever the defined address
 * is addressed, CONFIG_I2C_SCL_GATE_GPIO is set to high. When the I2C
 * transaction is done, the pin is set back to low.
 */
#undef CONFIG_I2C_SCL_GATE_PORT
#undef CONFIG_I2C_SCL_GATE_ADDR
#undef CONFIG_I2C_SCL_GATE_GPIO

/*
 * Some chip supports two owned slave address. The second slave address is used
 * for other purpose such as board specific i2c commands. This option can be
 * set if user of the second slave address requires larger host packet buffer
 * size.
 */
#define CONFIG_I2C_EXTRA_PACKET_SIZE 0

/*
 * I2C multi-port controller.
 *
 * If CONFIG_I2C_MULTI_PORT_CONTROLLER is defined, a single on-chip I2C
 * controller may have multiple I2C ports attached. Therefore, I2c operations
 * must lock the controller (not just the port) to prevent hardware access
 * conflicts.
 */
#undef CONFIG_I2C_MULTI_PORT_CONTROLLER

/*****************************************************************************/
/* Current/Power monitor */

/*
 * Compile driver for INA219 or INA231. These two flags may not be both
 * defined.
 */
#undef CONFIG_INA219
#undef CONFIG_INA231

/*****************************************************************************/
/* Inductive charging */

/* Enable inductive charging support */
#undef CONFIG_INDUCTIVE_CHARGING

/******************************************************************************/

/* Support NXP PCA9534 I/O expander. */
#undef CONFIG_IO_EXPANDER_PCA9534

/*****************************************************************************/

/* Number of IRQs supported on the EC chip */
#undef CONFIG_IRQ_COUNT

/*
 * The IT8320 supports e-flash clock up to 48 MHz (IT8390 maximum is 32 MHz).
 * Enable it if we want better performance of fetching instruction from e-flash.
 *
 * This is valid with PLL frequency equal to 48/96MHz only.
 */
#undef CONFIG_IT83XX_FLASH_CLOCK_48MHZ

/* To define it, if I2C channel C and PECI used at the same time. */
#undef CONFIG_IT83XX_SMCLK2_ON_GPC7

/*****************************************************************************/
/* Keyboard config */

/*
 * The Silego reset chip sits in between the EC and the physical keyboard on
 * column 2.  To save power in low-power modes, some Silego variants require
 * the signal to be inverted so that the open-drain output from the EC isn't
 * costing power due to the pull-up resistor in the Silego.
 */
#undef CONFIG_KEYBOARD_COL2_INVERTED

/*
 * Config KSO to start from a different KSO pin. This is to allow some chips
 * to use alternate functions on KSO pins.
 */
#define CONFIG_KEYBOARD_KSO_BASE 0

/*
 * For certain board configurations, KSI2 will be stuck asserted for all
 * scan columns if the power button is held. We must be aware of this case
 * in order to correctly handle recovery mode key combinations.
 */
#undef CONFIG_KEYBOARD_PWRBTN_ASSERTS_KSI2

/* Enable extra debugging output from keyboard modules */
#undef CONFIG_KEYBOARD_DEBUG

/* The board uses a negative edge-triggered GPIO for keyboard interrupts. */
#undef CONFIG_KEYBOARD_IRQ_GPIO

/* Compile code for 8042 keyboard protocol */
#undef CONFIG_KEYBOARD_PROTOCOL_8042

/* Compile code for MKBP keyboard protocol */
#undef CONFIG_KEYBOARD_PROTOCOL_MKBP

/* Support keyboard factory test scanning */
#undef CONFIG_KEYBOARD_FACTORY_TEST

/*
 * Keyboard config (struct keyboard_scan_config) is in board.c.  If this is
 * not defined, default values from common/keyboard_scan.c will be used.
 */
#undef CONFIG_KEYBOARD_BOARD_CONFIG

/*
 * Support for boot key combinations (e.g. refresh key being held on boot to
 * trigger recovery).
 */
#define CONFIG_KEYBOARD_BOOT_KEYS

/* Add support for the new key. */
#undef CONFIG_KEYBOARD_NEW_KEY

/*
 * Minimum CPU clocks between scans.  This ensures that keyboard scanning
 * doesn't starve the other EC tasks of CPU when running at a decreased system
 * clock.
 */
#undef CONFIG_KEYBOARD_POST_SCAN_CLOCKS

/*  Print keyboard scan time intervals. */
#undef CONFIG_KEYBOARD_PRINT_SCAN_TIMES

/*
 * Support for extra runtime key combinations (e.g. alt+volup+h/r for hibernate
 * and warm reboot, respectively).
 */
#define CONFIG_KEYBOARD_RUNTIME_KEYS

/*
 * Allow the keyboard scan code set tables to be modified at runtime.
 */
#undef CONFIG_KEYBOARD_SCANCODE_MUTABLE

/*
 * Call board-supplied keyboard_suppress_noise() function when the debounced
 * keyboard state changes.  Some boards use this to send a signal to the audio
 * codec to suppress typing noise picked up by the microphone.
 */
#undef CONFIG_KEYBOARD_SUPPRESS_NOISE

/*
 * Enable keyboard testing functionality. This enables a message which receives
 * a list of keyscan events from the AP and processes them.  This will cause
 * keypresses to appear on the AP through the same mechanism as a normal
 * keyboard press.
 *
 * This can be used to spoof keyboard events, so is not normally defined,
 * except during internal testing.
 */
#undef CONFIG_KEYBOARD_TEST

/*
 * Enable quasi-bidirectional buffers for KSO pins. It has an open-drain output
 * and a low-impedance pull-up. The low-impedance pull-up is active when ec
 * changes the output data buffers from 0 to 1, thereby reducing the
 * low-to-high transition time.
 */
#undef CONFIG_KEYBOARD_KSO_HIGH_DRIVE

/*****************************************************************************/

/* Support common LED interface */
#undef CONFIG_LED_COMMON

/* Standard LED behavior according to spec given that we have a red-green
 * bicolor led for charging and one power led
 */
#undef CONFIG_LED_POLICY_STD

/*
 * Support common PWM-controlled LEDs that conform to the Chrome OS LED
 * behaviour specification.
 */
#undef CONFIG_LED_PWM

/*
 * Here are some recommended color settings by default, but a board can change
 * the colors to one of "enum ec_led_colors" as they see fit.
 */
#define CONFIG_LED_PWM_CHARGE_COLOR EC_LED_COLOR_AMBER
#define CONFIG_LED_PWM_NEAR_FULL_COLOR EC_LED_COLOR_GREEN
#define CONFIG_LED_PWM_CHARGE_ERROR_COLOR EC_LED_COLOR_RED
#define CONFIG_LED_PWM_SOC_ON_COLOR EC_LED_COLOR_GREEN
#define CONFIG_LED_PWM_SOC_SUSPEND_COLOR EC_LED_COLOR_GREEN
#define CONFIG_LED_PWM_LOW_BATT_COLOR EC_LED_COLOR_AMBER

/*
 * How many PWM LEDs does the system have that will be controlled by the common
 * PWM LED policy?  Currently, this may be at most 2.
 */
#undef CONFIG_LED_PWM_COUNT

/*
 * LEDs for LED_POLICY STD may be inverted.  In this case they are active low
 * and the GPIO names will be GPIO_LED..._L.
 */
#undef CONFIG_LED_BAT_ACTIVE_LOW
#undef CONFIG_LED_POWER_ACTIVE_LOW

/* Support for LED driver chip(s) */
#undef CONFIG_LED_DRIVER_DS2413  /* Maxim DS2413, on one-wire interface */
#undef CONFIG_LED_DRIVER_LM3630A /* LM3630A, on I2C interface */
#undef CONFIG_LED_DRIVER_LP5562  /* LP5562, on I2C interface */

/* Offset in flash where little firmware will live. */
#undef CONFIG_LFW_OFFSET

/*
 * Compile lid switch support.
 *
 * This is enabled by default because all boards other than reference boards
 * are for laptops with lid switchs.  Reference boards #undef it.
 */
#define CONFIG_LID_SWITCH

/*
 * GPIOs to use to detect that the lid is opened.
 *
 * This is a X-macro composed of a list of LID_OPEN(GPIO_xxx) elements defining
 * all the GPIOs to check to find whether the lid is currently opened.
 * If not defined, it is using GPIO_LID_OPEN.
 */
#undef CONFIG_LID_SWITCH_GPIO_LIST

/*
 * Support for turning the lightbar power rails on briefly when the AP is off.
 * Enabling this requires implementing the board-specific lb_power() function
 * to do it (see lb_common.h).
 */
#undef CONFIG_LIGHTBAR_POWER_RAILS

/*
 * For tap sequence, show the last segment in dim to give a better idea of
 * battery percentage.
 */
#undef CONFIG_LIGHTBAR_TAP_DIM_LAST_SEGMENT

/* Program memory offset for little firmware loader. */
#undef CONFIG_LOADER_MEM_OFF

/* Size of little firmware loader. */
#undef CONFIG_LOADER_SIZE

/* Little firmware loader storage offset. */
#undef CONFIG_LOADER_STORAGE_OFF

/*
 * Low power idle options. These are disabled by default and all boards that
 * want to use low power idle must define it. When using the LFIOSC, the low
 * frequency clock will be used to conserve even more power when possible.
 *
 * GPIOs which need to trigger interrupts in low power idle must specify the
 * GPIO_INT_DSLEEP flag in gpio_list[].
 *
 * Note that for some processors (e.g. LM4), an active JTAG connection will
 * prevent the EC from using low-power idle.
 */
#undef CONFIG_LOW_POWER_IDLE
#undef CONFIG_LOW_POWER_USE_LFIOSC

/*
 * Enable deep sleep during S0 (ignores SLEEP_MASK_AP_RUN).
 */
#undef CONFIG_LOW_POWER_S0

/* Support LPC interface */
#undef CONFIG_LPC

/* Base address of low power RAM. */
#undef CONFIG_LPRAM_BASE

/* Size of low power RAM. */
#undef CONFIG_LPRAM_SIZE

/* Use Link-Time Optimizations to try to reduce the firmware code size */
#undef CONFIG_LTO

/* Provide rudimentary malloc/free like services for shared memory. */
#undef CONFIG_MALLOC

/* Need for a math library */
#undef CONFIG_MATH_UTIL

/* Include code to do online compass calibration */
#undef CONFIG_MAG_CALIBRATE

/* Presence of a Bosh Sensortec BMM150 magnetometer behind a BMI160. */
#undef CONFIG_MAG_BMI160_BMM150

/* Microchip EC SRAM start address */
#undef CONFIG_MEC_SRAM_BASE_START

/* Microchip EC SRAM end address */
#undef CONFIG_MEC_SRAM_BASE_END

/* Microchip EC SRAM size */
#undef CONFIG_MEC_SRAM_SIZE

/*
 * Define Megachips DisplayPort to HDMI protocol converter/level shifter serial
 * interface.
 */
#undef CONFIG_MCDP28X0

/* Define clock input to MFT module. */
#undef CONFIG_MFT_INPUT_LFCLK

/* Support MKBP event */
#undef CONFIG_MKBP_EVENT

/* MKBP events are sent using host event */
#undef CONFIG_MKBP_USE_HOST_EVENT

/*
 * With this option, we can define the MKBP wakeup events in this mask (as a
 * white list) in board level, those events allow to interrupt AP during S3.
 */
#undef CONFIG_MKBP_WAKEUP_MASK

/* Support memory protection unit (MPU) */
#undef CONFIG_MPU

/* Do not try hold I/O pins at frozen level during deep sleep */
#undef CONFIG_NO_PINHOLD

/* Support one-wire interface */
#undef CONFIG_ONEWIRE

/* Support One Time Protection structure */
#undef CONFIG_OTP

/* Support PECI interface to x86 processor */
#undef CONFIG_PECI

/*
 * Maximum operating temperature in degrees Celcius used on some x86
 * processors. CPU chip temperature is reported relative to this value and
 * is never reported greater than this value. Processor asserts PROCHOT#
 * and starts throttling frequency and voltage at this temp. Operation may
 * become unreliable if temperature exceeds this limit.
 */
#undef CONFIG_PECI_TJMAX

/* Support physical presence detection (via a physical button) */
#undef CONFIG_PHYSICAL_PRESENCE

/* Enable (unsafe!) developer debug features for physical presence */
#undef CONFIG_PHYSICAL_PRESENCE_DEBUG_UNSAFE

/*****************************************************************************/
/* PMU config */

/*
 * Enable hard-resetting the PMU from the EC.  The implementation is rather
 * hacky; it simply shorts out the 3.3V rail to force the PMIC to panic.  We
 * need this unfortunate hack because it's the only way to reset the I2C engine
 * inside the PMU.
 */
#undef CONFIG_PMU_HARD_RESET

/*
 * Enable this config to make console UART self sufficient (no other
 * initialization required before uart_init(), no interrupts, uart_tx_char()
 * does not exit until character finished transmitting).
 *
 * This is useful during early hardware bringup, each platform needs to
 * implement its own code to support this.
 */
#undef CONFIG_POLLING_UART

/* Define length of history buffer for port80 messages. */
#define CONFIG_PORT80_HISTORY_LEN 128

/*
 * Enable/Disable printing of port80 messages in interrupt context. By default,
 * this is disabled.
 */
#define CONFIG_PORT80_PRINT_IN_INT 0

/* Compile common code to support power button debouncing */
#undef CONFIG_POWER_BUTTON

/* Force the active state of the power button : 0(default if unset) or 1 */
#undef CONFIG_POWER_BUTTON_ACTIVE_STATE

/* Allow the power button to send events while the lid is closed */
#undef CONFIG_POWER_BUTTON_IGNORE_LID

/* Support sending the power button signal to x86 chipsets */
#undef CONFIG_POWER_BUTTON_X86

/* Set power button state idle at init. Implemented only for npcx. */
#undef CONFIG_POWER_BUTTON_INIT_IDLE

/*
 * Enable delay between DSW_PWROK and PWRBTN assertion.
 * If enabled, DSW_PWROK_TO_PWRBTN_US and get_time_dsw_pwrok must be defined
 * as well.
 */
#undef CONFIG_DELAY_DSW_PWROK_TO_PWRBTN

/*
 * The time in usec required for PMC to be ready to detect power button press.
 * Refer to the timing diagram for G3 to S0 on PDG for details.
 */
#define CONFIG_DSW_PWROK_TO_PWRBTN_US (95 * MSEC)


/* Compile common code for AP power state machine */
#undef CONFIG_POWER_COMMON

/* Disable the power-on transition when the lid is opened */
#undef CONFIG_POWER_IGNORE_LID_OPEN

/* Enable a task-safe way to control the PP5000 rail. */
#undef CONFIG_POWER_PP5000_CONTROL

/* Support stopping in S5 on shutdown */
#undef CONFIG_POWER_SHUTDOWN_PAUSE_IN_S5

/*
 * Detect power signal interrupt storms, defined as more than
 * CONFIG_POWER_SIGNAL_INTERRUPT_STORM_DETECT_THRESHOLD occurences of a single
 * power signal interrupt within one second.
 */
#undef CONFIG_POWER_SIGNAL_INTERRUPT_STORM_DETECT_THRESHOLD

/* Use part of the EC's data EEPROM to hold persistent storage for the AP. */
#undef CONFIG_PSTORE

/* Support S0ix */
#undef CONFIG_POWER_S0IX

/*
 * Allow the host to self-report its sleep state, in case there is some delay
 * between the host beginning to enter the sleep state and power signals
 * actually reflecting the new state.
 */
#undef CONFIG_POWER_TRACK_HOST_SLEEP_STATE

/*
 * Implement the '%li' printf format as a *32-bit* integer format,
 * as it might be expected by non-EC code.
 */
#undef CONFIG_PRINTF_LEGACY_LI_FORMAT

/*****************************************************************************/
/* Support PWM control */
#undef CONFIG_PWM

/* Define clock input to PWM module. */
#undef CONFIG_PWM_INPUT_LFCLK

/*****************************************************************************/
/* Support PWM output to display backlight */
#undef CONFIG_PWM_DISPLIGHT

/* Support PWM output to keyboard backlight */
#undef CONFIG_PWM_KBLIGHT

/* Base address of RAM for the chip */
#undef CONFIG_RAM_BASE

/*
 * CONFIG_DATA_RAM_SIZE and CONFIG_RAM_SIZE indicate size of all data RAM
 * available on the chip in bytes and size of data RAM available for EC in
 * bytes, respectively.
 * Usually, CONFIG_DATA_RAM_SIZE = CONFIG_RAM_SIZE but some chips need to
 * allocate RAM for the mask ROM. Then CONFIG_DATA_RAM_SIZE > CONFIG_RAM_SIZE.
 */
#undef CONFIG_DATA_RAM_SIZE
#undef CONFIG_RAM_SIZE

/* Enable rbox peripheral */
#undef CONFIG_RBOX

/* Enable RDD peripheral */
#undef CONFIG_RDD

/* Support IR357x Link voltage regulator debugging / reprogramming */
#undef CONFIG_REGULATOR_IR357X

/* Support RMA auth challenge-response */
#undef CONFIG_RMA_AUTH
/* If that's defined, the server public key and ID must also be defined */
#undef CONFIG_RMA_AUTH_SERVER_PUBLIC_KEY  /* 32 bytes: {0xNN, 0xNN, ... 0xNN} */
#undef CONFIG_RMA_AUTH_SERVER_KEY_ID      /* 6-bit key ID, 0xMM */

/* Enable hardware Random Number generator support */
#undef CONFIG_RNG

/* Support verifying 2048-bit RSA signature */
#undef CONFIG_RSA

/* Define the RSA key size. */
#undef CONFIG_RSA_KEY_SIZE

/* Use RSA exponent 3 instead of F4 (65537) */
#undef CONFIG_RSA_EXPONENT_3

/*
 * Adjust the compiler optimization flags for the RSA code to get a speed-up
 * at the expense of a small code size delta.
 */
#undef CONFIG_RSA_OPTIMIZED

/*
 * Verify the RW firmware using the RSA signature.
 * (for accessories without software sync)
 */
#undef CONFIG_RWSIG

/*
 * Disable rwsig jump when the reset source is hard pin-reset. This only work
 * for the case where rwsig task is not used.
 */
#undef CONFIG_RWSIG_DONT_CHECK_ON_PIN_RESET

/*
 * When RWSIG verification is performed as a task, time to wait from signature
 * verification to an automatic jump to RW (if AP does not request the wait to
 * be interrupted).
 */
#define CONFIG_RWSIG_JUMP_TIMEOUT (1000 * MSEC)

/*
 * Defines what type of futility signature type should be used.
 * RWSIG should be used for new designs.
 * Old adapters use the USBPD1 futility signature type.
 */
#undef CONFIG_RWSIG_TYPE_RWSIG
#undef CONFIG_RWSIG_TYPE_USBPD1

/*
 * By default the pubkey and sig are put at the end of the first and second
 * half of the total flash, and take up the minimum space possible. You can
 * override those defaults with these.
 */
#undef CONFIG_RO_PUBKEY_ADDR
#undef CONFIG_RO_PUBKEY_SIZE
#undef CONFIG_RW_SIG_ADDR
#undef CONFIG_RW_SIG_SIZE

/* Size of the serial number if needed */
#undef CONFIG_SERIALNO_LEN

/****************************************************************************/
/* Shared objects library. */

/* Support shared objects library between RO and RW. */
#undef CONFIG_SHAREDLIB

/* Size of shared objects library. */
#undef CONFIG_SHAREDLIB_SIZE

/* Program memory offset of shared objects library. */
#undef CONFIG_SHAREDLIB_MEM_OFF

/* Storage  offset of sharedobjects library. */
#undef CONFIG_SHAREDLIB_STORAGE_OFF

/*
 * If defined, the hash module will save its last computed hash when jumping
 * between EC images.
 */
#undef CONFIG_SAVE_VBOOT_HASH

/* Allow the board to use a GPIO for the SCI# signal. */
#undef CONFIG_SCI_GPIO

/* Support computing SHA-1 hash */
#undef CONFIG_SHA1

/* Support computing of other hash sizes (without the VBOOT code) */
#undef CONFIG_SHA256

/* Unroll some loops in SHA256_transform for better performance. */
#undef CONFIG_SHA256_UNROLLED

/* Emulate the CLZ (Count Leading Zeros) in software for CPU lacking support */
#undef CONFIG_SOFTWARE_CLZ

/* Emulate the CLZ (Count Trailing Zeros) in software for CPU lacking support */
#undef CONFIG_SOFTWARE_CTZ

/* Support smbus interface */
#undef CONFIG_SMBUS

/* Support SPI interfaces */
#undef CONFIG_SPI

/* Support deprecated SPI protocol version 2. */
#undef CONFIG_SPI_PROTOCOL_V2

/*
 * Support SPI Slave interfaces. The first board supporting this is cr50 and
 * in its parlance SPI_SLAVE is called SPS. This convention might be
 * reconsidered later, and the use of "SPI" in different config options needs
 * to be cleand up. (crbug.com/512613).
 */
#undef CONFIG_SPS

/* Define the SPI port to use to access SPI accelerometer */
#undef CONFIG_SPI_ACCEL_PORT

/* Support SPI flash */
#undef CONFIG_SPI_FLASH

/* Support SPI flash protection register translation */
#undef CONFIG_SPI_FLASH_REGS

/* Define the SPI port to use to access the flash */
#undef CONFIG_SPI_FLASH_PORT

/* Select any of the following SPI flash configs that your board uses. */
#undef CONFIG_SPI_FLASH_GD25LQ40
#undef CONFIG_SPI_FLASH_GD25Q41B
#undef CONFIG_SPI_FLASH_W25Q128
#undef CONFIG_SPI_FLASH_W25Q40
#undef CONFIG_SPI_FLASH_W25Q64
#undef CONFIG_SPI_FLASH_W25Q80
#undef CONFIG_SPI_FLASH_W25X40

/* SPI flash part supports SR2 register */
#undef CONFIG_SPI_FLASH_HAS_SR2

/* Define the SPI port to use to access the fingerprint sensor */
#undef CONFIG_SPI_FP_PORT

/* Support JEDEC SFDP based Serial NOR flash */
#undef CONFIG_SPI_NOR

/* Enable SPI_NOR debugging providing additional console output while
 * initializing Serial NOR Flash devices including SFDP discovery. */
#undef CONFIG_SPI_NOR_DEBUG

/* Maximum Serial NOR flash command size, in Bytes */
#undef CONFIG_SPI_NOR_MAX_MESSAGE_SIZE

/* Maximum Serial NOR flash read size, in Bytes */
#undef CONFIG_SPI_NOR_MAX_READ_SIZE

/* Maximum Serial NOR flash write size, in Bytes. Note this must be a power of
 * two. */
#undef CONFIG_SPI_NOR_MAX_WRITE_SIZE

/* If defined will enable block (64KiB) erase operations. */
#undef CONFIG_SPI_NOR_BLOCK_ERASE

/* If defined will read the sector/block to be erased first and only initiate
 * the erase operation if not already in an erased state. The read operation
 * (performed in CONFIG_SPI_NOR_MAX_READ_SIZE chunks) is aborted early if a
 * non "0xff" byte is encountered.
 * !! Make sure there is enough stack space to host a
 * !! CONFIG_SPI_NOR_MAX_READ_SIZE sized buffer before enabling.
 */
#undef CONFIG_SPI_NOR_SMART_ERASE

/* SPI master feature */
#undef CONFIG_SPI_MASTER

/* SPI master halfduplex/3-wire mode */
#undef CONFIG_SPI_HALFDUPLEX

/* Support STM32 SPI1 as master. */
#undef CONFIG_STM32_SPI1_MASTER

/* SPI master configure gpios on init */
#undef CONFIG_SPI_MASTER_CONFIGURE_GPIOS

/* Support SPI masters without GPIO-specified Chip Selects, instead rely on the
 * SPI master port's hardwired CS pin. */
#undef CONFIG_SPI_MASTER_NO_CS_GPIOS

/* Support MCHP MEC family GP-SPI master(s)
 * Define to 0x01 for GPSPI0 only.
 * Define to 0x02 for GPSPI1 only.
 * Define to 0x03 for both controllers.
 */
#undef CONFIG_MCHP_GPSPI

/* Support testing SPI slave controller driver. */
#undef CONFIG_SPS_TEST

/* Default stack size to use for tasks, in bytes */
#undef CONFIG_STACK_SIZE

/* Use 32-bit timer for clock source on stm32. */
#undef CONFIG_STM_HWTIMER32

/* Compile charger detect for STM32 */
#undef CONFIG_STM32_CHARGER_DETECT

/* Fake hibernate mode */
#undef CONFIG_STM32L_FAKE_HIBERNATE

/*
 * Compile common code to handle simple switch inputs such as the recovery
 * button input from the servo debug interface.
 */
#undef CONFIG_SWITCH

/* Support dedicated recovery signal from servo board */
#undef CONFIG_SWITCH_DEDICATED_RECOVERY

/*
 * System should remain unlocked even if write protect is enabled.
 *
 * NOTE: This should ONLY be defined during bringup, and should never be
 * defined on a shipping / released platform.
 */
#undef CONFIG_SYSTEM_UNLOCKED

/*
 * Device can be a tablet as well as a clamshell.
 */
#undef CONFIG_TABLET_MODE

/*
 * Add a virtual switch to indicate when we are in tablet mode.
 */
#undef CONFIG_TABLET_MODE_SWITCH

/*
 * Microchip Trace FIFO Debug Port
 */
#undef CONFIG_MCHP_TFDP

/*****************************************************************************/
/* Task config */

/*
 * List of enabled tasks in ascending priority order.  This is normally
 * defined in each board's ec.tasklist file.
 *
 * For each task, use the macro TASK_ALWAYS(n, r, d, s) for base tasks and
 * TASK_NOTEST(n, r, d, s) for tasks that can be excluded in test binaries,
 * where :
 * 'n' is the name of the task
 * 'r' is the main routine of the task
 * 'd' is an opaque parameter passed to the routine at startup
 * 's' is the stack size in bytes; must be a multiple of 8
 */
#undef CONFIG_TASK_LIST

/*
 * List of test tasks.  Same format as CONFIG_TASK_LIST, but used to define
 * additional tasks for a unit test.  Normally defined in
 * test/{testname}.tasklist.
 */
#undef CONFIG_TEST_TASK_LIST

/*
 * List of tasks used by CTS
 *
 * cts.tasklist contains tasks run only for CTS. These tasks are added to the
 * tasks registered in ec.tasklist with higher priority.
 *
 * If a CTS suite does not define its own cts.tasklist, the common list is used
 * (i.e. cts/cts.tasklist).
 */
#undef CONFIG_CTS_TASK_LIST

/*
 * Enable task profiling.
 *
 * Boards may #undef this to reduce image size and RAM usage.
 */
#define CONFIG_TASK_PROFILING

/*****************************************************************************/
/* Temperature sensor config */

/* Compile common code for temperature sensor support */
#undef CONFIG_TEMP_SENSOR

/* Support particular temperature sensor chips */
#undef CONFIG_TEMP_SENSOR_ADT7481	/* ADT 7481 sensor, on I2C bus */
#undef CONFIG_TEMP_SENSOR_BD99992GW	/* BD99992GW PMIC, on I2C bus */
#undef CONFIG_TEMP_SENSOR_EC_ADC        /* Thermistors on EC's own ADC */
#undef CONFIG_TEMP_SENSOR_G781		/* G781 sensor, on I2C bus */
#undef CONFIG_TEMP_SENSOR_G782		/* G782 sensor, on I2C bus */
#undef CONFIG_TEMP_SENSOR_SB_TSI	/* SB_TSI sensor, on I2C bus */
#undef CONFIG_TEMP_SENSOR_TMP006	/* TI TMP006 sensor, on I2C bus */
#undef CONFIG_TEMP_SENSOR_TMP411	/* TI TMP411 sensor, on I2C bus */
#undef CONFIG_TEMP_SENSOR_TMP432	/* TI TMP432 sensor, on I2C bus */
#undef CONFIG_TEMP_SENSOR_F75303	/* Fintek  F75303 sensor, on I2C bus */

#undef CONFIG_THERMISTOR_NCP15WB	/* NCP15WB thermistor */

/*
 * If defined, active-high GPIO which indicates temperature sensor chips are
 * powered.  If not defined, temperature sensors are assumed to be always
 * powered.
 */
#undef CONFIG_TEMP_SENSOR_POWER_GPIO

/* Compile common code for throttling the CPU based on the temp sensors */
#undef CONFIG_THROTTLE_AP
/*
 * If defined, dptf is enabled to manage thermals.
 *
 * NOTE: This doesn't mean that thermal control is completely taken care by
 * DPTF. We have some hybrid solutions where the EC still manages the fans.
 */
#undef CONFIG_DPTF

/*****************************************************************************/
/* Touchpad config */

/* Enable touchpad, you must pick a driver (currently, only Elan exists) */
#undef CONFIG_TOUCHPAD

/* Enable Elan driver */
#undef CONFIG_TOUCHPAD_ELAN

/* Set I2C port and address (8-bit) */
#undef CONFIG_TOUCHPAD_I2C_PORT
#undef CONFIG_TOUCHPAD_I2C_ADDR

/*
 * Enable touchpad FW update over USB update protocol, and define touchpad
 * virtual address and size.
 */
#undef CONFIG_TOUCHPAD_VIRTUAL_OFF
#undef CONFIG_TOUCHPAD_VIRTUAL_SIZE

/*
 * Include hashes of the touchpad FW in the EC image, passed as TOUCHPAD_FW
 * parameter to make command.
 */
#undef CONFIG_TOUCHPAD_HASH_FW

/*****************************************************************************/
/* TPM-like configuration */

/* Speak the TPM SPI Hardware Protocol on the SPI slave interface */
#undef CONFIG_TPM_SPS
/* Speak to the TPM 2.0 hardware protocol on the I2C slave interface */
#undef CONFIG_TPM_I2CS
/* Record TPM events in circular buffer */
#undef CONFIG_TPM_LOGGING

/*****************************************************************************/
/* USART stream config */
#undef CONFIG_STREAM_USART

/*
 * Each USART stream can be individually enabled and accessible using the
 * stream interface provided in the usart_config struct.
 */
#undef CONFIG_STREAM_USART1
#undef CONFIG_STREAM_USART2
#undef CONFIG_STREAM_USART3
#undef CONFIG_STREAM_USART4

/*****************************************************************************/
/* U2F config: second factor authentication */
#undef CONFIG_U2F

/*****************************************************************************/
/* USB stream config */
#undef CONFIG_STREAM_USB

/*****************************************************************************/
/* UART config */

/* Baud rate for UARTs */
#define CONFIG_UART_BAUD_RATE 115200

/* Allow bit banging of a UARTs pins and bypassing the UART block. */
#undef CONFIG_UART_BITBANG

/* UART index (number) for EC console */
#undef CONFIG_UART_CONSOLE

/* UART index (number) for host UART, if present */
#undef CONFIG_UART_HOST

/* Use uart_input_filter() to filter UART input. See prototype in uart.h */
#undef CONFIG_UART_INPUT_FILTER

/*
 * Allow switching the EC console UART to an alternate pad. This must be
 * used for short transactions only, and EC is only able to receive data on
 * that alternate pad after it has been explicitly switched.
 */
#undef CONFIG_UART_PAD_SWITCH

/*
 * UART receive buffer size in bytes.  Must be a power of 2 for macros in
 * common/uart_buffering.c to work properly.  Must be larger than
 * CONFIG_CONSOLE_INPUT_LINE_SIZE to copy and paste scripts.
 */
#define CONFIG_UART_RX_BUF_SIZE 128

/* Use DMA for UART input */
#undef CONFIG_UART_RX_DMA

/*
 * On some platforms, UART receive DMA can't trigger an interrupt when a single
 * character is received.  Those platforms poll for characters every HOOK_TICK.
 * When a character is received, make this many additional checks between then
 * and the next HOOK_TICK, to increase responsiveness of the console to input.
 */
#define CONFIG_UART_RX_DMA_RECHECKS 5

/*
 * UART transmit buffer size in bytes.  Must be a power of 2 for macros in
 * common/uart_buffering.c to work properly.
 */
#define CONFIG_UART_TX_BUF_SIZE 512

/* Use DMA for UART output */
#undef CONFIG_UART_TX_DMA

/* The DMA channel for UART.  If not defined, default to UART1. */
#undef CONFIG_UART_TX_DMA_CH
#undef CONFIG_UART_RX_DMA_CH

/* The DMA peripheral request signal for UART TX. STM32 only. */
#undef CONFIG_UART_TX_DMA_PH

/* The DMA channel mapping config for stm32f4. */
#undef CONFIG_UART_TX_REQ_CH
#undef CONFIG_UART_RX_REQ_CH


/*****************************************************************************/
/* USB PD config */

/* Include all USB Power Delivery modules */
#undef CONFIG_USB_POWER_DELIVERY

/* Support for USB PD alternate mode */
#undef CONFIG_USB_PD_ALT_MODE

/* Support for USB PD alternate mode of Downward Facing Port */
#undef CONFIG_USB_PD_ALT_MODE_DFP

/* Check if max voltage request is allowed before each request */
#undef CONFIG_USB_PD_CHECK_MAX_REQUEST_ALLOWED

/* Default state of PD communication disabled flag */
#undef CONFIG_USB_PD_COMM_DISABLED

/*
 * Do not enable PD communication in RO as a security measure.
 * We don't want to allow communication to outside world until
 * we jump to RW. This can by overridden with the removal of
 * the write protect screw to allow for easier testing.
 */
#undef CONFIG_USB_PD_COMM_LOCKED

/* Respond to custom vendor-defined messages over PD */
#undef CONFIG_USB_PD_CUSTOM_VDM

/* Default USB data role when a USB PD debug accessory is seen */
#define CONFIG_USB_PD_DEBUG_DR PD_ROLE_DFP

/*
 * Define to have a fixed PD Task debug level.
 * Undef to allow runtime change via console command.
 */
#undef CONFIG_USB_PD_DEBUG_LEVEL

/*
 * Define if this board can enable VBUS discharge (eg. through a GPIO-controlled
 * discharge circuit, or through port controller registers) to discharge VBUS
 * rapidly on disconnect. Will be defined automatically when one of the below
 * options is defined.
 */
#undef CONFIG_USB_PD_DISCHARGE

/* Define if discharge circuit is EC GPIO-controlled. */
#undef CONFIG_USB_PD_DISCHARGE_GPIO

/* Define if discharge circuit is using PD discharge registers on TCPC. */
#undef CONFIG_USB_PD_DISCHARGE_TCPC

/* Define if discharge circuit is using PD discharge registers on PPC. */
#undef CONFIG_USB_PD_DISCHARGE_PPC

/* Define if this board can act as a dual-role PD port (source and sink) */
#undef CONFIG_USB_PD_DUAL_ROLE

/* Define if this board can used TCPC-controlled DRP toggle */
#undef CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE

/* Initial DRP / toggle policy */
#define CONFIG_USB_PD_INITIAL_DRP_STATE PD_DRP_TOGGLE_OFF

/*
 * Define if VBUS source GPIOs (GPIO_USB_C*_5V_EN) are active-low (and named
 * (..._L) rather than default active-high.
 */
#undef CONFIG_USB_PD_5V_EN_ACTIVE_LOW

/* Dynamic USB PD source capability */
#undef CONFIG_USB_PD_DYNAMIC_SRC_CAP

/* Support USB PD flash. */
#undef CONFIG_USB_PD_FLASH

/* Check whether PD is the sole power source before flash erase operation */
#undef CONFIG_USB_PD_FLASH_ERASE_CHECK

/* Define if this board, operating as a sink, can give power back to a source */
#undef CONFIG_USB_PD_GIVE_BACK

/* Enable USB PD Rev3.0 features */
#undef CONFIG_USB_PD_REV30

/* Major and Minor ChromeOS specific PD device Hardware IDs. */
#undef CONFIG_USB_PD_HW_DEV_ID_BOARD_MAJOR
#undef CONFIG_USB_PD_HW_DEV_ID_BOARD_MINOR

/* HW & SW version for alternate mode discover identity response (4bits each) */
#undef CONFIG_USB_PD_IDENTITY_HW_VERS
#undef CONFIG_USB_PD_IDENTITY_SW_VERS

/* USB PD MCU slave address for host commands */
#define CONFIG_USB_PD_I2C_SLAVE_ADDR 0x3c

/* Define if using internal comparator for PD receive */
#undef CONFIG_USB_PD_INTERNAL_COMP

/* Record main PD events in a circular buffer */
#undef CONFIG_USB_PD_LOGGING

/* The size in bytes of the FIFO used for event logging */
#define CONFIG_EVENT_LOG_SIZE 512

/* Save power by waking up on VBUS rather than polling CC */
#define CONFIG_USB_PD_LOW_POWER

/* Allow chip to go into low power idle even when a PD device is attached */
#undef CONFIG_USB_PD_LOW_POWER_IDLE_WHEN_CONNECTED

/* Number of USB PD ports */
#undef CONFIG_USB_PD_PORT_COUNT

/* Simple DFP, such as power adapter, will not send discovery VDM on connect */
#undef CONFIG_USB_PD_SIMPLE_DFP

/* Use comparator module for PD RX interrupt */
#define CONFIG_USB_PD_RX_COMP_IRQ

/* Use TCPC module (type-C port controller) */
#undef CONFIG_USB_PD_TCPC

/* Enable TCPC to enter low power mode */
#undef CONFIG_USB_PD_TCPC_LOW_POWER

/*
 * Track VBUS level in TCPC module. This will only be needed if we're acting
 * as an external TCPC.
 */
#undef CONFIG_USB_PD_TCPC_TRACK_VBUS

/*
 * Choose one of the following TCPMs (type-C port manager) to manage TCPC. The
 * TCPM stub is used to make direct function calls to TCPC when TCPC is on
 * the same MCU. The TCPCI TCPM uses the standard TCPCI i2c interface to TCPC.
 */
#undef CONFIG_USB_PD_TCPM_STUB
#undef CONFIG_USB_PD_TCPM_TCPCI
#undef CONFIG_USB_PD_TCPM_FUSB302
#undef CONFIG_USB_PD_TCPM_ITE83XX
#undef CONFIG_USB_PD_TCPM_ANX74XX
#undef CONFIG_USB_PD_TCPM_ANX7688
#undef CONFIG_USB_PD_TCPM_PS8751
#undef CONFIG_USB_PD_TCPM_PS8805

/*
 * Use this option if the TCPC port controller supports the optional register
 * 18h CONFIG_STANDARD_OUTPUT to steer the high-speed muxes.
 */
#undef CONFIG_USB_PD_TCPM_MUX

/*
 * The TCPM must know whether VBUS is present in order to make proper state
 * transitions. In addition, charge_manager must know about VBUS presence in
 * order to make charging decisions. VBUS state can be determined by various
 * methods:
 * - Some TCPCs can detect and report the presence of VBUS.
 * - In some configurations, charger ICs can report the presence of VBUS.
 * - On some boards, dedicated VBUS interrupt pins are available.
 * - Some power path controllers (PPC) can report the presence of VBUS.
 *
 * Exactly one of these should be defined for all boards that run the PD
 * state machine.
 */
#undef CONFIG_USB_PD_VBUS_DETECT_TCPC
#undef CONFIG_USB_PD_VBUS_DETECT_CHARGER
#undef CONFIG_USB_PD_VBUS_DETECT_GPIO
#undef CONFIG_USB_PD_VBUS_DETECT_PPC
#undef CONFIG_USB_PD_VBUS_DETECT_NONE


/* Define the type-c port controller I2C base address. */
#define CONFIG_TCPC_I2C_BASE_ADDR 0x9c

/* Use this option to enable Try.SRC mode for Dual Role devices */
#undef CONFIG_USB_PD_TRY_SRC

/* Set the default minimum battery percentage for Try.Src to be enabled */
#define CONFIG_USB_PD_TRY_SRC_MIN_BATT_SOC 1

/* Alternative configuration keeping only the TX part of PHY */
#undef CONFIG_USB_PD_TX_PHY_ONLY

/* Use DAC as reference for comparator at 850mV. */
#undef CONFIG_PD_USE_DAC_AS_REF

/* USB Product ID. */
#undef CONFIG_USB_PID

/* USB Type-C Power Path Controllers (PPC) */
#undef CONFIG_USBC_PPC_SN5S330

/* Support for USB type-c superspeed mux */
#undef CONFIG_USBC_SS_MUX

/*
 * Only configure USB type-c superspeed mux when DFP (for chipsets that
 * don't support being a UFP)
 */
#undef CONFIG_USBC_SS_MUX_DFP_ONLY

/* Support v1.1 type-C connection state machine */
#undef CONFIG_USBC_BACKWARDS_COMPATIBLE_DFP

/* Support for USB type-c vconn. Not needed for captive cables. */
#undef CONFIG_USBC_VCONN

/* Support VCONN swap */
#undef CONFIG_USBC_VCONN_SWAP

/* USB Binary device Object Store support */
#undef CONFIG_USB_BOS

/* USB Device version of product */
#undef CONFIG_USB_BCD_DEV

/*
 * Used during generation of VIF for USB Type-C Compliance Testing.
 * Indicates whether the UUT can communicate with USB 2.0 or USB 3.1 as a host
 * or as the Downstream Facing Port of a hub.
 */
#undef CONFIG_VIF_TYPE_C_CAN_ACT_AS_HOST

/*
 * Used during generation of VIF for USB Type-C Compliance Testing.
 * Indicates whether the UUT has a captive cable.
 */
#undef CONFIG_VIF_CAPTIVE_CABLE

/*****************************************************************************/

/* Compile chip support for the USB device controller */
#undef CONFIG_USB

/* Support USB isochronous handler */
#undef CONFIG_USB_ISOCHRONOUS

/* Support USB blob handler. */
#undef CONFIG_USB_BLOB

/* Common USB / BC1.2 charger detection routines */
#undef CONFIG_USB_CHARGER

/* External BC1.2 charger detection devices. */
#undef CONFIG_BC12_DETECT_BQ24392
#undef CONFIG_BC12_DETECT_PI3USB9281
/* Number of Pericom PI3USB9281 chips present in system */
#undef CONFIG_BC12_DETECT_PI3USB9281_CHIP_COUNT


/* Enable USB serial console module. */
#undef CONFIG_USB_CONSOLE

/* Support USB HID interface. */
#undef CONFIG_USB_HID

/* Support USB HID keyboard interface. */
#undef CONFIG_USB_HID_KEYBOARD

/* Support USB HID keyboard backlight. */
#undef CONFIG_USB_HID_KEYBOARD_BACKLIGHT

/* Support USB HID touchpad interface. */
#undef CONFIG_USB_HID_TOUCHPAD

/* HID touchpad logical dimensions */
#undef CONFIG_USB_HID_TOUCHPAD_LOGICAL_MAX_X
#undef CONFIG_USB_HID_TOUCHPAD_LOGICAL_MAX_Y
/* HID touchpad physical dimensions (tenth of mm) */
#undef CONFIG_USB_HID_TOUCHPAD_PHYSICAL_MAX_X
#undef CONFIG_USB_HID_TOUCHPAD_PHYSICAL_MAX_Y

/* USB device buffers and descriptors */
#undef CONFIG_USB_RAM_ACCESS_SIZE
#undef CONFIG_USB_RAM_ACCESS_TYPE
#undef CONFIG_USB_RAM_BASE
#undef CONFIG_USB_RAM_SIZE

/* Disable automatic connection of USB peripheral */
#undef CONFIG_USB_INHIBIT_CONNECT

/* Disable automatic initialization of USB peripheral */
#undef CONFIG_USB_INHIBIT_INIT

/* Support control of multiple PHY */
#undef CONFIG_USB_SELECT_PHY
/* Select which USB PHY will be used at startup */
#undef CONFIG_USB_SELECT_PHY_DEFAULT

/* Support simple control of power to the device's USB ports */
#undef CONFIG_USB_PORT_POWER_DUMB

/*
 * Support supplying USB power in S3, if the host leaves the port enabled when
 * entering S3.
 */
#undef CONFIG_USB_PORT_POWER_IN_S3

/*
 * Support smart power control to the device's USB ports, using
 * dedicated power control chips.  This potentially enables automatic
 * negotiation of supplying more power to peripherals.
 */
#undef CONFIG_USB_PORT_POWER_SMART

/*
 * Support smart power control to the device's USB ports, however only CDP and
 * SDP are supported.  Usually this is the case if all the control lines to the
 * charging port controller are hard-wired.
 */
#undef CONFIG_USB_PORT_POWER_SMART_CDP_SDP_ONLY

/*
 * Override the default charging mode for USB smart power control.
 * Value is selected from usb_charge_mode in include/usb_charge.h
 */
#undef CONFIG_USB_PORT_POWER_SMART_DEFAULT_MODE

/*
 * Smart USB power control can use a full set of control signals to the USB
 * port power chip, or a reduced set.  If this is defined, use the reduced set.
 */
#undef CONFIG_USB_PORT_POWER_SMART_SIMPLE

/*  Number of smart USB power ports. */
#define CONFIG_USB_PORT_POWER_SMART_PORT_COUNT 2

/*
 * Smart USB power control current limit pins may be inverted.  In this case
 * they are active low and the GPIO names will be GPIO_USBn_ILIM_SEL_L.
 */
#undef CONFIG_USB_PORT_POWER_SMART_INVERTED

/*
 * Support waking up host by setting the K-state on the data lines (requires
 * CONFIG_USB_SUSPEND to be set as well).
 */
#undef CONFIG_USB_REMOTE_WAKEUP

/* Support programmable USB device iSerial field. */
#undef CONFIG_USB_SERIALNO

/* Support reporting of configuration bMaxPower in mA */
#define CONFIG_USB_MAXPOWER_MA 500

/* Support reporting as self powered in USB configuration. */
#undef CONFIG_USB_SELF_POWERED

/* Support correct handling of USB suspend (host-initiated). */
#undef CONFIG_USB_SUSPEND

/* Default pull-up value on the USB-C ports when they are used as source. */
#define CONFIG_USB_PD_PULLUP TYPEC_RP_1A5
/*
 * Override the pull-up value when only zero or one port is actively sourcing
 * current and we can advertise more current than what is defined by
 * `CONFIG_USB_PD_PULLUP`.
 * Should be defined with one of the tcpc_rp_value.
 */
#undef CONFIG_USB_PD_MAX_SINGLE_SOURCE_CURRENT


/******************************************************************************/
/* stm32f4 dwc usb configs. */

/* Set USB speed to FS rather than HS */
#undef CONFIG_USB_DWC_FS

/******************************************************************************/
/* USB port switch */

/* Support the ITE IT5205 Type-C USB alternate mode mux. */
#undef CONFIG_USB_MUX_IT5205

/* Support the Pericom PI3USB30532 USB3.0/DP1.2 Matrix Switch */
#undef CONFIG_USB_MUX_PI3USB30532

/* Support the Parade PS8740 Type-C Redriving Switch */
#undef CONFIG_USB_MUX_PS8740

/* Support the Parade PS8743 Type-C Redriving Switch */
#undef CONFIG_USB_MUX_PS8743

/* 'Virtual' USB mux under host (not EC) control */
#undef CONFIG_USB_MUX_VIRTUAL

/*****************************************************************************/
/* USB GPIO config */
#undef CONFIG_USB_GPIO

/*****************************************************************************/
/* USB SPI config */
#undef CONFIG_USB_SPI

/*****************************************************************************/
/* USB I2C config */
#undef CONFIG_USB_I2C

/* Allowed read/write count for USB over I2C */
#define CONFIG_USB_I2C_MAX_WRITE_COUNT 60
#define CONFIG_USB_I2C_MAX_READ_COUNT 60

/*****************************************************************************/
/* USB Power monitoring interface config */
#undef CONFIG_USB_POWER

/*****************************************************************************/
/*
 * USB stream signing config. This allows data read over UART or SPI
 * to have a signature generated that can be used to validate the data
 * offline based on H1's registered key. Used by mn50.
 */
#undef CONFIG_STREAM_SIGNATURE


/*****************************************************************************/

/* Support early firmware selection */
#undef CONFIG_VBOOT_EFS

/* Support computing hash of code for verified boot */
#undef CONFIG_VBOOT_HASH

/* Support for secure temporary storage for verified boot */
#undef CONFIG_VSTORE

/* Number of supported slots for secure temporary storage */
#undef CONFIG_VSTORE_SLOT_COUNT

/*****************************************************************************/
/* Watchdog config */

/*
 * Compile watchdog timer support.  The watchdog timer will reboot the system
 * if the hook task (which is the lowest-priority task on the system) gets
 * starved for CPU time and isn't able to fire its HOOK_TICK event.
 */
#define CONFIG_WATCHDOG

/*
 * Try to detect a watchdog that is about to fire, and print a trace.  This is
 * required on chips such as STM32 where the watchdog timer simply reboots the
 * system without any early warning.
 */
#undef CONFIG_WATCHDOG_HELP

/* Watchdog period in ms; see also AUX_TIMER_PERIOD_MS */
#define CONFIG_WATCHDOG_PERIOD_MS 1600

/*
 * Fire auxiliary timer 500ms before watchdog timer expires. This leaves
 * some time for debug trace to be printed.
 */
#define CONFIG_AUX_TIMER_PERIOD_MS (CONFIG_WATCHDOG_PERIOD_MS - 500)

/*****************************************************************************/
/* WebUSB config */

/*
 * Enable the WebUSB support and define its URL.
 * Export a WebUSB Platform Descriptor in the Binary Object Store descriptor.
 * The WebUSB landing page URL is equal to 'CONFIG_WEBUSB_URL' plus the
 * https:// prefix.
 * This requires CONFIG_USB_BOS.
 */
#undef CONFIG_WEBUSB_URL

/*****************************************************************************/

/*
 * Support controlling power to WiFi, WWAN (3G/LTE), and/or bluetooth modules.
 */
#undef CONFIG_WIRELESS

/*
 * Support for WiFi devices that must remain powered in suspend.  Set to the
 * combination of EC_WIRELESS_SWITCH flags (from ec_commands.h) which should
 * be set in suspend.
 */
#undef CONFIG_WIRELESS_SUSPEND

/* WiFi power control signal is active-low. */
#undef CONFIG_WLAN_POWER_ACTIVE_LOW

/* Support Wake-on-Voice */
#undef CONFIG_WAKE_ON_VOICE

/*
 * Write protect signal is active-high.  If this is defined, there must be a
 * GPIO named GPIO_WP; if not defined, there must be a GPIO names GPIO_WP_L.
 */
#undef CONFIG_WP_ACTIVE_HIGH

/*
 * The write protect signal is always asserted,
 * independantly of the GPIO existence or current value.
 */
#undef CONFIG_WP_ALWAYS

/*
 * If needed to allocate some free space in the base of the RO or RW section
 * of the image, define these to be equal the required size of the free space.
 */
#undef CONFIG_RO_HEAD_ROOM
#undef CONFIG_RW_HEAD_ROOM

/* Firmware upgrade options. */
/* Firmware updates using other than HC channel(s). */
#undef CONFIG_NON_HC_FW_UPDATE
#undef CONFIG_USB_FW_UPDATE
/* A different config for the same update. TODO(vbendeb): dedup these */
#undef CONFIG_USB_UPDATE

/* Add support for pairing over the USB update interface. */
#undef CONFIG_USB_PAIRING

/* PDU size for fw update over USB (or TPM). */
#define CONFIG_UPDATE_PDU_SIZE 1024

/*
 * If defined, charge_get_state returns a special status if battery is
 * discharging and battery is nearly full.
 */
#undef CONFIG_PWR_STATE_DISCHARGE_FULL

/*
 * Define this if a chip needs to add some information to the common 'version'
 * command output.
 */
#undef CONFIG_EXTENDED_VERSION_INFO

/*
 * Define this if board ID support is required. For g chip based boards it
 * allows to nail different images to different boards.
 */
#undef CONFIG_BOARD_ID_SUPPORT

/*
 * Define this to enable Cros Board Info support. I2C_EEPROM_PORT and
 * I2C_EEPROM_ADDR must be defined as well.
 */
#undef CONFIG_CROS_BOARD_INFO

/*****************************************************************************/
/*
 * Include board and core configs, since those hold the CONFIG_ constants for a
 * given configuration.  This guarantees they get included everywhere, and
 * fixes a fairly common bug where we gate out code with #ifndef
 * CONFIG_SOMETHING and but forget to include both of these.
 *
 * Board is included after chip, so that chip defaults can be overridden on a
 * per-board basis as needed.
 */
#ifdef __CROS_EC_CONFIG_CHIP_H
#error Include config.h instead of config_chip.h!
#endif
#ifdef __BOARD_H
#error Include config.h instead of board.h!
#endif

#include "config_chip.h"
#include "board.h"

/******************************************************************************/
/*
 * Set default data ram size unless it's customized by the chip.
 */
#ifndef CONFIG_DATA_RAM_SIZE
#define CONFIG_DATA_RAM_SIZE	CONFIG_RAM_SIZE
#endif

/******************************************************************************/
/*
 * Disable the built-in console history if using the experimental console.
 *
 * The experimental console keeps its own session-persistent history which
 * survives EC reboot.  It also requires CRC8 for command integrity.
 */
#ifdef CONFIG_EXPERIMENTAL_CONSOLE
#undef CONFIG_CONSOLE_HISTORY
#define CONFIG_CRC8
#endif /* defined(CONFIG_EXPERIMENTAL_CONSOLE) */

/******************************************************************************/
/*
 * Throttle AP must have temperature sensor enabled to get the readings for
 * thermal throttling.
 */
#if defined(CONFIG_THROTTLE_AP) && !defined(CONFIG_TEMP_SENSOR)
#define CONFIG_TEMP_SENSOR
#endif

/******************************************************************************/
/*
 * DPTF must have temperature sensor enabled to get the readings for
 * generating DPTF thresholds events.
 */
#if defined(CONFIG_DPTF) && !defined(CONFIG_TEMP_SENSOR)
#define CONFIG_TEMP_SENSOR
#endif


/******************************************************************************/
/* The Matrix Keyboard Protocol depends on MKBP events. */
#ifdef CONFIG_KEYBOARD_PROTOCOL_MKBP
#define CONFIG_MKBP_EVENT
#endif

/******************************************************************************/
/* Set generic orientation config if a specific orientation config is set. */
#if defined(CONFIG_KX022_ORIENTATION_SENSOR) || \
	defined(CONFIG_BMI160_ORIENTATION_SENSOR)
#ifndef CONFIG_ACCEL_FIFO
#error CONFIG_ACCEL_FIFO must be defined to use hw orientation sensor support
#endif
#define CONFIG_ORIENTATION_SENSOR
#endif

/*****************************************************************************/
/* Define CONFIG_BATTERY if board has a battery. */
#if defined(CONFIG_BATTERY_BQ20Z453) || \
	defined(CONFIG_BATTERY_BQ27541) || \
	defined(CONFIG_BATTERY_BQ27621) || \
	defined(CONFIG_BATTERY_MAX17055) || \
	defined(CONFIG_BATTERY_SMART)
#define CONFIG_BATTERY
#endif

/*****************************************************************************/
/* Define CONFIG_USBC_PPC if board has a USB Type-C Power Path Controller. */
#if defined(CONFIG_USBC_PPC_SN5S330)
#define CONFIG_USBC_PPC
#endif /* "has a PPC" */

/*****************************************************************************/
/*
 * Define CONFIG_USB_PD_VBUS_MEASURE_CHARGER if the charger on the board
 * supports VBUS measurement.
 */
#if defined(CONFIG_CHARGER_BD9995X) || \
	defined(CONFIG_CHARGER_RT9466) || \
	defined(CONFIG_CHARGER_RT9467)
#define CONFIG_USB_PD_VBUS_MEASURE_CHARGER
#endif

/*****************************************************************************/
/*
 * Define CONFIG_CHARGER_NARROW_VDC for chargers that use a Narrow VDC power
 * architecture.
 */
#if defined(CONFIG_CHARGER_ISL9237) || defined(CONFIG_CHARGER_ISL9238)
#define CONFIG_CHARGER_NARROW_VDC
#endif

/*****************************************************************************/
/*
 * Define CONFIG_BUTTON_TRIGGERED_RECOVERY if a board has a dedicated recovery
 * button.
 */
#ifdef CONFIG_DEDICATED_RECOVERY_BUTTON
#define CONFIG_BUTTON_TRIGGERED_RECOVERY
#endif /* defined(CONFIG_DEDICATED_RECOVERY_BUTTON) */


#ifdef CONFIG_LED_PWM_COUNT
#define CONFIG_LED_PWM
#endif /* defined(CONFIG_LED_PWM_COUNT) */

/*****************************************************************************/
/*
 * Define derived configuration options for EC-EC communication
 */
#ifdef CONFIG_EC_EC_COMM_BATTERY
#ifdef CONFIG_EC_EC_COMM_MASTER
#define CONFIG_EC_EC_COMM_BATTERY_MASTER
#define CONFIG_BATTERY_V2
#define CONFIG_BATTERY_COUNT 2
#endif

#ifdef CONFIG_EC_EC_COMM_SLAVE
#define CONFIG_EC_EC_COMM_BATTERY_SLAVE
#define CONFIG_BATTERY_V2
#define CONFIG_BATTERY_COUNT 1
#endif
#endif /* CONFIG_EC_EC_COMM_BATTERY */

/*****************************************************************************/
/* Define derived USB PD Discharge common path */
#if defined(CONFIG_USB_PD_DISCHARGE_GPIO) || \
	defined(CONFIG_USB_PD_DISCHARGE_TCPC) || \
	defined(CONFIG_USB_PD_DISCHARGE_PPC)
#define CONFIG_USB_PD_DISCHARGE
#endif

/*****************************************************************************/
/*
 * Handle task-dependent configs.
 *
 * This prevent sub-modules from being compiled when the task and parent module
 * are not present.
 */

#ifndef HAS_TASK_CHIPSET
#undef CONFIG_CHIPSET_APOLLOLAKE
#undef CONFIG_CHIPSET_BRASWELL
#undef CONFIG_CHIPSET_CANNONLAKE
#undef CONFIG_CHIPSET_MEDIATEK
#undef CONFIG_CHIPSET_RK3399
#undef CONFIG_CHIPSET_ROCKCHIP
#undef CONFIG_CHIPSET_SKYLAKE
#undef CONFIG_CHIPSET_STONEY
#undef CONFIG_POWER_COMMON
#undef CONFIG_POWER_TRACK_HOST_SLEEP_STATE
#endif

#ifndef HAS_TASK_KEYPROTO
#undef CONFIG_KEYBOARD_PROTOCOL_8042
/*
 * Note that we don't undef CONFIG_KEYBOARD_PROTOCOL_MKBP, because it doesn't
 * have its own task.
 */
#endif

#ifndef HAS_TASK_PDCMD
#undef CONFIG_HOSTCMD_PD
#endif

/* Certain console cmds are irrelevant without parent modules. */
#ifndef CONFIG_BATTERY
#undef CONFIG_CMD_PWR_AVG
#endif

#ifndef CONFIG_ADC
#undef CONFIG_CMD_ADC
#endif

/*****************************************************************************/
/*
 * Apply test config overrides last, since tests need to override some of the
 * config flags in non-standard ways to mock only parts of the system.
 */
#include "test_config.h"

/*
 * Sanity checks to make sure some of the configs above make sense.
 */

#if (CONFIG_AUX_TIMER_PERIOD_MS) < ((HOOK_TICK_INTERVAL_MS) * 2)
#error "CONFIG_AUX_TIMER_PERIOD_MS must be at least 2x HOOK_TICK_INTERVAL_MS"
#endif

#ifdef CONFIG_USB_SERIALNO
#define CONFIG_SERIALNO_LEN 28
#endif

#ifndef CONFIG_EC_MAX_SENSOR_FREQ_MILLIHZ
#define CONFIG_EC_MAX_SENSOR_FREQ_MILLIHZ \
	CONFIG_EC_MAX_SENSOR_FREQ_DEFAULT_MILLIHZ
#endif
#endif  /* __CROS_EC_CONFIG_H */
