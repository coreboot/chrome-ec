# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Note: this cmake system implements only a zephyr module, and is not
# intended to build a complete EC.  To build projects in platform/ec,
# you should continue to use the Makefile system.
#
# Googlers can find the design doc at go/zephyr-shim.

if(NOT DEFINED ZEPHYR_CURRENT_MODULE_DIR)
  message(FATAL_ERROR "This Cmake system implements only a Zephyr module, and
          should not be invoked directly.  Please continue to use the Makefile
          system for non-Zephyr builds.")
endif()

set(PLATFORM_EC "${ZEPHYR_CURRENT_MODULE_DIR}" CACHE PATH
    "Path to the platform/ec repo.")
# Zephyr 2.3 will set ZEPHYR_CURRENT_MODULE_DIR to the directory of the
# CMakeLists.txt file, whereas 2.4 will set it to the actual module
# directory.  Try to detect the condition by searching for
# zephyr/module.yml.
if(NOT EXISTS "${PLATFORM_EC}/zephyr/module.yml")
  set(PLATFORM_EC "${PLATFORM_EC}/..")
  assert_exists("${PLATFORM_EC}/zephyr/module.yml")
endif()

zephyr_include_directories_ifdef(
  CONFIG_PLATFORM_EC
  "${PLATFORM_EC}/zephyr/shim/include"
  "${PLATFORM_EC}/fuzz"
  "${PLATFORM_EC}/test"
  "${PLATFORM_EC}/include")

add_subdirectory_ifdef(CONFIG_PLATFORM_EC "shim")

zephyr_sources_ifdef(CONFIG_PLATFORM_EC         "${PLATFORM_EC}/common/base32.c")
zephyr_sources_ifdef(CONFIG_PLATFORM_EC_ESPI    "${PLATFORM_EC}/common/espi.c")
zephyr_sources_ifdef(CONFIG_PLATFORM_EC         "${PLATFORM_EC}/common/extpower_common.c")
zephyr_sources_ifdef(CONFIG_PLATFORM_EC_EXTPOWER_GPIO
                                                "${PLATFORM_EC}/common/extpower_gpio.c")
zephyr_sources_ifdef(CONFIG_SHELL               "${PLATFORM_EC}/common/gpio_commands.c")
zephyr_sources_ifdef(CONFIG_PLATFORM_EC_HOSTCMD "${PLATFORM_EC}/common/host_command.c")
zephyr_sources_ifdef(CONFIG_PLATFORM_EC_HOSTCMD "${PLATFORM_EC}/common/host_event_commands.c")
zephyr_sources_ifdef(CONFIG_PLATFORM_EC_I2C     "${PLATFORM_EC}/common/i2c_master.c")
zephyr_sources_ifdef(CONFIG_PLATFORM_EC_KEYBOARD_PROTOCOL_8042
                                                "${PLATFORM_EC}/common/keyboard_8042.c")
zephyr_sources_ifdef(CONFIG_PLATFORM_EC_LID_SWITCH
                                                "${PLATFORM_EC}/common/lid_switch.c")
zephyr_sources_ifdef(CONFIG_PLATFORM_EC_POWER_BUTTON
                                                "${PLATFORM_EC}/common/power_button.c")
zephyr_sources_ifdef(CONFIG_PLATFORM_EC_POWERSEQ_INTEL
                                                "${PLATFORM_EC}/common/power_button_x86.c")
zephyr_sources_ifdef(CONFIG_PLATFORM_EC         "${PLATFORM_EC}/common/queue.c")
zephyr_sources_ifdef(CONFIG_PLATFORM_EC_TIMER   "${PLATFORM_EC}/common/timer.c")
zephyr_sources_ifdef(CONFIG_PLATFORM_EC_POWERSEQ
                                                "${PLATFORM_EC}/power/common.c")
zephyr_sources_ifdef(CONFIG_PLATFORM_EC_POWERSEQ_INTEL
                                                "${PLATFORM_EC}/power/intel_x86.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_POWERSEQ_HOST_SLEEP
                                                "${PLATFORM_EC}/power/host_sleep.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_POWERSEQ_SC7180
                                                "${PLATFORM_EC}/power/qcom.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_PANIC
                                                "${PLATFORM_EC}/common/panic_output.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_PWM
                                                "${PLATFORM_EC}/common/pwm.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_SHA256_SW
                                                "${PLATFORM_EC}/common/sha256.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_SWITCH
                                                "${PLATFORM_EC}/common/switch.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_SWITCHCAP_LN9310
                                                "${PLATFORM_EC}/driver/ln9310.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_SPI_FLASH_REGS
                                                "${PLATFORM_EC}/common/spi_flash_reg.c")

zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_TABLET_MODE
                                                "${PLATFORM_EC}/common/tablet_mode.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_TEMP_SENSOR
                                                "${PLATFORM_EC}/common/thermal.c"
                                                "${PLATFORM_EC}/common/temp_sensor.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_THERMISTOR
                                                "${PLATFORM_EC}/driver/temp_sensor/thermistor.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_THROTTLE_AP
                                                "${PLATFORM_EC}/common/throttle_ap.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_TIMER
                                                "${PLATFORM_EC}/common/timer.c")

zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_CHARGER
                                                "${PLATFORM_EC}/common/usb_charger.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_PORT_POWER_DUMB
                                                "${PLATFORM_EC}/common/usb_port_power_dumb.c")

zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_POWER_DELIVERY
                                                "${PLATFORM_EC}/common/usb_common.c"
                                                "${PLATFORM_EC}/common/usbc/usbc_task.c"
                                                "${PLATFORM_EC}/common/usbc/usb_pd_timer.c"
                                                "${PLATFORM_EC}/common/usbc/usb_sm.c"
                                                "${PLATFORM_EC}/common/usbc_intr_task.c")

zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_CONSOLE_CMD_CHARGEN
                                                "${PLATFORM_EC}/common/chargen.c")

zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_CONSOLE_CMD_PD
                                                "${PLATFORM_EC}/common/usbc/usb_pd_console.c")

zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC_RETIMER_FW_UPDATE
                                                "${PLATFORM_EC}/common/usbc/usb_retimer_fw_update.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC_RETIMER_INTEL_BB
                                                "${PLATFORM_EC}/driver/retimer/bb_retimer.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC_SS_MUX
                                                "${PLATFORM_EC}/driver/usb_mux/usb_mux.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_MUX_VIRTUAL
                                                "${PLATFORM_EC}/driver/usb_mux/virtual.c")

zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_PD_LOGGING
                                                "${PLATFORM_EC}/common/event_log.c"
                                                "${PLATFORM_EC}/common/pd_log.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_PD_TBT_COMPAT_MODE
                                                "${PLATFORM_EC}/common/usbc/tbt_alt_mode.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_PD_USB4
                                                "${PLATFORM_EC}/common/usbc/usb_mode.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC_OCP
                                                "${PLATFORM_EC}/common/usbc_ocp.c")

zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_PD_ALT_MODE_DFP
                                                "${PLATFORM_EC}/common/usb_pd_alt_mode_dfp.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_PD_ALT_MODE_UFP
                                                "${PLATFORM_EC}/common/usb_pd_alt_mode_ufp.c")

zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_PD_DUAL_ROLE
                                                "${PLATFORM_EC}/common/usb_pd_dual_role.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_PD_CONSOLE_CMD
                                                "${PLATFORM_EC}/common/usb_pd_console_cmd.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_PD_HOST_CMD
                                                "${PLATFORM_EC}/common/usb_pd_host_cmd.c"
                                                "${PLATFORM_EC}/common/usbc/usb_pd_host.c")

zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_VPD
                                                "${PLATFORM_EC}/common/usbc/usb_tc_vpd_sm.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_CTVPD
                                                "${PLATFORM_EC}/common/usbc/usb_tc_ctvpd_sm.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_DRP_ACC_TRYSRC
                                                "${PLATFORM_EC}/common/usbc/usb_tc_drp_acc_trysrc_sm.c"
                                                "${PLATFORM_EC}/common/usbc/usb_pe_drp_sm.c"
                                                "${PLATFORM_EC}/common/usbc/usb_pd_dpm.c"
						"${PLATFORM_EC}/common/usbc/usbc_pd_policy.c"
                                                "${PLATFORM_EC}/common/usbc/dp_alt_mode.c")

zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_PRL_SM
                                                "${PLATFORM_EC}/common/usbc/usb_prl_sm.c")

zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_PD_TCPM_PS8751
                                                "${PLATFORM_EC}/driver/tcpm/ps8xxx.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_PD_TCPM_PS8805
                                                "${PLATFORM_EC}/driver/tcpm/ps8xxx.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_PD_TCPM_PS8815
                                                "${PLATFORM_EC}/driver/tcpm/ps8xxx.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_PD_TCPM_RT1715
                                                "${PLATFORM_EC}/driver/tcpm/rt1715.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_PD_TCPM_TUSB422
                                                "${PLATFORM_EC}/driver/tcpm/tusb422.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USB_PD_TCPM_TCPCI
                                                "${PLATFORM_EC}/driver/tcpm/tcpci.c")

zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC_PPC
                                                "${PLATFORM_EC}/common/usbc_ppc.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC_PPC_SN5S330
                                                "${PLATFORM_EC}/driver/ppc/sn5s330.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC_PPC_SYV682X
                                                "${PLATFORM_EC}/driver/ppc/syv682x.c")

zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_VBOOT_HASH
                                                "${PLATFORM_EC}/common/vboot_hash.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_VOLUME_BUTTONS
                                                "${PLATFORM_EC}/common/button.c")

zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_VBOOT
                                                "${PLATFORM_EC}/common/vboot/efs2.c")

zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_VSTORE
                                                "${PLATFORM_EC}/common/vstore.c")
