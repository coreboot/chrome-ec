#!/usr/bin/python
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
  'ECDSA': 0x18,
  # TODO(ngm): add support for SCHNORR.
  # 'SCHNORR': 0x1c
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

##
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
#
# TEST_SIGN:
# OP | CURVE_ID | SIGN_MODE | HASHING | DIGEST_LEN | DIGEST
#    @returns 0/1 | R_LEN | R | S_LEN | S
_TEST_SIGN = '{o:c}{c:c}{s:c}{h:c}{dl:s}{dig}'

# TEST_SIGN_ANY:
# OP | CURVE_ID | SIGN_MODE | HASHING | DIGEST_LEN | DIGEST | D_LEN | D
#    @returns 0/1 | R_LEN | R | S_LEN | S
_TEST_SIGN_ANY = '{o:c}{c:c}{s:c}{h:c}{dl:s}{dig}{pl:s}{pk}'

#
# TEST_VERIFY:
# OP | CURVE_ID | SIGN_MODE | HASHING | R_LEN | R | S_LEN | S
#   DIGEST_LEN | DIGEST
#    @returns 1 if successful
_TEST_VERIFY = '{o:c}{c:c}{sm:c}{h:c}{rs}{dl:s}{dig}'

# TEST_VERIFY_ANY:
# OP | CURVE_ID | SIGN_MODE | HASHING | R_LEN | R | S_LEN | S |
#   DIGEST_LEN | DIGEST | QX_LEN | QX | QY_LEN | QY
#    @returns 1 if successful
_TEST_VERIFY_ANY = _TEST_VERIFY + '{qxl:s}{qx}{qyl:s}{qy}'

# TEST_KEYDERIVE:
# OP | CURVE_ID | SEED_LEN | SEED
#    @returns 1 if successful
#
_TEST_KEYDERIVE = '{o:c}{c:c}{ml:s}{msg}'

# TEST_POINT:
# OP | CURVE_ID | QX_LEN | QX | QY_LEN | QY
#    @returns 1 if point is on curve

_TEST_POINT = '{o:c}{c:c}{qxl:s}{qx}{qyl:s}{qy}'

#
# TEST_KEYGEN:
# OP | CURVE_ID
#    @returns 0/1 | D_LEN | D | QX_LEN | QX | QY_LEN | QY
#
_TEST_KEYGEN = '{o:c}{c:c}'

def _sign_cmd(curve_id, hash_func, sign_mode, msg):
  op = _ECC_OPCODES['SIGN']
  digest = hash_func(msg).digest()
  digest_len = len(digest)
  return _TEST_SIGN.format(o=op, c=curve_id, s=sign_mode, h=_HASH['SHA256'],
                           dl=struct.pack('>H', digest_len), dig=digest)

def _sign_any_cmd(curve_id, hash_func, sign_mode, msg, pkey):
  op = _ECC_OPCODES['SIGN_ANY']
  digest = hash_func(msg).digest()
  digest_len = len(digest)
  return _TEST_SIGN_ANY.format(o=op, c=curve_id, s=sign_mode,
                               h=_HASH['SHA256'],
                               dl=struct.pack('>H', digest_len), dig=digest,
                               pl=struct.pack('>H', len(pkey)), pk=pkey)


def _verify_cmd(curve_id, hash_func, sign_mode, msg, sig):
  op = _ECC_OPCODES['VERIFY']
  digest = hash_func(msg).digest()
  digest_len = len(digest)
  return _TEST_VERIFY.format(o=op, c=curve_id, sm=sign_mode, h=_HASH['SHA256'],
                             rs=sig,
                             dl=struct.pack('>H', digest_len),
                             dig=digest)


def _verify_any_cmd(curve_id, hash_func, sign_mode, msg, sig, qx, qy):
  op = _ECC_OPCODES['VERIFY_ANY']
  digest = hash_func(msg).digest()
  digest_len = len(digest)
  return _TEST_VERIFY_ANY.format(o=op, c=curve_id, sm=sign_mode,
                                 h=_HASH['SHA256'],
                                 rs=sig,
                                 dl=struct.pack('>H', digest_len), dig=digest,
                                 qxl=struct.pack('>H', len(qx)), qx=qx,
                                 qyl=struct.pack('>H', len(qy)), qy=qy)

def _test_point_cmd(curve_id, qx, qy):
  op = _ECC_OPCODES['TEST_POINT']
  return _TEST_POINT.format(o=op, c=curve_id,
                               qxl=struct.pack('>H', len(qx)), qx=qx,
                               qyl=struct.pack('>H', len(qy)), qy=qy)

def _keygen_cmd(curve_id):
  op = _ECC_OPCODES['KEYGEN']
  return _TEST_KEYGEN.format(o=op, c=curve_id)


def _keyderive_cmd(curve_id, seed):
  op = _ECC_OPCODES['KEYDERIVE']
  seed_len = len(seed)
  return _TEST_KEYDERIVE.format(o=op, c=curve_id,
                                ml=struct.pack('>H', seed_len), msg=seed)


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
  msg = 'Hello CR50'

  for data in _SIGN_INPUTS:
    curve_id, sign_mode = data
    test_name = 'ECC-SIGN:%s:%s' % data
    cmd = _sign_cmd(_ECC_CURVES[curve_id], _HASH_FUNC[curve_id],
                    _SIGN_MODE[sign_mode], msg)
    wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.ECC, cmd))
    expected = '\x01'
    signature = tpm.unwrap_ext_response(subcmd.ECC, wrapped_response)
    if signature[:1] != expected:
      raise subcmd.TpmTestError('%s error:%s:%s' % (
        test_name, utils.hex_dump(signature[:1]), utils.hex_dump(expected)))
    cmd = _verify_cmd(_ECC_CURVES[curve_id], _HASH_FUNC[curve_id],
                      _SIGN_MODE[sign_mode], msg, signature[1:])
    wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.ECC, cmd))
    verified = tpm.unwrap_ext_response(subcmd.ECC, wrapped_response)

    if verified[:1] != expected:
      raise subcmd.TpmTestError('%s error:%s:%s' % (
        test_name, utils.hex_dump(verified[:1]), utils.hex_dump(expected)))
    print('%sSUCCESS: %s' % (utils.cursor_back(), test_name))

def _sign_test_any(tpm):
  msg = 'Hello CR50'

  for data in _SIGN_INPUTS:
    curve_id, sign_mode = data
    test_name = 'ECC-SIGN, Q:%s:%s' % data
    cmd = _sign_any_cmd(_ECC_CURVES[curve_id], _HASH_FUNC[curve_id],
                    _SIGN_MODE[sign_mode], msg, a2b(PKEY))
    wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.ECC, cmd))
    expected = '\x01'
    signature = tpm.unwrap_ext_response(subcmd.ECC, wrapped_response)
    if signature[:1] != expected:
      raise subcmd.TpmTestError('%s error:%s:%s' % (
        test_name, utils.hex_dump(signature[:1]), utils.hex_dump(expected)))
    # make sure properly supplied Q.x, Q.y works
    cmd = _verify_any_cmd(_ECC_CURVES[curve_id], _HASH_FUNC[curve_id],
                      _SIGN_MODE[sign_mode], msg, signature[1:],
                      a2b(NIST_P256_QX), a2b(NIST_P256_QY))
    wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.ECC, cmd))
    verified = tpm.unwrap_ext_response(subcmd.ECC, wrapped_response)
    if verified[:1] != expected:
      raise subcmd.TpmTestError('%s error:%s:%s' % (
        test_name, utils.hex_dump(verified[:1]), utils.hex_dump(expected)))
    print('%sSUCCESS: %s' % (utils.cursor_back(), test_name))

def _point_test(tpm):
  test_name = 'POINT-TEST: NIST-P256'
  cmd = _test_point_cmd(_ECC_CURVES['NIST-P256'],
                         a2b(NIST_P256_QX), a2b(NIST_P256_QY))
  wrapped_response = tpm.command(tpm.wrap_ext_command(subcmd.ECC, cmd))
  verified = tpm.unwrap_ext_response(subcmd.ECC, wrapped_response)
  expected = '\x01'
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
    expected = '\x01'
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
    expected = '\x01'
    if valid != expected:
      raise subcmd.TpmTestError('%s error:%s:%s' % (
        test_name, utils.hex_dump(valid), utils.hex_dump(expected)))
    print('%sSUCCESS: %s' % (utils.cursor_back(), test_name))


def ecc_test(tpm):
  _sign_test(tpm)
  _sign_test_any(tpm)
  _point_test(tpm)
  _keygen_test(tpm)
  _keyderive_test(tpm)
