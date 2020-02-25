#!/usr/bin/env python3
# -*- coding: utf-8 -*-"
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
'Unit tests for util_precompile.py'

import pickle
import unittest
import zlib

import util_precompile

class TestUtilPrecompile(unittest.TestCase):
    'Test class for testing various util_precompile.py functions'

    def test_generate_cmsg_line(self):
        'Test generating of cmsgX() lines from format messages/arguments'
        # A tuple of tuples of typical format strings, following the matching
        # number of parameters, the last element is the expected parameter
        # description mask, describing the variety of format specifications in
        # the format string.
        inputs = (('"%s\n', 'errmsgs[rv]', 3),
                  ('%d', 10, 1),
                  ('"no format"', ),
                  ('[%pT CCD state:', '((void *)0)', 7),
                  ('%08x: DIO%c%-2d  %2d %3s%3s%3s%4s ',
                   1, 2, 3, 4, 5, 6, 6, 7, 858984721))
        string_indices = []
        for inp in inputs:
            fmt = inp[0]
            if len(inp) > 1:
                params = inp[1:-1]
                mask = inp[-1]
            else:
                params = []
                mask = None
            blocks = fmt.split('%')[1:]
            line = util_precompile.generate_cmsg_line(
                fmt, params, fmt.split('%')[1:], 'chan', 'func')
            exp_start = 'cmsg%d(chan, ' % len(blocks)
            if params:
                args = '%s' % (', '.join(['(uintptr_t)(%s)' % x
                                          for x in params]))
                exp_end = ', %d, %s);\n' % (mask, args)
            else:
                exp_end = ');\n'
            try:
                self.assertTrue(line.startswith(exp_start))
                self.assertTrue(line.endswith(exp_end))
            except AssertionError:
                print('line: %s\nexp_start: %s\nexp_end: %s' % (
                    line, exp_start, exp_end))
                raise
            line = line.replace(exp_start, '', 1)
            line = line.replace(exp_end, '', 1)
            string_indices.append(int(line))

        # Verify the contents of the generated blob
        zipped = util_precompile.generate_blob()
        dump = zlib.decompress(zipped)
        strings = pickle.loads(dump).split('\0')
        for inp, index in zip(inputs, string_indices):
            string = inp[0]
            self.assertTrue(strings[index] == string)

    def test_tokenize(self):
        'Verify tokenize() function ability to parse vararg string'
        in_tokens = (
            '"simple string"',
            '"another string, with a comma"',
            '"string split" " in two"',
            '"string with \\"escaped\\" double quotes"',
            '(&(const struct hex_buffer_params)'
            '{ .buffer = (ec_efs_ctx.hash), .size = (32) })')
        out_tokens = util_precompile.tokenize(', '.join(in_tokens))
        for in_t, out_t in zip(in_tokens, out_tokens):
            self.assertEqual(in_t, out_t)

    def test_line_processor(self):
        'Test line processor class ability to consolidate preprocessor lines'
        # Reset the string dictionary.
        util_precompile.FMT_DICT = {}
        in_out_tuples = (
            (
                """ cprintf(CC_COMMAND, "Last attempt returned " "%d\\n", rv)

                ;""",

                ' cmsg1(CC_COMMAND, 0, 1, (uintptr_t)(rv));\n'
            ), (
                ' cprintf(CC_COMMAND, "ec_hash_secdata    : %ph\\n", '
                '(&(const struct hex_buffer_params)'
                '{ .buffer = (ec_efs_ctx.hash), .size = (32) }));',
                ' cmsg1(CC_COMMAND, 1, 5, (uintptr_t)((&(const struct '
                'hex_buffer_params)'
                '{ .buffer = (ec_efs_ctx.hash), .size = (32) })));\n'
            ), (
                'struct ec_params_get_cmd_versions {',
                'struct ec_params_get_cmd_versions {',
            ), (
                '  cprints(CC_CCD, "CCD test lab mode %sbled", v '
                '? "ena" : "disa");',
                ' cmsg1(CC_CCD, 2, 3, (uintptr_t)(v ? "ena" : "disa"));\n'
            ), (
                '  cprintf(CC_COMMAND, "%s: deleting var failed!\\n", '
                '__func__);',
                ' cmsg1(CC_COMMAND, 3, 6, (uintptr_t)4);\n'
            ), (
                ' return  cprintf(CC_COMMAND, "%s: done!\\n", __func__);',
                ' return cmsg1(CC_COMMAND, 5, 6, (uintptr_t)4);\n'
            )
        )
        line_processor = util_precompile.LineProcessor()
        for inp, outp in in_out_tuples:
            result = ''
            for line in inp.splitlines():
                section = line_processor.process_preprocessor_line(line)
                if section:
                    result += section
            self.assertEqual(result, outp)
            try:
                string_index = int(outp.split(',')[1])
            except IndexError:
                # This is the line without a print statement.
                continue
            fmt = inp.split(',')[1].strip()
            for key, value in util_precompile.FMT_DICT.items():
                if value == string_index:
                    fmt = fmt.replace('" "', '')
                    fmt = fmt.replace('\\n', '\n')
                    fmt = fmt.strip('"')
                    if 'cprints' in inp:
                        fmt = '[^T' + fmt
                    self.assertEqual(fmt, key)
                    break
            else:
                self.fail('did not find "%s" in the dictionary')


    def test_drop_escapes(self):
        'Verify proper conversion of escape characters'
        insouts = (('\\a\\b\\f\\n\\r\\t\\v\'"\\\\',
                    '\a\b\f\n\r\t\v\'"\\'),
                   ('line \\x1a with two hex escapes \\x1b\\n',
                    'line \x1a with two hex escapes \x1b\n'))
        for i, o in insouts:
            self.assertEqual(o, util_precompile.drop_escapes(i))

if __name__ == '__main__':
    unittest.main()
