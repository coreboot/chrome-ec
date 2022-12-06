#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2020 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Flash Cr50 using gsctool or cr50-rescue.

gsctool example:
util/flash_cr50.py --image cr50.bin.prod
util/flash_cr50.py --release prod

cr50-rescue example:
util/flash_cr50.py --image cr50.bin.prod -c cr50-rescue -p 9999
util/flash_cr50.py --release prod -c cr50-rescue -p 9999
"""

import argparse
import logging
import os
import pprint
import re
import select
import shutil
import subprocess
import sys
import tempfile
import threading
import time

import serial


# No GSC updaters take anywhere close to 2 minutes to run.
CMD_TIMEOUT = 120
CR50_FIRMWARE_BASE = '/opt/google/cr50/firmware/cr50.bin.'
UPDATERS = [ 'gsctool', 'cr50-rescue', 'brescue' ]
RELEASE_PATHS = {
    'prepvt': CR50_FIRMWARE_BASE + 'prepvt',
    'prod': CR50_FIRMWARE_BASE + 'prod',
}
# Dictionary mapping a setup to controls used to verify that the setup is
# correct. The keys are strings and the values are lists of strings.
REQUIRED_CONTROLS = {
    'cr50_uart': [
        r'raw_cr50_uart_pty:\S+',
        r'cr50_ec3po_interp_connect:\S+',
    ],
    'pch_disable': [
        r'pch_disable:\S+',
    ],
    'cr50_reset_odl': [
        r'cr50_reset_odl:\S+',
    ],
    'ec_uart': [
        r'ec_board:\S+',
    ],
    'flex': [
        r'servo_type:(servo_v2|servo_micro|c2d2)',
    ],
    'type-c_servo_v4': [
        r'root.dut_connection_type:type-c',
        r'servo_pd_role:\S+',
    ],
}
# Supported methods to resetting cr50.
SUPPORTED_RESETS = (
    'battery_cutoff',
    'console_reboot',
    'cr50_reset_odl',
    'pch_disable',
    'manual_reset',
)


class Error(Exception):
    """Exception class for flash_cr50 utility."""


def run_command(cmd, check_error=True):
    """Run the given command.

    Args:
        cmd: The command to run as a list of arguments.
        check_error: Raise an error if the command fails.

    Returns:
        (exit_status, The command output)

    Raises:
        The command error if the command fails and check_error is True.
    """
    result = subprocess.run(cmd,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT,
                            encoding='utf-8',
                            timeout=CMD_TIMEOUT,
                            check=check_error)
    msg = result.stdout or ''
    logging.debug('%r result %d:\n%s', ' '.join(cmd), result.returncode, msg)
    return result.returncode, msg.strip()


class Cr50Image():
    """Class to handle cr50 image conversions."""

    SUFFIX_LEN = 6
    RW_NAME_BASE = 'cr50.rw.'

    def __init__(self, image, artifacts_dir):
        """Create an image object that can be used by cr50 updaters."""
        self._remove_dir = False
        if not os.path.exists(image):
            raise Error('Could not find image: %s' % image)
        if not artifacts_dir:
            self._remove_dir = tempfile.mkdtemp()
            artifacts_dir = self._remove_dir
        if not os.path.isdir(artifacts_dir):
            raise Error('Directory does not exist: %s' % artifacts_dir)
        self._original_image = image
        self._artifacts_dir = artifacts_dir
        self._generate_file_names()

    def __del__(self):
        """Remove temporary files."""
        if self._remove_dir:
            shutil.rmtree(self._remove_dir)

    def _generate_file_names(self):
        """Create some filenames to use for image conversion artifacts."""
        self._tmp_rw_bin = os.path.join(self._artifacts_dir,
                                        self.RW_NAME_BASE + '.bin')
        self._tmp_rw_hex = os.path.join(self._artifacts_dir,
                                        self.RW_NAME_BASE + '.hex')
        self._tmp_cr50_bin = os.path.join(self._artifacts_dir,
                                          self.RW_NAME_BASE + '.orig.bin')

    def extract_rw_a_hex(self):
        """Extract RW_A.hex from the original image."""
        run_command(['cp', self.get_bin(), self._tmp_cr50_bin])
        run_command(['dd', 'if=' + self._tmp_cr50_bin, 'of=' + self._tmp_rw_bin,
                     'skip=16384', 'count=233472', 'bs=1'])
        run_command(['objcopy', '-I', 'binary', '-O', 'ihex',
                     '--change-addresses', '0x44000', self._tmp_rw_bin,
                     self._tmp_rw_hex])

    def get_rw_hex(self):
        """cr50-rescue uses the .hex file."""
        if not os.path.exists(self._tmp_rw_hex):
            self.extract_rw_a_hex()
        return self._tmp_rw_hex

    def get_bin(self):
        """Get the filename of the update image."""
        return self._original_image

    def get_original_basename(self):
        """Get the basename of the original image."""
        return os.path.basename(self._original_image)


class Servo():
    """Class to interact with servo."""

    # Wait 3 seconds for device to settle after running the dut control command.
    SHORT_WAIT = 3

    def __init__(self, port):
        """Initialize servo class.

        Args:
            port: The servo port for the device being updated.
        """
        self._port = port

    def dut_control(self, cmd, check_error=True, wait=False):
        """Run dut control commands

        Args:
            cmd: the command to run
            check_error: Raise RunCommandError if the command returns a non-zero
                         exit status.
            wait: If True, wait SHORT_WAIT seconds after running the command

        Returns:
            (exit_status, output string) - The exit_status will be non-zero if
            the command failed and check_error is True.

        Raises:
            RunCommandError if the command fails and check_error is False
        """
        dut_control_cmd = ['dut-control', cmd, '-p', self._port]
        exit_status, output = run_command(dut_control_cmd, check_error)
        if wait:
            time.sleep(self.SHORT_WAIT)
        return exit_status, output.split(':', 1)[-1]

    def get_raw_cr50_pty(self):
        """Return raw_cr50_pty. Disable ec3po, so the raw pty can be used."""
        # Disconnect EC3PO, raw_cr50_uart_pty will control the cr50
        # output and input.
        self.dut_control('cr50_ec3po_interp_connect:off', wait=True)
        return self.dut_control('raw_cr50_uart_pty')[1]

    def get_cr50_version(self):
        """Return the current cr50 version string."""
        # Make sure ec3po is enabled, so getting cr50_version works.
        self.dut_control('cr50_ec3po_interp_connect:on', wait=True)
        return self.dut_control('cr50_version')[1]


class Cr50Reset():
    """Class to enter and exit cr50 reset."""

    # A list of requirements for the setup. The requirement strings must match
    # something in the REQUIRED_CONTROLS dictionary.
    REQUIRED_SETUP = ()
    CCD = 'ccd'
    CCD_WATCHDOG_RE = r'(ccd.*):'

    def __init__(self, servo, name):
        """Make sure the setup supports the given reset_type.

        Args:
            servo: The Servo object for the device.
            name: The reset type.
        """
        self._servo = servo
        self._servo_type = self._servo.dut_control('servo_type')[1]
        match = re.search(self.CCD_WATCHDOG_RE, self._servo.dut_control('watchdog')[1])
        self._ccd_device = match.group(1) if match else ''
        self._reset_name = name
        self.verify_setup()
        self._original_watchdog_state = self.ccd_watchdog_enabled()

    def verify_setup(self):
        """Verify the setup has all required controls to flash cr50.

        Raises:
            Error if something is wrong with the setup.
        """
        # If this failed before and didn't cleanup correctly, the device may be
        # cutoff. Try to set the servo_pd_role to recover the device before
        # checking the device state.
        self._servo.dut_control('servo_pd_role:src', check_error=False)

        logging.info('Requirements for %s: %s', self._reset_name,
                     pprint.pformat(self.REQUIRED_SETUP))

        # Get the specific control requirements for the necessary categories.
        required_controls = []
        for category in self.REQUIRED_SETUP:
            required_controls.extend(REQUIRED_CONTROLS[category])

        logging.debug('Required controls for %r:\n%s', self._reset_name,
                      pprint.pformat(required_controls))
        setup_issues = []
        # Check the setup has all required controls in the correct state.
        for required_control in required_controls:
            control, exp_response = required_control.split(':')
            returncode, output = self._servo.dut_control(control, False)
            logging.debug('%s: got %s expect %s', control, output, exp_response)
            match = re.search(exp_response, output)
            if returncode:
                setup_issues.append('%s: %s' % (control, output))
            elif not match:
                setup_issues.append('%s: need %s found %s' %
                                    (control, exp_response, output))
            else:
                logging.debug('matched control: %s:%s', control, match.string)
                # Save controls, so they can be restored during cleanup.
                setattr(self, '_' + control, output)

        if setup_issues:
            raise Error('Cannot run update using %s. Setup issues: %s' %
                        (self._reset_name, setup_issues))
        logging.info('Device Setup: ok')
        logging.info('Reset Method: %s', self._reset_name)

    def cleanup(self):
        """Try to get the device out of reset and restore all controls."""
        logging.info('Cleaning up')
        self.restore_control('cr50_ec3po_interp_connect')

        # Toggle the servo v4 role if possible to try and get the device out of
        # cutoff.
        self._servo.dut_control('servo_pd_role:snk', check_error=False)
        self._servo.dut_control('servo_pd_role:src', check_error=False)
        self.restore_control('servo_pd_role')

        # Restore the ccd watchdog.
        self.enable_ccd_watchdog(self._original_watchdog_state)

    def restore_control(self, control):
        """Restore the control setting, if it has been saved.

        Args:
            control: The name of the servo control to restore.
        """
        setting = getattr(self, control, None)
        if setting is None:
            return
        self._servo.dut_control('%s:%s' % (control, setting))

    def ccd_watchdog_enabled(self):
        """Return True if servod is monitoring ccd"""
        if not self._ccd_device:
            return False
        watchdog_state = self._servo.dut_control('watchdog')[1]
        logging.debug(watchdog_state)
        return not re.search(self._ccd_device + ':.*disconnect ok',
                             watchdog_state)

    def enable_ccd_watchdog(self, enable):
        """Control the CCD watchdog.

        Servo will die if it's watching CCD and cr50 is held in reset. Disable
        the CCD watchdog, so it's ok for CCD to disconnect.

        This function does nothing if ccd_cr50 isn't in the servo type.

        Args:
            enable: If True, enable the CCD watchdog. Otherwise disable it.
        """
        if not self._ccd_device:
            logging.debug('Servo is not watching ccd device.')
            return

        if enable:
            self._servo.dut_control('watchdog_add:' + self._ccd_device)
        else:
            self._servo.dut_control('watchdog_remove:' + self._ccd_device)

        if self.ccd_watchdog_enabled() != enable:
            raise Error('Could not %sable ccd watchdog' %
                        ('en' if enable else 'dis'))

    def enter_reset(self):
        """Disable the CCD watchdog then run the reset cr50 function."""
        logging.info('Using %r to enter reset', self._reset_name)
        # Disable the CCD watchdog before putting servo into reset otherwise
        # servo will die in the middle of flashing cr50.
        self.enable_ccd_watchdog(False)
        try:
            self.run_reset()
        except Exception as e:
            logging.warning('%s enter reset failed: %s', self._reset_name, e)
            raise

    def exit_reset(self):
        """Exit cr50 reset."""
        logging.info('Recovering from %s', self._reset_name)
        try:
            self.recover_from_reset()
        except Exception as e:
            logging.warning('%s exit reset failed: %s', self._reset_name, e)
            raise

    def run_reset(self):
        """Start the cr50 reset process.

        Cr50 doesn't have to enter reset in this function. It just needs to do
        whatever setup is necessary for the exit reset function.
        """
        raise NotImplementedError()

    def recover_from_reset(self):
        """Recover from Cr50 reset.

        Cr50 has to hard or power-on reset during this function for rescue to
        work. Uart is disabled on deep sleep recovery, so deep sleep is not a
        valid reset.
        """
        raise NotImplementedError()


class Cr50ResetODLReset(Cr50Reset):
    """Class for using the servo cr50_reset_odl to reset cr50."""

    SIGNAL = 'cr50_reset_odl'
    REQUIRED_SETUP = [
        # Rescue is done through Cr50 uart. It requires a flex cable not ccd.
        'flex',
        # Cr50 rescue is done through cr50 uart.
        'cr50_uart',
    ]

    def __init__(self, servo, name):
        # Make sure the reset signal exists in the servo setup.
        self.REQUIRED_SETUP.append(self.SIGNAL)
        super().__init__(servo, name)

    def cleanup(self):
        """Use the Cr50 reset signal to hold Cr50 in reset."""
        try:
            self.restore_control(self.SIGNAL)
        finally:
            super().cleanup()

    def set_signal(self, signal):
        """Set the dut control."""
        logging.info("Setting %s", signal)
        self._servo.dut_control(signal)

    def run_reset(self):
        """Use cr50_reset_odl to hold Cr50 in reset."""
        self.set_signal('%s:on' % self.SIGNAL)

    def recover_from_reset(self):
        """Release the reset signal."""
        self.set_signal('%s:off' % self.SIGNAL)

class PCHDisableReset(Cr50ResetODLReset):
    """Class for using the servo pch_disable to reset cr50."""
    SIGNAL = 'pch_disable'


class BatteryCutoffReset(Cr50Reset):
    """Class for using a battery cutoff through EC commands to reset cr50."""

    REQUIRED_SETUP = (
        # Rescue is done through Cr50 uart. It requires a flex cable not ccd.
        'flex',
        # We need type c servo v4 to recover from battery_cutoff.
        'type-c_servo_v4',
        # Cr50 rescue is done through cr50 uart.
        'cr50_uart',
        # EC console needs to be read-write to issue cutoff command.
        'ec_uart',
    )
    CHECK_EC_RETRIES = 5
    WAIT_EC = 3

    def run_reset(self):
        """Use EC commands to cutoff the battery."""
        self._servo.dut_control('servo_pd_role:snk')

        if self._servo.dut_control('ec_board', check_error=False)[0]:
            logging.warning('EC is unresponsive. Cutoff may not work.')

        self._servo.dut_control('ec_uart_cmd:cutoff', check_error=False,
                                wait=True)
        self._servo.dut_control('ec_uart_cmd:reboot', check_error=False,
                                wait=True)

        for i in range(self.CHECK_EC_RETRIES):
            time.sleep(self.WAIT_EC)
            if self._servo.dut_control('ec_board', check_error=False)[0]:
                logging.info('Device is cutoff')
                return
            logging.info('%d: EC still responsive', i)
        raise Error('EC still responsive after cutoff')

    def recover_from_reset(self):
        """Connect power using servo v4 to recover from cutoff."""
        logging.info('"Connecting" adapter')
        self._servo.dut_control('servo_pd_role:src', wait=True)


class ManualReset(Cr50Reset):
    """Class for using a manual reset to reset Cr50."""

    REQUIRED_SETUP = (
        # Rescue is done through Cr50 uart. It requires a flex cable not ccd.
        'flex',
        # Cr50 rescue is done through cr50 uart.
        'cr50_uart',
    )

    PROMPT_WAIT = 5
    USER_RESET_TIMEOUT = 60

    def run_reset(self):
        """Nothing to do. User will reset cr50."""

    def recover_from_reset(self):
        """Wait for the user to reset cr50."""
        end_time = time.time() + self.USER_RESET_TIMEOUT
        while time.time() < end_time:
            logging.info('Press enter after you reset cr50')
            user_input = select.select([sys.stdin], [], [], self.PROMPT_WAIT)[0]
            if user_input:
                logging.info('User reset done')
                return
        logging.warning('User input timeout: assuming cr50 reset')


class ConsoleReboot(Cr50Reset):
    """Class for using a manual reset to reset Cr50."""

    REQUIRED_SETUP = (
        # Rescue is done through Cr50 uart. It requires a flex cable not ccd.
        'flex',
        # Cr50 rescue is done through cr50 uart.
        'cr50_uart',
    )
    REBOOT_CMD = '\n\nreboot\n\n'

    def run_reset(self):
        """Nothing to do."""

    def recover_from_reset(self):
        """Run reboot on the cr50 console."""
        # EC3PO is disconnected. Send reboot to the raw pty.
        raw_pty = self._servo.dut_control('raw_cr50_uart_pty')[1]
        logging.info('Sending cr50 reboot command %r', self.REBOOT_CMD)
        with serial.Serial(raw_pty, timeout=1) as ser:
            ser.write(self.REBOOT_CMD.encode('utf-8'))


class FlashCr50():
    """Class for updating cr50."""

    NAME = 'FlashCr50'
    PACKAGE = ''
    DEFAULT_UPDATER = ''

    def __init__(self, cmd):
        """Verify the update command exists.

        Args:
            cmd: The updater command.

        Raises:
            Error if no valid updater command was found.
        """
        updater = self.get_update_cmd(cmd)
        if not updater:
            emerge_msg = (('Try emerging ' + self.PACKAGE) if self.PACKAGE
                          else '')
            raise Error('Could not find %s command.%s' % (self, emerge_msg))
        self._updater = updater

    def get_update_cmd(self, cmd):
        """Find a valid updater command.

        Args:
            cmd: the updater command.

        Returns:
            A command string or None if none of the commands ran successfully.
            The command string will be the one supplied or the DEFAULT_UPDATER
            command.
        """
        if self.is_valid_update_cmd(cmd):
            return cmd

        use_default = (self.DEFAULT_UPDATER and
                       self.is_valid_update_cmd(self.DEFAULT_UPDATER))
        if use_default:
            logging.debug('%r failed using %r to update.', cmd,
                          self.DEFAULT_UPDATER)
            return self.DEFAULT_UPDATER
        return None

    @staticmethod
    def is_valid_update_cmd(cmd):
        """Verify the updater command.

        Returns:
            returns True if the command worked. False if it didn't.
        """
        logging.debug('Testing update command %r.', cmd)
        exit_status, output = run_command([cmd, '-h'], check_error=False)
        if 'Usage' in output:
            return True
        if exit_status:
            logging.debug('Could not run %r (%s): %s', cmd, exit_status, output)
        return False

    def update(self, image):
        """Try to update cr50 to the given image."""
        raise NotImplementedError()

    def __str__(self):
        """Use the updater name for the tostring."""
        return self.NAME


class GsctoolUpdater(FlashCr50):
    """Class to flash cr50 using gsctool."""

    NAME = 'gsctool'
    PACKAGE = 'ec-utils'
    DEFAULT_UPDATER = '/usr/sbin/gsctool'

    # Common failures exit with this status. Use STANDARD_ERRORS to map the
    # exit status to reasons for the failure.
    STANDARD_ERROR_REGEX = r'Error: status (\S+)'
    STANDARD_ERRORS = {
        '0x8': 'Rejected image with old header.',
        '0x9': 'Update too soon.',
        '0xc': 'Board id mismatch',
    }

    def __init__(self, cmd, usb_ser=None):
        """Generate the gsctool command.

        Args:
            cmd: gsctool updater command.
            usb_ser: The usb_ser number of the CCD device being updated.
        """
        super().__init__(cmd)
        self._gsctool_cmd = [self._updater]
        if usb_ser:
            self._gsctool_cmd.extend(['-n', usb_ser])

    def update(self, image):
        """Use gsctool to update cr50.

        Args:
            image: Cr50Image object.
        """
        update_cmd = self._gsctool_cmd[:]
        update_cmd.append(image.get_bin())
        exit_status, output = run_command(update_cmd, check_error=False)
        if not exit_status or (exit_status == 1 and 'image updated' in output):
            logging.info('update ok')
            return
        if exit_status == 3:
            match = re.search(self.STANDARD_ERROR_REGEX, output)
            if match:
                update_error = match.group(1)
                logging.info('Update error %s', update_error)
                raise Error(self.STANDARD_ERRORS[update_error])
        raise Error('gsctool update error: %s' % output.splitlines()[-1])


class Cr50RescueUpdater(FlashCr50):
    """Class to flash cr50 through servo micro uart."""

    NAME = 'cr50-rescue'
    PACKAGE = 'cr50-utils'
    DEFAULT_UPDATER = '/usr/bin/cr50-rescue'

    WAIT_FOR_UPDATE = CMD_TIMEOUT
    RESCUE_RESET_DELAY = 5

    def __init__(self, cmd, port, reset_type):
        """Initialize cr50-rescue updater.

        cr50-rescue can only be done through servo, because it needs access to
        a lot of dut-controls and cr50 uart through servo micro. During rescue
        Cr50 has to do a hard reset, so the user should supply a valid reset
        method for the setup that's being used.

        Args:
            cmd: The cr50-rescue command.
            port: The servo port of the device being updated.
            reset_type: A string (one of SUPPORTED_RESETS) that describes how
                        to reset Cr50 during cr50-rescue.
        """
        super().__init__(cmd)
        self._servo = Servo(port)
        self._rescue_thread = None
        self._rescue_process = None
        self._cr50_reset = self.get_cr50_reset(reset_type)
        self._image = ''

    def get_cr50_reset(self, reset_type):
        """Get the cr50 reset object for the given reset_type.

        Args:
            reset_type: a string describing how cr50 will be reset. It must be
                        in SUPPORTED_RESETS.

        Returns:
            The Cr50Reset object for the given reset_type.
        """
        if reset_type == 'battery_cutoff':
            return BatteryCutoffReset(self._servo, reset_type)
        if reset_type == 'console_reboot':
            return ConsoleReboot(self._servo, reset_type)
        if reset_type == 'cr50_reset_odl':
            return Cr50ResetODLReset(self._servo, reset_type)
        if reset_type == 'pch_disable':
            return PCHDisableReset(self._servo, reset_type)
        return ManualReset(self._servo, reset_type)

    def _set_updater_image(self, image):
        """Returns the rw hex image"""
        if run_command(['grep', '-q', 'cr50', image.get_bin()], False)[0]:
            raise Error("%r unsupported 'brescue' to update ti50" % self.NAME)
        self._image = image.get_rw_hex()

    def _get_rescue_cmd(self, pty):
        """Return the cr50-rescue command."""
        return [self._updater, '-v', '-i', self._image, '-d', pty]

    def update(self, image):
        """Use cr50-rescue to update cr50 then cleanup.

        Args:
            image: Cr50Image object.
        """
        self._set_updater_image(image)
        try:
            self._run_update()
        finally:
            self._restore_state()

    def start_rescue_process(self):
        """Run cr50-rescue in a process, so it can be killed it if it hangs."""
        pty = self._servo.get_raw_cr50_pty()

        rescue_cmd = self._get_rescue_cmd(pty)
        logging.info('Starting cr50-rescue: %s', ' '.join(rescue_cmd))

        self._rescue_process = subprocess.Popen(rescue_cmd)
        self._rescue_process.communicate()
        logging.info('Rescue Finished')

    def _start_rescue_thread(self):
        """Start cr50-rescue."""
        self._rescue_thread = threading.Thread(target=self.start_rescue_process)
        self._rescue_thread.start()

    def _run_update(self):
        """Run the Update"""
        # Enter reset before starting rescue, so any extra cr50 messages won't
        # interfere with cr50-rescue.
        self._cr50_reset.enter_reset()

        self._start_rescue_thread()

        time.sleep(self.RESCUE_RESET_DELAY)
        # Resume from cr50 reset.
        self._cr50_reset.exit_reset()

        self._rescue_thread.join(self.WAIT_FOR_UPDATE)

        logging.info('cr50_version:%s', self._servo.get_cr50_version())

    def _restore_state(self):
        """Try to get the device out of reset and restore all controls"""
        try:
            self._cr50_reset.cleanup()
        finally:
            self._cleanup_rescue_thread()

    def _cleanup_rescue_thread(self):
        """Cleanup the rescue process and handle any errors."""
        if not self._rescue_thread:
            return
        if self._rescue_thread.is_alive():
            logging.info('Killing cr50-rescue process')
            self._rescue_process.terminate()
            self._rescue_thread.join()

        self._rescue_thread = None
        if self._rescue_process.returncode:
            logging.info('cr50-rescue failed.')
            logging.info('stderr: %s', self._rescue_process.stderr)
            logging.info('stdout: %s', self._rescue_process.stdout)
            logging.info('returncode: %s', self._rescue_process.returncode)
            raise Error('cr50-rescue failed (%d)' %
                        self._rescue_process.returncode)


class BrescueUpdater(Cr50RescueUpdater):
    """Use brescue.sh to update gsc over uart."""

    NAME = 'brescue'
    BRESCUE_SCRIPT = 'brescue.sh'

    def __init__(self, cmd, port, reset_type):
        """Initialize brescue updater.

        Args:
            cmd: The cr50-rescue command.
            port: The servo port of the device being updated.
            reset_type: A string (one of SUPPORTED_RESETS) that describes how
                        to reset Cr50 during cr50-rescue.
        """
        # Make sure cr50-rescue exists, since brescue relies on it.
        super().__init__('cr50-rescue', port, reset_type)
        if cmd != self.NAME:
            self._updater = cmd
        else:
            script_dir = os.path.dirname(os.path.realpath(__file__))
            self._updater = os.path.join(script_dir, self.BRESCUE_SCRIPT)
        if not os.path.exists(self._updater):
            raise Error('%s does not exist' % self._updater)

    def _set_updater_image(self, image):
        """brescue converts the image into a hex file."""
        self._image = image.get_bin()

    def _get_rescue_cmd(self, pty):
        """Return the brescue command."""
        return [self._updater, self._image, pty]


def parse_args(argv):
    """Parse commandline arguments.

    Args:
        argv: command line args

    Returns:
        options: an argparse.Namespace.
    """
    usage = ('%s -i $IMAGE [ -c cr50-rescue -p $SERVO_PORT [ -r '
             '$RESET_METHOD]]' % os.path.basename(argv[0]))
    parser = argparse.ArgumentParser(usage=usage, description=__doc__)
    parser.add_argument('-d', '--debug', action='store_true', default=False,
                        help='enable debug messages.')
    parser.add_argument('-i', '--image', type=str,
                        help='Path to cr50 binary image.')
    parser.add_argument('-R', '--release', type=str,
                        choices=RELEASE_PATHS.keys(),
                        help='Type of cr50 release. Use instead of the image '
                        'arg.')
    parser.add_argument('-c', '--updater-cmd', type=str, default=UPDATERS[0],
                        help='Tool to update cr50. Supply a %r path' % UPDATERS)
    parser.add_argument('-s', '--serial', type=str, default='',
                        help='serial number to pass to gsctool.')
    parser.add_argument('-p', '--port', type=str, default='',
                        help='port servod is listening on (required for '
                        'rescue).')
    parser.add_argument('-r', '--reset-type', default='battery_cutoff',
                        choices=SUPPORTED_RESETS,
                        type=str, help='The method for cr50 reset.')
    parser.add_argument('-a', '--artifacts-dir', default=None, type=str,
                        help='Location to store artifacts')
    opts = parser.parse_args(argv[1:])
    if 'cr50-rescue' in opts.updater_cmd and not opts.port:
        raise parser.error('Servo port is required for cr50 rescue')
    return opts


def get_updater(opts):
    """Get the updater object."""
    if 'brescue' in opts.updater_cmd:
        return BrescueUpdater(opts.updater_cmd, opts.port, opts.reset_type)
    if 'cr50-rescue' in opts.updater_cmd:
        return Cr50RescueUpdater(opts.updater_cmd, opts.port, opts.reset_type)
    if 'gsctool' in opts.updater_cmd:
        return GsctoolUpdater(opts.updater_cmd, opts.serial)
    raise Error('Unsupported update command %r' % opts.updater_cmd)


def main(argv):
    """Update cr50 using gsctool or cr50-rescue."""
    opts = parse_args(argv)

    loglevel = logging.INFO
    log_format = '%(asctime)s - %(levelname)7s'
    if opts.debug:
        loglevel = logging.DEBUG
        log_format += ' - %(lineno)3d:%(funcName)-15s'
    log_format += ' - %(message)s'
    logging.basicConfig(level=loglevel, format=log_format)

    image = Cr50Image(RELEASE_PATHS.get(opts.release, opts.image),
                      opts.artifacts_dir)
    flash_cr50 = get_updater(opts)

    logging.info('Using %s to update to %s', flash_cr50,
                 image.get_original_basename())
    flash_cr50.update(image)


if __name__ == '__main__':
    sys.exit(main(sys.argv))
