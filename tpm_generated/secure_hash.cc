// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crypto/secure_hash.h"

#if defined(OPENSSL_IS_BORINGSSL)
#include <openssl/mem.h>
#else
#include <openssl/crypto.h>
#endif
#include <openssl/sha.h>
#include <stddef.h>

#include "base/logging.h"
#include "base/notreached.h"
#include "base/pickle.h"
#include "crypto/openssl_util.h"

namespace crypto {

namespace {

class SecureHashSHA256 : public SecureHash {
 public:
  SecureHashSHA256() {
    SHA256_Init(&ctx_);
  }

  SecureHashSHA256(const SecureHashSHA256& other) : SecureHash() {
    memcpy(&ctx_, &other.ctx_, sizeof(ctx_));
  }

  ~SecureHashSHA256() override {
    OPENSSL_cleanse(&ctx_, sizeof(ctx_));
  }

  void Update(const void* input, size_t len) override {
    SHA256_Update(&ctx_, static_cast<const unsigned char*>(input), len);
  }

  void Finish(void* output, size_t len) override {
    ScopedOpenSSLSafeSizeBuffer<SHA256_DIGEST_LENGTH> result(
        static_cast<unsigned char*>(output), len);
    SHA256_Final(result.safe_buffer(), &ctx_);
  }

  SecureHash* Clone() const override {
    return new SecureHashSHA256(*this);
  }

  size_t GetHashLength() const override { return SHA256_DIGEST_LENGTH; }

 private:
  SHA256_CTX ctx_;
};

}  // namespace

SecureHash* SecureHash::Create(Algorithm algorithm) {
  switch (algorithm) {
    case SHA256:
      return new SecureHashSHA256();
    default:
      NOTIMPLEMENTED();
      return NULL;
  }
}

}  // namespace crypto
