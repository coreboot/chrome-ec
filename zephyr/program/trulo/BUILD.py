# Copyright 2024 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Define zmake projects for trulo."""


def register_trulo_project(
    project_name,
    chip="npcx9/npcx9m3f",
    kconfig_files=None,
):
    """Register a variant of Trulo."""
    if kconfig_files is None:
        kconfig_files = [
            # Common to all projects.
            here / "program.conf",
            # Project-specific KConfig customization.
            here / project_name / "project.conf",
        ]

    register_npcx_project(
        project_name=project_name,
        zephyr_board=chip,
        dts_overlays=[
            here / project_name / "project.overlay",
        ],
        kconfig_files=kconfig_files,
        inherited_from=["trulo"],
    )


register_trulo_project(
    project_name="trulo",
)

register_trulo_project(
    project_name="uldrenite",
    chip="npcx9/npcx9m7fb",
    kconfig_files=[
        # Common to all projects.
        here / "program.conf",
        # Parent project's config
        here / "uldrenite" / "project.conf",
    ],
)

# Note for reviews, do not let anyone edit these assertions, the addresses
# must not change after the first RO release.
assert_rw_fwid_DO_NOT_EDIT(project_name="trulo", addr=0x40144)
assert_rw_fwid_DO_NOT_EDIT(project_name="uldrenite", addr=0x40144)
