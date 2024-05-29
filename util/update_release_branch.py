#!/usr/bin/env python3
# Copyright 2021 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Release branch updater tool.

This is a tool to merge from the main branch into a release branch.

Inspired by the fingerprint release process:
http://go/cros-fingerprint-firmware-branching-and-signing and now used by other
boards.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import textwrap


BUG_NONE_PATTERN = re.compile("none", flags=re.IGNORECASE)


def git_commit_msg(cros_main, branch, head, merge_head, rel_paths, cmd):
    """Generates a merge commit message based off of relevant changes.

    This function obtains the relevant commits from the given relative paths in
    order to extract the bug numbers. It constructs the git commit message
    showing the command used to find the relevant commits.

    Args:
        cros_main: String indicating the origin branch name
        branch: String indicating the release branch name
        head: String indicating the HEAD refspec
        merge_head: String indicating the merge branch refspec.
        rel_paths: String containing all the relevant paths for this particular
                   baseboard or board.
        cmd: String of the input command.

    Returns:
        A String containing the git commit message with the exception of the
        Signed-Off-By field and Change-ID field.
    """
    relevant_commits_cmd, relevant_commits, relevant_hdr = get_relevant_commits(
        head, merge_head, "--oneline", rel_paths
    )

    _, relevant_bugs, _ = get_relevant_commits(head, merge_head, "", rel_paths)
    relevant_bugs = set(re.findall("BUG=(.*)", relevant_bugs))
    # Filter out "none" from set of bugs
    filtered = []
    for bug_line in relevant_bugs:
        bug_line = bug_line.replace(",", " ")
        bugs = bug_line.split(" ")
        for bug in bugs:
            if bug and not BUG_NONE_PATTERN.match(bug):
                filtered.append(bug)
    relevant_bugs = filtered

    # TODO(b/179509333): remove Cq-Include-Trybots line when regular CQ and
    # firmware CQ do not behave differently.
    commit_msg_template = """
Merge remote-tracking branch {CROS_MAIN} into {BRANCH}

Generated by: {COMMAND_LINE}

{RELEVANT_COMMITS_HEADER}

{RELEVANT_COMMITS_CMD}

{RELEVANT_COMMITS}

{BUG_FIELD}
TEST=`make -j buildall`

Force-Relevant-Builds: all
"""
    # Wrap the commands and bug field such that we don't exceed 72 cols.
    relevant_commits_cmd = textwrap.fill(relevant_commits_cmd, width=72)
    cmd = textwrap.fill(cmd, width=72)
    # Wrap at 68 cols to save room for 'BUG='
    bugs = textwrap.wrap(" ".join(relevant_bugs), width=68)
    bug_field = ""
    for line in bugs:
        bug_field += "BUG=" + line + "\n"
    # Remove the final newline since the template adds it for us.
    bug_field = bug_field[:-1]

    return commit_msg_template.format(
        CROS_MAIN=cros_main,
        BRANCH=branch,
        RELEVANT_COMMITS_CMD=relevant_commits_cmd,
        RELEVANT_COMMITS=relevant_commits,
        RELEVANT_COMMITS_HEADER=relevant_hdr,
        BUG_FIELD=bug_field,
        COMMAND_LINE=cmd,
    )


def get_relevant_boards(baseboard):
    """Searches the EC repo looking for boards that use the given baseboard.

    Args:
        baseboard: String containing the baseboard to consider

    Returns:
        A list of strings containing the boards based off of the baseboard.
    """
    proc = subprocess.run(
        ["git", "grep", "BASEBOARD:=" + baseboard, "--", "board/"],
        stdout=subprocess.PIPE,
        encoding="utf-8",
        check=True,
    )
    boards = []
    res = proc.stdout.splitlines()
    for line in res:
        boards.append(line.split("/")[1])
    return boards


def get_relevant_commits(head, merge_head, fmt, relevant_paths):
    """Find relevant changes since last merge.

    Searches git history to find changes since the last merge which modify
    files present in relevant_paths.

    Args:
        head: String indicating the HEAD refspec
        merge_head: String indicating the merge branch refspec.
        fmt: An optional string containing the format option for `git log`
        relevant_paths: String containing all the relevant paths for this
                        particular baseboard or board.

    Returns:
        A tuple containing the arguments passed to the git log command and
        stdout, and the header for the message
    """
    if not relevant_paths:
        return "", "", ""
    if fmt:
        cmd = [
            "git",
            "log",
            fmt,
            head + ".." + merge_head,
            "--",
            relevant_paths,
        ]
    else:
        cmd = ["git", "log", head + ".." + merge_head, "--", relevant_paths]

    # Pass cmd as a string to subprocess.run() since we need to run with shell
    # equal to True.  The reason we are using shell equal to True is to take
    # advantage of the glob expansion for the relevant paths.
    cmd = " ".join(cmd)
    proc = subprocess.run(
        cmd, stdout=subprocess.PIPE, encoding="utf-8", check=True, shell=True
    )
    return "".join(proc.args), proc.stdout, "Relevant changes:"


def merge_repo(
    base, cros_main, cmd_checkout, strategy, cmd, prunelist, relevant_paths
):
    """Merge changes from ToT into this repo's branch.

    For this repo, merge the changes from ToT to the branch set
    in the merge command.

    Args:
        base: String indicating the Source directory of repo.
        cros_main: String indicating the origin branch name
        cmd_checkout: String list containing the checkout command to use.
        strategy: String list containing the merge strategy,
        cmd: String containing the command line
        prunelist: String list containing the files to remove.
        relevant_paths: String containing all the relevant paths for this
                        particular baseboard or board.
    """
    # Change directory to the repo being merged
    print(f'Starting merge in "{base}"')
    print(f'Checkout command: "{" ".join(cmd_checkout)}"')
    os.chdir(base)
    # Check if we are already in merge process
    result = subprocess.run(
        ["git", "rev-parse", "--quiet", "--verify", "MERGE_HEAD"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )

    if result.returncode:
        # Let's perform the merge
        print("Updating remote...")
        subprocess.run(["git", "remote", "update"], check=True)
        subprocess.run(cmd_checkout, check=True)
        cmd_merge = [
            "git",
            "merge",
            "--no-ff",
            "--no-commit",
            cros_main,
            "-s",
        ]
        cmd_merge.extend(strategy)
        print(f'Merge command: "{" ".join(cmd_merge)}"')
        try:
            subprocess.run(cmd_merge, check=True)
        except subprocess.CalledProcessError:
            # We've likely encountered a merge conflict due to new OWNERS file
            # modifications. If we're removing the owners, we'll delete them.
            if prunelist:
                # Find the unmerged files
                unmerged = (
                    subprocess.run(
                        ["git", "diff", "--name-only", "--diff-filter=U"],
                        stdout=subprocess.PIPE,
                        encoding="utf-8",
                        check=True,
                    )
                    .stdout.rstrip()
                    .split()
                )

                # Prune OWNERS files
                for file in unmerged:
                    if file in prunelist:
                        subprocess.run(["git", "rm", file], check=False)
                        unmerged.remove(file)

                print("Removed non-root OWNERS files.")
                if unmerged:
                    print(
                        "Unmerged files still exist! You need to manually resolve this."
                    )
                    print("\n".join(unmerged))
                    sys.exit(1)
            else:
                raise
    else:
        print(
            "We have already started merge process.",
            "Attempt to generate commit.",
        )
    # Check whether any commit is needed.
    changes = subprocess.run(
        ["git", "status", "--porcelain"],
        stdout=subprocess.PIPE,
        encoding="utf-8",
        check=True,
    ).stdout.rstrip()
    if not changes:
        print("No changes have been found, skipping commit.")
        return

    print("Generating commit message...")
    branch = subprocess.run(
        ["git", "rev-parse", "--abbrev-ref", "HEAD"],
        stdout=subprocess.PIPE,
        encoding="utf-8",
        check=True,
    ).stdout.rstrip()
    head = subprocess.run(
        ["git", "rev-parse", "--short", "HEAD"],
        stdout=subprocess.PIPE,
        encoding="utf-8",
        check=True,
    ).stdout.rstrip()
    merge_head = subprocess.run(
        ["git", "rev-parse", "--short", "MERGE_HEAD"],
        stdout=subprocess.PIPE,
        encoding="utf-8",
        check=True,
    ).stdout.rstrip()

    print("Typing as fast as I can...")
    commit_msg = git_commit_msg(
        cros_main, branch, head, merge_head, relevant_paths, cmd
    )
    subprocess.run(["git", "commit", "-m", commit_msg], check=True)
    subprocess.run(["git", "commit", "--amend"], check=True)


def find_src_base():
    """Find the path to the base of the checkout (e.g., ~/chromiumos)."""
    for path in Path(__file__).resolve().parents:
        if (path / ".repo").is_dir():
            return path
    raise FileNotFoundError(
        "Unable to locate the checkout of the ChromiumOS source"
    )


def main(argv):
    """Generates a merge commit from ToT to a desired release branch.

    For the commit message, it finds all the commits that have modified a
    relevant path. By default this is the baseboard or board directory.  The
    user may optionally specify a path to a text file which contains a longer
    list of relevant files.  The format should be in the glob syntax that git
    log expects.

    Args:
        argv: A list of the command line arguments passed to this script.
    """
    # Set up argument parser.
    parser = argparse.ArgumentParser(
        description=(
            "A script that generates a "
            "merge commit from cros/main"
            " to a desired release "
            "branch.  By default, the "
            '"recursive" merge strategy '
            'with the "theirs" strategy '
            "option is used."
        )
    )
    parser.add_argument("--baseboard")
    parser.add_argument("--board")
    parser.add_argument(
        "release_branch",
        help=(
            "The name of the target release branch, "
            "without the trailing '-main'."
        ),
    )
    parser.add_argument(
        "--remote_prefix",
        help=(
            "The name of the remote branch prefix (default cros). "
            "Private repos typically use cros-internal instead."
        ),
        default="cros",
    )
    parser.add_argument(
        "--srcbase",
        help=("The base directory where the src tree exists."),
        default=find_src_base(),
    )
    parser.add_argument(
        "--relevant_paths_file",
        help=(
            "A path to a text file which includes other "
            "relevant paths of interest for this board "
            "or baseboard"
        ),
    )
    parser.add_argument(
        "--merge_strategy",
        "-s",
        default="recursive",
        help="The merge strategy to pass to `git merge -s`",
    )
    parser.add_argument(
        "--strategy_option",
        "-X",
        help=("The strategy option for the chosen merge strategy"),
    )
    parser.add_argument(
        "--remove_owners",
        "-r",
        action=("store_true"),
        help=("Remove non-root OWNERS level files if present"),
    )
    parser.add_argument(
        "--zephyr",
        "-z",
        action=("store_true"),
        help=("If set, treat the board as a Zephyr based program"),
    )

    opts = parser.parse_args(argv[1:])

    baseboard_dir = ""
    board_dir = ""

    if opts.baseboard:
        # If a zephyr board, no baseboard allowed
        if opts.zephyr:
            raise Exception("--baseboard not allowed for Zephyr boards")
        # Dereference symlinks so "git log" works as expected.
        baseboard_dir = os.path.relpath("baseboard/" + opts.baseboard)
        baseboard_dir = os.path.relpath(os.path.realpath(baseboard_dir))

        boards = get_relevant_boards(opts.baseboard)
    elif opts.board:
        if opts.zephyr:
            board_dir = os.path.relpath("zephyr/program/" + opts.board)
        else:
            board_dir = os.path.relpath("board/" + opts.board)
        board_dir = os.path.relpath(os.path.realpath(board_dir))
        boards = [opts.board]
    else:
        # With no board or baseboard, not sure whether this should proceed
        raise Exception("no board or baseboard specified")

    print("Gathering relevant paths...")
    relevant_paths = []
    if opts.baseboard:
        relevant_paths.append(baseboard_dir)
    elif opts.board:
        relevant_paths.append(board_dir)

    if not opts.zephyr:
        for board in boards:
            relevant_paths.append("board/" + board)

    # Check for the existence of a file that has other paths of interest.
    # Also check for 'relevant-paths.txt' in the board directory
    if opts.relevant_paths_file and os.path.exists(opts.relevant_paths_file):
        with open(
            opts.relevant_paths_file, "r", encoding="utf-8"
        ) as relevant_paths_file:
            for line in relevant_paths_file:
                if not line.startswith("#"):
                    relevant_paths.append(line.rstrip())
    if os.path.exists("util/getversion.sh"):
        relevant_paths.append("util/getversion.sh")
    relevant_paths = " ".join(relevant_paths)

    # Prune OWNERS files if desired
    prunelist = []
    if opts.remove_owners:
        for root, dirs, files in os.walk("."):
            for name in dirs:
                if "build" in name:
                    continue
            for name in files:
                if "OWNERS" in name:
                    path = os.path.join(root, name)
                    prunelist.append(path[2:])  # Strip the "./"

        # Remove the top level OWNERS file from the prunelist.
        try:
            prunelist.remove("OWNERS")
        except ValueError:
            pass

        if prunelist:
            print("Not merging the following OWNERS files:")
            for path in prunelist:
                print("  " + path)

    # Create the merge and checkout commands to use.
    cmd_checkout = [
        "git",
        "checkout",
        "-B",
        opts.release_branch,
        opts.remote_prefix + "/" + opts.release_branch,
    ]
    if opts.merge_strategy == "recursive" and not opts.strategy_option:
        opts.strategy_option = "theirs"
    print(
        f'Using "{opts.merge_strategy}" merge strategy',
        (
            f"with strategy option '{opts.strategy_option if opts.strategy_option else ''}'"
        ),
    )
    cros_main = opts.remote_prefix + "/" + "main"
    strategy = [
        opts.merge_strategy,
    ]
    if opts.strategy_option:
        strategy.append("-X" + opts.strategy_option)
    cmd = " ".join(argv)

    # Merge each of the repos
    merge_repo(
        os.path.join(opts.srcbase, "src/platform/ec"),
        cros_main,
        cmd_checkout,
        strategy,
        cmd,
        prunelist,
        relevant_paths,
    )
    if opts.zephyr:
        # Strip off any trailing -main or -master from branch name
        if opts.release_branch.endswith("-main"):
            opts.release_branch = opts.release_branch[:-5]
        if opts.release_branch.endswith("-master"):
            opts.release_branch = opts.release_branch[:-7]
        cmd_checkout = [
            "git",
            "checkout",
            "-B",
            opts.release_branch,
            opts.remote_prefix + "/" + opts.release_branch,
        ]
        prunelist = []
        if opts.remove_owners:
            # Remove the top level OWNERS file from the list
            # to avoid any conflict with the modified branch file.
            prunelist.append("OWNERS")
        merge_repo(
            os.path.join(opts.srcbase, "src/third_party/zephyr/main"),
            cros_main,
            cmd_checkout,
            strategy,
            cmd,
            prunelist,
            [],
        )
        merge_repo(
            os.path.join(opts.srcbase, "src/third_party/zephyr/picolibc"),
            cros_main,
            cmd_checkout,
            strategy,
            cmd,
            prunelist,
            [],
        )
        # cmsis repo has different remote
        cros_main = opts.remote_prefix + "/" + "chromeos-main"
        merge_repo(
            os.path.join(opts.srcbase, "src/third_party/zephyr/cmsis"),
            cros_main,
            cmd_checkout,
            strategy,
            cmd,
            prunelist,
            [],
        )
    print(
        (
            "Finished! **Please review the commit(s) to see if they're to your "
            "liking.**"
        )
    )


if __name__ == "__main__":
    main(sys.argv)
