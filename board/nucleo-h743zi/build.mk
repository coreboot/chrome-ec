# Copyright 2019 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Board specific files build

BASEBOARD:=nucleo-h743zi

board-y=board.o

# Enable on device tests
test-list-y=\
       abort \
       aes \
       boringssl \
       cec \
       compile_time_macros \
       crc \
       debug \
       exception \
       flash_physical \
       flash_write_protect \
       libc_printf \
       mpu \
       mutex \
       pingpong \
       printf \
       queue \
       rng_benchmark \
       rollback \
       rollback_entropy \
       rsa3 \
       rtc \
       sbrk \
       scratchpad \
       sha256 \
       sha256_unrolled \
       static_if \
       stdlib \
       timer_dos \
       utils \
       utils_str \