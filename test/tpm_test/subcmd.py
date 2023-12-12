# -*- coding: utf-8 -*-
# Copyright 2015 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Subcommand codes that specify the crypto module."""

# Keep these codes in sync with include/tpm_vendor_cmds.h
AES = 0
HASH = 1
RSA = 2
ECC = 3
FW_UPGRADE = 4
HKDF = 5
ECIES = 6
U2F_GENERATE = 44
U2F_SIGN = 45
U2F_ATTEST = 46
DRBG_TEST = 50
FIPS_CMD = 55
# The same exception class used by all tpmtest modules.
class TpmTestError(Exception):
    """TpmTestError exception class"""
