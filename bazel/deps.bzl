# Copyright 2024 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def _ish_deps_impl(module_ctx):
    http_archive(
        name = "ish-coreboot-sdk-i386-elf",
        build_file = "//platform/rules_cros_firmware/cros_firmware:BUILD.gcs_subtool",
        sha256 = "72f0b55516120e0919f10ddf28c53a429ccc8132685b6dbd6a8dcefeba92fcc5",
        url = "https://storage.googleapis.com/chromiumos-sdk/toolchains/coreboot-sdk-i386-elf/11.3.0-r2/5ba88fb0227c76584851bd9cbb24d785e31a717b.tar.zst",
    )

    return module_ctx.extension_metadata(
        root_module_direct_deps = [
            "ish-coreboot-sdk-i386-elf",
        ],
        root_module_direct_dev_deps = [],
        reproducible = True,
    )

ish_deps = module_extension(
    implementation = _ish_deps_impl,
)
