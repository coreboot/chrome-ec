#!/bin/bash
# Copyright 2019 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# NIST toolset needs sudo emerge dev-libs/libdivsufsort and bz2
set -e
TMP_PATH="/tmp/ea"
NIST_URL="https://github.com/usnistgov/SP800-90B_EntropyAssessment.git"
TRNG_OUT="${TMP_PATH}/trng_output"
EA_LOG="ea_non_iid.log"
rm -rf "${TMP_PATH}"
git clone --depth 1 "${NIST_URL}" "${TMP_PATH}"
# build entropy assessment tool using mode for non independent and identically
# distributed data, as for  H1 TRNG we can't justify the oppposite
make -j -C "${TMP_PATH}/cpp/" non_iid restart
rm -f "${TRNG_OUT}"
# -t0 requests raw random samples from TRNG
./tpmtest.py -t0 -o "${TRNG_OUT}"
if [[ ! -f "${TRNG_OUT}" ]]; then
    echo "${TRNG_OUT} does not exist"
    exit 1
fi
rm -f "${EA_LOG}"
"${TMP_PATH}/cpp/ea_non_iid" -a "${TRNG_OUT}" | tee "${EA_LOG}"
entropy="$(awk '/min/ {print $5}' "${EA_LOG}")"
if [[ -z "${entropy}" ]]; then
    entropy="$(awk '/H_original/ {print $2}' "${EA_LOG}")"
fi
echo "Minimal entropy ${entropy}"
"${TMP_PATH}/cpp/ea_restart" "${TRNG_OUT}" "${entropy}" | tee -a "${EA_LOG}"
