#!/usr/bin/env python3
# Copyright 2023 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Build, bundle, or test all of the EC boards.

This is the entry point for the custom firmware builder workflow recipe.  It
gets invoked by chromite/api/controller/firmware.py.
"""

import argparse
import multiprocessing
import os
import subprocess
import sys

# pylint: disable=import-error
from google.protobuf import json_format

# TODO(crbug/1181505): Code outside of chromite should not be importing from
# chromite.api.gen.  Import json_format after that so we get the matching one.
from chromite.api.gen.chromite.api import firmware_pb2


def build(opts):
    """Builds all targets in extra/usb_updater

    Note that when we are building unit tests for code coverage, we don't
    need this step. It builds EC **firmware** targets, but unit tests with
    code coverage are all host-based. So if the --code-coverage flag is set,
    we don't need to build the firmware targets and we can return without
    doing anything but creating the metrics file and giving an informational
    message.
    """
    # Write empty metrics file as there is nothing to report but recipe needs
    # the file to exist.
    metrics = firmware_pb2.FwBuildMetricList()  # pylint: disable=no-member
    with open(opts.metrics, "w", encoding="utf-8") as f:
        f.write(json_format.MessageToJson(metrics))

    cmd = [
        "make",
        "BOARD=cr50",
        f"-j{opts.cpus}",
        "-C",
        "extra/usb_updater",
    ]
    print(f'# Running {" ".join(cmd)}.')
    subprocess.run(cmd, cwd=os.path.dirname(__file__), check=True)


def bundle(opts):
    """Bundles all of the EC targets."""
    # Provide an empty metadata file since the file is required, but we
    # don't have any artifacts that needs to be uploadeddd
    if opts.metadata:
        metadata = (
            firmware_pb2.FirmwareArtifactInfo()  # pylint: disable=no-member
        )
        with open(opts.metadata, "w", encoding="utf-8") as f:
            f.write(json_format.MessageToJson(metadata))


def test(opts):
    """Tests all of the EC targets."""
    del opts  # Unused.


def main(args):
    """Builds, bundles, or tests all of the EC targets.

    Additionally, the tool reports build metrics.
    """
    opts = parse_args(args)

    if not hasattr(opts, "func"):
        print("Must select a valid sub command!")
        return -1

    # Run selected sub command function
    try:
        opts.func(opts)
    except subprocess.CalledProcessError:
        return 1
    else:
        return 0


def parse_args(args):
    """Parses command-line arguments."""
    parser = argparse.ArgumentParser(description=__doc__)

    parser.add_argument(
        "--cpus",
        default=multiprocessing.cpu_count(),
        help="The number of cores to use.",
    )

    parser.add_argument(
        "--metrics",
        dest="metrics",
        required=True,
        help="File to write the json-encoded MetricsList proto message.",
    )

    parser.add_argument(
        "--metadata",
        required=False,
        help="Full pathname for the file in which to write build artifact "
        "metadata.",
    )

    parser.add_argument(
        "--output-dir",
        required=False,
        help="Full pathanme for the directory in which to bundle build "
        "artifacts.",
    )

    parser.add_argument(
        "--code-coverage",
        required=False,
        action="store_true",
        help="Build host-based unit tests for code coverage.",
    )

    parser.add_argument(
        "--bcs-version",
        dest="bcs_version",
        default="",
        required=False,
        # TODO(b/180008931): make this required=True.
        help="BCS version to include in metadata.",
    )

    # Would make this required=True, but not available until 3.7
    sub_cmds = parser.add_subparsers()

    build_cmd = sub_cmds.add_parser("build", help="Builds all firmware targets")
    build_cmd.set_defaults(func=build)

    build_cmd = sub_cmds.add_parser(
        "bundle", help="Does nothing, kept for compatibility"
    )
    build_cmd.set_defaults(func=bundle)

    test_cmd = sub_cmds.add_parser(
        "test", help="Does nothing, kept for compatibility"
    )
    test_cmd.set_defaults(func=test)

    return parser.parse_args(args)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
