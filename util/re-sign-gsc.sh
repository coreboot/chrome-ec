#!/bin/bash
# Copyright 2023 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script allows to quickly re-sign a GSC image to a different set of
# Device IDs. The script works with both Cr50 and Ti50, it must be run in the
# appropriate top level directory, and the respective GSC binary has to be
# present in the respective ./build directory.
#
# Two command line parameters are required, the device IDs of the GSC.
#
# The generated binary is saved in {cr,ti}50.<sha>.<devid0>.<devid1>.bin where
# <sha> is the git sha of the current source tree. The suffix '-dirty' is
# added to the <sha> component in case there are changes in any of the files
# under git control.

set -ue

SCRIPT_NAME="$(basename "$0")"
TMPD="$(mktemp -d "/tmp/${SCRIPT_NAME}.XXXXX")"
NOCLEAN="${NOCLEAN:-}"
if [[ -z ${NOCLEAN} ]]; then
  trap 'rm -rf "${TMPD}"' EXIT
fi

# PKCS11 connector library needed for codesigner to access keys in Cloud KMS.
PKCS11_MODULE_PATH="/usr/lib64/libkmsp11.so"

# Cloud KMS path to the location of the GSC signing keys.
KMS_PROJECT_PATH="projects/gsc-cloud-kms-signing/locations/us/keyRings/gsc-node-locked-signing-keys/cryptoKeys"

# Make sure there is a codesigner in the path.
CODESIGNER=""
for f in  cr50-codesigner \
            ../../cr50-utils/software/tools/codesigner/codesigner \
            codesigner; do
  if command -v "${f}" > /dev/null 2>&1; then
    CODESIGNER="${f}"
    break
  fi
done
if [[ -z ${CODESIGNER} ]]; then
  echo "SCRIPT_NAME error: can't find codesigner" >&2
  exit 1
fi

# Re-sign a single RW section.
re_sign_rw() {
  local tmp_file="$1"
  local flash_base="$2"
  local rw_base="$3"
  local codesigner_params
  local rw_size
  local rw_bin_base
  local rw_bin_size
  local skip

  codesigner_params=()

  # Retrieve the rest of the codesigner command line arguments, which are this
  # function's arguments after the three fixed ones.
  shift 3
  while [[ $# != 0 ]]; do
    codesigner_params+=( "$1" )
    shift
  done

  # Determine RW size. It is 4 bytes at offset 808 into the signed header.
  skip=$(( rw_base + 808 ))
  rw_size="$(hexdump -s "${skip}" -n4 -e '1/4 "%d"' "${tmp_file}")"

  # Extract the RW section to re-sign, dropping the existing header.
  rw_bin_base=$(( rw_base + 1024  ))
  rw_bin_size=$(( rw_size - 1024 ))
  dd if="${full_bin}" of="${TMPD}/rw.bin" skip="${rw_bin_base}" bs=1 \
     count="${rw_bin_size}" status="none"

  # Convert it to hex for signing
  objcopy -I binary -O ihex --change-addresses $(( rw_bin_base + flash_base )) \
          "${TMPD}/rw.bin" "${TMPD}/rw.hex"

  # Sign.
  "${CODESIGNER}" "${codesigner_params[@]}" --input="${TMPD}/rw.hex" \
                  --output="${TMPD}/rw.signed"

  # Paste the result back into the original binary.
  dd if="${TMPD}/rw.signed" of="${tmp_file}" seek="${rw_base}" conv="notrunc" \
     status="none" bs=1
}

main () {
  local bin_size
  local dev_id0
  local dev_id1
  local flash_base
  local full_bin
  local manifest
  local output
  local prefix
  local rw_a_base
  local rw_b_base
  local rw_key
  local sha
  local tmp_file
  local xml

  if [[ $# -ne 2 ]]; then
    echo "${SCRIPT_NAME} error:" >&2
    echo " Two command line arguments are required, dev_id0 and dev_id1" >&2
    exit 1
  fi

  dev_id0="$1"
  dev_id1="$2"

  full_bin=""
  for f in  build/ti50/dauntless/dauntless/full_image.signed.bin \
    build/cr50/ec.bin; do
    if [[ -f ${f} ]]; then
      full_bin="${f}"
      break
    fi
  done

  if [[ -z ${full_bin} ]]; then
    echo "${SCRIPT_NAME} error: GSC binary not found" >&2
    exit 1
  fi

  codesigner_params=(
    --dev_id0="${dev_id0}"
    --dev_id1="${dev_id1}"
    --format=bin
    --ihex
    --no-icache
    --override-keyid
    --padbank
  )

  bin_size="$(stat -c '%s' "${full_bin}")"
  case "${bin_size}" in
    (524288) rw_a_base=16384 # RO area size is fixed at 16K
             rw_b_base=$(( bin_size / 2 + rw_a_base ))
             rw_key="util/signer/cr50-hsm-node-locked-key.pem.pub"
             manifest="util/signer/ec_RW-manifest-dev.json"
             xml="util/signer/fuses.xml"
             codesigner_params+=(
               --b
               --pkcs11_engine="${PKCS11_MODULE_PATH}:0:${KMS_PROJECT_PATH}/cr50-hsm-node-locked-key/cryptoKeyVersions/1"
             )
             flash_base=262144
             prefix="cr50"
             KMS_PKCS11_CONFIG="$(readlink -f chip/g/config.yaml)"
             ;;
    (1048576) local rw_bases
              # Third and sixths lines showing signed header magic are base
              # addresses of RW_A and RW_B.
              mapfile -t rw_bases < <(od -Ax -t x1 "${full_bin}" |awk '
                    /^....00 fd ff ff ff/ {
                       line = line + 1;
                       if (line % 3 == 0) {
                               print strtonum("0x"$1)
                       }
                   }')
              rw_a_base="${rw_bases[0]}"
              rw_b_base="${rw_bases[1]}"
              rw_key="ports/dauntless/signing/ti50_dev.key"
              manifest="ports/dauntless/signing/manifest.TOT.json"
              xml="ports/dauntless/signing/fuses.xml"
              codesigner_params+=(
                --dauntless
                --pkcs11_engine="${PKCS11_MODULE_PATH}:0:${KMS_PROJECT_PATH}/ti50-node-locked-key/cryptoKeyVersions/1"
              )
              flash_base=524288
              prefix="ti50"
              KMS_PKCS11_CONFIG="$(readlink -f ports/dauntless/config.yaml)"

              ;;
    (*) echo "What is ${full_bin}?" >&2
        exit 1
        ;;
  esac

  # Determine the current git tree state. This would match the binary image's
  # version string if it was built from this tree.
  sha="$(git rev-parse --short HEAD)"
  if git status --porcelain 2>&1 | grep -qv '^ *?'; then
    sha="${sha}-dirty"
  fi

  #  Check if the output file already exists.
  output="$(printf "${prefix}.${sha}.%08x-%08x.bin" "${dev_id0}" "${dev_id1}")"
  if [[ -f ${output} ]]; then
    echo "${output} is already there"
    exit 0
  fi

  # Make sure all necessary files are present.
  for f in "${rw_key}" "${manifest}" "${xml}"; do
    if [[ ! -f ${f} ]]; then
      echo "File ${f} not found" >&2
      exit 1
    fi
  done

  # Clean up the manifest.
  sed -zE 's/"board_[^,]+,\s*//g;s/"info"[^}]+},\s*/"info": { },/' \
      "${manifest}" > "${TMPD}/manifest.json"

  tmp_file="${TMPD}/full.bin"
  cp "${full_bin}" "${tmp_file}"

  codesigner_params+=(
      --json "${TMPD}/manifest.json"
    --key "${rw_key}"
      -x "${xml}"
  )

  echo "Re-signing a ${prefix} image"

  export KMS_PKCS11_CONFIG

  re_sign_rw "${tmp_file}" "${flash_base}" "${rw_a_base}" \
             "${codesigner_params[@]}"
  re_sign_rw "${tmp_file}" "${flash_base}" "${rw_b_base}" \
             "${codesigner_params[@]}"

  cp "${tmp_file}" "${output}"
}

main "$@"
