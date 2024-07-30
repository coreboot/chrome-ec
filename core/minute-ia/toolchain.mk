# Copyright 2024 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# force gcc
CROSS_COMPILE_CC_NAME?=gcc

# Select Minute-IA bare-metal toolchain
$(call set-option,CROSS_COMPILE,$(CROSS_COMPILE_i386),\
	/opt/coreboot-sdk/bin/i386-elf-)
