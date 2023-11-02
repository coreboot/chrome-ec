#!/bin/bash
# Copyright 2023 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Wrapper script for retrieving and interpreting AP RO verification status of
# a ChromeOS device connected to the host over CCD.

set -uo pipefail

PROGRAM="explain_ap_ro_verification_status"
if ! command -v "${PROGRAM}" > /dev/null 2>&1; then
  echo "The '${PROGRAM}' utility is not installed, run update_chroot"
  exit 1
fi

status="$(gsctool -W 2>&1 | awk '/expanded_aprov_status:/ {print "0x"$2}' )"
# shellcheck disable=SC2181
if [[ $? != 0 ]]; then
  echo "Failed to retrieve status, is your DUT CCD connected?" >&2
  exit 1
fi

"${PROGRAM}" "${status}"
