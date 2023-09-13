# Copyright 2015 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CC ?= gcc
PKG_CONFIG ?= pkg-config
PROGRAMS := gsctool
LIBS    :=
CFLAGS  += -std=gnu99 \
	-g \
	-Wall \
	-Werror \
	-Wpointer-arith \
	-Wcast-align \
	-Wcast-qual \
	-Wundef \
	-Wsign-compare \
	-Wredundant-decls \
	-Wmissing-declarations

ifneq ($(DEBUG),)
CFLAGS += -O0
else
CFLAGS += -O3
endif

#
# Add libusb-1.0 required flags
#
LIBS_CONFIG := $(shell $(PKG_CONFIG) --libs   libusb-1.0)
CFLAGS_CONFIG := $(shell $(PKG_CONFIG) --cflags libusb-1.0)
CFLAGS  += -I../../include -I../../util -I../../fuzz -I../../test

# Enable large file support.
CPPFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE

VPATH = ../../util

BOARD := cr50

LIBS_g  := $(shell $(PKG_CONFIG) --libs libcrypto)
CFLAGS_g := $(shell $(PKG_CONFIG) --cflags libcrypto)

LIBS    += $(LIBS_CONFIG) $(LIBS_g)
CFLAGS  += $(CFLAGS_CONFIG) $(CFLAGS_g)
CPPFLAGS  += -I../../board/cr50 -I ../../chip/g

all: $(PROGRAMS)

GSCTOOL_SOURCES := gsctool.c desc_parser.c usb_if.c verify_ro.c
GSCTOOL_OBJS := $(patsubst %.c,%.o,$(GSCTOOL_SOURCES))
DEPS := $(patsubst %.c,%.d,$(GSCTOOL_SOURCES)) dp.d

# chip/g updater
gsctool: $(GSCTOOL_OBJS) Makefile
	$(CC) $(CFLAGS) $(GSCTOOL_OBJS) $(LDFLAGS) $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -MMD -MF $(basename $@).d -o $@ $<

gsctool.o:  generated_version.h

generated_version.h: $(GSCTOOL_SOURCES)
	# Make sure ${BOARD} not set to anything when invoking getversion.sh,
	# as even when building with Cr50 enabled, other directories do not
	# matter for gsctool.
	@(cd ../../; BOARD= util/getversion.sh) > $@

.PHONY: clean
clean:
	rm -rf $(PROGRAMS) *~ *.o *.d dp generated_version.h

dp: CFLAGS += -O0
dp: desc_parser.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DTEST_PARSER -MMD -MF $(basename $@).d \
		desc_parser.c -o $@

-include $(DEPS)
