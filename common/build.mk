# -*- makefile -*-
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Common files build
#

common-y=util.o
common-y+=version.o printf.o queue.o queue_policies.o

common-$(CONFIG_ACCELGYRO_BMA255)+=math_util.o
common-$(CONFIG_ACCELGYRO_BMI160)+=math_util.o
common-$(CONFIG_ACCELGYRO_LSM6DS0)+=math_util.o
common-$(CONFIG_ACCELGYRO_LSM6DSM)+=math_util.o
common-$(CONFIG_ACCEL_LIS2DH)+=math_util.o
common-$(CONFIG_ACCEL_KXCJ9)+=math_util.o
common-$(CONFIG_ACCEL_KX022)+=math_util.o
common-$(CONFIG_ADC)+=adc.o
common-$(HAS_TASK_ALS)+=als.o
common-$(CONFIG_AP_HANG_DETECT)+=ap_hang_detect.o
common-$(CONFIG_BACKLIGHT_LID)+=backlight_lid.o
common-$(CONFIG_BASE32)+=base32.o
common-$(CONFIG_BATTERY)+=battery.o
common-$(CONFIG_BLUETOOTH_LE)+=bluetooth_le.o
common-$(CONFIG_BLUETOOTH_LE_STACK)+=btle_hci_controller.o btle_ll.o
common-$(CONFIG_BUTTON_COUNT)+=button.o
common-$(CONFIG_CAPSENSE)+=capsense.o
common-$(CONFIG_CASE_CLOSED_DEBUG_V1)+=ccd_config.o
common-$(CONFIG_CHARGE_MANAGER)+=charge_manager.o
common-$(CONFIG_CHARGE_RAMP_HW)+=charge_ramp.o
common-$(CONFIG_CHARGE_RAMP_SW)+=charge_ramp.o charge_ramp_sw.o
common-$(CONFIG_CHARGER)+=charger.o
common-$(CONFIG_CHARGER_PROFILE_OVERRIDE_COMMON)+=charger_profile_override.o
common-$(CONFIG_CHARGER_V2)+=charge_state_v2.o
common-$(CONFIG_CMD_I2CWEDGE)+=i2c_wedge.o
common-$(CONFIG_COMMON_GPIO)+=gpio.o gpio_commands.o
common-$(CONFIG_COMMON_PANIC_OUTPUT)+=panic_output.o
common-$(CONFIG_COMMON_RUNTIME)+=hooks.o main.o system.o
common-$(CONFIG_COMMON_TIMER)+=timer.o
common-$(CONFIG_CRC8)+= crc8.o
common-$(CONFIG_CURVE25519)+=curve25519.o
ifneq ($(CORE),cortex-m0)
common-$(CONFIG_CURVE25519)+=curve25519-generic.o
endif
common-$(CONFIG_DEVICE_EVENT)+=device_event.o
common-$(CONFIG_DEVICE_STATE)+=device_state.o
common-$(CONFIG_DPTF)+=dptf.o
common-$(CONFIG_EXTENSION_COMMAND)+=extension.o
common-$(CONFIG_EXTPOWER_GPIO)+=extpower_gpio.o
common-$(CONFIG_FANS)+=fan.o pwm.o
common-$(CONFIG_FLASH)+=flash.o
common-$(CONFIG_FLASH_NVCOUNTER)+=nvcounter.o
common-$(CONFIG_FLASH_NVMEM)+=nvmem.o
common-$(CONFIG_FLASH_NVMEM_VARS)+=nvmem_vars.o
common-$(CONFIG_FMAP)+=fmap.o
common-$(CONFIG_GESTURE_SW_DETECTION)+=gesture.o
common-$(CONFIG_HOSTCMD_EVENTS)+=host_event_commands.o
common-$(CONFIG_HOSTCMD_GET_UPTIME_INFO)+=uptime.o
common-$(CONFIG_HOSTCMD_PD)+=host_command_master.o
common-$(CONFIG_HOSTCMD_RTC)+=rtc.o
common-$(CONFIG_I2C_MASTER)+=i2c_master.o
common-$(CONFIG_I2C_SLAVE)+=i2c_slave.o
common-$(CONFIG_I2C_VIRTUAL_BATTERY)+=virtual_battery.o
common-$(CONFIG_INDUCTIVE_CHARGING)+=inductive_charging.o
common-$(CONFIG_KEYBOARD_PROTOCOL_8042)+=keyboard_8042.o \
	keyboard_8042_sharedlib.o
common-$(CONFIG_KEYBOARD_PROTOCOL_MKBP)+=keyboard_mkbp.o
common-$(CONFIG_KEYBOARD_TEST)+=keyboard_test.o
common-$(CONFIG_LED_COMMON)+=led_common.o
common-$(CONFIG_LED_POLICY_STD)+=led_policy_std.o
common-$(CONFIG_LID_ANGLE)+=motion_lid.o math_util.o
common-$(CONFIG_LID_ANGLE_UPDATE)+=lid_angle.o
common-$(CONFIG_LID_SWITCH)+=lid_switch.o
common-$(CONFIG_LPC)+=acpi.o port80.o ec_features.o
common-$(CONFIG_MAG_CALIBRATE)+= mag_cal.o math_util.o vec3.o mat33.o mat44.o
common-$(CONFIG_MKBP_EVENT)+=mkbp_event.o
common-$(CONFIG_ONEWIRE)+=onewire.o
common-$(CONFIG_PHYSICAL_PRESENCE)+=physical_presence.o
common-$(CONFIG_POWER_BUTTON)+=power_button.o
common-$(CONFIG_POWER_BUTTON_X86)+=power_button_x86.o
common-$(CONFIG_PSTORE)+=pstore_commands.o
common-$(CONFIG_PWM)+=pwm.o
common-$(CONFIG_PWM_KBLIGHT)+=pwm_kblight.o
common-$(CONFIG_RMA_AUTH)+=rma_auth.o
common-$(CONFIG_RSA)+=rsa.o
common-$(CONFIG_ROLLBACK)+=rollback.o
common-$(CONFIG_RWSIG)+=rwsig.o vboot/common.o
common-$(CONFIG_RWSIG_TYPE_RWSIG)+=vboot/vb21_lib.o
common-$(CONFIG_MATH_UTIL)+=math_util.o
common-$(CONFIG_SHA1)+= sha1.o
common-$(CONFIG_SHA256)+=sha256.o
common-$(CONFIG_SMBUS)+= smbus.o
common-$(CONFIG_SOFTWARE_CLZ)+=clz.o
common-$(CONFIG_CMD_SPI_XFER)+=spi_commands.o
common-$(CONFIG_SPI_FLASH)+=spi_flash.o spi_flash_reg.o
common-$(CONFIG_SPI_FLASH_REGS)+=spi_flash_reg.o
common-$(CONFIG_SPI_NOR)+=spi_nor.o
common-$(CONFIG_SWITCH)+=switch.o
common-$(CONFIG_SW_CRC)+=crc.o
common-$(CONFIG_TABLET_MODE)+=tablet_mode.o
common-$(CONFIG_TEMP_SENSOR)+=temp_sensor.o
common-$(CONFIG_THROTTLE_AP)+=thermal.o throttle_ap.o
common-$(CONFIG_TPM_I2CS)+=i2cs_tpm.o
common-$(CONFIG_TPM_LOGGING)+=event_log.o tpm_log.o
common-$(CONFIG_U2F)+=u2f.o
common-$(CONFIG_USB_I2C)+=usb_i2c.o
common-$(CONFIG_USB_CHARGER)+=usb_charger.o
common-$(CONFIG_USB_PORT_POWER_DUMB)+=usb_port_power_dumb.o
common-$(CONFIG_USB_PORT_POWER_SMART)+=usb_port_power_smart.o
common-$(CONFIG_USB_POWER_DELIVERY)+=usb_pd_protocol.o usb_pd_policy.o
common-$(CONFIG_USB_PD_LOGGING)+=event_log.o pd_log.o
common-$(CONFIG_USB_PD_TCPC)+=usb_pd_tcpc.o
common-$(CONFIG_USB_UPDATE)+=usb_update.o update_fw.o
common-$(CONFIG_VBOOT_EFS)+=vboot/vboot.o
common-$(CONFIG_VBOOT_HASH)+=sha256.o vboot_hash.o
common-$(CONFIG_VSTORE)+=vstore.o
common-$(CONFIG_WEBUSB_URL)+=webusb_desc.o
common-$(CONFIG_WIRELESS)+=wireless.o
common-$(HAS_TASK_CHIPSET)+=chipset.o
common-$(HAS_TASK_CONSOLE)+=console.o console_output.o uart_buffering.o
common-$(CONFIG_CMD_MEM)+=memory_commands.o
common-$(HAS_TASK_HOSTCMD)+=host_command.o ec_features.o
common-$(HAS_TASK_PDCMD)+=host_command_pd.o
common-$(HAS_TASK_KEYSCAN)+=keyboard_scan.o
common-$(HAS_TASK_LIGHTBAR)+=lb_common.o lightbar.o
common-$(HAS_TASK_MOTIONSENSE)+=motion_sense.o sensor_common.o
common-$(HAS_TASK_TPM)+=tpm_registers.o

ifeq ($(CONFIG_MALLOC),y)
common-$(CONFIG_COMMON_RUNTIME)+=shmalloc.o
else
common-$(CONFIG_COMMON_RUNTIME)+=shared_mem.o
endif

ifeq ($(CTS_MODULE),)
common-$(TEST_BUILD)+=test_util.o
else
common-y+=test_util.o
endif
common-$(TEST_BUILD)+=sensor_common.o

ifneq ($(CONFIG_RSA_OPTIMIZED),)
$(out)/RW/common/rsa.o: CFLAGS+=-O3
$(out)/RO/common/rsa.o: CFLAGS+=-O3
endif

ifneq ($(CONFIG_TOUCHPAD_HASH_FW),)
$(out)/RO/common/update_fw.o: $(out)/touchpad_fw_hash.h
$(out)/RW/common/update_fw.o: $(out)/touchpad_fw_hash.h

$(out)/touchpad_fw_hash.h: $(out)/util/gen_touchpad_hash $(out)/.touchpad_fw
	$(call quiet,tp_hash,TPHASH )

# We only want to recompute the hash if: $(TOUCHPAD_FW) variable value has
# changed, or the file pointed at by $(TOUCHPAD_FW) has changed. We do this
# by recording the latest $(TOUCHPAD_FW) file information in .touchpad_fw.

touchpad_fw_ls := $(shell ls -l "$(TOUCHPAD_FW)" 2>&1)
old_touchpad_fw_ls := $(shell cat $(out)/.touchpad_fw 2>/dev/null)

$(out)/.touchpad_fw: $(TOUCHPAD_FW)
	@echo "$(touchpad_fw_ls)" > $@

ifneq ($(touchpad_fw_ls),$(old_touchpad_fw_ls))
.PHONY: $(out)/.touchpad_fw
endif
endif
