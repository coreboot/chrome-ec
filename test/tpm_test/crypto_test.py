# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module for testing cryptography functions using extended commands."""

from __future__ import print_function

import struct
import xml.etree.ElementTree as ET

import subcmd
import utils

# Basic crypto operations
DECRYPT = 0
ENCRYPT = 1

def get_attribute(tdesc, attr_name, required=True):
    """Retrieve an attribute value from an XML node.

    Args:
        tdesc: an Element of the ElementTree, a test descriptor containing
               necessary information to run a single encryption/description
               session.
        attr_name: a string, the name of the attribute to retrieve.
        required: a Boolean, if True - the attribute must be present in the
                descriptor, otherwise it is considered optional

    Returns:
        The attribute value as a string (ascii or binary)

    Raises:
        subcmd.TpmTestError: on various format errors, or in case a required
             attribute is not found, the error message describes the problem.
    """
    # Fields stored in hex format by default.
    default_hex = ('aad', 'cipher_text', 'iv', 'key', 'tag')

    data = tdesc.find(attr_name)
    if data is None:
        if required:
            raise subcmd.TpmTestError('node "%s" does not have attribute "%s"' %
                                      (tdesc.get('name'), attr_name))
        return ''

    # Attribute is present, does it have to be decoded from hex?
    cell_format = data.get('format')
    if not cell_format:
        if attr_name in default_hex:
            cell_format = 'hex'
        else:
            cell_format = 'ascii'
    elif cell_format not in ('hex', 'ascii'):
        raise subcmd.TpmTestError('%s:%s, unrecognizable format "%s"' %
                                  (tdesc.get('name'), attr_name, cell_format))

    text = ' '.join(x.strip() for x in data.text.splitlines() if x)
    if cell_format == 'ascii':
        return text

    # Drop spaces from hex representation.
    text = text.replace(' ', '')

    # Convert hex-text to little-endian binary (in 4-byte word chunks)
    value = ''
    for block in range(len(text)/8):
        try:
            value += struct.pack('<I', int('0x%s' % text[8*block:8*(block+1)],
                                           16))
        except ValueError:
            raise subcmd.TpmTestError('%s:%s %swrong hex value' %
                                      (tdesc.get('name'), attr_name,
                                       utils.hex_dump(text)))

    # Unpack remaining hex text, without introducing a zero pad.
    for block in range(-1, -(len(text) % 8), -1):
        value += chr(int(text[2*block:len(text) + (2*block)+2], 16))

    return value

SUPPORTED_MODES = {
  'AES': (subcmd.AES, {
    'ECB': 0,
    'CTR': 1,
    'CBC': 2,
    'GCM': 3,
    'OFB': 4,
    'CFB': 5
  }),
}

# pylint: disable=too-many-arguments, too-many-locals
def crypto_run(node_name, op_type, key, init_vec, aad, in_text, out_text, tpm):
    """Perform a basic operation(encrypt or decrypt).

    This function creates an extended command with the requested parameters,
    sends it to the device, and then compares the response to the expected
    value.

    Args:
        node_name: a string, the name of the XML node this data comes from. The
        format of the name is "<enc type>:<submode> ....", where <enc type> is
        the major encryption mode (say AED or DES) and submode - a variant of
        the major scheme, if exists.
        op_type: an int, encodes the operation to perform (encrypt/decrypt),
        passed directly to the device as a field in the extended command
        key: a binary string
        init_vec: a binary string, might be empty
        aad: additional authenticated data
        in_text: a binary string, the input of the encrypt/decrypt operation
        out_text: a binary string, might be empty, the expected output of the
        operation. Note that it could be shorter than actual output (padded to
        integer number of blocks), in which case only its length of bytes is
        compared debug_mode: a Boolean, if True - enables tracing on the console
        tpm: a TPM object to send extended commands to an initialized TPM

    Returns:
        The actual binary string, result of the operation, if the
        comparison with the expected value was successful.

    Raises:
        subcmd.TpmTestError: in case there were problems parsing the node name,
        or verifying the operation results.
    """
    mode_name, submode_name = node_name.split(':')
    submode_name = submode_name[:3].upper()

    # commands below will raise exception if incorrect mode/submode used
    try:
        mode_cmd, submodes = SUPPORTED_MODES[mode_name.upper()]
        submode = submodes[submode_name]
    except KeyError:
        raise subcmd.TpmTestError('unrecognizable mode in node "%s"' %
                                  node_name)

    cmd = '%c' % op_type    # Encrypt or decrypt
    cmd += '%c' % submode   # A particular type of a generic algorithm.
    cmd += '%c' % len(key)
    cmd += key
    cmd += '%c' % len(init_vec)
    if init_vec:
        cmd += init_vec
    cmd += '%c' % len(aad)
    if aad:
        cmd += aad
    cmd += struct.pack('>H', len(in_text))
    cmd += in_text
    if tpm.debug_enabled():
        print('%d:%d cmd size' % (op_type, mode_cmd),
              len(cmd), utils.hex_dump(cmd))
    wrapped_response = tpm.command(tpm.wrap_ext_command(mode_cmd, cmd))
    real_out_text = tpm.unwrap_ext_response(mode_cmd, wrapped_response)
    if out_text:
        if len(real_out_text) > len(out_text):
            real_out_text = real_out_text[:len(out_text)]  # Ignore padding
        if real_out_text != out_text:
            if tpm.debug_enabled():
                print('Out text mismatch in node %s:\n' % node_name)
            else:
                raise subcmd.TpmTestError(
                  'Out text mismatch in node %s, operation %s:\n'
                  'In text:%sExpected out text:%sReal out text:%s' % (
                    node_name, 'ENCRYPT' if op_type == ENCRYPT else 'DECRYPT',
                    utils.hex_dump(in_text),
                    utils.hex_dump(out_text),
                    utils.hex_dump(real_out_text)))
    return real_out_text


def crypto_test(tdesc, tpm):
    """Perform a single test described in the xml file.

    The xml node contains all pertinent information about the test inputs and
    outputs.

    Args:
        tdesc: an Element of the ElementTree, a test descriptor containing
        necessary information to run a single encryption/description session.
        tpm: a TPM object to send extended commands to an initialized TPM

    Raises:
        subcmd.TpmTestError: on various execution errors, the details are
        included in the error message.
    """
    node_name = tdesc.get('name')
    key = get_attribute(tdesc, 'key')
    if len(key) not in (16, 24, 32):
        raise subcmd.TpmTestError('wrong key size "%s:%s"' % (
          node_name,
          ''.join('%2.2x' % ord(x) for x in key)))
    init_vec = get_attribute(tdesc, 'iv', required=False)
    if init_vec and not node_name.startswith('AES:GCM') and len(init_vec) != 16:
        raise subcmd.TpmTestError('wrong iv size "%s:%s"' % (
          node_name,
          ''.join('%2.2x' % ord(x) for x in init_vec)))
    clear_text = get_attribute(tdesc, 'clear_text', required=False)
    if clear_text:
        clear_text_len = get_attribute(tdesc, 'clear_text_len', required=False)
        if clear_text_len:
            clear_text = clear_text[:int(clear_text_len)]
    else:
        clear_text_len = None
    if tpm.debug_enabled():
        print('clear text size', len(clear_text))
    cipher_text = get_attribute(tdesc, 'cipher_text', required=False)
    if clear_text_len:
        cipher_text = cipher_text[:int(clear_text_len)]
    tag = get_attribute(tdesc, 'tag', required=False)
    aad = get_attribute(tdesc, 'aad', required=False)
    if aad:
        aad_len = get_attribute(tdesc, 'aad_len', required=False)
        if aad_len:
            aad = aad[:int(aad_len)]
    real_cipher_text = crypto_run(node_name, ENCRYPT, key, init_vec,
                                  aad or '', clear_text, cipher_text + tag, tpm)
    crypto_run(node_name, DECRYPT, key, init_vec, aad or '',
               real_cipher_text[:len(real_cipher_text) - len(tag)],
               clear_text + tag, tpm)
    print(utils.cursor_back() + 'SUCCESS: %s' % node_name)

def crypto_tests(tpm, xml_file):
    """Run AES cryptographic tests"""
    tree = ET.parse(xml_file)
    root = tree.getroot()
    for child in root:
        crypto_test(child, tpm)
