#!/usr/bin/env python3
# -*- coding: utf-8 -*-"
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""ap_ro_hash.py - script for generating hashes of AP RO sections.

This script is supposed to run as part of the factory image of a Chrome OS
device, with limited Chrome OS specific Python modules availability.

The command line parameters are a set of one or more AP RO firmware address
ranges. A range could be expressed as either two colon separated hex numbers
(the offset of the range base address into the flash space and the size of the
range) or a text string (the name of a flash map section).

The script does the following:

 - using the flashrom utility read the fmap section from the AP firmware chip;

 - using dump_fmap retrieve addresses of all sections of the image, of
   interest is the WP_RO section which encompasses the entire RO space of the
   flash chip;

 - verify input parameters: make sure that input ranges and fmap sections do
   not overlap, and that all off the ranges and sections fall into WP_RO;

 - create a layout file to direct flashrom to read only the ranges of interest
   and read the flash again, this time creating a file with the required
   sections present;

 - calculate the sha256 sum of the required sections;

 - prepare and send to the Cr50 a vendor command including the ranges and the
   calculated checksum;

 - examine the Cr50 return code and report errors, if any.
"""

import argparse
import hashlib
import os
import struct
import subprocess
import sys
import syslog
import tempfile


class ApRoHashCmdLineError(Exception):
    'Exceptions due to command line arguments errors'
    pass


class ApRoHashExecutionError(Exception):
    'Exceptions due to command system command execution errors'
    pass


class ApRoTpmResponseError(Exception):
    'Exceptions due to TPM command execution errors'
    pass

VENDOR_CC_SEED_AP_RO_CHECK = 54

# Code returned by Cr50 if a vendor command is not supported.
VENDOR_RC_NO_SUCH_COMMAND_ERROR = 0x57f

# The tag and format are the same for command and response.
TPM_TAG = 0x8001
HEADER_FMT = '>H2LH'

class Logger(object):
    """A class to support printing into a log and on the console when required.

    Attributes:
        _print_to_console: bool, if true - print on the console in addition to
            the syslog.
    """
    def __init__(self):
        self._print_to_console = False

    def _write(self, prefix, end, *args):
        """Common function for message handling.

        All messages are sent to syslog. Error and fatal messages are also
        always sent to the console to stderr.

        If _print_to_console is True, the regular messages are also printed to
        stdout.

        If prefix is set to FATAL, the program terminates.

        Args:
            prefix: string, one of '', 'ERROR', or 'FATAL'
            end: string, a character to add in the end (facilitates printing
                the newline the same as print())
            args: strings to print, concatenated with a space.
        """
        text = ' '.join(args) + end
        if prefix:
            text = '%s: %s' % (prefix, text)

        syslog.syslog(text)
        if self._print_to_console or prefix in ('ERROR', 'FATAL'):
            if prefix:
                outf = sys.stderr
            else:
                outf = sys.stdout
            print(text, file=outf, end='')
        if prefix == 'FATAL':
            sys.exit(1)

    def enable_verbose(self):
        'Enable verbose logging (printing on the console).'
        self._print_to_console = True

    def log(self, *args, end='\n'):
        'Process regular log message.'
        self._write('', end, *args)

    def error(self, *args, end='\n'):
        'Process non fatal error message.'
        self._write('ERROR', end, *args)

    def fatal(self, *args, end='\n'):
        'Process fatal level error message.'
        self._write('FATAL', end, *args)


LOG = Logger()

class Cr50TpmPacket(object):
    """Class to represent a TPM vendor command packet.

    Cr50 TPM vendor command response packets have the following format, all
    header fields are in big endian order:

    <tag><total length><vendor command><sub command><payload, variable length>
      |        |             |              +---- 2 bytes
      |        |             +------------------- 4 bytes
      |        +--------------------------------- 4 bytes
      +------------------------------------------ 2 bytes

    This class allows to accumulate data of the vendor command payload, and
    generate the full vendor command packet when requested.

    Attributes:
        _subcmd: int, the vendor subcommand
        _data: bytes, the vendor command payload
    """
    TPM_COMMAND = 0x20000000

    def __init__(self, subcmd):
        self._subcmd = subcmd
        self._data = bytes()

    def add_data(self, data):
        'Add data to the vendor command payload.'
        self._data += data

    def packet(self):
        """Generate full vendor command packet using subcommand and payload.

        Returns:
            A byte array which is a fully formatted vendor command packet.
        """
        header = struct.pack(HEADER_FMT,
                             TPM_TAG,
                             struct.calcsize(HEADER_FMT) + len(self._data),
                             self.TPM_COMMAND,
                             self._subcmd)
        return bytearray(header) + self._data


class Cr50TpmResponse(object):
    """Class to represent a TPM response packet.

    Cr50 TPM vendor command response packets have the following format, all
    header fields are in big endian order:

    <tag><total length><return code><sub command>[<data (optional)>]
      |        |             |           +---- 2 bytes
      |        |             +---------------- 4 bytes
      |        +------------------------------ 4 bytes
      +--------------------------------------- 2 bytes

    This class takes a byte buffer, runs basic verification (expected 'tag',
    'total length' matching the buffer length, 'sub command' matching the
    expected value when provided. If verification succeeds, the 'total
    length', 'return code' and 'data' fields are save for future reference.

    If verification fails, appropriate exceptions are raised.

    Attributes:
        _rc: int, 'return code' from packet header.
        _length: int, number of bytes in the packet
        _payload: bytes, contents of the data field, could be empty
    """
    def __init__(self, response, exp_subcmd=None):
        header_size = struct.calcsize(HEADER_FMT)
        if len(response) < header_size:
            raise ApRoTpmResponseError(
                'response too short (%d bytes)' % len(response))
        tag, self._length, self._rc, subcmd = struct.unpack_from(HEADER_FMT,
                                                                 response)
        if tag != TPM_TAG:
            raise ApRoTpmResponseError('unexpected TPM tag %04x' % tag)
        if self._length != len(response):
            raise ApRoTpmResponseError('length mismatch (%d != %d)' %
                                       (self._length, len(response)))
        if self._length not in (header_size, header_size + 1):
            raise ApRoTpmResponseError('unexpected response length %d' %
                                       (self._length))

        if exp_subcmd != None and exp_subcmd != subcmd:
            raise ApRoTpmResponseError('subcommand mismatch (%04x != %04x)' %
                                       (subcmd, exp_subcmd))
        self._payload = response[header_size:]

    @property
    def rc(self):
        'Get the response return code.'
        return self._rc

    @property
    def payload(self):
        'Get the response payload.'
        return self._payload


class TpmChannel(object):
    """Class to represent a channel to communicate with the TPM

    Communications could happen over /dev/tpm0 directly, if it is available,
    or through trunksd using the trunks_send utility.

    Attributes:
        _os_dev: int, file device number of the file opened to communicate
                 with the TPM. Set to None if opening attempt returned an
                 error.
        _response: bytes, data received from trunks_send. Not applicable in
                 case communications use /dev/tpm0.
    """
    def __init__(self):
        try:
            self._os_dev = os.open('/dev/tpm0', os.O_RDWR)
            LOG.log('will use /dev/tpm0')
        except OSError:
            self._os_dev = None
            LOG.log('will use trunks_send')
        self._response = bytes()

    def write(self, data):
        """Send command to the TPM.

        Args:
            data: byte array, the fully TPM command (i.e. Cr50 vendor
                command).
        """
        if self._os_dev:
            LOG.log('will call write to send %d bytes' % len(data), end='')
            rv = os.write(self._os_dev, data)
            LOG.log(', sent %d' % rv)
            return
        command = '/usr/sbin/trunks_send --raw ' + ' '.join(
            '%02x' % x for x in data)
        rv = run(command, ignore_error=False)[0]
        rv_hex = [rv[2*x:2*x+2] for x in range(int(len(rv)/2))]
        self._response = bytes([int(x, 16) for x in rv_hex])

    def read(self):
        """Read TPM response.

        Read the response directly from /dev/tpm0 or use as a response the
        string returned by the previous trunks_send invocation.

        Returns:
            A byte array, the contents of the TPM response packet.
        """
        if self._os_dev:
            # We don't expect much, but let's allow for a long response packet.
            return os.read(self._os_dev, 1000)
        rv = self._response
        self._response = bytes()
        return rv


def run(command, ignore_error=True):
    """Run system command.

    The command could be passed as a string (in which case every word in the
    string is considered a separate command line element) or as a list of
    strings.

    Args:
        command: string or list of strings. If string is given, it is
            converted into a list of strings before proceeding.
        ignore_error: Bool, if set to False and command execution fails, raise
            the exception.

    Returns:
        A tuple of two possibly multiline strings, the stdio and stderr
        generated while the command was being executed.

    Raises:
        ApRoHashExecutionError in case the executed command reported a non
            zero return status, and ignore_error is False
    """
    if isinstance(command, str):
        command = command.split()
    LOG.log('will run "%s"' % ' '.join(command))

    rv = subprocess.run(command,
                        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if ignore_error and rv.returncode != 0:
        raise ApRoHashExecutionError(' '.join(command))

    return(rv.stdout.decode('ascii'), rv.stderr.decode('ascii'))


def send_to_cr50(ranges, digest):
    """Program RO hash information in Cr50.

    This function creates a vendor command packet which will allow Cr50 to
    save ranges and the sha256 hash of the AP flash RO space which needs to be
    verified.

    This vendor command payload has the following format:

    <32 bytes of sha256 digest><range>[<range>..]

    The Cr50 response contains the return code (rc) in the header set to zero
    on success, or a generic error on failure. In case of failure the actual
    error code is delivered as the last byte of the response packet.

    Args:
        ranges: a list of two tuples of ints, base addresses and sizes of
            hashed flash areas
        digest: combined 32 bit sha256 digest of all ranges

    Returns:
        An integer value, zero on success, of the actual error code on failure.
    """
    subcmd = VENDOR_CC_SEED_AP_RO_CHECK
    p = Cr50TpmPacket(subcmd)
    p.add_data(digest)
    for r in ranges:
        for n in r:
            p.add_data(bytearray(struct.pack('<L', n)))
    channel = TpmChannel()
    channel.write(p.packet())
    tpm_response = Cr50TpmResponse(channel.read(), subcmd)
    # Return payload value if present, if not - the header RC value.
    if tpm_response.payload:
        return int(tpm_response.payload[0])
    return tpm_response.rc


def read_fmap(tmpd):
    """Read AP firmware fmap section and convert it into a dictionary.

    The flashrom utility is used to read the only section of the AP firmware,
    the fmap, into a file. Then the dump_fmap utility is used to print the
    fmap contents, which consists of a set of section descriptions of the
    following structure:

       area:            22
       area_offset:     0x00c00000
       area_size:       0x00400000 (4194304)
       area_name:       WP_RO

    This function parses the section descriptions to generate a dictionary
    where the keys are area_names, and values are two int tuples of
    (area_offset, area_size).

    Args:
        tmpd: string, directory to use to store temp files.

    Returns:
        The generated dictionary.
    """
    fmap_file = os.path.join(tmpd, 'fmap.bin')
    run('flashrom -i FMAP -r %s' % fmap_file, ignore_error=False)
    fmap_text = run('dump_fmap ' + fmap_file, ignore_error=False)[0]

    fmap = {}
    offset = 0
    size = 0
    for line in fmap_text.splitlines():
        tokens = line.split()
        if tokens[0] == 'area_offset:':
            offset = int(tokens[1], 16)
            continue
        if tokens[0] == 'area_size:':
            size = int(tokens[1], 16)
            continue
        if tokens[0] == 'area_name:':
            fmap[tokens[1]] = (offset, size)
            LOG.log('%20s: %08x:%08x' % (tokens[1], offset, offset + size - 1))
            continue
    return fmap


def verify_ranges(ranges, limit):
    """Verify that all ranges fall into the limit.

    Args:
        ranges: a sorted list of two tuples of ints, base addresses and sizes
            of hashed flash areas
        limit: a range all ranges' elements must fit into

    Returns:
        A string describing non compliant ranges, an empty string if no problems
        have been found.
    """
    base = limit[0]
    top = base + limit[1]
    errors = []
    for i, r in enumerate(ranges):
        rerrors = []
        rbase = r[0]
        rtop = rbase + r[1]
        if rbase < base or rtop > top:
            rerrors.append('is outside the RO area',)
        if i > 0:
            prev_r = ranges[i - 1]
            if prev_r[0] + prev_r[1] > rbase:
                rerrors.append('overlaps with %x:%x' % (prev_r[0], prev_r[1],))
        if rerrors:
            errors.append(' '.join(['Range %x:%x:' % (r[0], r[1]),] + rerrors))

    return '\n'.join(errors)


def read_ro_ranges(check_file_name, tmpd, ranges):
    """Read firmware ranges into a file.

    Based on the passed in ranges create a layout file to store the list of
    sections to be read by flashrom, and read these sections of the AP
    firmware into a file.

    Args:
        check_file_name: string, name of the file to read into
        tmpd: string, name of the directory used for the output and layout files
        ranges: a list of two int elements, base address and size of each area
    """
    layout_file = os.path.join(tmpd, 'layout')
    read_cmd = ['flashrom', '-l', layout_file, '-r', check_file_name]
    with open(layout_file, 'w') as lf:
        for i, r in enumerate(ranges):
            name = 'section%d' % i
            lf.write('%x:%x %s\n' % (r[0], r[0] + r[1] - 1, name))
            read_cmd += ['-i', name]
    run(read_cmd, ignore_error=False)


def calculate_hash(ro_file, ranges):
    """Calculate sha256 hashes of ranges in the file.

    Args:
        ro_file: string, name of the file containing the areas to be hashed
        ranges: a list of two int elements, base address and size of each area

    Returns:
        A byte array, the calculated sha256 hash of all areas concatenated.
    """
    sha256 = hashlib.sha256()
    with open(ro_file, 'rb') as rof:
        for offset, size in ranges:
            rof.seek(offset)
            while size:
                buf = rof.read(size)
                if not buf:
                    raise ApRoHashCmdLineError(
                        'image is too small for range starting at 0x%x'
                        % offset)
                sha256.update(buf)
                size -= len(buf)

    return sha256.digest()


usage_str = """
%s: [-v] <range>|<fmap_area> [<range>|<fmap_area>...]
         <range>: two colon separated hex values, AP flash area offset and size
         <fmap_area>: symbolic name of the area as reported by dump_fmap
         All ranges and fmap areas must fit into the WP_RO FMAP area
"""  % sys.argv[0].split('/')[-1]

def get_args(args):
    """Prepare argument parser and retrieve command line arguments.

    Returns the parser object with a namespace with all present optional
    arguments set.
    """
    parser = argparse.ArgumentParser(usage=usage_str)
    parser.add_argument(
        '--verbose', '-v',
        type=bool,
        help=('enable verbose logging on the console'),
        default=False)

    return parser.parse_known_args(args)


def main(args):
    'Main function, receives a list of strings, command line arguments'

    # Map of possible error codes returned by Cr50 (both vendor command and
    # subcommand level errors) into strings.
    error_codes = {
        1 : 'Vendor command too short',
        2 : 'Vendor command size mismatch',
        3 : 'Bad offset value',
        4 : 'Bad range size',
        5 : 'Already programmed',
        VENDOR_RC_NO_SUCH_COMMAND_ERROR : 'Insufficient C50 version',
    }

    nsp, rest = get_args(args)

    if nsp.verbose:
        LOG.enable_verbose()

    ranges = []     # Ranges from the command line, including FMAP sections.

    # Let's keep all temp files in an ephemeral temp directory.
    with tempfile.TemporaryDirectory() as tmpd:
        bad_ranges = []	       # Invalid ranges, if any.
        bad_section_names = [] # Invalid section names, if any.

        fmap = read_fmap(tmpd)


        for arg in rest:
            if ':' in arg:
                try:
                    ranges.append(([int('%s' % x, 16) for
                                    x in arg.split(':', 1)]),)
                except ValueError:
                    bad_ranges.append(arg)
                continue
            if arg not in fmap:
                bad_section_names.append(arg)
                continue
            ranges.append(fmap[arg])

        if not ranges:
            raise ApRoHashCmdLineError('no hashing ranges specified')

        error_msg = ''
        if bad_ranges:
            error_msg += 'Ranges %s not valid\n' % ' '.join(bad_ranges)
        if bad_section_names:
            error_msg += ('Section(s) "%s" not in FMAP\n' %
                          '" "'.join(bad_section_names))

        # Make sure the list is sorted by the first element.
        ranges.sort(key=lambda x: x[0])

        # Make sure ranges do not overlap and fall into the WP_RO section.
        error_msg += verify_ranges(ranges, fmap['WP_RO'])
        if error_msg:
            raise ApRoHashCmdLineError(error_msg)

        ro_check_file = os.path.join(tmpd, 'ro_check.bin')
        read_ro_ranges(ro_check_file, tmpd, ranges)
        digest = calculate_hash(ro_check_file, ranges)

    rv = send_to_cr50(ranges, digest)
    if rv != 0:
        err_str = error_codes.get(rv, 'Unknown')
        LOG.error('Cr50 returned %s%x (%s)' % (
            # Add 0x prefix if value exceeds 9.
            '0x' if rv > 9 else '',
            rv, err_str))
        return rv
    print('SUCCEEDED')
    return 0

if __name__ == '__main__':
    try:
        main(sys.argv[1:])
    except ApRoHashCmdLineError as e:
        LOG.fatal('%s' % e)
    except ApRoHashExecutionError as e:
        LOG.fatal('command \"%s\" failed' % e)
    except ApRoTpmResponseError as e:
        LOG.fatal('TPM response problem: \"%s\"' % e)
