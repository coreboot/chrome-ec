// Copyright 2014 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TRUNKS_AUTHORIZATION_DELEGATE_H_
#define TRUNKS_AUTHORIZATION_DELEGATE_H_

#include <string>

namespace trunks {

inline constexpr uint8_t kContinueSession = 1;

// AuthorizationDelegate is an interface passed to TPM commands. The delegate
// takes care of providing the authorization data for commands and verifying
// authorization data for responses. It also handles parameter encryption for
// commands and parameter decryption for responses.
class AuthorizationDelegate {
 public:
  AuthorizationDelegate() {}
  AuthorizationDelegate(const AuthorizationDelegate&) = delete;
  AuthorizationDelegate& operator=(const AuthorizationDelegate&) = delete;

  virtual ~AuthorizationDelegate() {}

  // Provides authorization data for a command which has a cpHash value of
  // |command_hash|. The availability of encryption for the command is indicated
  // by |is_*_parameter_encryption_possible|. On success, |authorization| is
  // populated with the exact octets for the Authorization Area of the command.
  // Returns true on success.
  virtual bool GetCommandAuthorization(
      const std::string& command_hash,
      bool is_command_parameter_encryption_possible,
      bool is_response_parameter_encryption_possible,
      std::string* authorization) = 0;

  // Checks authorization data for a response which has a rpHash value of
  // |response_hash|. The exact octets from the Authorization Area of the
  // response are given in |authorization|. Returns true iff the authorization
  // is valid.
  virtual bool CheckResponseAuthorization(const std::string& response_hash,
                                          const std::string& authorization) = 0;

  // Encrypts |parameter| if encryption is enabled. Returns true on success.
  virtual bool EncryptCommandParameter(std::string* parameter) = 0;

  // Decrypts |parameter| if encryption is enabled. Returns true on success.
  virtual bool DecryptResponseParameter(std::string* parameter) = 0;

  // Returns the current TPM-generated nonce that is associated with the
  // authorization session. Returns true on success.
  virtual bool GetTpmNonce(std::string* nonce) = 0;
};

}  // namespace trunks

#endif  // TRUNKS_AUTHORIZATION_DELEGATE_H_
