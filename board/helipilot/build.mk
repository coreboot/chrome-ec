# Copyright 2023 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Helipilot board specific files build
#

BASEBOARD:=helipilot

board-y=

# Do not build rsa test because this board uses RSA exponent 3 and the rsa test
# will fail on device.
# TODO(b/314131076): Fix cortexm_fpu test for helipilot
# TODO(b/314131510): Fix stm32f_rtc test (or create a new one) for helipilot
test-list-y=\
       abort \
       aes \
       always_memset \
       benchmark \
       boringssl_crypto \
       compile_time_macros \
       cortexm_fpu \
       crc \
       debug \
       exception \
       flash_physical \
       flash_write_protect \
       fpsensor \
       fpsensor_auth_crypto_stateful \
       fpsensor_auth_crypto_stateless \
       fpsensor_hw \
       ftrapv \
       global_initialization \
       libc_printf \
       libcxx \
       malloc \
       mpu \
       mutex \
       panic \
       panic_data \
       pingpong \
       printf \
       queue \
       rng_benchmark \
       rollback \
       rollback_entropy \
       rsa3 \
       rtc \
       rtc_npcx9 \
       sbrk \
       scratchpad \
       sha256 \
       sha256_unrolled \
       static_if \
       stdlib \
       std_vector \
       system_is_locked \
       timer \
       timer_dos \
       tpm_seed_clear \
       uart \
       unaligned_access \
       unaligned_access_benchmark \
       utils \
       utils_str \

# Note that this variable includes the trailing "/"
_hatch_fp_cur_dir:=$(dir $(lastword $(MAKEFILE_LIST)))
-include $(_hatch_fp_cur_dir)../../private/board/helipilot/build.mk
