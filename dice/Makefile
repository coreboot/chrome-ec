# Copyright 2024 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CC ?= gcc
PROGRAM := test_dice
LIBS    := -lcrypto
CFLAGS  += -std=gnu99 \
	-g \
	-Wall \
	-Werror \
	-Wpointer-arith \
	-Wcast-align \
	-Wundef \
	-Wsign-compare \
	-Wredundant-decls \
	-Wmissing-declarations
RESULT_FILES := test_handover.cbor test_dice_chain.cbor
PCR0_OPTION := normal
PROGRAM_OPTIONS := $(RESULT_FILES) $(PCR0_OPTION)

ifneq ($(DEBUG),)
CFLAGS += -O0
else
CFLAGS += -O3
endif

ifneq ($(DICE_DEBUG),)
CFLAGS += -DDICE_DEBUG
endif

all: test

test: $(PROGRAM)
	./$(PROGRAM) $(PROGRAM_OPTIONS)

SOURCES := test.c dice.c platform_host.c
OBJS := $(patsubst %.c,%.o,$(SOURCES))
DEPS := $(patsubst %.c,%.d,$(SOURCES))

$(PROGRAM): $(OBJS) Makefile
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) $(LIBS) -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -MMD -MF $(basename $@).d -o $@ $<

.PHONY: clean
clean:
	rm -rf $(PROGRAM) *~ *.o *.d $(RESULT_FILES)

-include $(DEPS)
