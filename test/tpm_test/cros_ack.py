#!/usr/bin/python
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module for reading a chip identifier, and sending back
corresponding endorsement cerificate blobs."""

from __future__ import print_function

import argparse
from collections import namedtuple
import hashlib
import os
import struct
import sys
import traceback

from tpmtest import TPM
import utils
import subcmd

# Suppressing pylint warning about an import not at the top of the file. The
# path needs to be set *before* the last import.
# pylint: disable=C6204
root_dir = os.path.dirname(os.path.abspath(sys.argv[0]))
sys.path.append(os.path.join(root_dir, '..', '..', 'build', 'tpm_test'))


CrosTPMPersoResponseHeader_v0 = namedtuple(
    'CrosTPMPersoResponseHeader_v0',
    ['tag',
     'size',
     'command_code',
     'subcommand_code'])
# TPM command header is serialized big-endian.
CrosTPMPersoResponseHeader_v0_FMT = '> 2s I 4s H'
assert struct.Struct(CrosTPMPersoResponseHeader_v0_FMT).size == 12

CrosAckResponse_v0 = namedtuple(
    'CrosAckResponse_v0',
        ['magic',
         'payload_version',
         'n_keys',
         'names',
         'filler',
         'checksum'])
# ACK response is serialized little-endian.
CrosAckResponse_v0_FMT = '< I H H 960s 12s 32s'
assert struct.Struct(CrosAckResponse_v0_FMT).size == 1012
# TODO(ngm): reduce ack size to speed up transfers.
assert struct.Struct(CrosTPMPersoResponseHeader_v0_FMT).size + \
  struct.Struct(CrosAckResponse_v0_FMT).size == 1024

CrosOkResponse_v0_FMT = '<H'
assert struct.Struct(CrosOkResponse_v0_FMT).size == 2

CrosEndorsementRSABlob_v0 = namedtuple(
  'CrosEndorsementRSABlob_v0',
  ['data'])
CrosEndorsementRSABlob_v0_FMT = '2048s'
assert struct.Struct(CrosEndorsementRSABlob_v0_FMT).size == 2048

CrosEndorsementECCBlob_v0 = namedtuple(
  'CrosEndorsementECCBlob_v0',
  ['data'])
CrosEndorsementECCBlob_v0_FMT = '2048s'
assert struct.Struct(CrosEndorsementECCBlob_v0_FMT).size == 2048

# TODO(ngm): drop the blob size.
CrosEndorsementBlob_v0 = namedtuple(
  'CrosEndorsementBlob_v0',
  ['rsa_blob',
   'ecc_blob'])
CrosEndorsementBlob_v0_FMT = ' '.join([
  CrosEndorsementRSABlob_v0_FMT, CrosEndorsementECCBlob_v0_FMT])
assert struct.Struct(CrosEndorsementBlob_v0_FMT).size ==  \
  struct.Struct(CrosEndorsementRSABlob_v0_FMT).size  + \
  struct.Struct(CrosEndorsementECCBlob_v0_FMT).size


FLAGS = None
PERSO_FILENAME_LEN = 31
TPM_HEADER_SIZE = struct.Struct(CrosTPMPersoResponseHeader_v0_FMT).size


def chip_filename(chip_id):
  # Filenames are of the form:
  #   00:2205413401000003:028039:0200 or
  #   00:2205413401000003:028039:0200.res or
  #   00_2205413401000003_028039_0200 or
  #   00_2205413401000003_028039_0200.res
  chip_id_underscores = chip_id.replace(':', '_')
  candidate_filenames = map(
    lambda x: os.path.join(FLAGS.input, x),
    [chip_id, chip_id + '.res',
     chip_id_underscores, chip_id_underscores + '.res'])

  for filename in candidate_filenames:
    if os.path.exists(filename):
      return filename

  return None


#
#  This method delivers endorsement certificates to
#  the chip based on the following flow.
#
#  +----------+               +------+
#  | CROS_ACK |               | CHIP |
#  +----------+               +------+
#
#              +------------->
#                ACK REQUEST
#
#              <-------------+
#                ACK RESPONSE
#
#              +------------->
#                  RSA BLOB
#
#              <-------------+
#                 OK RESPONSE
#
#              +------------->
#                  ECC BLOB
#
#              <-------------+
#                OK RESPONSE
#
def cros_ack():
  t = TPM(debug_mode=FLAGS.debug,
          # Skip sending the TPM2_Startup() command.
          try_startup=False)
  wrapped_response = t.command(t.wrap_ext_command(subcmd.MANUFACTURE_ACK, ''))
  if len(wrapped_response) <= TPM_HEADER_SIZE:
    print('Error: manufacture not supported')
    return False

  # Skip over TPM header and unpack Ack response.
  ack_body = wrapped_response[TPM_HEADER_SIZE:]
  if len(ack_body) == 2:
    print('Error: %d' % struct.unpack('<H', ack_body))
    return False

  ack = CrosAckResponse_v0._make(struct.unpack(
    CrosAckResponse_v0_FMT, ack_body))

  expected_checksum = hashlib.sha256(
    ack_body[:len(ack_body) - hashlib.sha256().digest_size]).digest()
  if expected_checksum != ack.checksum:
    print('Error: ACK checksum mismatch')
    return False

  chip_id = ack.names[:PERSO_FILENAME_LEN]

  perso_data = ''
  chip_perso_data_file = chip_filename(chip_id)
  if not chip_perso_data_file:
    print('Error: data file for chip id: %s not found' % chip_id)
    return False
  with open(chip_perso_data_file) as f:
    perso_data = f.read()
  print(utils.cursor_back() + 'CHIP ID: %s' % chip_id)

  assert len(perso_data) == struct.Struct(CrosEndorsementBlob_v0_FMT).size
  blob = CrosEndorsementBlob_v0._make(
    struct.Struct(CrosEndorsementBlob_v0_FMT).unpack(perso_data))

  # Blobs include serialized TPM header.
  response = t.command(blob.rsa_blob)
  # Skip over TPM header and unpack Ok response.
  rsa_ok = struct.unpack(CrosOkResponse_v0_FMT, response[TPM_HEADER_SIZE:])[0]
  print(utils.cursor_back() + 'RSA PERSO RESULT: %d' % rsa_ok)

  # Blobs include serialized TPM header.
  response = t.command(blob.ecc_blob)
  # Skip over TPM header and unpack Ok response.
  ecc_ok = struct.unpack(CrosOkResponse_v0_FMT, response[TPM_HEADER_SIZE:])[0]
  print(utils.cursor_back() + 'ECC PERSO RESULT: %d' % ecc_ok)

  if rsa_ok or ecc_ok:
    print(utils.cursor_back() + 'FAIL')
    return False
  else:
    print(utils.cursor_back() + 'SUCCESS')
    return True


def parse_args():
  parser = argparse.ArgumentParser(
    description='A script for injecting endorsement certificates')
  parser.add_argument(
    '-i', '--input', type=str,
    help='Path to the directory containing endorsement blobs',
    required=True)
  parser.add_argument(
    '-d', '--debug',
    help='If specified, then print debug information',
    action='store_true')

  global FLAGS
  FLAGS = parser.parse_args()

  if not os.path.isdir(FLAGS.input):
    print('Error: --input should point to a directory')
    sys.exit(1)


if __name__ == '__main__':
  parse_args()

  try:
    success = cros_ack()
    if success:
      sys.exit(0)
    else:
      sys.exit(1)
  except subcmd.TpmTestError as e:
    exc_file, exc_line = traceback.extract_tb(sys.exc_traceback)[-1][:2]
    print('\nError in %s:%s: ' % (os.path.basename(exc_file), exc_line), e)
    if FLAGS.debug:
      traceback.print_exc()
    sys.exit(1)
