# Copyright 2021 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Define zmake projects for npcx9_evb."""

register_npcx_project(
    project_name="npcx9",
    zephyr_board="npcx_evb/npcx9m6f",
    dts_overlays=[
        "gpio.dts",
        "interrupts.dts",
        "fan.dts",
        "keyboard.dts",
    ],
)

# Note for reviews, do not let anyone edit these assertions, the addresses
# must not change after the first RO release.
assert_rw_fwid_DO_NOT_EDIT(project_name="npcx9", addr=0x7FFE0)
