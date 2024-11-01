// Copyright 2024 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hex.h"

namespace trunks {

std::string HexEncode(const char* data, std::size_t len) {
  static constexpr char kHexChars[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                       '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  std::string ret;
  ret.reserve(len * 2);
  for (int i = 0; i < len; i++) {
    ret.push_back(kHexChars[data[i] >> 4]);
    ret.push_back(kHexChars[data[i] & 0xf]);
  }
  return ret;
}

}  // namespace trunks
