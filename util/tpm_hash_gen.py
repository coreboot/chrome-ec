#!/usr/bin/python
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""A program for calculating Cr50 TPM image identifier used in UMA.

The code of this program is a Python equivalent of the code of function
GetFingerprint() defined in

https://chromium.googlesource.com/chromiumos/platform2/+/16911186/cryptohome/tpm.cc#78

"""

import hashlib
import re
import sys

class MyError(Exception):
    """Local Exception wrapper."""
    pass


def get_git_versions(cr50_file):
    """Find git versions in a Cr50 binary image.

    The Cr50 version string is generated at build time by the
    ./util/getversion.sh script. The version string is built based on the
    following template:

    cr50_v<vers>-<git sha> tpm2:<vers>-<git sha> ... <year>-<month>-<day>

    Since the Cr50 image includes sections for RW_A and RW_B, the same version
    string is supposed to be present in the image twice.

    This function reads the passed in file name, looks for the version, makes
    sure that the version is present twice and both instances are identical,
    then extracts git hashes of the ec and tpm2 trees from the version string
    and returns them as a list of two integers.

    Args:
       cr50_file: A string, name of the Cr50 image file.

    Returns:
      A list of two integers, hashes of ec and tpm2 trees at build time.

    Raises:
      MyError: if consistent version string was not found in the file.

    """
    git_vers_regexp = re.compile(b'cr50_v[12].* tpm2.* 20[12][0-9]')
    text = open(cr50_file, 'rb').read()
    strings = git_vers_regexp.findall(text)
    hashes = []

    if len(strings) != 2 or strings[0] != strings[1]:
        raise MyError('Could not find git version string in %s' % cr50_file)

    for piece in strings[0].decode().split():
        if piece.startswith('cr50_') or piece.startswith('tpm2'):
            hashes.append(piece.split('-')[1])

    return [int(x, 16) for x in  hashes]


def hash_cr50_file(cr50_file):
    """Calculate UMA key of a Cr50 image.

    Find out git hashes of the ec and tpm2 components of the passed in Cr50
    image and then generate a file hash following the same pattern which is
    used by
    https://chromium.googlesource.com/chromiumos/platform2/+/16911186/cryptohome/tpm.cc#78

    Args:
       cr50_file: A string, name of the Cr50 image file.

    Returns:
       An int, cr50 image hash calculated following the fixed template.
    """
    git_versions = get_git_versions(cr50_file)
    hash_const = '322e3000000000000000007443524f5300000001'
    tpm_hash_template = '%s%8.8x%8.8x%16.16x%s'
    vendor_specific = 'xCG fTPM'

    hash_input = tpm_hash_template % (hash_const, git_versions[0],
                                      git_versions[1], len(vendor_specific),
                                      vendor_specific)

    h = hashlib.sha256()
    h.update(bytes(hash_input, 'ascii'))

    hex_digest = h.hexdigest()
    uma_digest_list = [hex_digest[2*x:2*x + 2] for x in range(4)]
    hex_uma_key = ''.join(uma_digest_list[::-1])
    uma_key = int(hex_uma_key, 16) & 0x7fffffff
    return uma_key


def main(cr50_files):
    """Calculate and report UMA keys for passed in Cr50 images.

    Args:
      cr50_files: A list of strings, names of Cr50 files to hash.
    """
    for cr50_file in cr50_files:
        key = hash_cr50_file(cr50_file)
        print('%08x %-9d %s' % (key, key, cr50_file))

if __name__ == '__main__':
    try:
        main(sys.argv[1:])
    except MyError as e:
        print('Error:', e)
        sys.exit(1)
    sys.exit(0)
