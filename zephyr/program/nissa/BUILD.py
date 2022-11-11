# Copyright 2021 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Define zmake projects for nissa."""

# Nivviks and Craask, Pujjo, Xivu has NPCX993F, Nereid and Joxer, Yaviks has ITE81302


def register_nissa_project(
    project_name,
    chip="it81302bx",
):
    """Register a variant of nissa."""
    register_func = register_binman_project
    if chip.startswith("npcx"):
        register_func = register_npcx_project

    chip_kconfig = {"it81302bx": "it8xxx2", "npcx9m3f": "npcx"}[chip]

    return register_func(
        project_name=project_name,
        zephyr_board=chip,
        dts_overlays=[here / project_name / "project.overlay"],
        kconfig_files=[
            here / "program.conf",
            here / f"{chip_kconfig}_program.conf",
            here / project_name / "project.conf",
        ],
    )


nivviks = register_nissa_project(
    project_name="nivviks",
    chip="npcx9m3f",
)

nereid = register_nissa_project(
    project_name="nereid",
    chip="it81302bx",
)

craask = register_nissa_project(
    project_name="craask",
    chip="npcx9m3f",
)

pujjo = register_nissa_project(
    project_name="pujjo",
    chip="npcx9m3f",
)

xivu = register_nissa_project(
    project_name="xivu",
    chip="npcx9m3f",
)

joxer = register_nissa_project(
    project_name="joxer",
    chip="it81302bx",
)

yaviks = register_nissa_project(
    project_name="yaviks",
    chip="it81302bx",
)
