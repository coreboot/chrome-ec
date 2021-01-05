# -*- makefile -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Drivers for off-chip devices
#

# Note that this variable includes the trailing "/"
_driver_cur_dir:=$(dir $(lastword $(MAKEFILE_LIST)))

# Current/Power monitor
driver-$(CONFIG_INA219)$(CONFIG_INA231)+=ina2xx.o
driver-$(CONFIG_INA3221)+=ina3221.o

# Thermistors
driver-$(CONFIG_THERMISTOR)+=temp_sensor/thermistor.o
driver-$(CONFIG_THERMISTOR_NCP15WB)+=temp_sensor/thermistor_ncp15wb.o
