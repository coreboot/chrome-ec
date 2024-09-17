// Copyright 2015 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TRUNKS_TRUNKS_EXPORT_H_
#define TRUNKS_TRUNKS_EXPORT_H_

// Use this for any class or function that needs to be exported from libtrunks.
// E.g. TRUNKS_EXPORT void foo();
#define TRUNKS_EXPORT __attribute__((__visibility__("default")))

#endif  // TRUNKS_TRUNKS_EXPORT_H_
