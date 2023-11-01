# Copyright 2023 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
package(default_visibility = ["//platform/ti50/common/libs:__subpackages__"])

load("@rules_rust//rust:defs.bzl", "rust_proc_macro")

rust_proc_macro(
    name = "enum_as",
    srcs = glob(["**/*.rs"]),
    edition = "2021",
)
