# Copyright 2023 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
package(default_visibility = ["//platform:__subpackages__"])

load("@rules_rust//rust:defs.bzl", "rust_test")

rust_test(
    name = "test",
    edition = "2021",
    srcs = ["src/lib.rs"],
    proc_macro_deps = [
        "//platform/gsc-utils/rust/enum_utils",
    ],
)
