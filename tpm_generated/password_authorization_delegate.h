// Copyright 2014 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TRUNKS_PASSWORD_AUTHORIZATION_DELEGATE_H_
#define TRUNKS_PASSWORD_AUTHORIZATION_DELEGATE_H_

#include <string>

#include "authorization_delegate.h"
#include "tpm_generated.h"
#include "trunks_export.h"

namespace trunks {

// PasswdAuthorizationDelegate is an implementation of the AuthorizationDelegate
// interface. This delegate is used for password based authorization. Upon
// initialization of this delegate, we feed in the plaintext password. This
// password is then used to authorize the commands issued with this delegate.
// This delegate performs no parameter encryption.
class TRUNKS_EXPORT PasswordAuthorizationDelegate
    : public AuthorizationDelegate {
 public:
  explicit PasswordAuthorizationDelegate(const std::string& password);
  PasswordAuthorizationDelegate(const PasswordAuthorizationDelegate&) = delete;
  PasswordAuthorizationDelegate& operator=(
      const PasswordAuthorizationDelegate&) = delete;

  ~PasswordAuthorizationDelegate() override;
  // AuthorizationDelegate methods.
  bool GetCommandAuthorization(const std::string& command_hash,
                               bool is_command_parameter_encryption_possible,
                               bool is_response_parameter_encryption_possible,
                               std::string* authorization) override;
  bool CheckResponseAuthorization(const std::string& response_hash,
                                  const std::string& authorization) override;
  bool EncryptCommandParameter(std::string* parameter) override;
  bool DecryptResponseParameter(std::string* parameter) override;
  bool GetTpmNonce(std::string* nonce) override;

 private:
  TPM2B_AUTH password_;
};

}  // namespace trunks

#endif  // TRUNKS_PASSWORD_AUTHORIZATION_DELEGATE_H_
