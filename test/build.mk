# -*- makefile -*-
# Copyright 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Device test binaries
test-list-y ?= pingpong timer_calib timer_dos timer_jump mutex utils utils_str
#disable: powerdemo

# Emulator tests
ifneq ($(TEST_LIST_HOST),)
test-list-host=$(TEST_LIST_HOST)
else
test-list-host = aes
test-list-host += base32
test-list-host += button
test-list-host += cec
test-list-host += console_edit
test-list-host += crc32
test-list-host += ec_comm
test-list-host += entropy
test-list-host += flash
test-list-host += flash_log
test-list-host += float
test-list-host += fp
test-list-host += hooks
test-list-host += host_command
test-list-host += interrupt
test-list-host += is_enabled
test-list-host += is_enabled_error
test-list-host += math_util
test-list-host += mutex
test-list-host += nvmem
test-list-host += pingpong
test-list-host += pinweaver
test-list-host += power_button
test-list-host += printf
test-list-host += queue
test-list-host += rma_auth
test-list-host += rsa
test-list-host += rsa3
test-list-host += rtc
test-list-host += sha256
test-list-host += sha256_unrolled
test-list-host += shmalloc
test-list-host += static_if
test-list-host += static_if_error
test-list-host += system
test-list-host += thermal
test-list-host += timer_dos
test-list-host += uptime
test-list-host += utils
test-list-host += utils_str
test-list-host += vboot
test-list-host += x25519
endif

aes-y=aes.o
base32-y=base32.o
button-y=button.o
cec-y=cec.o
console_edit-y=console_edit.o
crc32-y=crc32.o
ec_comm-y=ec_comm.o
entropy-y=entropy.o
flash-y=flash.o
flash_log-y=flash_log.o
hooks-y=hooks.o
host_command-y=host_command.o
interrupt-y=interrupt.o
is_enabled-y=is_enabled.o
math_util-y=math_util.o
mutex-y=mutex.o
nvmem-y=nvmem.o nvmem_tpm2_mock.o
pingpong-y=pingpong.o
pinweaver-y=pinweaver.o
power_button-y=power_button.o
powerdemo-y=powerdemo.o
printf-y=printf.o
queue-y=queue.o
rma_auth-y=rma_auth.o
rsa-y=rsa.o
rsa3-y=rsa.o
rtc-y=rtc.o
sha256-y=sha256.o
sha256_unrolled-y=sha256.o
shmalloc-y=shmalloc.o
static_if-y=static_if.o
stress-y=stress.o
system-y=system.o
thermal-y=thermal.o
timer_calib-y=timer_calib.o
timer_dos-y=timer_dos.o
uptime-y=uptime.o
utils-y=utils.o
utils_str-y=utils_str.o
vboot-y=vboot.o
float-y=fp.o
fp-y=fp.o
x25519-y=x25519.o

TPM2_ROOT := $(CROS_WORKON_SRCROOT)/src/third_party/tpm2
$(out)/RO/common/new_nvmem.o: CFLAGS += -I$(TPM2_ROOT) -I chip/g
$(out)/RO/test/nvmem.o: CFLAGS += -I$(TPM2_ROOT)
$(out)/RO/test/nvmem_tpm2_mock.o: CFLAGS += -I$(TPM2_ROOT)

host-is_enabled_error: TEST_SCRIPT=is_enabled_error.sh
is_enabled_error-y=is_enabled_error.o.cmd

host-static_if_error: TEST_SCRIPT=static_if_error.sh
static_if_error-y=static_if_error.o.cmd
