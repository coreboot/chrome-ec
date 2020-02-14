# -*- makefile -*-
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Baseboard specific files build
#

baseboard-y=baseboard.o
baseboard-$(VARIANT_DEDEDE_EC_NPCX796FC)+=variant_ec_npcx796fc.o
