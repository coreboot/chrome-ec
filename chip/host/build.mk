# -*- makefile -*-
# Copyright 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# emulator specific files build
#

CORE:=host

chip-y=clock.o flash.o gpio.o i2c.o lpc.o persistence.o reboot.o registers.o \
       spi_controller.o system.o trng.o uart.o
ifndef CONFIG_KEYBOARD_NOT_RAW
chip-$(HAS_TASK_KEYSCAN)+=keyboard_raw.o
endif

ifeq ($(CONFIG_DCRYPTO),y)
CPPFLAGS += -I$(abspath ./board/cr50/dcrypto)
dirs-y += board/cr50/dcrypto
LDFLAGS_EXTRA += -lcrypto
endif

ifeq ($(CONFIG_DCRYPTO_MOCK),y)
CPPFLAGS += -I$(abspath ./board/cr50)
dirs-y += board/cr50/dcrypto
endif

dirs-y += chip/host/dcrypto

chip-$(CONFIG_DCRYPTO)+= dcrypto/aes.o
chip-$(CONFIG_DCRYPTO)+= dcrypto/app_cipher.o
chip-$(CONFIG_DCRYPTO)+= dcrypto/app_key.o
chip-$(CONFIG_DCRYPTO)+= dcrypto/sha256.o

# Object files that can be shared with the Cr50 dcrypto implementation
chip-$(CONFIG_DCRYPTO)+= ../../board/cr50/dcrypto/hmac_sw.o
chip-$(CONFIG_DCRYPTO)+= ../../board/cr50/dcrypto/hash_api.o
chip-$(CONFIG_DCRYPTO)+= ../../board/cr50/dcrypto/sha1.o
chip-$(CONFIG_DCRYPTO)+= ../../board/cr50/dcrypto/sha256.o
ifeq ($(CONFIG_UPTO_SHA512),y)
chip-$(CONFIG_DCRYPTO)+= ../../board/cr50/dcrypto/sha512.o
endif
chip-$(CONFIG_DCRYPTO)+= ../../board/cr50/dcrypto/hmac_drbg.o
chip-$(CONFIG_DCRYPTO)+= ../../board/cr50/dcrypto/p256.o
chip-$(CONFIG_DCRYPTO)+= ../../board/cr50/dcrypto/compare.o
chip-$(CONFIG_DCRYPTO)+= ../../board/cr50/dcrypto/hkdf.o

# We still want raw SHA & HMAC implementations for mocked dcrypto
chip-$(CONFIG_DCRYPTO_MOCK)+= ../../board/cr50/dcrypto/sha256.o
chip-$(CONFIG_DCRYPTO_MOCK)+= ../../board/cr50/dcrypto/hmac_sw.o