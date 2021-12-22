#!/bin/bash
# Copyright 2021 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script shows mapping of /dev/ttyUSBx links to actual device files in
# the /dev tree. The mapping makes it easy to follow various connected devices
# to their /dev/ttyUSBx links.

TMPF=$(mktemp '/tmp/maptty.XXXXX')
trap 'rm -rf ${TMPF}' EXIT

# Create a text file each line in which maps soft links found in /dev to their
# actual device files, in particular to /dev/ttyUSBx devices.
for f in $(find /dev -type l | grep -Ev '(pci-|char/)'); do
  rn="$(readlink -f "${f}")";
  echo "${rn}|${f}" >> "${TMPF}"
done

# For all /dev/ttyUSBx devices print all their soft links.
for n in $(ls /dev/ttyUSB* | cut -c12- | sort -n); do
  tty="/dev/ttyUSB${n}"
  links=( $(awk -F'|' -vtty="${tty}" '{if ($1 == tty) {print $2}}' "${TMPF}" |
              sort) )
  printf "%-13s %s\n" "${tty}" "${links[0]}"
  for link in "${links[@]:1}"; do
    printf "%13s %s\n" " " "${link}"
  done
done
