// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRYPTO_SECURE_HASH_H_
#define CRYPTO_SECURE_HASH_H_

#include <stddef.h>

namespace crypto {

// A wrapper to calculate secure hashes incrementally, allowing to
// be used when the full input is not known in advance.
class SecureHash {
 public:
  enum Algorithm {
    SHA256,
  };

  SecureHash(const SecureHash&) = delete;
  SecureHash& operator=(const SecureHash&) = delete;

  virtual ~SecureHash() {}

  static SecureHash* Create(Algorithm type);

  virtual void Update(const void* input, size_t len) = 0;
  virtual void Finish(void* output, size_t len) = 0;
  virtual size_t GetHashLength() const = 0;

  // Create a clone of this SecureHash. The returned clone and this both
  // represent the same hash state. But from this point on, calling
  // Update()/Finish() on either doesn't affect the state of the other.
  virtual SecureHash* Clone() const = 0;

 protected:
  SecureHash() {}
};

}  // namespace crypto

#endif  // CRYPTO_SECURE_HASH_H_
