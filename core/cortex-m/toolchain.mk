# Copyright 2024 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Force gcc compiler
CROSS_COMPILE_CC_NAME:=gcc

CROSS_COMPILE_ARM_DEFAULT:=arm-eabi
COREBOOT_TOOLCHAIN:=arm
USE_COREBOOT_SDK:=1

# Use coreboot-sdk
$(call set-option,CROSS_COMPILE,\
	$(CROSS_COMPILE_arm),\
	arm-eabi)
