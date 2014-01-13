#!/usr/bin/env bash
#
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

echo
echo "> Patches here but not on firmware-rambi."
echo "< Patches on firmware-rambi but not here."
echo "========================================="

git --no-pager log --left-right --graph --cherry-pick --oneline \
	cros/firmware-rambi-5216.B...HEAD
