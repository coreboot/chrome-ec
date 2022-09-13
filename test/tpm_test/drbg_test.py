# -*- coding: utf-8 -*-
# Copyright 2019 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module for testing hash functions using extended commands."""

from __future__ import print_function

from binascii import a2b_hex as a2b

import lab_vectors
import subcmd
import utils

# A standard empty response to DRBG extended commands.
EMPTY_DRBG_RESPONSE = bytes([0x80, 0x01,
                             0x00, 0x00, 0x00, 0x0c,
                             0x00, 0x00, 0x00, 0x00,
                             0x00, subcmd.DRBG_TEST])

DRBG_INIT = 0
DRBG_RESEED = 1
DRBG_GENERATE = 2
DRBG_GROUP_INIT = 3

TEST_INPUTS = (
    (DRBG_GROUP_INIT, 32),
    (DRBG_INIT,
     ('C40894D0C37712140924115BF8A3110C7258532365BB598F81B127A5E4CB8EB0',
      'FBB1EDAF92D0C2699F5C0A7418D308B09AC679FFBB0D8918C8E62D35091DD2B9',
      '2B18535D739F7E75AF4FF0C0C713DD4C9B0A6803D2E0DB2BDE3C4F3650ABF750')),
    (DRBG_RESEED,
     ('4D58A621857706450338CCA8A1AF5CD2BD9305F3475CF1A8752518DD8E8267B6',
      '0153A0A1D7487E2EE9915E2CAA8488F97239C67595F418D9503D0B11CC07044E', '')),
    (DRBG_GENERATE,
     ('39AE66C2939D1D73EF21AE22988B04CC7E8EA2D790C75E1FC6ACC7FEEEF90F98',
      '',
      False)),
    (DRBG_GENERATE,
     ('B8031829E07B09EEEADEBA149D0AC9F08B110197CD8BBDDC32744BCD66FCF3C4',
      'A1307377F6B472661BC3C6D44C035FB20A13CCB04D6601B2425FC4DDA3B6D7DF',
      True)),
    (DRBG_INIT,
     ('3A2D261884010CCB4C2C4D7B323CCB7BD4515089BEB749C565A7492710922164',
      '9E4D22471A4546F516099DD4D737967562D1BB77D774B67B7FE4ED893AE336CF',
      '5837CAA74345CC2D316555EF820E9F3B0FD454D8C5B7BDE68E4A176D52EE7D1C')),
    (DRBG_GENERATE,
     ('4D87985505D779F1AD98455E04199FE8F2FE8E550E6FEB1D26177A2C5B744B9F',
      '',
      False)),
    (DRBG_GENERATE,
     ('85D011A3B36AC6B25A792F213A1C22C80BFD1C5B47BCA04CD0D9834BB466447B',
      'B03863C42C9396B4936D83A551871A424C5A8EDBDC9D1E0E8E89710D58B5CA1E',
      True)),
)

# DRBG_TEST command structure:
#
# field       |    size  |              note
# ==========================================================================
# mode        |    1     | 0 - DRBG_INIT, 1 - DRBG_RESEED, 2 - DRBG_GENERATE
# p0_len      |    2     | size of first input in bytes
# p0          |  p0_len  | entropy for INIT & SEED, input for GENERATE
# p1_len      |    2     | size of second input in bytes (for INIT & RESEED)
#             |          | or size of expected output for GENERATE
# p1          |  p1_len  | nonce for INIT & SEED
# p2_len      |    2     | size of third input in bytes for DRBG_INIT
# p2          |  p2_len  | personalization for INIT & SEED
#
# DRBG_INIT (entropy, nonce, perso)
# DRBG_RESEED (entropy, additional input 1, additional input 2)
# DRBG_INIT and DRBG_RESEED returns empty response
# (up to a maximum of 128 bytes)
# DRBG_INIT and DRBG_RESEED commands follow same format
def _drbg_init_cmd(drbg_op, entropy, nonce, perso):
    return drbg_op.to_bytes(1, 'big') +\
          len(entropy).to_bytes(2, 'big') + entropy +\
          len(nonce).to_bytes(2, 'big') + nonce +\
          len(perso).to_bytes(2, 'big') + perso

# DRBG_GENERATE (p0_len, p0 - additional input 1, p1_len - size of output)
# DRBG_GENERATE returns p1_len bytes of generated data
def _drbg_gen_cmd(inp, outlen):
    return DRBG_GENERATE.to_bytes(1, 'big') +\
          len(inp).to_bytes(2, 'big') + inp +\
          outlen.to_bytes(2, 'big')


def drbg_init(tpm, drbg_params):
    """Run the drbg reseed command with the given drbg params.

    Args:
        tpm: a tpm object used to communicate with the device
        drbg_params: a tuple (entropy string, nonce string, perso string)

    Raises:
        subcmd.TpmTestError: on unexpected target responses
    """
    entropy, nonce, perso = drbg_params
    cmd = _drbg_init_cmd(DRBG_INIT, a2b(entropy), a2b(nonce), a2b(perso))
    response = tpm.command(tpm.wrap_ext_command(subcmd.DRBG_TEST, cmd))
    if response != EMPTY_DRBG_RESPONSE:
        raise subcmd.TpmTestError('Unexpected response to '
                                  'DRBG_INIT: %s' %
                                  (utils.hex_dump(response)))

def drbg_reseed(tpm, drbg_params):
    """Run the drbg reseed command with the given drbg params.

    Args:
        tpm: a tpm object used to communicate with the device
        drbg_params: a tuple (entropy string, input1 string, input2 string)

    Raises:
        subcmd.TpmTestError: on unexpected target responses
    """
    entropy, inp1, inp2 = drbg_params
    cmd = _drbg_init_cmd(DRBG_RESEED, a2b(entropy), a2b(inp1), a2b(inp2))
    response = tpm.command(tpm.wrap_ext_command(subcmd.DRBG_TEST, cmd))
    if response != EMPTY_DRBG_RESPONSE:
        raise subcmd.TpmTestError('Unexpected response to '
                                  'DRBG_RESEED: %s' %
                                  (utils.hex_dump(response)))

def drbg_generate(tpm, drbg_params, outlen):
    """Run the drbg reseed command with the given drbg params.

    Args:
        tpm: a tpm object used to communicate with the device
        drbg_params: a tuple (entropy string, nonce string, perso string)
        outlen: the response size for the generate command

    Returns:
        The response from the generate command if the params say to check the
        result otherwise return ''

    Raises:
        subcmd.TpmTestError: on unexpected target responses
    """
    result_str = ''
    inp, expected, check_result = drbg_params
    cmd = _drbg_gen_cmd(a2b(inp), outlen)
    response = tpm.command(tpm.wrap_ext_command(subcmd.DRBG_TEST, cmd))
    if check_result:
        result = response[12:]
        result_hexdump = utils.hex_dump(result)
        if expected and a2b(expected) != result:
            raise subcmd.TpmTestError('error:\nexpected %s'
                                      '\nreceived %s' %
                                      (utils.hex_dump(a2b(expected)),
                                       result_hexdump))
        result_str = ''.join(result_hexdump.split()).upper()
    return result_str

def drbg_test_inputs(tpm, test_inputs):
    """Runs DRBG test case.

    Args:
        tpm: a tpm object used to communicate with the device
        test_inputs: a list of tuples (drbg_op, drbg_params)

    Returns:
        a list of tuples with the generated responses (tgid, tcid, result_str)

    Raises:
        subcmd.TpmTestError: on unexpected target responses
    """

    tcid = 0
    tgid = 0
    test_results = []
    for test in test_inputs:
        drbg_op, drbg_params = test
        if drbg_op == DRBG_GROUP_INIT:
            tgid += 1
            print('Start Test Group', tgid)
            outlen = drbg_params
        elif drbg_op == DRBG_INIT:
            drbg_init(tpm, drbg_params)

        elif drbg_op == DRBG_RESEED:
            drbg_reseed(tpm, drbg_params)

        elif drbg_op == DRBG_GENERATE:
            generate_response = drbg_generate(tpm, drbg_params, outlen)
            # drbg_generate will return an empty string if the test case says
            # to ignore the input. Only save responses the test cares about.
            if generate_response:
                tcid += 1
                test_results.append((tgid, tcid, generate_response))

    print('%sSUCCESS: %s' % (utils.cursor_back(), 'DRBG test'))
    return test_results

def drbg_test(tpm, request, expected, result_dir):
    """Runs DRBG test cases.

    Args:
        tpm: a tpm object used to communicate with the device

    Raises:
        subcmd.TpmTestError: on unexpected target responses
    """
    drbg_test_inputs(tpm, TEST_INPUTS)

    if not request:
        return
    print("Running through lab inputs from", request)
    # Use lab inputs
    lab = lab_vectors.DRBGLabTest(request, expected, result_dir)
    test_inputs = lab.get_test_inputs()
    results = drbg_test_inputs(tpm, test_inputs)
    lab.save_test_results(results)
