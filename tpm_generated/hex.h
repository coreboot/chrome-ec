// Copyright 2024 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TPM_GENERATED_HEX_H_
#define TPM_GENERATED_HEX_H_

#include <string>

namespace trunks {

std::string HexEncode(const char* data, std::size_t len);

}  // namespace trunks

#endif  // TPM_GENERATED_HEX_H_
