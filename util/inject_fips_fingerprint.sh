#!/bin/bash
#
# Copyright 2021 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Calculate hash of fips module and inject it into the .elf file.

main() {
  local objcopy="${1}"
  local objdump="${2}"
  local rw_elf_in="${3}"
  local base="${rw_elf_in%.elf}"
  local rw_elf_out="${rw_elf_in}.fips"
  local checksum_section=".text.fips_checksum"
  local fips_checksum="${base}.fips.checksum"
  local fips_checksum_dump="${fips_checksum}.dump"
  local size
  local sections
  local fips_start
  local fips_end
  local fips_offset
  local file_offset
  local base_addr
  local result

  if [ ! -f "${rw_elf_in}" ] ; then
    echo "  ${rw_elf_in} doesn't exist"
    return 1
  fi

  echo "${rw_elf_in} ${rw_elf_out}"
  sections=$( objdump -t "${rw_elf_in}" )

  if [[ "${sections}" =~ "${checksum_section}" ]] ; then
    echo "  get fips checksum"
  else
    echo "  no fips checksum"
    return 1
  fi
  vals=( $(${objdump}  -x -j .text "${rw_elf_in}" | awk '
  {
          if ($2 == ".text" ) {
                  file_offs = $6
                  base_addr = $5
          }
          if ($5 == "__fips_module_start") {fips_start = $1 }
          if ($5 == "__fips_module_end") {fips_end = $1 }
  }
  END { printf "0x%s 0x%s 0x%s 0x%s\n", file_offs, base_addr, fips_start,
  fips_end }') )

  file_offset=${vals[0]}
  base_addr=${vals[1]}
  fips_start=${vals[2]}
  fips_end=${vals[3]}
  size=$((fips_end - fips_start))
  fips_offset=$((file_offset + fips_start - base_addr))

  result=$(dd if="${rw_elf_in}" skip="${fips_offset}" count="${size}" bs=1 | \
               sha256sum)

  echo "${result%% *}" > "${fips_checksum}"
  echo "${result%% *}" | xxd -r -p  > "${fips_checksum_dump}"

  cp "${rw_elf_in}" "${rw_elf_out}"
  ${objcopy} --update-section "${checksum_section}"="${fips_checksum_dump}" \
		"${rw_elf_out}"
}

main "$@"
