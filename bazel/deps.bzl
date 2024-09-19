# Copyright 2024 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def _cr50_deps_impl(module_ctx):
    http_archive(
        name = "cr50-coreboot-sdk-arm-eabi",
        build_file = "//platform/rules_cros_firmware/cros_firmware:BUILD.gcs_subtool",
        sha256 = "135790b3c755bdfb2ab78e481e2ed5575f0d488036112f1601fc60a5e9fc6695",
        # Select gcc 11.3.0 to maintain certification.  Match the build version used in the ebuild(Most recent GCC 11.3.0 build).
        url = "https://storage.googleapis.com/chromiumos-sdk/toolchains/coreboot-sdk-arm-eabi/11.3.0-r2/9e5f6c4935a7a421512c3b2758c46c78485dcc27.tar.zst",
    )

    return module_ctx.extension_metadata(
        root_module_direct_deps = [
            "cr50-coreboot-sdk-arm-eabi",
        ],
        root_module_direct_dev_deps = [],
        reproducible = True,
    )

cr50_deps = module_extension(
    implementation = _cr50_deps_impl,
)
