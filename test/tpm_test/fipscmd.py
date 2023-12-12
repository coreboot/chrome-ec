#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2023 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module for testing u2f functions using extended commands."""
from binascii import a2b_hex as a2b
import subcmd
import utils

FIPS_CMD_GET_STATUS = 0
FIPS_CMD_ON = 1
FIPS_CMD_TEST = 2
FIPS_CMD_U2F_STATUS = 13

def fips_get_status(tpm):
    cmd = FIPS_CMD_GET_STATUS.to_bytes(1, 'big')
    wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.FIPS_CMD, cmd))
    response = tpm.unwrap_ext_response(subcmd.FIPS_CMD, wrapped_response)
    mode = response[:4]
    return int.from_bytes(mode, "big", signed=False)

def fips_test(tpm):
    cmd = FIPS_CMD_TEST.to_bytes(1, 'big')
    wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.FIPS_CMD, cmd))
    response = tpm.unwrap_ext_response(subcmd.FIPS_CMD, wrapped_response)
    mode = response[:4]
    return int.from_bytes(mode, "big", signed=False)

def fips_u2f_status(tpm):
    cmd = FIPS_CMD_U2F_STATUS.to_bytes(1, 'big')
    wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.FIPS_CMD, cmd))
    response = tpm.unwrap_ext_response(subcmd.FIPS_CMD, wrapped_response)
    mode = response[:1]
    return int.from_bytes(mode, "big", signed=False)

def fips_u2f_update(tpm):
    cmd = FIPS_CMD_ON.to_bytes(1, 'big')
    wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.FIPS_CMD, cmd))
    tpm.unwrap_ext_response(subcmd.FIPS_CMD, wrapped_response)
    return


def tpm_start(tpm):
    tpm_startup = [0x80, 0x01,         # TPM_ST_NO_SESSIONS
            0x00, 0x00, 0x00, 0x0c, # commandSize = 12
            0x00, 0x00, 0x01, 0x44, # TPM_CC_Startup
            0x00, 0x00,             # TPM_SU_CLEAR
      ]
    tpm_startup_cmd = bytes(tpm_startup)
    response = tpm.command(tpm_startup_cmd)
    return response

def fips_cmd_test(tpm):
    """Run FIPS CMD tests"""

    tpm_start(tpm)
    print('FIPS cmd test');
    mode = fips_get_status(tpm)
    print(f'FIPS mode: {mode:08x}')
    mode = fips_test(tpm)
    print(f'FIPS mode: {mode:08x}')
    u2f_status = fips_u2f_status(tpm)
    print(f'U2F keys mode: {u2f_status:02x}')
    print('Force update to FIPS')
    fips_u2f_update(tpm)
    u2f_status = fips_u2f_status(tpm)
    print(f'U2F keys mode: {u2f_status:02x}')
    print('%sSUCCESS: %s' % (utils.cursor_back(), 'FIPS_CMD test'))
