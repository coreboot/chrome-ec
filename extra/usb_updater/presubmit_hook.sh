#!/bin/bash
# Copyright 2024 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

commit=$1
shift
files=$*

if [[ ${files} == *"usb_updater"* ]]; then
  if ! git log -n 1 "${commit}" | \
     grep -q "gscdevboard.GSCFactoryUpdate"; then
    echo 'Run and add "TEST=BED=DT make tast' \
         'TAST_EXPR=gscdevboard.GSCFactoryUpdate.ti50_0_21_1"' \
         'to commit msg for gsctool changes. See go/gsc-bed to set custom.mk' \
         'up in a way to work with BED= for local test set up.'
    exit 1
  fi
fi
