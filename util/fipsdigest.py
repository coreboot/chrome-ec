#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2023 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Compute SHA256 digest of specified file and output it in binary file
"""

import getopt
import hashlib
import os
import sys

def usage():
    """Print usage information"""
    print(f'Syntax: {os.path.basename(__file__)} -f source -o file\n'
          '     -f source binary file to compute digest\n'
          '     -o output file with binary digest\n'
          '     -h - this help\n')

def main():
    """Compute SHA256 of specified file"""
    try:
        opts, _ = getopt.getopt(sys.argv[1:], 'i:o:h', 'help')
    except getopt.GetoptError as err:
        print(str(err))
        usage()
        sys.exit(2)
    source = ""
    output = ""
    for option, arg in opts:
        if option == '-i':
            source = arg
        elif option == '-o':
            output = arg
        elif option in ('-h', '--help'):
            usage()
            sys.exit(0)
    if not source or not output:
            usage()
            sys.exit(0)
    with open(source, 'rb') as in_file:
        with open(output, 'wb') as out_file:
            ctx = hashlib.sha256()
            ctx.update(in_file.read())
            out_file.write(ctx.digest())

if __name__ == '__main__':
    main()
