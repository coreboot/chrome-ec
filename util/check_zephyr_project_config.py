#!/usr/bin/env python3

# Copyright 2023 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Validate Zephyr project configuration files."""

import argparse
import logging
import os
import pathlib
import site
import sys
import tempfile

EC_BASE = pathlib.Path(__file__).parent.parent

if "ZEPHYR_BASE" in os.environ:
    ZEPHYR_BASE = pathlib.Path(os.environ.get("ZEPHYR_BASE"))
else:
    ZEPHYR_BASE = pathlib.Path(
        EC_BASE.resolve().parent.parent / "third_party" / "zephyr" / "main"
    )

site.addsitedir(ZEPHYR_BASE / "scripts")
site.addsitedir(ZEPHYR_BASE / "scripts" / "kconfig")
# pylint:disable=import-error,wrong-import-position
import kconfiglib
import zephyr_module

# pylint:enable=import-error,wrong-import-position

# Known configuration file extensions.
CONF_FILE_EXT = (".conf", ".overlay", "_defconfig")


def _parse_args(argv):
    parser = argparse.ArgumentParser(description=__doc__)

    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Verbose Output"
    )
    parser.add_argument(
        "-d",
        "--dt-has",
        action="store_true",
        help="Check for options that depends on a DT_HAS_..._ENABLE symbol.",
    )
    parser.add_argument(
        "CONFIG_FILE",
        nargs="*",
        help="List of config files to be checked, non config files are ignored.",
        type=pathlib.Path,
    )

    return parser.parse_args(argv)


def _init_log(verbose):
    """Initialize a logger object."""
    console = logging.StreamHandler()
    console.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))

    log = logging.getLogger(__file__)
    log.addHandler(console)

    if verbose:
        log.setLevel(logging.DEBUG)

    return log


class KconfigCheck:
    """Validate Zephyr project configuration files.

    Attributes:
        verbose: whether to enable verbose mode logging
    """

    def __init__(self, verbose):
        self.log = _init_log(verbose)
        self.fail_count = 0

        # Preload the upstream Kconfig.
        self.program_kconf = {None: self._init_kconfig(None)}

    def _init_kconfig(self, filename):
        """Initialize a kconfiglib object with all boards and arch options.

        Args:
            filename: the path of the Kconfig file to load.

        Returns:
            A kconfiglib.Kconfig object.
        """
        with tempfile.TemporaryDirectory() as temp_dir:
            modules = zephyr_module.parse_modules(
                ZEPHYR_BASE, extra_modules=[EC_BASE]
            )

            kconfig = ""
            for module in modules:
                kconfig += zephyr_module.process_kconfig(
                    module.project, module.meta
                )

            # generate Kconfig.modules file
            with open(pathlib.Path(temp_dir) / "Kconfig.modules", "w") as file:
                file.write(kconfig)

            # generate empty Kconfig.dts file
            with open(pathlib.Path(temp_dir) / "Kconfig.dts", "w") as file:
                file.write("")

            os.environ["ZEPHYR_BASE"] = str(ZEPHYR_BASE)
            os.environ["srctree"] = str(ZEPHYR_BASE)
            os.environ["KCONFIG_BINARY_DIR"] = temp_dir
            os.environ["ARCH_DIR"] = "arch"
            os.environ["ARCH"] = "*"
            os.environ["BOARD_DIR"] = "boards/*/*"

            if not filename:
                filename = os.path.join(ZEPHYR_BASE, "Kconfig")

            self.log.info("Loading Kconfig: %s", filename)

            return kconfiglib.Kconfig(filename)

    def _kconf_from_path(self, path):
        """Return a Kconfig object for the specified path.

        If path resides under zephyr/program, find the name of the program and
        look for a corresponding program specific Kconfig file. If one is
        present, return a corresponding Kconfig object for the program.

        Stores a list of per-program Kconfig objects internally, so each
        program Kconfig is only loaded once.

        Args:
            path: the path of the Kconfig file to load.

        Returns:
            A kconfiglib.Kconfig object.
        """
        program_path = pathlib.Path(EC_BASE, "zephyr", "program")
        file_path = pathlib.Path(path).resolve()

        program = None
        program_kconfig = None
        if program_path in file_path.parents:
            idx = file_path.parents.index(program_path)
            program = file_path.parts[-(idx + 1)]
            kconfig_path = pathlib.Path(program_path, program, "Kconfig")
            if kconfig_path.is_file():
                program_kconfig = kconfig_path

        self.log.info(
            "Path: %s, program: %s, program_kconfig: %s",
            path,
            program,
            program_kconfig,
        )

        if program not in self.program_kconf:
            if not program_kconfig:
                self.program_kconf[program] = self.program_kconf[None]
            else:
                self.program_kconf[program] = self._init_kconfig(
                    program_kconfig
                )

        return self.program_kconf[program]

    def _fail(self, *args):
        """Report a fail in the error log and increment the fail counter."""
        self.fail_count += 1
        self.log.error(*args)

    def _filter_config_files(self, files):
        """Yield files with known config suffixes from the command line."""
        for file in files:
            if not file.exists():
                self.log.info("Ignoring %s: file has been removed", file)
                continue

            if not file.name.endswith(CONF_FILE_EXT):
                self.log.info("Ignoring %s: unrecognized suffix", file)
                continue

            yield file

    def _check_dt_has(self, file_name):
        """Check file_name for known automatic config options.

        Check file_name for any explicitly enabled option that has a dependency
        on a devicetree symbol. These are normally enabled automatically so
        there's no point enabling them explicitly.
        """
        kconf = self._kconf_from_path(file_name)

        symbols = {}
        for name, val in kconf.syms.items():
            dep = kconfiglib.expr_str(val.direct_dep)
            if "DT_HAS_" in dep:
                symbols[name] = dep

        self.log.info("Checking %s", file_name)

        with open(file_name, "r") as file:
            for line_num, line in enumerate(file.readlines(), start=1):
                for name in symbols:
                    match = f"CONFIG_{name}=y"
                    if line.startswith(match):
                        dep = symbols[name]
                        self._fail(
                            "%s:%d: unnecessary config option %s (depends on %s)",
                            file_name,
                            line_num,
                            match,
                            dep,
                        )

    def run_checks(self, files, dt_has):
        """Run all config checks."""
        config_files = self._filter_config_files(files)

        for file in config_files:
            if dt_has:
                self._check_dt_has(file)

        return self.fail_count


def main(argv):
    """Main function"""
    args = _parse_args(argv)

    kconfig_checker = KconfigCheck(args.verbose)

    return kconfig_checker.run_checks(args.CONFIG_FILE, args.dt_has)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
