# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module for testing ecc functions using extended commands."""
from binascii import a2b_hex as a2b
import hashlib
import os
import struct

import subcmd
import utils

_ECC_OPCODES = {
  'SIGN': 0x00,
  'VERIFY': 0x01,
  'KEYGEN': 0x02,
  'KEYDERIVE': 0x03,
  'TEST_POINT': 0x04,
  'VERIFY_ANY': 0x05,
  'SIGN_ANY': 0x06,
}

_ECC_CURVES = {
  'NIST-P256': 0x03,
}

# TPM2 signature codes.
_SIGN_MODE = {
  'NONE': 0x00,
  'ECDSA': 0x18
}

# TPM2 ALG codes.
_HASH = {
  'NONE': 0x10,
  'SHA1': 0x04,
  'SHA256': 0x0B
}

_HASH_FUNC = {
  'NIST-P256': hashlib.sha256
}

NIST_P256_QX = ('12c3d6a2679ca8ee3c4d927f204ed5bc'
                'b4577a04b0ac02b2a36ab3e9e10781de')
NIST_P256_QY = ('5c85ad7413971172fca5738fee9d0e7b'
                'c59ffd8a626d689bc6cca4b58665521d')

PKEY = ('fc441e07744e48f109b7e66b29482f7b'
        '7e3ec91fa27fd4870991b289fea0d20a')

#
# Field size:
# FIELD          LENGTH
# OP             1
# CURVE_ID       1
# SIGN_MODE      1
# HASHING        1
# MSG_LEN        2 (big endian)
# MSG            MSG_LEN
# R_LEN          2 (big endian)
# R              R_LEN
# S_LEN          2 (big endian)
# S              S_LEN
# DIGEST_LEN     2 (big endian)
# DIGEST         DIGEST_LEN
# D_LEN          2 (big endian)
# D              D_LEN
# QX_LEN         2 (big endian)
# QX             QX_LEN
# QY_LEN         2 (big endian)
# QY             QX_LEN
#
# Command formats:

# TEST_SIGN:
# OP | CURVE_ID | SIGN_MODE | HASHING | DIGEST_LEN | DIGEST
#    @returns 0/1 | R_LEN | R | S_LEN | S
def _sign_cmd(curve_id, hash_func, sign_mode, msg):
    digest = hash_func(msg).digest()
    return struct.pack('>BBBBH', _ECC_OPCODES['SIGN'], curve_id, sign_mode,
                      _HASH['NONE'], len(digest)) + digest

# TEST_VERIFY:
# OP | CURVE_ID | SIGN_MODE | HASHING | R_LEN | R | S_LEN | S
#   DIGEST_LEN | DIGEST
#    @returns 1 if successful
# below we assume sig = [R_LEN | R | S_LEN | S] as it came from SIGN
def _verify_cmd(curve_id, hash_func, sign_mode, msg, sig):
    digest = hash_func(msg).digest()
    return struct.pack('>BBBB', _ECC_OPCODES['VERIFY'], curve_id, sign_mode,
                      _HASH['NONE']) + sig +\
                      len(digest).to_bytes(2, 'big') + digest

# TEST_SIGN_ANY:
# OP | CURVE_ID | SIGN_MODE | HASHING | DIGEST_LEN | DIGEST | D_LEN | D
#    @returns 0/1 | R_LEN | R | S_LEN | S
def _sign_any_cmd(curve_id, hash_func, sign_mode, msg, pkey):
    digest = hash_func(msg).digest()
    return struct.pack('>BBBBH', _ECC_OPCODES['SIGN_ANY'], curve_id, sign_mode,
                      _HASH['NONE'], len(digest)) + digest +\
                        len(pkey).to_bytes(2, 'big') + pkey

# TEST_VERIFY_ANY:
# OP | CURVE_ID | SIGN_MODE | HASHING | R_LEN | R | S_LEN | S |
#   DIGEST_LEN | DIGEST | QX_LEN | QX | QY_LEN | QY
#    @returns 1 if successful
# pylint: disable=too-many-arguments
def _verify_any_cmd(curve_id, hash_func, sign_mode, msg, sig, q_x, q_y):
    digest = hash_func(msg).digest()
    return struct.pack('>BBBB', _ECC_OPCODES['VERIFY_ANY'], curve_id, sign_mode,
                      _HASH['NONE']) + sig +\
                      len(digest).to_bytes(2, 'big') + digest +\
                      len(q_x).to_bytes(2, 'big') + q_x+ \
                      len(q_y).to_bytes(2, 'big') + q_y

# TEST_POINT:
# OP | CURVE_ID | QX_LEN | QX | QY_LEN | QY
#    @returns 1 if point is on curve
def _test_point_cmd(curve_id, q_x, q_y):
    return struct.pack('>BB', _ECC_OPCODES['TEST_POINT'], curve_id) +\
          len(q_x).to_bytes(2, 'big') + q_x+ \
          len(q_y).to_bytes(2, 'big') + q_y

#
# TEST_KEYGEN:
# OP | CURVE_ID
#    @returns 0/1 | D_LEN | D | QX_LEN | QX | QY_LEN | QY
def _keygen_cmd(curve_id):
    return struct.pack('>BB', _ECC_OPCODES['KEYGEN'], curve_id)

# TEST_KEYDERIVE:
# OP | CURVE_ID | SEED_LEN | SEED
#    @returns 1 if successful
def _keyderive_cmd(curve_id, seed):
    return struct.pack('>BBH', _ECC_OPCODES['KEYDERIVE'], curve_id,
                       len(seed)) + seed

_SIGN_INPUTS = (
  ('NIST-P256', 'ECDSA'),
)


_KEYGEN_INPUTS = (
  ('NIST-P256',),
)


_KEYDERIVE_INPUTS = (
  # Curve-id, random seed size.
  ('NIST-P256', 32),
)


def _sign_test(tpm):
    msg = b'Hello CR50'

    for data in _SIGN_INPUTS:
        curve_id, sign_mode = data
        test_name = 'ECC-SIGN:%s:%s' % data
        cmd = _sign_cmd(_ECC_CURVES[curve_id], _HASH_FUNC[curve_id],
                        _SIGN_MODE[sign_mode], msg)
        wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.ECC, cmd))
        expected = b'\x01'
        signature = tpm.unwrap_ext_response(subcmd.ECC, wrapped_response)
        if signature[:1] != expected:
            raise subcmd.TpmTestError('%s error:%s:%s' % (
              test_name, utils.hex_dump(signature[:1]),
              utils.hex_dump(expected)))
        cmd = _verify_cmd(_ECC_CURVES[curve_id], _HASH_FUNC[curve_id],
                          _SIGN_MODE[sign_mode], msg, signature[1:])
        wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.ECC, cmd))
        verified = tpm.unwrap_ext_response(subcmd.ECC, wrapped_response)

        if verified[:1] != expected:
            raise subcmd.TpmTestError('%s error:%s:%s' % (
              test_name, utils.hex_dump(verified[:1]),
              utils.hex_dump(expected)))
        print('%sSUCCESS: %s' % (utils.cursor_back(), test_name))

def _sign_test_any(tpm):
    msg = b'Hello CR50'

    for data in _SIGN_INPUTS:
        curve_id, sign_mode = data
        test_name = 'ECC-SIGN, Q:%s:%s' % data
        cmd = _sign_any_cmd(_ECC_CURVES[curve_id], _HASH_FUNC[curve_id],
                            _SIGN_MODE[sign_mode], msg, a2b(PKEY))
        wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.ECC, cmd))
        expected = b'\x01'
        signature = tpm.unwrap_ext_response(subcmd.ECC, wrapped_response)
        if signature[:1] != expected:
            raise subcmd.TpmTestError('%s error:%s:%s' % (
              test_name, utils.hex_dump(signature[:1]),
              utils.hex_dump(expected)))
        # make sure properly supplied Q.x, Q.y works
        cmd = _verify_any_cmd(_ECC_CURVES[curve_id], _HASH_FUNC[curve_id],
                              _SIGN_MODE[sign_mode], msg, signature[1:],
                              a2b(NIST_P256_QX), a2b(NIST_P256_QY))
        wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.ECC, cmd))
        verified = tpm.unwrap_ext_response(subcmd.ECC, wrapped_response)
        if verified[:1] != expected:
            raise subcmd.TpmTestError('%s error:%s:%s' % (
              test_name, utils.hex_dump(verified[:1]),
              utils.hex_dump(expected)))
        print('%sSUCCESS: %s' % (utils.cursor_back(), test_name))

def _point_test(tpm):
    test_name = 'POINT-TEST: NIST-P256'
    cmd = _test_point_cmd(_ECC_CURVES['NIST-P256'],
                          a2b(NIST_P256_QX), a2b(NIST_P256_QY))
    wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.ECC, cmd))
    verified = tpm.unwrap_ext_response(subcmd.ECC, wrapped_response)
    expected = b'\x01'
    if verified != expected:
        raise subcmd.TpmTestError('%s error:%s:%s' % (
          test_name, utils.hex_dump(verified), utils.hex_dump(expected)))
    print('%sSUCCESS: %s' % (utils.cursor_back(), test_name))

def _keygen_test(tpm):
    for data in _KEYGEN_INPUTS:
        curve_id, = data
        test_name = 'ECC-KEYGEN:%s' % data
        cmd = _keygen_cmd(_ECC_CURVES[curve_id])
        wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.ECC, cmd))
        valid = tpm.unwrap_ext_response(subcmd.ECC, wrapped_response)
        expected = b'\x01'
        if valid[:1] != expected:
            raise subcmd.TpmTestError('%s error:%s:%s' % (
              test_name, utils.hex_dump(valid[:1]), utils.hex_dump(expected)))
        print('%sSUCCESS: %s' % (utils.cursor_back(), test_name))


def _keyderive_test(tpm):
    for data in _KEYDERIVE_INPUTS:
        curve_id, seed_bytes = data
        seed = os.urandom(seed_bytes)
        test_name = 'ECC-KEYDERIVE:%s' % data[0]
        cmd = _keyderive_cmd(_ECC_CURVES[curve_id], seed)
        wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.ECC, cmd))
        valid = tpm.unwrap_ext_response(subcmd.ECC, wrapped_response)
        expected = b'\x01'
        if valid != expected:
            raise subcmd.TpmTestError('%s error:%s:%s' % (
              test_name, utils.hex_dump(valid), utils.hex_dump(expected)))
        print('%sSUCCESS: %s' % (utils.cursor_back(), test_name))


def ecc_test(tpm):
    """Run ECDSA tests"""
    _sign_test(tpm)
    _sign_test_any(tpm)
    _point_test(tpm)
    _keygen_test(tpm)
    _keyderive_test(tpm)
