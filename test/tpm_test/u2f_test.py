# -*- coding: utf-8 -*-
# Copyright 2021 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module for testing u2f functions using extended commands."""
from binascii import a2b_hex as a2b
import hashlib
import os
import struct

import subcmd
import utils


# U2F_GENERATE:
# origin [32] | user [32] | flag | [auth_secret[32]]
def u2f_generate(tpm, origin, user, flag, auth):
    origin = origin[:32].ljust(32, b'\0')
    user = user[:32].ljust(32, b'\0')
    auth = auth[:32].ljust(32, b'\0')

    # U2F_Sign receives prehashed credentials, U2F_Generate - hashed
    auth = hashlib.sha256(auth).digest()
    cmd = origin + user + flag.to_bytes(1, 'big') + auth
    wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.U2F_GENERATE, cmd))
    response = tpm.unwrap_ext_response(subcmd.U2F_GENERATE, wrapped_response)
    public_key = response[:65]
    kh = response[65:]
    return public_key, kh

def u2f_sign(tpm, origin, user, auth, kh, msg, flag, fail=False):
    origin = origin[:32].ljust(32, b'\0')
    user = user[:32].ljust(32, b'\0')
    auth = auth[:32].ljust(32, b'\0')
    msg = msg[:32].ljust(32, b'\0')

    # determine version by size of key handle
    if (len(kh) == 64):
        cmd = origin + user + kh + msg + flag.to_bytes(1, 'big')
    else:
        cmd = origin + user + auth + msg + flag.to_bytes(1, 'big') + kh

    if fail==False:
        wrapped_response = tpm.command(tpm.wrap_ext_command(
                                       subcmd.U2F_SIGN, cmd))
        response = tpm.unwrap_ext_response(subcmd.U2F_SIGN, wrapped_response)
        sig = response[:64]
    else:
        response, size, response_code = tpm.command_unchecked(
                            tpm.wrap_ext_command(subcmd.U2F_SIGN, cmd))
        if size != 12:
             raise subcmd.TpmTestError('Unexpected response: '
                                       + utils.hex_dump(response))
        if tpm.debug_enabled():
            print('U2F sign response: ', hex(response_code))
        return b''
    return sig

def u2f_attest(tpm, origin, user, challenge, kh, public_key, corp_format=False, fail=False):
    origin = origin[:32].ljust(32, b'\0')
    user = user[:32].ljust(32, b'\0')
    if not corp_format:
        challenge = challenge[:32].ljust(32, b'\0')
        g2f_cmd = b'\0' + origin + challenge + kh + public_key
        cmd = user + b'\0' + len(g2f_cmd).to_bytes(1, 'big') + g2f_cmd
    else:
        challenge = challenge[:16].ljust(16, b'\0')
        salt = b'\0' * 65
        corp_data = challenge + public_key + salt
        corp_cmd = corp_data + origin + kh
        cmd = user + b'\1' + len(corp_cmd).to_bytes(1, 'big') + corp_cmd

    if fail==False:
        wrapped_response = tpm.command(tpm.wrap_ext_command(
                                       subcmd.U2F_ATTEST, cmd))
        response = tpm.unwrap_ext_response(subcmd.U2F_ATTEST, wrapped_response)
        sig = response[:64]
    else:
        response, size, response_code = tpm.command_unchecked(
                            tpm.wrap_ext_command(subcmd.U2F_ATTEST, cmd))
        if size != 12:
             raise subcmd.TpmTestError('Unexpected response: '
                                       + utils.hex_dump(response))
        print('response: ', hex(response_code))
        return b''
    return sig

def tpm_start(tpm):
    tpm_startup = [0x80, 0x01,         # TPM_ST_NO_SESSIONS
            0x00, 0x00, 0x00, 0x0c, # commandSize = 12
            0x00, 0x00, 0x01, 0x44, # TPM_CC_Startup
            0x00, 0x00,             # TPM_SU_CLEAR
      ]
    tpm_startup_cmd = bytes(tpm_startup)
    response = tpm.command(tpm_startup_cmd)
    return response

def g2f_get_cert(tpm):
    g2f_read = [0x80, 0x02,          # TPM_ST_SESSIONS
      0x00, 0x00, 0x00, 0x23,        # size
      0x00, 0x00, 0x01, 0x4e,        # TPM_CC_NV_READ
      0x01, 0x3f, 0xff, 0x02,        # authHandle : TPMI_RH_NV_AUTH
      0x01, 0x3f, 0xff, 0x02,        # nvIndex    : TPMI_RH_NV_INDEX
      0x00, 0x00, 0x00, 0x09,        # authorizationSize : UINT32
      0x40, 0x00, 0x00, 0x09,        # sessionHandle : empty password
      0x00, 0x00, 0x00, 0x00, 0x00,  # nonce, sessionAttributes, hmac
      0x01, 0x3b,                    # nvSize   : UINT16
      0x00, 0x00                     # nvOffset : UINT16
      ]
    g2f_read_cmd = bytes(g2f_read)
    response = tpm.command(g2f_read_cmd)
    if len(response) <= 16 or response.count(0) > 100:
         raise subcmd.TpmTestError('Unexpected G2F response: '
                                       + utils.hex_dump(response))

    print('G2F cert len', len(response))
    return response

def u2f_test(tpm):
    """Run U2F tests"""
    origin = b'1'
    user = b'2'
    auth = b'3'
    msg = b'12345'


    tpm_start(tpm)
    print('G2F read cert');
    g2f_get_cert(tpm)
    print('U2F_GENERATE v0');
    public_key0, khv0 = u2f_generate(tpm, origin, user, 0, auth)
    if tpm.debug_enabled():
        print('key_handle v0 = ',utils.hex_dump(khv0), len(khv0))
        print('public_key v0 = ',utils.hex_dump(public_key0), len(public_key0))

    print('U2F_GENERATE v1');
    public_key1, khv1 = u2f_generate(tpm, origin, user, 8, auth)
    if tpm.debug_enabled():
        print('key_handle v1 = ',utils.hex_dump(khv1), len(khv1))

    print('U2F_GENERATE v2');
    public_key2, khv2 = u2f_generate(tpm, origin, user, 24, auth)
    if tpm.debug_enabled():
        print('key_handle v2 = ',utils.hex_dump(khv2), len(khv2))

    print('U2F_SIGN v0');
    sig1 = u2f_sign(tpm, origin, user, auth, khv0, msg, 2)
    if tpm.debug_enabled():
        print('sig v0 = ',utils.hex_dump(sig1), len(sig1))

    print('U2F_SIGN v0 to fail');
    sig1 = u2f_sign(tpm, user, origin, auth, khv0, msg, 2, fail=True)
    if tpm.debug_enabled():
        print('sig v0 = ',utils.hex_dump(sig1), len(sig1))

    print('U2F_SIGN v1');
    sig1 = u2f_sign(tpm, origin, user, auth, khv1, msg, 2)
    if tpm.debug_enabled():
        print('sig v1 = ',utils.hex_dump(sig1), len(sig1))

    print('U2F_SIGN v1 to fail');
    sig1 = u2f_sign(tpm, user, origin, auth, khv1, msg, 2, fail=True)
    if tpm.debug_enabled():
        print('sig v1 = ',utils.hex_dump(sig1), len(sig1))


    print('U2F_SIGN v2');
    sig1 = u2f_sign(tpm, origin, user, auth, khv2, msg, 2)
    if tpm.debug_enabled():
        print('sig v2 = ',utils.hex_dump(sig1), len(sig1))

    print('U2F_SIGN v2 to fail');
    sig1 = u2f_sign(tpm, user, origin, auth, khv2, msg, 2, fail=True)
    if tpm.debug_enabled():
        print('sig v2 = ',utils.hex_dump(sig1), len(sig1))

    print('U2F_ATTEST v0');
    sig_attest = u2f_attest(tpm, origin, user, auth, khv0, public_key0)
    if tpm.debug_enabled():
        print('sig attest = ',utils.hex_dump(sig_attest), len(sig_attest))

    print('U2F_ATTEST corp');
    sig_attest = u2f_attest(tpm, origin, user, auth, khv0, public_key0, corp_format=True)
    if tpm.debug_enabled():
        print('sig attest = ',utils.hex_dump(sig_attest), len(sig_attest))
    print('%sSUCCESS: %s' % (utils.cursor_back(), 'U2F test'))
