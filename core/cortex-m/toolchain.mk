# Copyright 2024 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Force gcc compiler
CROSS_COMPILE_CC_NAME:=gcc

# Use coreboot-sdk
$(call set-option,CROSS_COMPILE,\
	$(CROSS_COMPILE_arm),\
	/opt/coreboot-sdk/bin/arm-eabi-)
