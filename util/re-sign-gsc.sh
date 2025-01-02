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
# The image to re-sign can be supplied as an optional third param.
#
# The generated binary is saved in {cr,ti}50.<sha>.<devid0>.<devid1>.bin where
# <sha> is the git sha of the original image. The suffix '-dirty' is added to
# the <sha> component if "+" is in the {cr,ti}50 version string. "+" means that
# there were changes in some of the files under git control when the original
# image was built.

set -ue

SCRIPT_NAME="$(basename "$0")"
SCRIPT_DIR="$(dirname "$0")"
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
            "${SCRIPT_DIR}/../../cr50-utils/software/tools/codesigner/codesigner" \
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

# Make sure there is a gsctool in the path.
GSCTOOL=""
for f in  gsctool \
	    "${SCRIPT_DIR}/../extra/usb_updater/gsctool"; do
  if command -v "${f}" > /dev/null 2>&1; then
    GSCTOOL="${f}"
    break
  fi
done
if [[ -z ${GSCTOOL} ]]; then
  echo "SCRIPT_NAME error: can't find gsctool" >&2
  exit 1
fi

# Update the manifest
update_manifest() {
  local full_bin="${1}"
  local manifest="${2}"
  local epoch
  local major
  local minor
  local rw_ver

  # Remove the board id and info rollback bits.
  sed -i -zE 's/"board_[^,]+,\s*//g;s/"info"[^}]+},\s*/"info": { },/' \
	  "${manifest}"

  rw_ver="$("${GSCTOOL}" "-M" "-b" "${full_bin}" | \
	  awk -F= '/IMAGE_RW_FW_VER/ {print $2}')"
  IFS='.' read -r epoch major minor <<<"${rw_ver}"
  echo "RW: ${rw_ver}"
  sed "s/epoch\": [0-9]*/epoch\": ${epoch}/" "${manifest}" -i
  sed "s/major\": [0-9]*/major\": ${major}/" "${manifest}" -i
  sed "s/minor\": [0-9]*/minor\": ${minor}/" "${manifest}" -i
}

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
  local gsc_dir
  local manifest
  local output
  local prefix
  local rw_a_base
  local rw_b_base
  local rw_key
  local rw_ver
  local sha
  local tmp_file
  local xml

  full_bin=""
  if [[ $# -eq 3 ]]; then
    full_bin="$3"
  elif [[ $# -ne 2 ]]; then
    echo "${SCRIPT_NAME} error:" >&2
    echo " Two command line arguments are required, dev_id0 and dev_id1" >&2
    echo " The image path is an optional third argument" >&2
    exit 1
  fi

  dev_id0="$1"
  dev_id1="$2"

  if [[ -z ${full_bin} ]] ; then
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
  else
    if [[ -f ${full_bin} ]] ; then
      echo "resigning supplied bin ${full_bin}"
    else
      echo "could not find ${full_bin}"
      exit 1
    fi
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
             gsc_dir="${SCRIPT_DIR}/../../cr50"
	     key_name="cr50-hsm-backed-node-locked-key"
             rw_key="${gsc_dir}/util/signer/${key_name}.pem.pub"
             manifest="${gsc_dir}/util/signer/ec_RW-manifest-dev.json"
             xml="${gsc_dir}/util/signer/fuses.xml"
             codesigner_params+=(
               --b
               --pkcs11_engine="${PKCS11_MODULE_PATH}:0:${KMS_PROJECT_PATH}/${key_name}/cryptoKeyVersions/1"
             )
             flash_base=262144
             prefix="cr50"
             KMS_PKCS11_CONFIG="$(readlink -f "${gsc_dir}/chip/g/config.yaml")"
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
              gsc_dir="${SCRIPT_DIR}/../../ti50/common"
              rw_key="${gsc_dir}/ports/dauntless/signing/ti50_dev.key"
              manifest="${gsc_dir}/ports/dauntless/signing/manifest.TOT.json"
              xml="${gsc_dir}/ports/dauntless/signing/fuses.xml"
              codesigner_params+=(
                --dauntless
                --pkcs11_engine="${PKCS11_MODULE_PATH}:0:${KMS_PROJECT_PATH}/ti50-node-locked-key/cryptoKeyVersions/1"
              )
              flash_base=524288
              prefix="ti50"
              KMS_PKCS11_CONFIG="$(readlink -f "${gsc_dir}/ports/dauntless/config.yaml")"
              ;;
    (*) echo "What is ${full_bin}?" >&2
        exit 1
        ;;
  esac

  # Extract the sha from the original image version string. Find the version
  # string from the image. Use the sha from the ti50 or cr50 repo.
  rw_ver="$(strings "${full_bin}" | grep -m 1 "${prefix}_.*tpm" | \
	  sed -E "s/.*(${prefix}\S*).*/\1/")"
  sha="${rw_ver/*[-+]/}"
  # If the rw version contains a "+" then the repo was not clean when the image
  # was built. Always re-sign these images.
  if [[ "${rw_ver}" =~ \+ ]] ; then
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

  tmp_file="${TMPD}/full.bin"
  cp "${full_bin}" "${tmp_file}"

  cp "${manifest}" "${TMPD}/manifest.json"
  # Clear the board id and rollback info mask. Update the manifest to use
  # the same version as the original image.
  update_manifest "${tmp_file}" "${TMPD}/manifest.json"

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
  echo "signed image at ${output}"
}

main "$@"
