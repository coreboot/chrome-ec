# -*- makefile -*-
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Host tools build
#

host-util-bin=stm32mon ec_parse_panicinfo

ec_parse_panicinfo-objs=ec_parse_panicinfo.o ec_panicinfo.o
