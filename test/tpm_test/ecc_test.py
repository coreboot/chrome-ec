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


# tuples of (d = pkey, x, y)
P256_POINTS = (
  ('fc441e07744e48f109b7e66b29482f7b7e3ec91fa27fd4870991b289fea0d20a',
   '12c3d6a2679ca8ee3c4d927f204ed5bcb4577a04b0ac02b2a36ab3e9e10781de',
   '5c85ad7413971172fca5738fee9d0e7bc59ffd8a626d689bc6cca4b58665521d'),
  ('0d7841823ef146c3fc533397ce349e7423c9c0fc6d2bd5864cb8c3c4a28387b3',
   '108072a654ffc96d948e4593d632055cd7dee6a1d9d9377c1a1a642565a51cb0',
   '0716a5bbc736ffb97ef78d612055bb7187ed90bce82918ea07a4eb7ecd572ea7'),
  ('00fda48d4e3e1fe4afa320bfed836b4cdccccecf6fdbdb05b986801a5654cc09',
   '9a38b6ca7263068fe65ce570a6625263179223ab177f502f204c2c7ac4d8586e',
   'f1b1a398c21c02c7fe4a8252952005d0a7686ca3cfe05501f9879be160503562'),
  ('b72d249ee3610c9119dfb4a2d6925172d4b162b70c3c850256e70565df384e2f',
   '0011bf6d971e1c1f7d87056ca978e02cc860bb0ba05b86039f84d6902d04e66e',
   '5077a07410161fb449009baf0c010077254543fceb062d02c72349dfebeedd48'),
  ('b72d249ee3610c9119dfb4a2d6925172d4b162b70c3c850256e70565df384e2f',
   '0011bf6d971e1c1f7d87056ca978e02cc860bb0ba05b86039f84d6902d04e66e',
   '5077a07410161fb449009baf0c010077254543fceb062d02c72349dfebeedd48'),
  ('15fcbfecee4c9f4a769d74226e8ffe86d4deb5e35d704d2bd465c02051c7d3d6',
   '42b2006a77f9687c0a2ae6ba2ad114b27fb53cfdc1dd49ebcd9e398a3cd6e12b',
   '0091ee137d5946416f7cab8f825f8537af38a57713f19e3555fd0e9a16935fad'),
  ('27a1fc7e676ec9a598f75259c5030bcc95ad7bd2a91d4d8cffa5f390f33cef00',
   'ab99caa4908e426554e86f71086d14ac99ded8ed23cea72f9bece8062d2d3d8f',
   'ee5524ccb2872c58d6d22295efff9d091c5e52866e5c494df1d14509bcab1e00'),
# points with zero bytes removed:
  ('fda48d4e3e1fe4afa320bfed836b4cdccccecf6fdbdb05b986801a5654cc09',
   '9a38b6ca7263068fe65ce570a6625263179223ab177f502f204c2c7ac4d8586e',
   'f1b1a398c21c02c7fe4a8252952005d0a7686ca3cfe05501f9879be160503562'),
  ('b72d249ee3610c9119dfb4a2d6925172d4b162b70c3c850256e70565df384e2f',
   '11bf6d971e1c1f7d87056ca978e02cc860bb0ba05b86039f84d6902d04e66e',
   '5077a07410161fb449009baf0c010077254543fceb062d02c72349dfebeedd48'),
  ('15fcbfecee4c9f4a769d74226e8ffe86d4deb5e35d704d2bd465c02051c7d3d6',
   '42b2006a77f9687c0a2ae6ba2ad114b27fb53cfdc1dd49ebcd9e398a3cd6e12b',
   '91ee137d5946416f7cab8f825f8537af38a57713f19e3555fd0e9a16935fad')
   )

def _sign_test_any(tpm):
    msg = b'Hello CR50'
    curve_id = 'NIST-P256'
    sign_mode = 'ECDSA'
    counter = 1
    for data in P256_POINTS:
        d, x, y = data
        test_name = 'ECC-SIGN, Q:%s:%s %d' % (curve_id, sign_mode, counter)
        cmd = _sign_any_cmd(_ECC_CURVES[curve_id], _HASH_FUNC[curve_id],
                            _SIGN_MODE[sign_mode], msg, a2b(d))
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
                              a2b(x), a2b(y))
        wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.ECC, cmd))
        verified = tpm.unwrap_ext_response(subcmd.ECC, wrapped_response)
        if verified[:1] != expected:
            raise subcmd.TpmTestError('%s error:%s:%s' % (
              test_name, utils.hex_dump(verified[:1]),
              utils.hex_dump(expected)))
        print('%sSUCCESS: %s' % (utils.cursor_back(), test_name))
        counter += 1

def _point_test(tpm):
    curve_id = 'NIST-P256'
    counter = 1
    for data in P256_POINTS:
        test_name = 'POINT-TEST, Q:%s: %d' % (curve_id, counter)
        _, x, y = data
        cmd = _test_point_cmd(_ECC_CURVES[curve_id], a2b(x), a2b(y))
        wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.ECC, cmd))
        verified = tpm.unwrap_ext_response(subcmd.ECC, wrapped_response)
        expected = b'\x01'
        if verified != expected:
            raise subcmd.TpmTestError('%s error:%s:%s' % (
              test_name, utils.hex_dump(verified), utils.hex_dump(expected)))
        print('%sSUCCESS: %s' % (utils.cursor_back(), test_name))
        counter += 1

def _keygen_test(tpm):
    for data in _KEYGEN_INPUTS:
        curve_id, = data
        test_name = 'ECC-KEYGEN:%s' % data
        cmd = _keygen_cmd(_ECC_CURVES[curve_id])
        counter = 1
        while counter < 100:
            wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.ECC,
                                                               cmd))
            valid = tpm.unwrap_ext_response(subcmd.ECC, wrapped_response)
            expected = b'\x01'
            if valid[:1] != expected:
                raise subcmd.TpmTestError('%s error:%s:%s' % (
                    test_name, utils.hex_dump(valid[:1]),
                    utils.hex_dump(expected)))
            counter += 1
            # print keys where x or y starts with zero
            if valid[37:38] == b'\x00' or valid[71:72] == b'\x00':
                print('d=', valid[3:35].hex())
                print('x=', valid[37:69].hex())
                print('y=', valid[71:103].hex())

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
