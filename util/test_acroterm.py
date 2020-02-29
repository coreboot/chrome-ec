#!/usr/bin/env python3
# -*- coding: utf-8 -*-"
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
'Unit tests for acroterm.py'

import sys
import unittest

import acroterm

class TestPacket(unittest.TestCase):
    'Test various packet failures'

    def report_packet_errors(self):
        """Write packet error strings into stderr

        Called if number of error strings does not match test expectations.
        """
        if not self.p.errors:
            return
        sys.stderr.write('unexpected error set:\n')
        for error in self.p.errors:
            sys.stderr.write('%s\n' % error)

    def good_packet(self, packet):
        'Verify good packet handling'
        if self.p.errors:
            self.report_packet_errors()
            self.fail()
        d = self.p.get_decoded()
        self.assertEqual(d, '[13581998.891532/t1]')
        self.assertEqual(self.p.next_seq, (packet[1] & 0xf) + 1)

    def bad_seq(self, _):
        'Verify bad sequence number handling'
        if len(self.p.errors) != 1:
            self.report_packet_errors()
            self.fail()
        self.assertEqual(self.p.errors[0],
                         '(missing packet(s)); got 0 expect 1')
        self.assertEqual(self.p.next_seq, 1)

    def with_data(self, packet):
        'Verify both cases of packet with data'
        if packet[-1] != 0xc1:
            if len(self.p.errors) != 1:
                self.report_packet_errors()
                self.fail()
            self.assertEqual(self.p.errors[0],
                             '(packet data missing end magic; may be corrupt)')
        else:
            if self.p.errors:
                self.report_packet_errors()
                self.fail()
        d = self.p.get_decoded()
        self.assertEqual(d, '[13590588.826124/t1] 67305985->134678021')

    def test_acorpora_packet(self):
        'Test various good and bad packets'

        # Tuple of two-tuples, the first element in the tuple pair is the
        # packet to send to the Packet class, the second element is the
        # function to invoke once the packet has been sent.
        packets = (
            ((0xc0, 0, 1, 0, 12, 34, 56, 78, 90, 12, 0, 0, 33),
             self.good_packet),
            ((0xc0, 0, 1, 0, 12, 34, 56, 78, 91, 12, 0, 0, 55),
             self.bad_seq),
            # Packet with valid data, but with an incorrect trailing character.
            ((0xc0, 0x41, 1, 4, 12, 34, 56, 78, 92, 12, 13, 0, 149,
              0x24, 0x2d, 0x3e, 0x24, 0x22, 1, 2, 3, 4, 5, 6, 7, 8, 0),
             self.with_data),
            # A valid packet with data.
            ((0xc0, 0x42, 1, 4, 12, 34, 56, 78, 92, 12, 13, 0, 180,
              0x24, 0x2d, 0x3e, 0x24, 0x22, 1, 2, 3, 4, 5, 6, 7, 8, 0xc1),
             self.with_data),
            )
        self.p = acroterm.Packet()
        for packet, handler in packets:
            for b in packet:
                self.p.add_byte(b)
            handler(packet)
            self.p.errors = []
            self.p.get_decoded()

if __name__ == '__main__':
    unittest.main()
