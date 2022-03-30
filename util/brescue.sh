#!/bin/bash
# Copyright 2021 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# A script to facilitate rescue update of GSC targets, supports both Cr50 and
# Ti50.
#
# The two input parameters are the file name of the full binary mage (both ROs
# and both RWs) and the name of the UART device connected to the GSC console.
#
# The script carves out the RW_A from the binary, converts it into hex format
# and starts the rescue utility to send the RW to the chip. The user is
# supposed to reset the chip to trigger the rescue session.
#
# If invoked with RESCUE environment variable set to a path, the script will try
# to use the rescue binary at that path.
#
# If invoked with nonempty NOCLEAN environment variable, the script preserves
# the hex RW image it creates.
#
# If invoked with nonempty EARLY environment variable, the script passes the
# --early option to rescue.

TMPD="$(mktemp -d "/tmp/$(basename "$0").XXXXX")"

for r in ${RESCUE} rescue cr50-rescue; do
  if type "${r}" > /dev/null 2>&1; then
    RESCUE="${r}"
    break
  fi
done

if [[ -z ${RESCUE} ]]; then
  echo "rescue utility is not found, can not continue" >&2
  exit 1
fi
if [[ -z "${NOCLEAN}" ]]; then
  trap 'rm -rf "${TMPD}"' EXIT
fi

# --early could help to improve success rate depending on setup.
early=''
if [[ -n "${EARLY}" ]]; then
  early='--early'
fi

dest_hex="${TMPD}/rw.hex"
dest_bin="${TMPD}/rw.bin"
errorf="${TMPD}/error"

usage() {
  cat >&2 <<EOF
Two parameters are required, the name of the valid GSC binary image
and the name of the H1 console tty device
EOF
  exit 1
}

# Determine RW_A offset of the Ti50 image. The header magic pattern is used to
# find headers in the binary.
rw_offset() {
  local src
  local base

  src="$1"
  # The second grep term filters out the 'fake' cryptolib header, which does
  # not contain a valid signature, the signature field is filled with SSSSS
  # (0x53...).
  base=$(/usr/bin/od -Ax -t x1 -v "${src}" |
    grep -E '^....00 fd ff ff ff' | grep -v '00 fd ff ff ff 53 53 53' |
    head -2 |
    tail -1 |
    sed 's/ .*//')
  printf '%d' "0x${base}"
}

if [[ $# != 2 ]]; then
  usage
fi

source="$1"
device="$2"

if [[ ! -f ${source} ]]; then
  usage
fi

if [[ ${device} != /dev/*  || ! -e ${device} ]]; then
  usage
fi

if [[ -n $(lsof "${device}" 2>/dev/null) ]]; then
  echo "${device} is in use, make sure it is available" >&2
  exit 1
fi

# Use Cr50 or Ti50 options base on the file size.
case "$(stat -c '%s' "${source}")" in
  (524288)
    skip=16384
    count=233472
    chip_extension=''
    addr=0x44000
    ;;
  (1048576)
    skip=$(rw_offset "${source}")
    count=$(( 1048576/2 - "${skip}" ))
    chip_extension='--dauntless'
    addr="$(printf '0x%x' $(( 0x80000 + "${skip}" )))"
    ;;
  (*)
    echo "Unrecognized input file" >&2
    exit 1
    ;;
esac

# Carve out RW_A in binary form.
if ! dd if="${source}" of="${dest_bin}" skip="${skip}" count="${count}" bs=1 \
  2>"${errorf}"; then
  echo "Failed to carve out the RW bin:" >&2
  cat "${errorf}" >&2
  exit 1
fi
echo "carved out binary ${dest_bin} mapped to ${addr}"

# Convert binary to hex.
if ! objcopy -I binary -O ihex --change-addresses "${addr}" \
  "${dest_bin}" "${dest_hex}"; then
  echo "Failed to convert to hex" >&2
  exit 1
fi
echo "converted to ${dest_hex}, waiting for target reset"

echo "${RESCUE}"  "${chip_extension}" -d "${device}" "${early}" -v -i \
  "${dest_hex}"
"${RESCUE}"  "${chip_extension}" -d "${device}" "${early}" -v -i "${dest_hex}"
