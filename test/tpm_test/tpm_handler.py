#!/usr/bin/python
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module for initializing and driving a TPM device.

Two kinds of connections are supported - one through a USB FTDI SPI adapter,
and another one - a direct connection to /dev/tpm.

The user does not have the ability to choose the interface, USB FTDI SPI is
tried first, and if it does not succeed /dev/tpm0 is tried.
"""

import os
import sys

import subcmd

# Suppressing pylint warning about an import not at the top of the file. The
# path needs to be set *before* the last import.
# pylint: disable=C6204
root_dir = os.path.dirname(os.path.abspath(sys.argv[0]))
sys.path.append(os.path.join(root_dir, '..', '..', 'build', 'tpm_test'))
try:
  # Absence of this module indicates that /dev/tpm0 should be used.
  import ftdi_spi_tpm
except ImportError:
  pass

class FtdiSpi(object):
  def __init__(self):
    self._handle = ftdi_spi_tpm

  def Init(self, freq, debug_mode):
    if not self._handle.FtdiSpiInit(freq, debug_mode):
      raise subcmd.TpmTestError('Failed to connect to FTDI SPI')
    return True

  def SendAndReceive(self, cmd_data):
    return self._handle.FtdiSendCommandAndWait(cmd_data)

class DevTpm(object):
  _TPM_DEV_ = '/dev/tpm0'
  _MAX_RESPONSE_SIZE = 1024

  def __init__(self):
    self._debug = False
    try:
      self._fd = os.open(self._TPM_DEV_, os.O_RDWR)
    except OSError:
      raise subcmd.TpmTestError('Failed to open %s' % self._TPM_DEV)

  def __del__(self):
    os.close(self._fd)

  def Init(self, unused_, debug_mode):
    self.debug_mode = debug_mode
    return True

  def SendAndReceive(self, cmd_data):
    count = 0
    while count != len(cmd_data):
      count += os.write(self._fd, cmd_data[count:])

    response = os.read(self._fd, self._MAX_RESPONSE_SIZE)
    return response

class TpmHandlerFactory(object):
    def GetHandler(self):
        if 'ftdi_spi_tpm' in sys.modules:
            return FtdiSpi()
        else:
            return DevTpm()
