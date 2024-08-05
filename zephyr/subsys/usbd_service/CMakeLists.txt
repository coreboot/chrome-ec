# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_USB_DEVICE_STACK_NEXT usbd_init.c)

if (CONFIG_CROS_EC_RW)
  zephyr_library_sources_ifdef(CONFIG_USBD_HID_KEYBOARD usbd_hid_kb.c)
  zephyr_library_sources_ifdef(CONFIG_USBD_HID_TOUCHPAD usbd_hid_tp.c)
  zephyr_library_sources_ifdef(CONFIG_USBD_HID_VIVALDI hid_vivaldi.c)
endif()
