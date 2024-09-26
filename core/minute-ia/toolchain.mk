# Copyright 2024 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# force gcc
CROSS_COMPILE_CC_NAME?=gcc

CROSS_COMPILE_X86_DEFAULT:=i386-elf
COREBOOT_TOOLCHAIN:=x86
USE_COREBOOT_SDK:=1

# Select Minute-IA bare-metal toolchain
$(call set-option,CROSS_COMPILE,$(CROSS_COMPILE_i386),i386-elf)
