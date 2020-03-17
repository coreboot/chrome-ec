#!/usr/bin/env python3
# -*- coding: utf-8 -*-"
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""acroterm.py - Terminal program for Acropora RTOS

Loosely based on miniterm.py from PySerial.

Eventually:
- Switch to/from binary cmsg() mode
- Local console history/editing
- Integrate with coverage tool?
"""

import argparse
import atexit
import fcntl
import glob
import math
import pickle
import os
import re
import struct
import subprocess
import sys
import tempfile
import termios
import threading
import zlib

import serial

# ------------------------------------------------------------------------------

def fatal(desc):
    """Print an error and exit."""
    sys.stderr.write('\nacroterm error: %s\n' % desc)
    sys.exit(1)


def notice(desc):
    """Print a notification to stderr."""
    sys.stderr.write('\nacroterm: %s\n' % desc)


def crc8(buf):
    """CRC-8"""
    c = 0
    for d in buf:
        c ^= d << 8
        for _ in range(8):
            if c & 0x8000:
                c ^= (0x1070 << 3)
            c <<= 1
    return c >> 8

# ------------------------------------------------------------------------------

class PacketError(Exception):
    """Base packet error exception.

    TODO(vbendeb): should this be rolled into PacketDataLenError()?
    """
    def __init__(self, packet):
        super().__init__()
        self.packet = packet


class PacketDataLenError(PacketError):
    """Exception to throw if packet length error is detected"""
    def __init__(self, packet, count):
        super().__init__(packet)
        self.count = count

# Constants from enum cmsg_channel
CMSG_CHAN_DEFAULT = 0
CMSG_CHAN_TASK_CMSG = 1
CMSG_CHAN_SYSTEM = 2
CMSG_CHAN_INTERRUPT = 3
CMSG_CHAN_INIT = 4
CMSG_CHAN_EXCEPTION = 5
CMSG_CHAN_CUSTOM = 0x40
CMSG_CHAN_CUSTOM_LAST = 0x7f
CMSG_CHAN_FLAG_TASK = 0x80
CMSG_CHAN_FLAG_ASYNC = 0x40
CMSG_CHAN_FLAG_SYSCALL = 0x20
CMSG_CHAN_MASK_TASK_ID = 0x1f

# Constants from enum syscall_cmsg_format
CMSG_FORMAT_DONE = 0
CMSG_FORMAT_SIGNED = 1
CMSG_FORMAT_UNSIGNED = 2
CMSG_FORMAT_HEX = 3
CMSG_FORMAT_CHAR = 4
CMSG_FORMAT_ERR = 5
CMSG_FORMAT_STRING = 6
CMSG_FORMAT_64BIT = 7
CMSG_FORMAT_BUFFER_BASED = 12
CMSG_FORMAT_BUF_STRUCT = CMSG_FORMAT_BUFFER_BASED
CMSG_FORMAT_BUF_STRING = 13
CMSG_FORMAT_BUF_BYTES = 14
CMSG_FORMAT_BUF_WORDS = 15
# 64-bit pseudo-formats
CMSG_FORMAT_SIGNED64 = 100 + CMSG_FORMAT_SIGNED
CMSG_FORMAT_UNSIGNED64 = 100 + CMSG_FORMAT_UNSIGNED
CMSG_FORMAT_HEX64 = 100 + CMSG_FORMAT_HEX
CMSG_FORMAT_TIME64 = 100 + CMSG_FORMAT_CHAR

class Packet(object):
    """Console output packet from acropora"""

    MAGIC = 0xc0
    END_MAGIC = 0xc1
    FORMAT = struct.Struct('<x3BI2HB')

    # Dict of struct handlers indexed by type.  Add with set_struct_handler().
    # Handlers take (struct type index, bytearray) and return string.
    struct_handlers = {}

    @staticmethod
    def default_struct_handler(stype, buf):
        """Default struct handler"""
        return 'BadStruct#%d(%d)' % (stype, len(buf))

    @staticmethod
    def set_struct_handler(stype, handler):
        """Assign structure handler for a structure type"""
        Packet.struct_handlers[stype] = handler

    def __init__(self):
        self.reset()
        self.decoded = ''
        self.errors = []
        self.next_seq = None
        self.last_timestamp = 0
        self.channel = None
        self.data_len = None

    def reset(self):
        """Reset the packet state."""
        self.data = bytearray()
        self.expect_len = 0
        self.timestr = ''

    def get_decoded(self):
        """Return the last decoded packet, or an empty string if none."""
        d = self.decoded
        self.decoded = ''
        return d

    def get_errors(self):
        """Return the last errors or an empty list if none."""
        e = self.errors
        self.errors = []
        return e

    def add_byte(self, b):
        """Add a byte to the packet.  Returns True if the byte was consumed."""
        if not self.expect_len:
            # Not in a packet
            if b != self.MAGIC:
                return False

            # Now starting a packet
            self.expect_len = self.FORMAT.size

        self.data.append(b)

        if len(self.data) == self.expect_len:
            if self.expect_len == self.FORMAT.size:
                if not self.validate_header():
                    self.reset()
            else:
                self.check_trailer_and_decode_packet()

        return True

    def next_data(self, count):
        """Returns the next <count> bytes of data as a bytearray."""

        if len(self.data) < count:
            raise PacketDataLenError(self, count)

        d = self.data[:count]
        self.data = self.data[count:]
        return d

    def unpack_ph(self, header):
        """Class specific header unpack structure

        Saves class unique fields in the instance, returns the common ones to
        the caller.
        """
        (b1, chan, self.const_str_len, time_lo,
         time_hi, data_len, crc) = self.FORMAT.unpack(header)

        self.param_count = b1 >> 5
        return b1, chan, time_lo, time_hi, data_len, crc

    def validate_header(self):
        """Validate the packet header.

        Returns:
            True if there is more data needed.
        """
        if self.expect_len != self.FORMAT.size:
            return False

        header = self.data[:self.FORMAT.size]
        b1, chan, time_lo, time_hi, data_len, crc = self.unpack_ph(header)

        if crc != crc8(header[:-1]):
            print('Bad packet size')
            return False

        self.channel = chan

        timestamp = time_hi << 32 | time_lo
        if timestamp < self.last_timestamp:
            # Reboot will restart the sequence at 0
            self.next_seq = 0
        self.last_timestamp = timestamp
        self.timestr = '%d.%06d' % (timestamp // 1000000, timestamp % 1000000)

        # Flag dropped packets
        if b1 & 0x10:
            self.errors.append('(sender dropped packet(s))')

        sequence = b1 & 0x0f
        if self.next_seq is not None and sequence != self.next_seq:
            self.errors.append('(missing packet(s)); got %d expect %d' %
                               (sequence, self.next_seq))
        self.next_seq = (sequence + 1) % 16

        self.data_len = data_len

        if not data_len:
            # No data; just header
            self.check_trailer_and_decode_packet()
            return False

        self.expect_len += data_len + 1 # +1 for packet end
        return True

    def decode_param(self, fmt, param):
        """Decode one param and return as string"""
        dout = ''

        # TODO: maybe tidier to pass param in as a bytearray and unpack it here?
        if fmt == CMSG_FORMAT_SIGNED:
            if param > 1 << 31:
                param -= 1 << 32
            dout += '%d' % param
        elif fmt == CMSG_FORMAT_SIGNED64:
            if param > 1 << 63:
                param -= 1 << 64
            dout += '%d' % param
        elif fmt == CMSG_FORMAT_UNSIGNED or fmt == CMSG_FORMAT_UNSIGNED64:
            dout += '%u' % param
        elif fmt == CMSG_FORMAT_HEX:
            dout += '0x%08x' % param
        elif fmt == CMSG_FORMAT_HEX64:
            dout += '0x%016x' % param
        elif fmt == CMSG_FORMAT_CHAR:
            dout += "'%c'" % param
        elif fmt == CMSG_FORMAT_TIME64:
            if param == (1 << 64) - 1:
                dout += '(FOREVER)'
            else:
                dout += '%d.%06d' % (param // 1000000, param % 1000000)
        elif fmt == CMSG_FORMAT_ERR:
            err_type = param >> 30
            if param == 0:
                dout += 'ErrNone'
            elif err_type < 2:
                fileno = (param >> 19) & 0x7ff
                lineno = (param >> 8) & 0x7ff   # Or instance
                dout += 'Err#%d' % (param & 0xff)
                if fileno:
                    dout += ':File#%d' % fileno
                if lineno:
                    dout += ':%s#%d' % (
                        'Instance' if err_type else 'Line', lineno)
            elif err_type == 2:
                err_subtype = (param >> 28) & 0x03
                if err_subtype == 0:
                    dout += 'ErrSub#%d:Code#%d' % (
                        (param >> 16) & 0xfff, param & 0xffff)
                else:
                    dout += 'ErrReserved#%08x' % param
            else:
                dout += 'ErrLegacy0x%08x' % (param & 0x3fffffff)
        elif fmt in (CMSG_FORMAT_STRING, CMSG_FORMAT_BUF_STRING,
                     CMSG_FORMAT_BUF_STRUCT, CMSG_FORMAT_BUF_BYTES,
                     CMSG_FORMAT_BUF_WORDS):
            size = param & 0xffff
            if size == 0xffff:
                dout += ('(BadStrPtr)' if fmt == CMSG_FORMAT_STRING else
                         '(bad size/offs)')
            else:
                buf = self.next_data(size)
                if fmt == CMSG_FORMAT_STRING or fmt == CMSG_FORMAT_BUF_STRING:
                    dout += buf.decode(encoding='utf-8', errors='replace')
                elif fmt == CMSG_FORMAT_BUF_STRUCT:
                    stype = param >> 16
                    handler = Packet.struct_handlers.get(
                        stype, Packet.default_struct_handler)
                    dout += handler(stype, buf)
                elif fmt == CMSG_FORMAT_BUF_BYTES:
                    dout += ' '.join('%02x' % x for x in buf)
                elif fmt == CMSG_FORMAT_BUF_WORDS:
                    count = (param & 0xffff) // 4
                    words = struct.unpack('=%dL' % count, buf)
                    dout += ' '.join('%08x' % x for x in words)
        else:
            if fmt > 100:
                dout += 'BadFormatL%d' % (fmt - 100)
            else:
                dout += 'BadFormat%d' % fmt

        return dout

    def check_trailer_and_decode_packet(self):
        'Verify trailer presence and decode packet'
        # Consume packet header.
        self.next_data(self.FORMAT.size)

        # Flag (but keep processing) bad data: header is still fine.
        if self.data_len and ((not self.data) or self.data[-1] !=
                              self.END_MAGIC):
            self.errors.append(
                '(packet data missing end magic; may be corrupt)')
        self.decode_packet()
        self.reset()

    def decode_packet(self):
        """Decode a packet, now that it's all shown up."""
        channel = self.channel

        self.decoded += '[%s/' % self.timestr
        if channel == CMSG_CHAN_DEFAULT:
            self.decoded += '??'
        elif channel == CMSG_CHAN_INTERRUPT:
            self.decoded += 'I.'
        elif channel == CMSG_CHAN_INIT:
            self.decoded += 'i.'
        elif channel == CMSG_CHAN_EXCEPTION:
            self.decoded += 'E.'
        elif channel & CMSG_CHAN_TASK_CMSG:
            if channel & CMSG_CHAN_FLAG_SYSCALL:
                self.decoded += 'A' if channel & CMSG_CHAN_FLAG_ASYNC else 'T'
            else:
                self.decoded += 'a' if channel & CMSG_CHAN_FLAG_ASYNC else 't'
            self.decoded += '%d' % (channel & CMSG_CHAN_MASK_TASK_ID)
        else:
            self.decoded += '%02x' % channel
        self.decoded += ']'

        # Decode data
        const_str = ''
        param_decoded = []
        try:
            if self.const_str_len:
                const_str = self.next_data(self.const_str_len
                  ).decode(encoding='utf-8', errors='replace')

            param_count = self.param_count
            if param_count:
                # Unpack format nibbles
                formats = []
                fbuf = self.next_data((self.param_count + 1) // 2)
                for f in fbuf:
                    formats += [f & 0xf, f >> 4]

                # Unpack params
                params = struct.unpack('=' + ('L' * param_count),
                                       self.next_data(4 * param_count))

                p = 0
                while p < param_count:
                    param = params[p]
                    fmt = formats[p]
                    p += 1
                    if fmt == CMSG_FORMAT_64BIT:
                        param |= params[p] << 32
                        fmt = formats[p] + 100  # TODO: define constant
                        p += 1

                    param_decoded.append(self.decode_param(fmt, param))

        except PacketDataLenError:
            print(' (bad len)')

        # Handle format chars inside the decoded string
        # TODO: use regex rather than scanning a char at a time
        if const_str:
            self.decoded += ' '

        const_chars = [c for c in const_str]
        while const_chars:
            c = const_chars.pop(0)
            if c == '$':
                if param_decoded:
                    self.decoded += param_decoded.pop(0)
            elif c == '%':
                if const_chars:
                    self.decoded += const_chars.pop(0)
            else:
                self.decoded += c

        # Consume remaining params
        if param_decoded:
            self.decoded += ' ' + ' '.join(param_decoded)

# ------------------------------------------------------------------------------
# Packet struct handlers

CMSG_STRUCT_PMP_CSRS = 0
CMSG_STRUCT_MGPSCRATCH_CSRS = 1
CMSG_STRUCT_EXCEPTION_FRAME = 2
CMSG_STRUCT_COVERAGE_COUNTERS = 3
CMSG_STRUCT_TASK_PRINT_ONE = 4
CMSG_STRUCT_SHMEM_PRINT_ONE = 5
CMSG_STRUCT_64BIT_POINTER = 6

def handle_struct_pmp_csrs(_, buf):
    """Format for printing pmp_csrs acropora structure"""
    fields = struct.unpack('=4L16L', buf)
    pmpcfg = fields[0:4]
    pmpaddr = fields[4:20]
    out = '\n'

    for i in range(0, 16, 4):
        out += 'pmp%02d-%02d %08x: ' % (i, i + 3, pmpcfg[i // 4])
        out += '%08x-%08x %08x-%08x\n' % pmpaddr[i : i + 4]

    for i in range(16):
        cfg = pmpcfg[i >> 2] >> (8 * (i & 3))
        if cfg == 0:
            continue

        out += 'pmp%-2d: %02x %s%s%s%s' % (i, cfg,
                                           'R' if (cfg & (1 << 0)) else '-',
                                           'W' if (cfg & (1 << 1)) else '-',
                                           'X' if (cfg & (1 << 2)) else '-',
                                           'L' if (cfg & (1 << 7)) else '-')
        cfg_type = (cfg >> 3) & 3
        if cfg_type == 1: # TOR
            out += '%08x - %08x' % (pmpaddr[i - 1] if i else 0, pmpaddr[i])
        elif cfg_type == 2: # NA4
            out += '%08x' % pmpaddr[i]
        elif cfg_type == 3: # NAPOT
            addr = pmpaddr[i]
            out += '%08x size %08x' % ((addr & (addr + 1)) << 2,
                                       (addr ^ (addr + 1) + 1) << 2)
        else:
            out += '-'

        out += '\n'

    return out

Packet.set_struct_handler(CMSG_STRUCT_PMP_CSRS, handle_struct_pmp_csrs)


def handle_struct_mgpscratch_csrs(_, buf):
    """Format for printing mgpscratch_csrs acropora structure"""
    fields = struct.unpack('=16L', buf)
    out = '\n'

    for i in range(0, 16, 4):
        out += 'gen%02d-%02d  ' % (i, i + 3)
        out += '%08x %08x %08x %08x\n' % fields[i : i + 4]

    return out

Packet.set_struct_handler(CMSG_STRUCT_MGPSCRATCH_CSRS,
                          handle_struct_mgpscratch_csrs)


mcause_desc = {
        0:'Instruction address misaligned',
        1:'Instruction access fault',
        2:'Illegal instruction',
        3:'Breakpoint',
        4:'Load address misaligned',
        5:'Load access fault',
        6:'Store/AMO address misaligned',
        7:'Store/AMO access fault',
        8:'Environment call from U-mode',
        9:'Environment call from S-mode',
        10:'NMI',
        11:'Environment call from M-mode',
        12:'Instruction page fault',
        13:'Load page fault',
        14:'Reserved',
        15:'Store/AMO page fault',
        48:'Watchdog'
}

def handle_struct_exception_frame(_, buf):
    """Format for printing exception_frame acropora structure"""
    out = ''

    fieldvals = struct.unpack('=14L12L7L8LL', buf)

    fieldnames = ['gp', 'tp', 'sp', 'mcause', 'mepc', 'mtval', 'mstatus',
                  'mscratch', 'mie', 'mip', 'mtvec', 'mnmivec', 'trapflgs',
                  'reserved0']
    fieldnames += ['s%d' % i for i in range(12)]
    fieldnames += ['t%d' % i for i in range(7)]
    fieldnames += ['a%d' % i for i in range(8)]
    fieldnames += ['ra']
    fields = dict(zip(fieldnames, fieldvals))

    mstatus = fields['mstatus']
    out += '%s-MODE EXCEPTION ' % ('M' if (mstatus & (3 << 11)) else 'U')

    mcause = fields['mcause']
    out += '%08x: %s\n' % (mcause, mcause_desc.get(mcause, '?'))

    fieldlines = (('s0', 'gp', 'mstatus', 'mie'),
                  ('s1', 'ra', 'mepc', 'mip'),
                  ('s2', 'sp', 'mtvec', 'mnmivec'),
                  ('s3', 'tp', 'mtval', 'mscratch'),
                  ('s4', 'a0', 't0', 'trapflgs'),
                  ('s5', 'a1', 't1'),
                  ('s6', 'a2', 't2'),
                  ('s7', 'a3', 't3'),
                  ('s8', 'a4', 't4'),
                  ('s9', 'a5', 't5'),
                  ('s10', 'a6', 't6'),
                  ('s11', 'a7'))
    fieldlinelen = (3, 3, 7, 8)

    for fline in fieldlines:
        fl = zip(fline, fieldlinelen)
        out += '    '.join('%-*s %08x' %
                           (l, f, fields[f]) for f, l in fl) + '\n'

    return out

Packet.set_struct_handler(CMSG_STRUCT_EXCEPTION_FRAME,
                          handle_struct_exception_frame)


def handle_struct_coverage_counters(_, buf):
    """Format for printing coverage_counters acropora structure"""
    # TODO: this should just write them to coverage directly; no need to parse
    # later.
    count = len(buf) // 8
    counters = struct.unpack('=%dQ' % count, buf)
    return ' '.join(str(c) for c in counters)

Packet.set_struct_handler(CMSG_STRUCT_COVERAGE_COUNTERS,
                          handle_struct_coverage_counters)


def handle_struct_task_print_one(_, buf):
    """Format for printing task_print_one acropora structure"""
    fieldvals = struct.unpack('=LLLLQLLLL16c', buf)
    fieldnames = ['flags', 'events', 'events_enabled', 'events_async_enabled',
                  'wake_time', 'mepc', 'msg_head', 'msg_tail', 'msg_queue_mask',
                  'name']
    fields = dict(zip(fieldnames, fieldvals))
    out = ''

    flags = fields['flags']
    out += 'R' if (flags & (1 << 0)) else 's'
    out += 'T' if (flags & (1 << 1)) else '.'

    out += ' %(mepc)08x %(events)08x' % fields
    out += ' %(events_enabled)08x %(events_async_enabled)08x' % fields
    out += ' %(msg_head)04x %(msg_tail)04x %(msg_queue_mask)04x' % fields

    wake_time = fields['wake_time']
    if wake_time == 0xffffffffffffffff:
        out += '     (FOREVER)'
    else:
        out += '%7d.%06d' % (wake_time // 1000000, wake_time % 1000000)

    out += ' ' + fields['name'].decode()
    return out

Packet.set_struct_handler(CMSG_STRUCT_TASK_PRINT_ONE,
                          handle_struct_task_print_one)


def handle_struct_shmem_print_one(_, buf):
    """Format for printing shmem_print _one acropora  structure"""
    fieldvals = struct.unpack('=LLLLLBBBB', buf)
    fieldnames = ['addr', 'max_size', 'allocated_size', 'allocated_owner_mask',
                  'flags', 'handle', 'owner', 'requested_owner', 'map_count']
    fields = dict(zip(fieldnames, fieldvals))
    out = ''

    out += '%(handle)2d %(flags)02x %(addr)08x' % fields
    out += ' %(allocated_size)6d/%(max_size)6d m%(map_count)d' % fields

    if fields['owner'] != 32:  # SHMEM_OWNER_NONE
        out += ' o%d' % fields['owner']
    if fields['requested_owner'] != 32:  # SHMEM_OWNER_NONE
        out += '->%d' % fields['requested_owner']

    return out

Packet.set_struct_handler(CMSG_STRUCT_SHMEM_PRINT_ONE,
                          handle_struct_shmem_print_one)

def handle_struct_64bit_pointer(_, buf):
    """Unpack and convert to text a 64 bit value"""
    fields = struct.unpack('=Q', buf)

    return '0x%16x' % fields

Packet.set_struct_handler(CMSG_STRUCT_64BIT_POINTER,
                          handle_struct_64bit_pointer)

# ------------------------------------------------------------------------------

class Console(object):
    """OS abstraction for console (input/output codec, no echo)"""

    def __init__(self):
        self.fd = sys.stdin.fileno()
        self.old = termios.tcgetattr(self.fd)
        atexit.register(self.cleanup)

    def setup(self):
        """Set console to read single characters, no echo"""
        attr = termios.tcgetattr(self.fd)
        attr[3] = attr[3] & ~termios.ICANON & ~termios.ECHO & ~termios.ISIG
        attr[6][termios.VMIN] = 1
        attr[6][termios.VTIME] = 0
        termios.tcsetattr(self.fd, termios.TCSANOW, attr)

    def cleanup(self):
        """Restore default console settings"""
        termios.tcsetattr(self.fd, termios.TCSAFLUSH, self.old)

    @staticmethod
    def getkey():
        """Read a single key from the console"""
        while True:
            try:
                c = sys.stdin.read(1)
                if c == '\x7f':
                    c = '\x08'  # Map the BS key (which yields DEL) to backspace
                return c
            except IOError as e:
                # When the programming command is running, it can eat stdin.
                if e.errno == 11: # Resource temporarily unavailable
                    pass
                else:
                    raise
    @staticmethod
    def write(text):
        """Write string"""
        try:
            sys.stdout.write(text)
        except UnicodeEncodeError:
            sys.stdout.write(str(bytes(text, 'utf-8')))
        sys.stdout.flush()

    def cancel(self):
        """Cancel getkey operation"""
        fcntl.ioctl(self.fd, termios.TIOCSTI, b'\0')

    def __enter__(self):
        self.cleanup()
        return self

    def __exit__(self, *args, **kwargs):
        self.setup()

# ------------------------------------------------------------------------------

class Acroterm(object):
    """Acropora terminal class.

    Receives characters from a serial interface or a program, passes them
    through a packet processing and/or directly sends them to the console
    depending on the input stream contents.
    """
    COLOR_DEFAULT = '\x1b[0m'
    COLOR_BLACK = '\x1b[0;30m'
    COLOR_RED = '\x1b[0;31m'
    COLOR_GREEN = '\x1b[0;32m'
    COLOR_BROWN = '\x1b[0;33m'
    COLOR_BLUE = '\x1b[0;34m'
    COLOR_PURPLE = '\x1b[0;35m'
    COLOR_CYAN = '\x1b[0;36m'
    COLOR_LGRAY = '\x1b[0;37m'
    COLOR_DGRAY = '\x1b[1;30m'
    COLOR_LRED = '\x1b[1;31m'
    COLOR_LGREEN = '\x1b[1;32m'
    COLOR_YELLOW = '\x1b[1;33m'
    COLOR_LBLUE = '\x1b[1;34m'
    COLOR_LPURPLE = '\x1b[1;35m'
    COLOR_LCYAN = '\x1b[1;36m'
    COLOR_LWHITE = '\x1b[1;37m'
    COLOR_BOLD = '\x1b[1m'
    COLOR_FAINT = '\x1b[2m'
    COLOR_ITALIC = '\x1b[3m'
    COLOR_UNDERLINE = '\x1b[4m'
    COLOR_BLINK = '\x1b[5m'
    COLOR_NEGATIVE = '\x1b[7m'
    COLOR_CROSSED = '\x1b[9m'

    color_theme = {
      'none': {},
      'light': {'default': COLOR_DEFAULT, 'normal': COLOR_BLUE,
                'error': COLOR_RED, 'not_packet': COLOR_GREEN,
                'program': COLOR_PURPLE},
      'dark':  {'default': COLOR_DEFAULT, 'normal': COLOR_LGREEN,
                'error': COLOR_LRED, 'not_packet': COLOR_LBLUE,
                'program': COLOR_PURPLE},
      'mono':  {'default': COLOR_DEFAULT, 'normal': COLOR_BOLD,
                'error': COLOR_NEGATIVE, 'not_packet': COLOR_FAINT,
                'program': COLOR_ITALIC}
    }

    def color(self, vtype):
        """Return text color according to the color scheme.

        Args:
            vtype: A string, type of the text (see color_theme dictionary
               above)
        """
    # Fall back to default and then empty string
        return self.color_themes.get(vtype, self.color_theme.get('default', ''))

    def __init__(self, args):
        # Config
        # TODO: maybe just save args?
        self.exit_char = '\x03' # Ctrl+C
        self.exit_sequence = args.remote_exit
        self.exit_seq_fail = args.remote_fail
        self.log_filename = args.log
        self.cmd_filter = args.cmd_filter
        self.timeout_secs = args.timeout
        self.coverage_filter = args.coverage_filter
        self.coverage_filter_active = False

        # Other state
        self.packet = Packet()
        self.current_line = ''
        self.exit_code = 0
        self.alive = True

        # Set up the console
        self.console = Console()
        self.console.setup()

        # Start logging
        if self.log_filename is not None:
            if self.log_filename:
                self.log_file = open(self.log_filename, 'wb')
            else:
                fd, name = tempfile.mkstemp(prefix='acroterm.', suffix='.log',
                                            dir='/tmp')
                self.log_file = os.fdopen(fd, 'wb')
                notice('Saving log in %s' % name)
        else:
            self.log_file = None

        # Colors for different display items
        self.color_themes = self.color_theme.get(args.color, 'none')

        # Start serial thread
        if args.tty:
            notice('Starting on %s; ^C=exit' % args.tty)
            self.serial = serial.Serial(port=args.tty,
                                        baudrate=args.baud,
                                        timeout=0)
            self.rx_thread = threading.Thread(target=self.serial_reader,
                                              name='rx')
            self.rx_thread.daemon = True
            self.rx_thread.start()
        else:
            self.serial = self.rx_thread = None

        # Start local input thread
        self.tx_thread = threading.Thread(target=self.writer, name='tx')
        self.tx_thread.daemon = True
        self.tx_thread.start()

        # Start command process
        if args.cmd:
            notice('Running command...')
            self.cmd_proc = subprocess.Popen(
                args.cmd,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                shell=True)
            self.cmd_thread = threading.Thread(target=self.cmd_reader,
                                               name='cmd')
            self.cmd_thread.start()
        else:
            self.cmd_proc = self.cmd_thread = None

        # Start timeout if necessary
        if self.timeout_secs > 0:
            self.timer = threading.Timer(self.timeout_secs,
                                         self.timeout_handler)
            self.timer.start()
        else:
            self.timer = None

        self.cr50_mode = type(self).__name__ == 'Cr50Term'

    def stop(self):
        """Set flag to stop worker threads"""
        self.alive = False
        if self.timer:
            self.timer.cancel()
        if self.serial:
            self.serial.cancel_read()
        self.console.cancel()
        if self.cmd_proc:
            self.cmd_proc.terminate()

    def join_tx(self):
        """Wait for the transmit thread to terminate"""
        self.tx_thread.join()

    def join_all(self):
        """Wait for all worker threads to terminate"""
        self.join_tx()

        if self.serial:
            self.serial.cancel_read()
        if self.rx_thread:
            self.rx_thread.join()
        if self.cmd_thread:
            self.cmd_thread.join()

    def close(self):
        """Close the terminal session"""
        if self.serial:
            self.serial.close()
        if self.log_file:
            self.log_file.close()

    def process_line(self, line):
        """Process a line of text for special sequences."""
        # TODO: Those really should be special packets, not just strings, now
        # that we can send packets...

        if self.exit_sequence and self.exit_sequence in line:
            notice('Exit from remote')
            self.stop()
        elif self.exit_seq_fail and self.exit_seq_fail in line:
            notice('Exit from remote - failure!')
            self.exit_code = 1
            self.stop()
        elif (self.coverage_filter and
              '***Dumping coverage***' in line):
            self.coverage_filter_active = True
            self.console.write('\nSaving')
        elif self.coverage_filter_active:
            if ':COV:' in line:
                self.console.write('.')
            if '***Coverage end***' in line:
                self.coverage_filter_active = False
                self.console.write('done.\n')

    def process_output(self, data):
        """Process chunk of received data.

        Could come over serial line or from a program, depending on how this
        script was invoked.

        If inside the packet - send the data to the packet handler, otherwise
        print it on the console.

        Args:
            data: a byte array, the received chunk.
        """
        self.log_file.write(data)
        # Scan for magic sequences
        for c in data:

            # If processing a packet or line requested exit, stop now
            if not self.alive:
                break

            # Handle packets
            if self.packet.add_byte(c):
                e = self.packet.get_errors()
                if e:
                    self.console.write(self.color('error'))
                    self.console.write('\n'.join(e))
                    self.console.write(self.color('default')+'\n')
                d = self.packet.get_decoded()
                if d:
                    if not self.coverage_filter_active:
                        self.console.write(self.color('normal'))
                        self.console.write(d)
                        self.console.write(self.color('default'))
                        if not self.cr50_mode:
                            self.console.write('\n')
                    self.process_line(d)
                continue

            # Note that unicode will only work *inside* packets.  It can't
            # work outside because Unicode code points overlap the packet
            # start/end byte values.
            c = chr(c)

            if not self.coverage_filter_active:
                self.console.write(self.color('not_packet'))
                self.console.write(c)
                self.console.write(self.color('default'))

            if c in '\r\n':
                self.process_line(self.current_line)
                self.current_line = ''
            else:
                self.current_line += c

    def serial_reader(self):
        """Loop and copy serial->console"""

        # TODO: option to dump data which is waiting at start (from a previous
        # run)

        try:
            while self.alive:
                # Read all that is there or wait for one byte
                data = self.serial.read(self.serial.in_waiting or 1)
                if data:
                    self.process_output(data)

        except (serial.SerialException, OSError):
            self.stop()
            raise

    def writer(self):
        """Loop and copy console->serial until self.exit_char is received."""
        try:
            while self.alive:
                try:
                    c = self.console.getkey()
                except KeyboardInterrupt:
                    c = self.exit_char

                # Could have died while waiting for key
                if not self.alive:
                    break

                if c == self.exit_char:
                    notice('Exit from console')
                    self.stop()             # exit app
                    break

                if self.serial:
                    self.serial.write(bytes(c, encoding='utf-8'))
                else:
                    self.cmd_proc.stdin.write(bytes(c, encoding='utf-8'))
                    self.cmd_proc.stdin.flush()
        except:
            self.stop()
            raise

    def cmd_reader(self):
        """Shell out to programming tool and filter output."""

        proc = self.cmd_proc

        if self.serial:
            # Command is for programming, with output filtering
            while True:
                raw_line = proc.stdout.readline()
                if raw_line:
                    line = raw_line.decode()
                    if self.cmd_filter and self.cmd_filter in line:
                        line = '#'
                    sys.stdout.write(self.color('program') +
                                     line + self.color('default'))
                    sys.stdout.flush()
                elif proc.poll() is not None:
                    break
        else:
            # Command is the program we want to run, with packet I/O
            while self.alive:
                data = proc.stdout.read1(1)
                if data:
                    self.process_output(data)
                else:
                    if proc.poll() is not None:
                        break

        self.cmd_proc = None

        notice('Command exited with code %d' % proc.returncode)
        if not self.serial:
            self.stop()

    def timeout_handler(self):
        """Close terminal on timeout."""
        notice('Hit timeout')
        self.stop()

int_param = re.compile(r'^[0-9.\-]*([l]{0,2}|z)[Xcdux]')
split_int = re.compile(r'^([0-9]+)\.([0-9]+)')
str_param = re.compile(r'^[0-9.\-]*s')
ptr_param = re.compile(r'^p[hPT]')
ll_struct = struct.Struct('<q')
ull_struct = struct.Struct('<Q')
int_struct = struct.Struct('<i')
uint_struct = struct.Struct('<I')
short_struct = struct.Struct('<H')

class Cr50Packet(Packet):
    'Cr50 specific packet class, handles messages of Cr50 format'

    FORMAT = struct.Struct('<x2BIHBHB')
    MAGIC = 0xc2

    def __init__(self, strings):
        self.strings = strings
        super().__init__()

    def unpack_ph(self, header):
        """Class specific header unpack structure

        Saves class unique fields in the instance, returns the common ones to
        the caller.
        """
        (b1, chan, time_lo, time_hi, data_len,
         self.str_index, crc) = self.FORMAT.unpack(header)

        return b1, chan, time_lo, time_hi, data_len, crc

    def process_format(self, str_index, data):
        """Process C format string converting it into Python format string

        Args:
            str_index: int, index of the source code format string in the
                strings list.
            data: binary blob containing parameters matching the format
                string.

        Returns:
            A Python format string suitable for printing
        """
        fstring = self.strings[str_index]
        if fstring.startswith('[^T'):
            fstring = '[%s %s]\n' % (self.timestr, fstring[3:])
        tokens = fstring.split('%')
        text = tokens[0]
        for token in tokens[1:]:
            if token[0] == '%':
                text += token
                continue
            m = int_param.search(token)
            if m:
                fend = m.span()[1]
                fmt = token[:fend]
                # Python doesn't know what z means in format.
                fmt = fmt.replace('z', '')
                rest = token[fend:]
                signed = fmt[-1] == 'd'
                if m.group(1) == 'll':
                    fmt = fmt.replace('ll', '', 1)
                    if signed:
                        s = ll_struct
                    else:
                        s = ull_struct
                else:
                    if signed:
                        s = int_struct
                    else:
                        s = uint_struct
                value = s.unpack_from(data)[0]
                data = data[s.size:]
                m = split_int.search(token)
                if m:
                    # This is a complex format spec used in EC codebase.
                    exp = m.groups()[1]
                    fvalue = value / math.pow(10, int(exp))
                    text += ('%f' % fvalue) + rest
                else:
                    text += ('%' + fmt + rest) % value
                continue
            if str_param.search(token):
                if data[0] == 0xff:
                    # This is a function name.
                    index = uint_struct.unpack_from(data[1:])[0]
                    st = self.strings[index]
                    data = data[5:]
                else:
                    eos = data.find(0)
                    param = data[:eos].decode('ascii')
                    st = param
                    data = data[eos + 1:]
                text += ('%' + token) % st
                continue
            m = ptr_param.search(token)
            if m:
                rest = token[m.span()[1]:]
                if token[1] == 'P':
                    s = uint_struct
                    fmt = '%08x'
                elif token[1] == 'T':
                    s = ull_struct
                    fmt = '%d'
                elif token[1] == 'h':
                    size = short_struct.unpack_from(data)[0]
                    data = data[short_struct.size:]
                    text += ' '.join('%02x' % x for x in data[:size]) + rest
                    data = data[size:]
                    continue
                else:
                    notice('unprocessed format %%%s' % token)
                    continue
                value = s.unpack_from(data)[0]
                if token[1] == 'T' and value == 0:
                    # current time, take it from the packet header.
                    value = self.last_timestamp
                text += (fmt + rest) % value
                data = data[s.size:]
                continue
        return text

    def decode_packet(self):
        """Decode a packet, now that it's all shown up.

        Sets self.decoded to the text of the decoded packet.
        """

        text = self.process_format(self.str_index,
                                   self.next_data(self.data_len))
        if self.data_len:
            self.next_data(1)  # Consume the trailing byte.
        self.decoded = text


class Cr50Term(Acroterm):
    'Cr50 specific Acroterm. Uses Cr50Packet instead of Packet'

    @staticmethod
    def parse_blob(cr50_str_blob):
        """Read and decode the format string blob

        Args:
            cr50_str_blob: name of the file containing the blob prepared by
                util_precompile.py.

        Returns:
            A list of strings, placed at their appropriate locations such that
            string index in the packet sent by Cr50 matches the format string
            it was generated with.

        Raises:
            FileNotFoundError if the file is not found.
        """
        try:
            zipped = open(cr50_str_blob, 'rb').read()
        except FileNotFoundError:
            fatal('Blob file %s not found' % cr50_str_blob)

        pickled = zlib.decompress(zipped)
        dump = pickle.loads(pickled)
        return dump.split('\0')

    def __init__(self, args):
        super().__init__(args)
        strings = self.parse_blob(args.cr50_str_blob)
        self.packet = Cr50Packet(strings)

# ------------------------------------------------------------------------------

def get_args():
    """Prepare argument parser and retrieve command line arguments.

    Returns the parser object with a namespace with all present optional
    arguments set.
    """
    parser = argparse.ArgumentParser(description='Acropora terminal')

    group = parser.add_argument_group('port settings')

    group.add_argument(
        '--tty',
        help=('TTY to use (can have wildcards), or "host" to run --cmd as '
              'the build target(default=%(default)s)'),
        default='/dev/ttyUltraTarget_*')

    group.add_argument(
        '--baud',
        help='Baud rate (default=%(default)s)',
        type=int,
        default=115200)

    group.add_argument(
        '--target',
        help='Run host executable instead of connecting to tty')

    group = parser.add_argument_group('exit settings')

    group.add_argument(
        '--timeout',
        help='Timeout in seconds (default=None)',
        type=int,
        default=0)

    group.add_argument(
        '--remote-exit',
        help='Exit if remote emits this string (default=%(default)s)',
        metavar='MATCH',
        default='***HANGUP***')

    group.add_argument(
        '--remote-fail',
        help='Exit with error if remote emits this string '
        '(default=%(default)s)',
        metavar='MATCH',
        default='***HANGUP-FAIL***')

    group = parser.add_argument_group('logging')

    group.add_argument(
        '--log',
        help='Logfile (default=/tmp/acroterm.*.log)',
        metavar='LOGFILE',
        default='')

    group.add_argument(
        '--color',
        help='Set color theme - none/light/dark/bw (default=%(default)s)',
        default='light')

    group.add_argument(
        '--no-log',
        help='Disable logfile',
        action='store_true')

    group.add_argument(
        '--no-cov-filter',
        help='Display coverage output instead of just logging it',
        dest='coverage_filter',
        action='store_false')

    group = parser.add_argument_group('command on host')

    group.add_argument(
        '--cmd',
        help='Command to run after terminal starts')

    group.add_argument(
        '--cmd-filter',
        help='Filter programming output containing this (default=%(default)s)',
        metavar='MATCH',
        default='adr:')

    group.add_argument(
        '--no-cmd-filter',
        help='Do not filter programming tool output',
        dest='cmd_filter',
        action='store_const', const=None)

    parser.add_argument(
        '--cr50_mode',
        help='Use Cr50 packet format',
        action='store_true')

    parser.add_argument(
        '--cr50_str_blob',
        help='Binary blob containing Cr50 strings (default=%(default)s)',
        default=os.path.normpath(os.path.join(os.path.dirname(__file__),
                                              '../build/cr50/RW/str_blob')))
    return parser.parse_args()

def main():
    """Main function.

    Parse command line arguments and start operation accordingly.
    """
    args = get_args()

    # Look for exactly one matching TTY, unless running target on host
    if args.tty == 'host':
        args.tty = None
        if not args.cmd:
            fatal('Running as host but no command specified')
    else:
        ttylist = glob.glob(args.tty)
        if not ttylist:
            fatal('TTY %s not found' % args.tty)
        elif len(ttylist) != 1:
            fatal('Multiple matches for TTY %s found:\n%s' %
                  (args.tty, '\n'.join(ttylist)))
        else:
            args.tty = ttylist[0]

    # Disable logging if needed
    if args.no_log:
        args.log = None

    # Start the terminal
    if args.cr50_mode:
        term = Cr50Term(args)
    else:
        term = Acroterm(args)

    # Keep running until the other side exits
    try:
        term.join_tx()
    except KeyboardInterrupt:
        pass

    notice('Cleaning up')
    term.join_all()
    term.close()

    sys.exit(term.exit_code)

if __name__ == '__main__':
    main()
