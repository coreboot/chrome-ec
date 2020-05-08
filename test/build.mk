# -*- makefile -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# on-board test binaries build
#

test-list-y=pingpong timer_calib timer_dos timer_jump mutex utils
#disable: powerdemo

test-list-$(BOARD_BDS)+=

# Samus has board-specific chipset code, and the tests don't
# compile with it. Disable them for now.
test-list-$(BOARD_SAMUS)=

# So does Cr50
test-list-$(BOARD_CR50)=

# For some tests, we are running out of RAM. Disable them for now.
test-list-$(BOARD_GLADOS_PD)=
test-list-$(BOARD_CHELL_PD)=
test-list-$(BOARD_OAK_PD)=
test-list-$(BOARD_SAMUS_PD)=

# Emulator tests
ifneq ($(TEST_LIST_HOST),)
test-list-host=$(TEST_LIST_HOST)
else
test-list-host = base32
test-list-host += console_edit
test-list-host += crc32
test-list-host += extpwr_gpio
test-list-host += flash
test-list-host += flash_log
test-list-host += hooks
test-list-host += interrupt
test-list-host += math_util
test-list-host += mutex
test-list-host += nvmem
test-list-host += pinweaver
test-list-host += power_button
test-list-host += queue
test-list-host += rma_auth
test-list-host += rsa
test-list-host += rsa3
test-list-host += sha256
test-list-host += shmalloc
test-list-host += system
test-list-host += timer_dos
test-list-host += utils
test-list-host += x25519
endif

base32-y=base32.o
battery_get_params_smart-y=battery_get_params_smart.o
bklight_lid-y=bklight_lid.o
bklight_passthru-y=bklight_passthru.o
button-y=button.o
charge_manager-y=charge_manager.o
charge_manager_drp_charging-y=charge_manager.o
charge_ramp-y+=charge_ramp.o
console_edit-y=console_edit.o
crc32-y=crc32.o
extpwr_gpio-y=extpwr_gpio.o
fan-y=fan.o
flash-y=flash.o
flash_log-y=flash_log.o
hooks-y=hooks.o
host_command-y=host_command.o
inductive_charging-y=inductive_charging.o
interrupt-scale=10
interrupt-y=interrupt.o
kb_8042-y=kb_8042.o
kb_mkbp-y=kb_mkbp.o
kb_scan-y=kb_scan.o
lid_sw-y=lid_sw.o
lightbar-y=lightbar.o
math_util-y=math_util.o
mutex-y=mutex.o
nvmem-y=nvmem.o nvmem_tpm2_mock.o
nvmem_vars-y=nvmem_vars.o
pingpong-y=pingpong.o
pinweaver-y=pinweaver.o
power_button-y=power_button.o
powerdemo-y=powerdemo.o
queue-y=queue.o
rma_auth-y=rma_auth.o
rsa-y=rsa.o
rsa3-y=rsa.o
sbs_charging-y=sbs_charging.o
sbs_charging_v2-y=sbs_charging_v2.o
sha256-y=sha256.o
shmalloc-y=shmalloc.o
stress-y=stress.o
system-y=system.o
thermal-y=thermal.o
timer_calib-y=timer_calib.o
timer_dos-y=timer_dos.o
usb_pd-y=usb_pd.o
usb_pd_giveback-y=usb_pd.o
utils-y=utils.o
x25519-y=x25519.o

TPM2_ROOT := $(CROS_WORKON_SRCROOT)/src/third_party/tpm2
$(out)/RO/common/new_nvmem.o: CFLAGS += -I$(TPM2_ROOT) -I chip/g
$(out)/RO/test/nvmem.o: CFLAGS += -I$(TPM2_ROOT)
$(out)/RO/test/nvmem_tpm2_mock.o: CFLAGS += -I$(TPM2_ROOT)
