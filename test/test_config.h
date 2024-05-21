/* Copyright 2013 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Per-test config flags */

#ifndef __TEST_TEST_CONFIG_H
#define __TEST_TEST_CONFIG_H

#ifndef TEST_BUILD
#error test_config.h should not be included in non-test build.
#endif

#ifndef __ASSEMBLER__
#include <stdint.h>
#endif

/* Host commands are sorted. */
#define CONFIG_HOSTCMD_SECTION_SORTED

/* Don't compile features unless specifically testing for them */
#undef CONFIG_VBOOT_HASH

/* Only disable this if we didn't explicitly enable it in Kconfig */
#ifndef CONFIG_PLATFORM_EC_USB_PD_LOGGING
#undef CONFIG_USB_PD_LOGGING
#endif

#ifdef TEST_ALWAYS_MEMSET
#define CONFIG_LIBCRYPTOC
#endif

#if defined(TEST_AES) || defined(TEST_CRYPTO_BENCHMARK)
#define CONFIG_BORINGSSL_CRYPTO
#endif

#ifdef TEST_BASE32
#define CONFIG_BASE32
#endif

#ifdef TEST_BATTERY_CONFIG
#define CONFIG_BATTERY_FUEL_GAUGE
#define CONFIG_BATTERY_CONFIG_IN_CBI
enum battery_type {
	BATTERY_C214,
	BATTERY_TYPE_COUNT,
};
#define CONFIG_FUEL_GAUGE
#endif

#ifdef TEST_BKLIGHT_LID
#define CONFIG_BACKLIGHT_LID
#endif

#ifdef TEST_BKLIGHT_PASSTHRU
#define CONFIG_BACKLIGHT_LID
#define CONFIG_BACKLIGHT_REQ_GPIO GPIO_PCH_BKLTEN
#endif

#ifdef TEST_CBI_WP
#define CONFIG_EEPROM_CBI_WP
#endif

#ifdef TEST_KB_8042
#define CONFIG_KEYBOARD_PROTOCOL_8042
#define CONFIG_8042_AUX
#define CONFIG_KEYBOARD_DEBUG
#endif

#ifdef TEST_KB_MKBP
#define CONFIG_KEYBOARD_PROTOCOL_MKBP
#define CONFIG_MKBP_EVENT
#define CONFIG_MKBP_USE_GPIO
#endif

#if defined(TEST_KB_SCAN) || defined(TEST_KB_SCAN_STRICT)
#define CONFIG_KEYBOARD_PROTOCOL_MKBP
#define CONFIG_MKBP_EVENT
#define CONFIG_MKBP_USE_GPIO
#ifdef TEST_KB_SCAN_STRICT
#define CONFIG_KEYBOARD_STRICT_DEBOUNCE
#endif
#endif

#ifdef TEST_MATH_UTIL
#define CONFIG_MATH_UTIL
#endif

#ifdef TEST_MAG_CAL
#define CONFIG_MAG_CALIBRATE
#endif

#ifdef TEST_STILLNESS_DETECTOR
#define CONFIG_FPU
#define CONFIG_ONLINE_CALIB
#define CONFIG_TEMP_CACHE_STALE_THRES (5 * SECOND)
#define CONFIG_MKBP_EVENT
#define CONFIG_MKBP_USE_GPIO
#endif

#ifdef TEST_FLOAT
#define CONFIG_FPU
#define CONFIG_MAG_CALIBRATE
#endif

#ifdef TEST_FP
#undef CONFIG_FPU
#define CONFIG_MAG_CALIBRATE
#endif

#if defined(TEST_FP_TRANSPORT) || defined(TEST_FPSENSOR_STATE) || \
	defined(TEST_FPSENSOR_CRYPTO) ||                          \
	defined(TEST_FPSENSOR_CRYPTO_WITH_MOCK) ||                \
	defined(TEST_FPSENSOR_UTILS) ||                           \
	defined(TEST_FPSENSOR_AUTH_CRYPTO_STATELESS) ||           \
	defined(TEST_FPSENSOR_AUTH_CRYPTO_STATEFUL) ||            \
	defined(TEST_FPSENSOR_AUTH_COMMANDS)
#define CONFIG_BORINGSSL_CRYPTO
#define CONFIG_ROLLBACK_SECRET_SIZE 32
#endif

#if defined(TEST_BORINGSSL_CRYPTO)
#define CONFIG_BORINGSSL_CRYPTO
#endif

#ifdef TEST_ROLLBACK_SECRET
#define CONFIG_ROLLBACK
#define CONFIG_ROLLBACK_SECRET_SIZE 32
#define CONFIG_ROLLBACK_OFF 1
#define CONFIG_ROLLBACK_SIZE 2
#undef CONFIG_ROLLBACK_UPDATE
#define FP_CONTEXT_TPM_BYTES 32
#endif

#ifdef TEST_MOTION_SENSE_FIFO
#define CONFIG_ACCEL_FIFO
#define CONFIG_ACCEL_FIFO_SIZE 256
#define CONFIG_ACCEL_FIFO_THRES 10
#endif

#ifdef TEST_KASA
#define CONFIG_FPU
#define CONFIG_ONLINE_CALIB
#define CONFIG_MKBP_EVENT
#define CONFIG_MKBP_USE_GPIO
#endif

#ifdef TEST_ACCEL_CAL
#define CONFIG_FPU
#define CONFIG_ONLINE_CALIB
#define CONFIG_ACCEL_CAL_MIN_TEMP 20.0f
#define CONFIG_ACCEL_CAL_MAX_TEMP 40.0f
#define CONFIG_ACCEL_CAL_KASA_RADIUS_THRES 0.1f
#define CONFIG_ACCEL_CAL_NEWTON_RADIUS_THRES 0.1f
#define CONFIG_MKBP_EVENT
#define CONFIG_MKBP_USE_GPIO
#endif

#ifdef TEST_NEWTON_FIT
#define CONFIG_FPU
#define CONFIG_ONLINE_CALIB
#define CONFIG_MKBP_EVENT
#define CONFIG_MKBP_USE_GPIO
#endif

#ifdef TEST_RGB_KEYBOARD
#define CONFIG_RGB_KEYBOARD
#define CONFIG_RGBKBD_DEMO_DOT
#endif

#ifdef TEST_NVIDIA_GPU
#define CONFIG_GPU_NVIDIA
#define GPIO_NVIDIA_GPU_ACOFF_ODL 123
#endif

#ifdef TEST_STILLNESS_DETECTOR
#define CONFIG_MKBP_EVENT
#define CONFIG_MKBP_USE_GPIO
#endif

#ifdef TEST_ONLINE_CALIBRATION
#define CONFIG_FPU
#define CONFIG_ONLINE_CALIB
#define CONFIG_MKBP_EVENT
#define CONFIG_MKBP_USE_GPIO
#endif

#ifdef TEST_ONLINE_CALIBRATION_SPOOF
#define CONFIG_FPU
#define CONFIG_ONLINE_CALIB
#define CONFIG_MKBP_EVENT
#define CONFIG_MKBP_USE_GPIO
#define CONFIG_ONLINE_CALIB_SPOOF_MODE
#endif /* TEST_ONLINE_CALIBRATION_SPOOF */

#ifdef TEST_GYRO_CAL
#define CONFIG_FPU
#define CONFIG_ONLINE_CALIB
#define CONFIG_MKBP_EVENT
#define CONFIG_MKBP_USE_GPIO
#endif

#if defined(CONFIG_ONLINE_CALIB) && !defined(CONFIG_TEMP_CACHE_STALE_THRES)
#define CONFIG_TEMP_CACHE_STALE_THRES (1 * SECOND)
#endif /* CONFIG_ONLINE_CALIB && !CONFIG_TEMP_CACHE_STALE_THRES */

#if defined(CONFIG_ONLINE_CALIB) || defined(TEST_BODY_DETECTION) ||        \
	defined(TEST_MOTION_ANGLE) || defined(TEST_MOTION_ANGLE_TABLET) || \
	defined(TEST_MOTION_LID) || defined(TEST_MOTION_SENSE_FIFO) ||     \
	defined(TEST_TABLET_BROKEN_SENSOR)
enum sensor_id {
	BASE,
	LID,
	SENSOR_COUNT,
};

#if defined(TEST_MOTION_ANGLE) || defined(TEST_MOTION_ANGLE_TABLET) || \
	defined(TEST_MOTION_LID) || defined(TEST_TABLET_BROKEN_SENSOR)
#define CONFIG_LID_ANGLE
#define CONFIG_LID_ANGLE_SENSOR_BASE BASE
#define CONFIG_LID_ANGLE_SENSOR_LID LID
#define CONFIG_TABLET_MODE
#endif /* LID ANGLE needed */

#define CONFIG_MOTION_FILL_LPC_SENSE_DATA

#endif /* sensor_id needed */

#if defined(TEST_MOTION_ANGLE)
#define CONFIG_ACCEL_FORCE_MODE_MASK           \
	((1 << CONFIG_LID_ANGLE_SENSOR_BASE) | \
	 (1 << CONFIG_LID_ANGLE_SENSOR_LID))
#define CONFIG_ACCEL_STD_REF_FRAME_OLD
#endif

#if defined(TEST_MOTION_ANGLE_TABLET) || defined(TEST_MOTION_LID)
#define CONFIG_ACCEL_FORCE_MODE_MASK           \
	((1 << CONFIG_LID_ANGLE_SENSOR_BASE) | \
	 (1 << CONFIG_LID_ANGLE_SENSOR_LID))
#endif

#if defined(TEST_TABLET_BROKEN_SENSOR) || defined(TEST_TABLET_NO_SENSOR) || \
	defined(TEST_MOTION_LID)
#define CONFIG_TABLET_MODE
#define CONFIG_GMR_TABLET_MODE
#endif

#ifdef TEST_TABLET_BROKEN_SENSOR
#define CONFIG_DYNAMIC_MOTION_SENSOR_COUNT
#endif

#if defined(TEST_BODY_DETECTION)
#define CONFIG_BODY_DETECTION
#define CONFIG_BODY_DETECTION_SENSOR BASE
#endif

#ifdef TEST_CRC
#define CONFIG_CRC8
#define CONFIG_SW_CRC
#endif

#ifdef TEST_RSA
#define CONFIG_RSA
#ifdef CONFIG_RSA_EXPONENT_3
#error Your board uses RSA exponent 3, please build rsa3 test instead!
#endif
#define CONFIG_RWSIG_TYPE_RWSIG
#endif

#ifdef TEST_RSA3
#define CONFIG_RSA
#define CONFIG_RSA_EXPONENT_3
#define CONFIG_RWSIG_TYPE_RWSIG
#endif

#ifdef TEST_SHA256
/* Test whichever sha256 implementation the platform provides. */
#endif

#ifdef TEST_SHA256_UNROLLED
#undef CONFIG_SHA256_HW_ACCELERATE
#define CONFIG_SHA256_SW
#define CONFIG_SHA256_UNROLLED
#endif

#ifdef TEST_SHMALLOC
#define CONFIG_SHARED_MALLOC
#endif

#ifdef TEST_SBS_CHARGING
#define CONFIG_BATTERY
#define CONFIG_BATTERY_V2
#define CONFIG_BATTERY_COUNT 1
#define CONFIG_BATTERY_MOCK
#define CONFIG_BATTERY_SMART
#define CONFIG_CHARGER
#define CONFIG_CHARGER_PROFILE_OVERRIDE
#define CONFIG_CHARGER_DEFAULT_CURRENT_LIMIT 4032
#define CONFIG_CHARGER_DISCHARGE_ON_AC
#define CONFIG_CHARGER_DISCHARGE_ON_AC_CUSTOM
#define CONFIG_I2C
#define CONFIG_I2C_CONTROLLER
int board_discharge_on_ac(int enabled);
#define I2C_PORT_MASTER 0
#define I2C_PORT_BATTERY 0
#define I2C_PORT_CHARGER 0
#define CONFIG_BATTERY_LOW_VOLTAGE_PROTECTION
#undef CONFIG_BATTERY_LOW_VOLTAGE_TIMEOUT
#define CONFIG_BATTERY_LOW_VOLTAGE_TIMEOUT (2 * SECOND)
#endif

#ifdef TEST_THERMAL
#define CONFIG_CHIPSET_CAN_THROTTLE
#define CONFIG_FANS 1
#define CONFIG_I2C
#define CONFIG_I2C_CONTROLLER
#define CONFIG_TEMP_SENSOR
#define CONFIG_THROTTLE_AP
#define CONFIG_THERMISTOR
#define CONFIG_THERMISTOR_NCP15WB
#define I2C_PORT_THERMAL 0
int ncp15wb_calculate_temp(uint16_t adc);
#endif

#ifdef TEST_FAN
#define CONFIG_FANS 1
#endif

#ifdef TEST_BUTTON
#define CONFIG_KEYBOARD_PROTOCOL_8042
#undef CONFIG_KEYBOARD_VIVALDI
#define CONFIG_VOLUME_BUTTONS
#define CONFIG_HOSTCMD_BUTTON
#endif

#ifdef TEST_BATTERY_GET_PARAMS_SMART
#define CONFIG_BATTERY_MOCK
#define CONFIG_BATTERY_SMART
#define CONFIG_CHARGER_DEFAULT_CURRENT_LIMIT 4032
#define CONFIG_I2C
#define CONFIG_I2C_CONTROLLER
#define I2C_PORT_MASTER 0
#define I2C_PORT_BATTERY 0
#define I2C_PORT_CHARGER 0
#endif

#ifdef TEST_LIGHTBAR
#define CONFIG_I2C
#define CONFIG_I2C_CONTROLLER
#define I2C_PORT_LIGHTBAR 0
#define CONFIG_ALS_LIGHTBAR_DIMMING 0
#endif

#ifdef TEST_USB_COMMON
#define CONFIG_USB_POWER_DELIVERY
#define CONFIG_USB_PD_TCPMV1
#define CONFIG_USB_PD_PORT_MAX_COUNT 1
#define CONFIG_USB_PD_TCPC
#define CONFIG_USB_PD_TCPM_STUB
#define CONFIG_SHA256_SW
#define CONFIG_SW_CRC
#endif

#ifdef TEST_USB_PD_PDO_FIXED
#define CONFIG_USB_POWER_DELIVERY
#define CONFIG_USB_PD_TCPMV1
#define CONFIG_USB_PD_PORT_MAX_COUNT 1
#define CONFIG_USB_PD_TCPC
#define CONFIG_USB_PD_TCPM_STUB
#define CONFIG_SHA256_SW
#define CONFIG_SW_CRC
#define CONFIG_USB_PD_ONLY_FIXED_PDOS
#endif

#if defined(TEST_USB_SM_FRAMEWORK_H3) || defined(TEST_USB_SM_FRAMEWORK_H2) || \
	defined(TEST_USB_SM_FRAMEWORK_H1) || defined(TEST_USB_SM_FRAMEWORK_H0)
#define CONFIG_TEST_SM
#endif

#if defined(TEST_USB_PRL_OLD) || defined(TEST_USB_PRL_NOEXTENDED)
#define CONFIG_USB_PD_PORT_MAX_COUNT 1
#define CONFIG_USB_PD_REV30

#if defined(TEST_USB_PRL_OLD)
#define CONFIG_USB_PD_EXTENDED_MESSAGES
#endif

#define CONFIG_USB_PD_TCPMV2
#undef CONFIG_USB_PE_SM
#undef CONFIG_USB_TYPEC_SM
#undef CONFIG_USB_PD_HOST_CMD
#undef CONFIG_USB_DPM_SM
#define CONFIG_USB_PRL_SM
#define CONFIG_USB_PD_TCPC
#define CONFIG_USB_PD_TCPM_STUB
#define CONFIG_USB_POWER_DELIVERY
#define CONFIG_SHA256_SW
#define CONFIG_SW_CRC
#endif

#if defined(TEST_USB_PD_TIMER)
#define CONFIG_USB_PD_PORT_MAX_COUNT 2
#define CONFIG_MATH_UTIL
#define CONFIG_TEST_USB_PD_TIMER
#endif

#if defined(TEST_USB_PRL)
#define CONFIG_USB_PD_PORT_MAX_COUNT 1
#define CONFIG_USB_PD_REV30
#define CONFIG_USB_PD_EXTENDED_MESSAGES
#define CONFIG_USB_PD_TCPMV2
#undef CONFIG_USB_PE_SM
#undef CONFIG_USB_DPM_SM
#undef CONFIG_USB_TYPEC_SM
#undef CONFIG_USB_PD_HOST_CMD
#define CONFIG_USB_PRL_SM
#define CONFIG_USB_POWER_DELIVERY
#endif

#if defined(TEST_USB_PE_DRP_OLD) || defined(TEST_USB_PE_DRP_OLD_NOEXTENDED)
#define CONFIG_TEST_USB_PE_SM
#define CONFIG_USB_PD_PORT_MAX_COUNT 1
#define CONFIG_USB_PE_SM
#define CONFIG_USB_PID 0x5036
#define CONFIG_USB_POWER_DELIVERY
#undef CONFIG_USB_PRL_SM
#define CONFIG_USB_PD_REV30

#if defined(TEST_USB_PE_DRP_OLD)
#define CONFIG_USB_PD_EXTENDED_MESSAGES
#endif

#define CONFIG_USB_PD_TCPMV2
#define CONFIG_USB_PD_DECODE_SOP
#undef CONFIG_USB_TYPEC_SM
#define CONFIG_USBC_VCONN
#define CONFIG_USB_PD_DISCHARGE_GPIO
#undef CONFIG_USB_PD_HOST_CMD
#define CONFIG_USB_PD_ALT_MODE_DFP
#define CONFIG_USB_PD_DP_MODE
#define CONFIG_USB_PD_DISCOVERY
#define CONFIG_USBC_SS_MUX
#define CONFIG_USB_PD_3A_PORTS 0 /* Host does not define a 3.0 A PDO */
#endif

#if defined(TEST_USB_PE_DRP) || defined(TEST_USB_PE_DRP_NOEXTENDED)
#define CONFIG_TEST_USB_PE_SM
#define CONFIG_USB_PD_PORT_MAX_COUNT 1
#define CONFIG_USB_PE_SM
#define CONFIG_USB_PID 0x5036
#define CONFIG_USB_POWER_DELIVERY
#undef CONFIG_USB_PRL_SM
#define CONFIG_USB_PD_REV30

#if defined(TEST_USB_PE_DRP)
#define CONFIG_USB_PD_EXTENDED_MESSAGES
#endif

#define CONFIG_USB_PD_TCPMV2
#define CONFIG_USB_PD_DECODE_SOP
#undef CONFIG_USB_TYPEC_SM
#define CONFIG_USBC_VCONN
#define CONFIG_USB_PD_DISCHARGE_GPIO
#undef CONFIG_USB_PD_HOST_CMD
#define CONFIG_USB_PD_ALT_MODE_DFP
#define CONFIG_USB_PD_DP_MODE
#define CONFIG_USB_PD_DISCOVERY
#define CONFIG_USBC_SS_MUX
#define I2C_PORT_HOST_TCPC 0
#define CONFIG_CHARGE_MANAGER
#define CONFIG_USB_PD_3A_PORTS 0 /* Host does not define a 3.0 A PDO */
#endif /* TEST_USB_PE_DRP || TEST_USB_PE_DRP_NOEXTENDED */

/* Common TypeC tests defines */
#if defined(TEST_USB_TYPEC_VPD) || defined(TEST_USB_TYPEC_CTVPD)
#define CONFIG_USB_PID 0x5036
#define VPD_HW_VERSION 0x0001
#define VPD_FW_VERSION 0x0001
#define USB_BCD_DEVICE 0
#define VPD_CT_CURRENT VPD_CT_CURRENT_3A
/* Vbus impedance in milliohms */
#define VPD_VBUS_IMPEDANCE 65

/* GND impedance in milliohms */
#define VPD_GND_IMPEDANCE 33

#define CONFIG_USB_PD_PORT_MAX_COUNT 1
#define CONFIG_USB_PD_REV30
#define CONFIG_USB_PD_EXTENDED_MESSAGES
#define CONFIG_USB_PD_TCPMV2
#define CONFIG_USB_PE_SM
#define CONFIG_USB_PRL_SM
#define CONFIG_USB_TYPEC_SM
#define CONFIG_USB_PD_TCPC
#define CONFIG_USB_PD_TCPM_STUB
#define CONFIG_USB_POWER_DELIVERY
#define CONFIG_SW_CRC
#undef CONFIG_USB_PD_HOST_CMD
#endif /* Common TypeC test defines */

#ifdef TEST_USB_TYPEC_VPD
#define CONFIG_USB_VPD
#endif

#ifdef TEST_USB_TYPEC_CTVPD
#define CONFIG_USB_CTVPD
#endif

#ifdef TEST_USB_TYPEC_DRP_ACC_TRYSRC
#define CONFIG_USB_DRP_ACC_TRYSRC
#define CONFIG_USB_PD_DUAL_ROLE
#define CONFIG_USB_PD_TRY_SRC
#define CONFIG_USB_TYPEC_SM
#define CONFIG_USB_PD_TCPMV2
#define CONFIG_USB_PD_PORT_MAX_COUNT 1
#define CONFIG_USBC_SS_MUX
#define CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE
#define CONFIG_USB_PD_TCPC_LOW_POWER
#define CONFIG_USB_PD_VBUS_DETECT_TCPC
/* Since we have no real HW to wait on, use a minimal debounce */
#undef CONFIG_USB_PD_TCPC_LPM_EXIT_DEBOUNCE
#define CONFIG_USB_PD_TCPC_LPM_EXIT_DEBOUNCE 1
#define CONFIG_USB_POWER_DELIVERY
#undef CONFIG_USB_PRL_SM
#undef CONFIG_USB_PE_SM
#undef CONFIG_USB_DPM_SM
#undef CONFIG_USB_PD_HOST_CMD
#endif

#ifdef TEST_USB_TCPMV2_COMPLIANCE
#define CONFIG_USB_DRP_ACC_TRYSRC
#define CONFIG_USB_PD_DUAL_ROLE
#define CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE
#define CONFIG_USB_PD_REV30
#define CONFIG_USB_PD_TCPC_LOW_POWER
#define CONFIG_USB_PD_TRY_SRC
#define CONFIG_USB_PD_TCPMV2
#define CONFIG_USB_PD_PORT_MAX_COUNT 1
#define CONFIG_USBC_SS_MUX
#define CONFIG_USB_PD_VBUS_DETECT_TCPC
#define CONFIG_USB_POWER_DELIVERY
#define CONFIG_TEST_USB_PE_SM
#define CONFIG_USB_PD_ALT_MODE_DFP
#define CONFIG_USB_PD_DP_MODE
#define CONFIG_USB_PD_DISCOVERY
#define CONFIG_USBC_VCONN
#define CONFIG_USBC_VCONN_SWAP
#define CONFIG_USB_PID 0x5036
#define CONFIG_USB_PD_TCPM_TCPCI
#define CONFIG_I2C
#define CONFIG_I2C_CONTROLLER
#define CONFIG_BATTERY
#define CONFIG_NUM_FIXED_BATTERIES 1
#define I2C_PORT_HOST_TCPC 0
#define CONFIG_USB_PD_DEBUG_LEVEL 3
#define CONFIG_USB_PD_EXTENDED_MESSAGES
#define CONFIG_USB_PD_DECODE_SOP
#define CONFIG_USB_PD_3A_PORTS 0 /* Host does not define a 3.0 A PDO */
#endif

#ifdef TEST_USB_PD_INT
#define CONFIG_USB_POWER_DELIVERY
#define CONFIG_USB_PD_TCPMV1
#define CONFIG_USB_PD_DUAL_ROLE
#define CONFIG_USB_PD_PORT_MAX_COUNT 1
#define CONFIG_USB_PD_TCPC
#define CONFIG_USB_PD_TCPM_STUB
#define CONFIG_SHA256_SW
#define CONFIG_SW_CRC
#endif

#if defined(TEST_USB_PD) || defined(TEST_USB_PD_GIVEBACK) || \
	defined(TEST_USB_PD_REV30)
#define CONFIG_USB_POWER_DELIVERY
#define CONFIG_USB_PD_TCPMV1
#define CONFIG_USB_PD_DUAL_ROLE
#define CONFIG_USB_PD_PORT_MAX_COUNT 2
#define CONFIG_USB_PD_TCPC
#define CONFIG_USB_PD_TCPM_STUB
#define CONFIG_SHA256_SW
#define CONFIG_SW_CRC
#ifdef TEST_USB_PD_REV30
#define CONFIG_USB_PD_REV30
#define CONFIG_USB_PD_EXTENDED_MESSAGES
#define CONFIG_USB_PID 0x5000
#endif
#ifdef TEST_USB_PD_GIVEBACK
#define CONFIG_USB_PD_GIVE_BACK
#endif
#endif /* TEST_USB_PD || TEST_USB_PD_GIVEBACK || TEST_USB_PD_REV30 */

#ifdef TEST_USB_PD_CONSOLE
#define CONFIG_USB_PD_EPR
#endif

#ifdef TEST_USB_PPC
#define CONFIG_USB_PD_PORT_MAX_COUNT 1
#define CONFIG_USB_PD_VBUS_DETECT_PPC
#define CONFIG_USBC_PPC
#define CONFIG_USBC_PPC_POLARITY
#define CONFIG_USBC_PPC_SBU
#define CONFIG_USBC_PPC_VCONN
#endif

#ifdef TEST_USB_PD_CONSOLE
#define CONFIG_USB_PD_PORT_MAX_COUNT 2
#define CONFIG_USB_PE_SM
#define CONFIG_CMD_PD
#define CONFIG_USB_PD_TCPMV2
#define CONFIG_USB_PD_TRY_SRC
#define CONFIG_USB_PD_DUAL_ROLE
#define CONFIG_USBC_VCONN
#define CONFIG_USBC_VCONN_SWAP
#define CONFIG_CMD_PD_TIMER
#undef CONFIG_USB_PD_HOST_CMD
#undef CONFIG_USB_PRL_SM
#undef CONFIG_USB_DPM_SM
#endif

#if defined(TEST_CHARGE_MANAGER) || defined(TEST_CHARGE_MANAGER_DRP_CHARGING)
#define CONFIG_CHARGE_MANAGER
#define CONFIG_USB_PD_3A_PORTS 0 /* Host does not define a 3.0 A PDO */
#define CONFIG_USB_PD_DUAL_ROLE
#define CONFIG_USB_PD_PORT_MAX_COUNT 2
#define CONFIG_USB_POWER_DELIVERY
#define CONFIG_BATTERY
#define CONFIG_BATTERY_SMART
#define CONFIG_I2C
#define CONFIG_I2C_CONTROLLER
#define I2C_PORT_BATTERY 0
#undef CONFIG_USB_PD_HOST_CMD
#endif /* TEST_CHARGE_MANAGER_* */

#ifdef TEST_CHARGE_MANAGER_DRP_CHARGING
#define CONFIG_CHARGE_MANAGER_DRP_CHARGING
#else
#undef CONFIG_CHARGE_MANAGER_DRP_CHARGING
#endif /* TEST_CHARGE_MANAGER_DRP_CHARGING */

#ifdef TEST_CHARGE_RAMP
#define CONFIG_CHARGE_RAMP_SW
#define CONFIG_USB_PD_PORT_MAX_COUNT 2
#undef CONFIG_USB_PD_HOST_CMD
#endif

#ifdef TEST_RTC
#define CONFIG_HOSTCMD_RTC
#endif

#ifdef TEST_VBOOT
#define CONFIG_RWSIG
#define CONFIG_SHA256_SW
#define CONFIG_RSA
#define CONFIG_RWSIG_TYPE_RWSIG
#define CONFIG_RW_B
#define CONFIG_RW_B_MEM_OFF CONFIG_RO_MEM_OFF
#undef CONFIG_RO_SIZE
#define CONFIG_RO_SIZE (CONFIG_FLASH_SIZE_BYTES / 4)
#undef CONFIG_RW_SIZE
#define CONFIG_RW_SIZE CONFIG_RO_SIZE
#define CONFIG_RW_A_STORAGE_OFF CONFIG_RW_STORAGE_OFF
#define CONFIG_RW_B_STORAGE_OFF (CONFIG_RW_A_STORAGE_OFF + CONFIG_RW_SIZE)
#define CONFIG_RW_A_SIGN_STORAGE_OFF \
	(CONFIG_RW_A_STORAGE_OFF + CONFIG_RW_SIZE - CONFIG_RW_SIG_SIZE)
#define CONFIG_RW_B_SIGN_STORAGE_OFF \
	(CONFIG_RW_B_STORAGE_OFF + CONFIG_RW_SIZE - CONFIG_RW_SIG_SIZE)
#endif

#ifdef TEST_X25519
#define CONFIG_CURVE25519
#endif /* TEST_X25519 */

#ifdef TEST_I2C_BITBANG
#define CONFIG_I2C
#define CONFIG_I2C_CONTROLLER
#define CONFIG_I2C_BITBANG
#define I2C_BITBANG_PORT_COUNT 1
#endif

#ifdef TEST_PANIC
#undef CONFIG_PANIC_STRIP_GPR
#endif

#ifdef HAVE_PRIVATE
#include "private_test_config.h"
#endif /* HAVE_PRIVATE */

#endif /* __TEST_TEST_CONFIG_H */
