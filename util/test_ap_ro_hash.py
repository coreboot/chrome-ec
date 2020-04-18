#!/usr/bin/env python3
# -*- coding: utf-8 -*-"
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
'Unit tests for ap_ro_hash.py'

import contextlib
import io
import tempfile
import unittest

from unittest.mock import patch

import ap_ro_hash

TEST_FILE_SIZE = 64 * 1024
TPM_OK_RESPONSE = (128, 1, 0, 0, 0, 12, 0, 0, 0, 0, 0, 54)

# pylint: disable=unused-argument
def mocked_run(command, ignore_error=True):
    """Test mock of ap_ro_hash.run().

    Adding this mock allows to simulate various aspects of expected
    Chrome OS device behavior.
    """
    # Simulated dump_fmap output.
    fmap_sample = """
area:            1
area_offset:     0x00000000
area_size:       0x00006000 (27576)
area_name:       RW_A
area:            2
area_offset:     0x00006000
area_size:       0x00006000 (27576)
area_name:       RW_B
area:            3
area_offset:     0x0000c000
area_size:       0x00004000 (16384)
area_name:       WP_RO
area:            4
area_offset:     0x0000c000
area_size:       0x00000040 (64)
area_name:       RO_VPD
area:            5
area_offset:     0x0000c040
area_size:       0x000017b0 (6064)
area_name:       RO_UNUSED
area:            6
area_offset:     0x0000d7f0
area_size:       0x00002810 (10256)
area_name:       RO_SECTION
"""
    if isinstance(command, list):
        command = ' '.join(command)
    if command.startswith('dump_fmap'):
        return fmap_sample.strip(), ''
    if command.startswith('/usr/sbin/trunks_send'):
        # Simulated trunks_send always reports success.
        return ''.join('%02x' % x for x in TPM_OK_RESPONSE), ''
    if command.startswith('flashrom'):
        if '-l' in command and '-r' in command:
            # This is a request to read flash sections for hashing. Let's
            # create a file with fixed contents large enough to cover all
            # sections. File will be saved in the temp directory created by
            # ap_ro_hash.main().
            fname = command.split('-r')[1].strip().split()[0]
            with open(fname, 'ab') as ff:
                togo = TEST_FILE_SIZE
                # Fill the file with repeating pattern of 0..255.
                filler = bytearray([x for x in range(256)])
                while togo:
                    size = min(togo, len(filler))
                    ff.write(filler[:size])
                    togo -= size
    return '', ''
# pylint: disable=

ap_ro_hash.run = mocked_run

class TestApRoHash(unittest.TestCase):
    'Test class for ap_ro_hash.py.'

    @patch('ap_ro_hash.sys.exit')
    @patch('ap_ro_hash.syslog')
    def test_logger(self, mock_syslog, mock_sysexit):
        'Verify the Logger class behavior.'
        # Intercept writes into stdout and stderr
        with io.StringIO() as stdo, io.StringIO() as stde:
            with contextlib.redirect_stdout(
                    stdo), contextlib.redirect_stderr(stde):

                logger = ap_ro_hash.Logger()

                def reset_all():
                    'Restore mocks and io before the next test.'
                    for m in  mock_syslog, mock_sysexit:
                        m.reset_mock()
                    for s in stdo, stde:
                        s.truncate(0)
                        s.seek(0)

                # Regular message goes to syslog only.
                logger.log('log message')
                self.assertTrue(mock_syslog.syslog.called)
                self.assertEqual(mock_syslog.syslog.call_args[0][0],
                                 'log message\n')
                self.assertFalse(mock_sysexit.called)
                self.assertEqual(stdo.getvalue(), '')
                self.assertEqual(stde.getvalue(), '')
                reset_all()

                # Error message goes to syslog and stderr.
                logger.error('error message')
                expected = 'ERROR: error message\n'
                self.assertTrue(mock_syslog.syslog.called)
                self.assertEqual(mock_syslog.syslog.call_args[0][0], expected)
                self.assertFalse(mock_sysexit.called)
                self.assertEqual(stdo.getvalue(), '')
                self.assertEqual(stde.getvalue(), expected)
                reset_all()

                # Error message goes to syslog and stderr, and triggers a call
                # to sys.exit().
                logger.fatal('fatal error')
                expected = 'FATAL: fatal error\n'
                self.assertTrue(mock_syslog.syslog.called)
                self.assertEqual(mock_syslog.syslog.call_args[0][0], expected)
                self.assertTrue(mock_sysexit.called)
                self.assertEqual(stdo.getvalue(), '')
                self.assertEqual(stde.getvalue(), expected)
                reset_all()

                # In verbose mode regular messages go into syslog and stdout.
                logger.enable_verbose()
                logger.log('log message')
                expected = 'log message\n'
                self.assertTrue(mock_syslog.syslog.called)
                self.assertEqual(mock_syslog.syslog.call_args[0][0], expected)
                self.assertFalse(mock_sysexit.called)
                self.assertEqual(stdo.getvalue(), expected)
                self.assertEqual(stde.getvalue(), '')
                reset_all()

                # Newline is not added if suppressed by the caller.
                logger.log('log message', end='')
                expected = 'log message'
                self.assertTrue(mock_syslog.syslog.called)
                self.assertEqual(mock_syslog.syslog.call_args[0][0], expected)
                self.assertFalse(mock_sysexit.called)
                self.assertEqual(stdo.getvalue(), expected)
                self.assertEqual(stde.getvalue(), '')

    def test_cr50_packet(self):
        'Verify proper creation of TPM the Vendor command.'
        p = ap_ro_hash.Cr50TpmPacket(10)
        empty_packet = bytearray([128, 1, 0, 0, 0, 12, 32, 0, 0, 0, 0, 10])
        self.assertEqual(p.packet(), empty_packet)
        data_packet = bytearray([128, 1, 0, 0, 0, 16, 32, 0, 0,
                                 0, 0, 10, 1, 2, 3, 4])
        p.add_data(bytearray([1, 2, 3, 4]))
        self.assertEqual(p.packet(), data_packet)

    def test_cr50_response(self):
        'Verify Cr50TpmResponse implementation.'
        response_data = bytearray([128, 1, 0, 0, 0, 12, 0, 0, 0, 0, 0, 10])

        def verify_assert(message, corrupt_offset=None):
            """Introduce verify that incorrect data triggers exception.

            Use response_data as the response body. If corrupt_offset is not
            None - modify the value at the offset before instantiating the
            packet, and then restore it in the end.
            """
            if corrupt_offset != None:
                response_data[corrupt_offset] += 1
            with self.assertRaises(ap_ro_hash.ApRoTpmResponseError) as cm:
                ap_ro_hash.Cr50TpmResponse(response_data, 10)
            self.assertTrue(str(cm.exception).startswith(message))
            if corrupt_offset != None:
                response_data[corrupt_offset] -= 1

        r = ap_ro_hash.Cr50TpmResponse(response_data)
        self.assertEqual(r.rc, 0)
        self.assertEqual(r.payload, bytearray([]))

        # Corrupt the tag field.
        verify_assert('unexpected TPM tag', 0)

        # Modify the length field.
        verify_assert('length mismatch', 5)

        # and modify the subcommand value
        verify_assert('subcommand mismatch', len(response_data) - 1)

        # Set the rc value.
        response_data[9] = 1
        r = ap_ro_hash.Cr50TpmResponse(response_data)
        self.assertEqual(r.rc, 1)
        self.assertEqual(r.payload, bytearray([]))

        # Add payload (and increase the length), and verify that the payload
        # is retrieved as expected.
        response_data[5] += 1
        response_data.append(5)
        r = ap_ro_hash.Cr50TpmResponse(response_data)
        self.assertEqual(r.rc, 1)
        self.assertEqual(r.payload, bytearray([5]))

        # Create a valid structured but too long response.
        response_data.append(5)
        response_data[5] += 1
        verify_assert('unexpected response length')

    @patch('ap_ro_hash.Logger._write')
    @patch('ap_ro_hash.os.open')
    def test_tpm_channel(self, mock_open, mock_log_write):
        """Verify TpmChannel implementation.

        Use mocks to simulate the OSError exception to trigger the switch
        between /dev/tpm0 and trunks_send.
        """
        ap_ro_hash.Log = ap_ro_hash.Logger()
        c = ap_ro_hash.TpmChannel()
        self.assertTrue(mock_open.called)
        self.assertEqual(mock_log_write.call_args[0][2], 'will use /dev/tpm0')
        mock_open.reset_mock()
        mock_log_write.reset_mock()
        mock_open.side_effect = OSError
        c = ap_ro_hash.TpmChannel()
        self.assertTrue(mock_open.called)
        self.assertEqual(mock_log_write.call_args[0][2], 'will use trunks_send')
        c.write(bytearray([1, 2, 3])) # Does not really matter what we write.
        ap_ro_hash.Cr50TpmResponse(c.read())
        self.assertEqual(c.read(), bytes())

    def test_get_fmap(self):
        'Verify proper processing of dump_fmap output.'
        # Equivalent representation of fmap_sample defined above.
        expected_fmap = dict({
            'RW_A': (0, 24576),
            'RW_B': (24576, 24576),
            'WP_RO': (49152, 16384),
            'RO_VPD': (49152, 64),
            'RO_UNUSED': (49216, 6064),
            'RO_SECTION': (55280, 10256),
        })
        with tempfile.TemporaryDirectory() as tmpd:
            fmap = ap_ro_hash.read_fmap(tmpd)
            self.assertEqual(len(fmap), len(expected_fmap))
            for k, v in fmap.items():
                self.assertTrue(k in expected_fmap)
                self.assertEqual(expected_fmap[k], v)

    def test_ranges_errors(self):
        'Verify proper detection of address range errors.'
        # Command line parameters and associated substrings to be found in the
        # corresponding error messages.
        bad_ranges_sets = (
            (['0:100'], 'is outside'),
            (['RW-A', 'RW_B'], 'not in FMAP', 'is outside'),
            (['RO_VPD', 'RO_UNUSED', '1000:100'], 'is outside'),
            (['RO_VPD', 'RO_UNUSED', 'c030:100'], 'overlaps'),
        )
        for rset in bad_ranges_sets:
            ranges = rset[0]
            with self.assertRaises(ap_ro_hash.ApRoHashCmdLineError) as cm:
                ap_ro_hash.main(ranges)
            for err_msg in rset[1:]:
                self.assertTrue(err_msg in str(cm.exception))

    @patch('ap_ro_hash.TpmChannel.__init__')
    @patch('ap_ro_hash.TpmChannel.write')
    @patch('ap_ro_hash.TpmChannel.read')
    def test_end_to_end(self, mock_cread, mock_cwrite, mock_cinit):
        """Test end to end processing.

        Includes validating command line arguments, generating the fmap
        dictionary from the mock dump_fmap output, calculating hash on the
        mock binary file and creating the vendor command including the sha256
        hash and the ranges.

        Mocking TpmChannel allows to verify the vendor command contents.
        """
        exp_vendor_command = bytes([
            0x80, 0x01, 0x00, 0x00, 0x00, 0x44, 0x20, 0x00, 0x00, 0x00, 0x00,
            0x36, 0xad, 0x75, 0x79, 0x56, 0x27, 0xdf, 0x99, 0x22, 0xa3, 0x01,
            0xd1, 0x66, 0xc6, 0xb4, 0xb4, 0xc8, 0x94, 0xf7, 0x94, 0x48, 0x6d,
            0x91, 0x7f, 0xbb, 0x2d, 0x85, 0x4c, 0x69, 0x72, 0xe2, 0xce, 0x5d,
            0x00, 0xc0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40, 0xc0, 0x00,
            0x00, 0x10, 0x00, 0x00, 0x00, 0xf0, 0xd7, 0x00, 0x00, 0x10, 0x28,
            0x00, 0x00])
        mock_cinit.return_value = None
        mock_cread.return_value = bytes(TPM_OK_RESPONSE)
        with io.StringIO() as stdo:
            with contextlib.redirect_stdout(stdo):
                ap_ro_hash.main(['RO_VPD', 'RO_SECTION', 'c040:10'])
            self.assertEqual(stdo.getvalue(), 'SUCCEEDED\n')
        self.assertEqual(mock_cwrite.call_args[0][0], exp_vendor_command)


if __name__ == '__main__':
    unittest.main()
