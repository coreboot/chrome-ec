# -*- makefile -*-
# Copyright 2012 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Cortex-M4 core OS files build
#

# Use coreboot-sdk
$(call set-option,CROSS_COMPILE,\
	$(CROSS_COMPILE_arm),\
	/opt/coreboot-sdk/bin/arm-eabi-)
# Force gcc compiler
cc-name:=gcc
# FPU compilation flags
CFLAGS_FPU-$(CONFIG_FPU)=-mfpu=fpv4-sp-d16 -mfloat-abi=hard

# CPU specific compilation flags
CFLAGS_CPU+=-mthumb -Os -mno-sched-prolog
CFLAGS_CPU+=-mno-unaligned-access
CFLAGS_CPU+=$(CFLAGS_FPU-y)

ifneq ($(CONFIG_LTO),)
CFLAGS_CPU+=-flto
LDFLAGS_EXTRA+=-flto
endif

# gcc 11.2 had a known issue which doesn't affect Cr50 build anymore
# but we can't remove -fno-ipa-modref as it changes FIPS module digest
GCC_VERSION := $(shell $(CROSS_COMPILE)$(cc-name) -dumpversion)
ifeq ("$(GCC_VERSION)","11.2.0")
# IPA modref pass crashes gcc 11.2 when LTO is used with partial linking
CFLAGS_CPU += -fno-ipa-modref

# Set an option to force LTO to generate target machine code
export CFLAGS_LTO_PARTIAL_LINK := -flinker-output=nolto-rel
endif

core-y=cpu.o init.o ldivmod.o llsr.o uldivmod.o vecttable.o
core-$(CONFIG_AES)+=aes.o
core-$(CONFIG_AES_GCM)+=ghash.o
core-$(CONFIG_COMMON_PANIC_OUTPUT)+=panic.o
core-$(CONFIG_COMMON_RUNTIME)+=switch.o task.o
core-$(CONFIG_WATCHDOG)+=watchdog.o
