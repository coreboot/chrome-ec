# -*- coding: utf-8 -*-
# Copyright 2019 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Tests for trng."""

from math import gcd
import struct
import subcmd
import utils

TRNG_TEST_FMT = '>HB'
TRNG_TEST_RSP_FMT = '>H2IH'
TRNG_TEST_CC = 0x33

TRNG_SAMPLE_SIZE = 1000 # minimal recommended by NIST is 1000 bytes per sample
TRNG_SAMPLE_COUNT = 1000000 # NIST require at least 1000000 of 8-bit samples

# Command structure, shared out of band with the test running on the target:
# field     |    size  |                  note
# ===================================================================
# text_len  |    2     | number of bytes to read, big endian
# type      |    1     | 0 = TRNG, other values reserved for extensions
def get_random_command(size, trng_op):
    """Encode get_random command"""
    return struct.pack(TRNG_TEST_FMT, size, trng_op)

def get_random_command_rsp(size):
    """Create expected response to get_random"""
    return struct.pack(TRNG_TEST_RSP_FMT, 0x8001,
                       struct.calcsize(TRNG_TEST_RSP_FMT) + size,
                       0, TRNG_TEST_CC)

def to_bitstring(s, n=1):
    """Split bytes into individual samples

    convert input packed byte array to n-bits in a byte representation
    used by NIST tests. It's designed to recover sequence of samples
    as it comes from TRNG, including the fact that rand_bytes() reverse
    byte order in every 32-bit chunk.
    """
    out = b''
    val_left = 0
    bits_left = 0
    while s:
        val = (struct.unpack('>I', s[0:4].rjust(4, b'\0'))[0] << bits_left) +\
              val_left
        bits_left += 8 * len(s[0:4])
        s = s[4:]
        while bits_left >= n:
            out += struct.pack('B', val & ((1 << n) - 1))
            val >>= n
            bits_left -= n
        val_left = val
    return out

def trng_test(tpm, trng_output, trng_mode, tsb=1):
    """Download entropy samples from TRNG

    Args:
        tpm: a tpm object used to communicate with the device
        trng_output: file name
        trng_mode: source of randomness [0 - TRNG]
        tsb: number of bits in each TRNG sample, should be in sync with
             TRNG configuration. Default is 1 bit per sample.

    Raises:
        subcmd.TpmTestError: on unexpected target responses
    """

    if trng_mode not in [0]:
        raise subcmd.TpmTestError('Unknown random source: %d' % trng_mode)

    # minimal recommended by NIST is 1000 samples per block
    # TRNG_SAMPLE_BITS is internal setting for TRNG which is important for
    # entropy analysis. TRNG internally gets a 16 bit sample (measurement of
    # time to collapse ring oscillator). Then some slice of these bits
    # [0:TRNG_SAMPLE_BITS] is added (packed) to TRNG FIFO which has internal
    # size of 2048 bits. Reading from TRNG always return 32 bits from FIFO.
    # To extract actual samples from packed 32-bit reading, need to reverse
    # and split read 32-bit into individual samples, each size TRNG_SAMPLE_BITS
    # bits. As it's only possible to read 32 bits at once and TRNG_SAMPLE_BITS
    # can be anything from 1 to 16, including non-power of 2, need to ensure
    # readings to result in non-fractional number of samples. At the same time
    # NIST requires minimum 1000 samples at once, and we also want to reduce
    # number of read requests which are adding overhead.
    # this variable should be divisible by 4 to match 32bit reads from TRNG

    if not 8 >= tsb > 0:
        raise subcmd.TpmTestError('NIST only supports 1 to 8 bits per sample')

    # compute number of bytes, which is multiple of 4 containing whole number
    # of samples, each size tsb bits. This a reduction of
    # tsb * 32 // gcd(tsb, 32) // 8
    lcm = (tsb * 4) // gcd(tsb, 4)

    # combine reads of small batches if possible, 2032 is max size of TPM
    # command response, adjusted to header size
    bytes_per_read = (2032 // lcm) * lcm

    samples_per_read = 8 * bytes_per_read // tsb

    if samples_per_read < 1000:
        raise subcmd.TpmTestError("Can't meet NIST requirement of min 1000 "
                                  'samples in batch - %d bytes only contain'
                                  ' %d samples' % (bytes_per_read,
                                  samples_per_read))
    print('TRNG bits per sample = ', tsb, 'Read size (bytes) =',
          bytes_per_read, 'Samples per read =', samples_per_read)

    remaining_samples = TRNG_SAMPLE_COUNT
    with open(trng_output, 'wb') as out_file:
        while remaining_samples:
            response = tpm.command(tpm.wrap_ext_command(TRNG_TEST_CC,
                                            get_random_command(bytes_per_read,
                                                               trng_mode)))
            if response[:12] != get_random_command_rsp(bytes_per_read):
                raise subcmd.TpmTestError("Unexpected response to '%s': %s" %
                                        ('trng', utils.hex_dump(response)))
            bits = to_bitstring(response[12:], tsb)
            bits = bits[:remaining_samples]
            out_file.write(bits)
            remaining_samples -= len(bits)
            print('%s %d%%\r' % (utils.cursor_back(),
                                ((TRNG_SAMPLE_COUNT - remaining_samples)*100)\
                                // TRNG_SAMPLE_COUNT), end='')
    print('%sSUCCESS: %s' % (utils.cursor_back(), trng_output))
