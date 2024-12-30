// Copyright 2014 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "password_authorization_delegate.h"

#include <android-base/logging.h>

#include "authorization_delegate.h"
#include "tpm_generated.h"

namespace trunks {

PasswordAuthorizationDelegate::PasswordAuthorizationDelegate(
    const std::string& password) {
  password_ = Make_TPM2B_DIGEST(password);
}

PasswordAuthorizationDelegate::~PasswordAuthorizationDelegate() {}

bool PasswordAuthorizationDelegate::GetCommandAuthorization(
    const std::string& command_hash,
    bool is_command_parameter_encryption_possible,
    bool is_response_parameter_encryption_possible,
    std::string* authorization) {
  TPMS_AUTH_COMMAND auth;
  auth.session_handle = TPM_RS_PW;
  auth.nonce.size = 0;
  auth.session_attributes = kContinueSession;
  auth.hmac = password_;

  TPM_RC serialize_error = Serialize_TPMS_AUTH_COMMAND(auth, authorization);
  if (serialize_error != TPM_RC_SUCCESS) {
    LOG(ERROR) << __func__ << ": could not serialize command auth.";
    return false;
  }
  return true;
}

bool PasswordAuthorizationDelegate::CheckResponseAuthorization(
    const std::string& response_hash, const std::string& authorization) {
  TPMS_AUTH_RESPONSE auth_response;
  std::string mutable_auth_string(authorization);
  std::string auth_bytes;
  TPM_RC parse_error;
  parse_error = Parse_TPMS_AUTH_RESPONSE(&mutable_auth_string, &auth_response,
                                         &auth_bytes);
  if (authorization.size() != auth_bytes.size()) {
    LOG(ERROR) << __func__ << ": Authorization string was of wrong length.";
    return false;
  }
  if (parse_error != TPM_RC_SUCCESS) {
    LOG(ERROR) << __func__ << ": could not parse authorization response.";
    return false;
  }
  if (auth_response.nonce.size != 0) {
    LOG(ERROR) << __func__ << ": received a non zero length nonce.";
    return false;
  }
  if (auth_response.hmac.size != 0) {
    LOG(ERROR) << __func__ << ": received a non zero length hmac.";
    return false;
  }
  if (auth_response.session_attributes != kContinueSession) {
    LOG(ERROR) << __func__ << ": received wrong session attributes.";
    return false;
  }
  return true;
}

bool PasswordAuthorizationDelegate::EncryptCommandParameter(
    std::string* parameter) {
  return true;
}

bool PasswordAuthorizationDelegate::DecryptResponseParameter(
    std::string* parameter) {
  return true;
}

bool PasswordAuthorizationDelegate::GetTpmNonce(std::string* nonce) {
  return false;
}

}  // namespace trunks
