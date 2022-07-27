# Copyright 2021 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Code to generate the ec_version.h file."""

import datetime
import getpass
import io
import os
import platform
import subprocess

import zmake.util as util


def _get_num_commits(repo):
    """Get the number of commits that have been made.

    If a Git repository is available, return the number of commits that have
    been made. Otherwise return a fixed count.

    Args:
        repo: The path to the git repo.

    Returns:
        An integer, the number of commits that have been made.
    """
    try:
        result = subprocess.run(
            ["git", "-C", repo, "rev-list", "HEAD", "--count"],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            encoding="utf-8",
        )
    except subprocess.CalledProcessError:
        commits = "9999"
    else:
        commits = result.stdout

    return int(commits)


def _get_revision(repo):
    """Get the current revision hash.

    If a Git repository is available, return the hash of the current index.
    Otherwise return the hash of the VCSID environment variable provided by
    the packaging system.

    Args:
        repo: The path to the git repo.

    Returns:
        A string, of the current revision.
    """
    try:
        result = subprocess.run(
            ["git", "-C", repo, "log", "-n1", "--format=%H"],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            encoding="utf-8",
        )
    except subprocess.CalledProcessError:
        # Fall back to the VCSID provided by the packaging system.
        # Format is 0.0.1-r425-032666c418782c14fe912ba6d9f98ffdf0b941e9 for
        # releases and 9999-032666c418782c14fe912ba6d9f98ffdf0b941e9 for
        # 9999 ebuilds.
        vcsid = os.environ.get("VCSID", "9999-unknown")
        revision = vcsid.rsplit("-", 1)[1]
    else:
        revision = result.stdout

    return revision


def get_version_string(project, zephyr_base, modules, static=False):
    """Get the version string associated with a build.

    Args:
        project: a string project name
        zephyr_base: the path to the zephyr directory
        modules: a dictionary mapping module names to module paths
        static: if set, create a version string not dependent on git
            commits, thus allowing binaries to be compared between two
            commits.

    Returns:
        A version string which can be placed in FRID, FWID, or used in
        the build for the OS.
    """
    major_version, minor_version, *_ = util.read_zephyr_version(zephyr_base)
    num_commits = 0

    if static:
        vcs_hashes = "STATIC"
    else:
        repos = {
            "os": zephyr_base,
            **modules,
        }

        for repo in repos.values():
            num_commits += _get_num_commits(repo)

        vcs_hashes = ",".join(
            "{}:{}".format(name, _get_revision(repo)[:6])
            for name, repo in sorted(
                repos.items(),
                # Put the EC module first, then Zephyr OS kernel, as
                # these are probably the most important hashes to
                # developers.
                key=lambda p: (p[0] != "ec", p[0] != "os", p),
            )
        )

    return "{}_v{}.{}.{}-{}".format(
        project,
        major_version,
        minor_version,
        num_commits,
        vcs_hashes,
    )


def write_version_header(version_str, output_path, tool, static=False):
    """Generate a version header and write it to the specified path.

    Generate a version header in the format expected by the EC build
    system, and write it out only if the version header does not exist
    or changes.  We don't write in the case that the version header
    does exist and was unchanged, which allows "zmake build" commands
    on an unchanged tree to be an effective no-op.

    Args:
        version_str: The version string to be used in the header, such
            as one generated by get_version_string.
        output_path: The file path to write at (a pathlib.Path
            object).
        tool: Name of the tool that is invoking this function ("zmake",
            "generate_ec_version.py", etc). Included in a comment in the
            header.
        static: If true, generate a header which does not include
            information like the username, hostname, or date, allowing
            the build to be reproducible.
    """
    output = io.StringIO()
    output.write(f"/* This file is automatically generated by {tool} */\n")

    def add_def(name, value):
        output.write("#define {} {}\n".format(name, util.c_str(value)))

    def add_def_unquoted(name, value):
        output.write("#define {} {}\n".format(name, value))

    add_def("VERSION", version_str)
    add_def("CROS_EC_VERSION32", version_str[:31])

    if static:
        add_def("BUILDER", "reproducible@build")
        add_def("DATE", "STATIC_VERSION_DATE")
    else:
        add_def("BUILDER", "{}@{}".format(getpass.getuser(), platform.node()))
        add_def("DATE", datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))

    # TODO(b/198475757): Add zmake support for getting CROS_FWID32
    add_def_unquoted("CROS_FWID32", "CROS_FWID_MISSING_STR")

    contents = output.getvalue()
    if not output_path.exists() or output_path.read_text() != contents:
        output_path.write_text(contents)
