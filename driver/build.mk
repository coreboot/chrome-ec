# -*- makefile -*-
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Drivers for off-chip devices
#

# Thermistors
driver-$(CONFIG_THERMISTOR_NCP15WB)+=temp_sensor/thermistor_ncp15wb.o
