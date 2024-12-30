// Copyright 2024 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ffi.h"

#include "password_authorization_delegate.h"

namespace trunks {

namespace {

constexpr TPMA_OBJECT kFixedTPM = 1U << 1;
constexpr TPMA_OBJECT kFixedParent = 1U << 4;
constexpr TPMA_OBJECT kSensitiveDataOrigin = 1U << 5;
constexpr TPMA_OBJECT kUserWithAuth = 1U << 6;
constexpr TPMA_OBJECT kNoDA = 1U << 10;
constexpr TPMA_OBJECT kRestricted = 1U << 16;
constexpr TPMA_OBJECT kDecrypt = 1U << 17;
constexpr TPMA_OBJECT kSign = 1U << 18;

// Returns a general public area for our keys. This default may be further
// manipulated to produce the public area for specific keys (such as SRK or
// AIK).
TPMT_PUBLIC DefaultPublicArea() {
  TPMT_PUBLIC public_area;
  memset(&public_area, 0, sizeof(public_area));
  public_area.type = TPM_ALG_ECC;
  public_area.name_alg = TPM_ALG_SHA256;
  public_area.auth_policy = Make_TPM2B_DIGEST("");
  public_area.object_attributes = kFixedTPM | kFixedParent;
  public_area.parameters.ecc_detail.scheme.scheme = TPM_ALG_NULL;
  public_area.parameters.ecc_detail.symmetric.algorithm = TPM_ALG_NULL;
  public_area.parameters.ecc_detail.curve_id = TPM_ECC_NIST_P256;
  public_area.parameters.ecc_detail.kdf.scheme = TPM_ALG_NULL;
  public_area.unique.ecc.x = Make_TPM2B_ECC_PARAMETER("");
  public_area.unique.ecc.y = Make_TPM2B_ECC_PARAMETER("");
  return public_area;
}

}  // namespace

std::unique_ptr<AuthorizationDelegate> PasswordAuthorizationDelegate_New(
    const std::string& password) {
  return std::make_unique<PasswordAuthorizationDelegate>(password);
}

TPM_RC SerializeCommand_Create(
    const TPMI_DH_OBJECT& parent_handle, const std::string& parent_handle_name,
    const TPM2B_SENSITIVE_CREATE& in_sensitive, const TPM2B_PUBLIC& in_public,
    const TPM2B_DATA& outside_info, const TPML_PCR_SELECTION& creation_pcr,
    std::string& serialized_command,
    const std::unique_ptr<AuthorizationDelegate>& authorization_delegate) {
  return Tpm::SerializeCommand_Create(
      parent_handle, parent_handle_name, in_sensitive, in_public, outside_info,
      creation_pcr, &serialized_command, authorization_delegate.get());
}

TPM_RC SerializeCommand_CreatePrimary(
    const TPMI_RH_HIERARCHY& primary_handle,
    const std::string& primary_handle_name,
    const TPM2B_SENSITIVE_CREATE& in_sensitive, const TPM2B_PUBLIC& in_public,
    const TPM2B_DATA& outside_info, const TPML_PCR_SELECTION& creation_pcr,
    std::string& serialized_command,
    const std::unique_ptr<AuthorizationDelegate>& authorization_delegate) {
  return Tpm::SerializeCommand_CreatePrimary(
      primary_handle, primary_handle_name, in_sensitive, in_public,
      outside_info, creation_pcr, &serialized_command,
      authorization_delegate.get());
}

TPM_RC ParseResponse_CreatePrimary(
    const std::string& response, TPM_HANDLE& object_handle,
    TPM2B_PUBLIC& out_public, TPM2B_CREATION_DATA& creation_data,
    TPM2B_DIGEST& creation_hash, TPMT_TK_CREATION& creation_ticket,
    std::string& name,
    const std::unique_ptr<AuthorizationDelegate>& authorization_delegate) {
  TPM2B_NAME tpm2b_name;
  TPM_RC rc = Tpm::ParseResponse_CreatePrimary(
      response, &object_handle, &out_public, &creation_data, &creation_hash,
      &creation_ticket, &tpm2b_name, authorization_delegate.get());
  if (rc != TPM_RC_SUCCESS) {
    return rc;
  }
  return Serialize_TPM2B_NAME(tpm2b_name, &name);
}

TPM_RC SerializeCommand_NV_ReadPublic(
    const TPMI_RH_NV_INDEX& nv_index, const std::string& nv_index_name,
    std::string& serialized_command,
    const std::unique_ptr<AuthorizationDelegate>& authorization_delegate) {
  return Tpm::SerializeCommand_NV_ReadPublic(nv_index, nv_index_name,
                                             &serialized_command,
                                             authorization_delegate.get());
}

std::unique_ptr<std::string> NameFromHandle(const TPM_HANDLE& handle) {
  std::string name;
  Serialize_TPM_HANDLE(handle, &name);
  return std::make_unique<std::string>(std::move(name));
}

std::unique_ptr<TPM2B_CREATION_DATA> TPM2B_CREATION_DATA_New() {
  return std::make_unique<TPM2B_CREATION_DATA>();
}

std::unique_ptr<TPM2B_DATA> TPM2B_DATA_New(const std::string& bytes) {
  return std::make_unique<TPM2B_DATA>(Make_TPM2B_DATA(bytes));
}

std::unique_ptr<TPM2B_DIGEST> TPM2B_DIGEST_New() {
  return std::make_unique<TPM2B_DIGEST>();
}

std::unique_ptr<TPM2B_PUBLIC> AttestationIdentityKeyTemplate() {
  TPMT_PUBLIC public_area = DefaultPublicArea();
  public_area.object_attributes |=
      (kSensitiveDataOrigin | kUserWithAuth | kNoDA | kRestricted | kSign);
  public_area.parameters.ecc_detail.scheme.scheme = TPM_ALG_ECDSA;
  public_area.parameters.ecc_detail.scheme.details.ecdsa.hash_alg =
      TPM_ALG_SHA256;
  return std::make_unique<TPM2B_PUBLIC>(Make_TPM2B_PUBLIC(public_area));
}

std::unique_ptr<TPM2B_PUBLIC> StorageRootKeyTemplate() {
  TPMT_PUBLIC public_area = DefaultPublicArea();
  public_area.object_attributes |=
      (kSensitiveDataOrigin | kUserWithAuth | kNoDA | kRestricted | kDecrypt);
  public_area.parameters.asym_detail.symmetric.algorithm = TPM_ALG_AES;
  public_area.parameters.asym_detail.symmetric.key_bits.aes = 128;
  public_area.parameters.asym_detail.symmetric.mode.aes = TPM_ALG_CFB;
  return std::make_unique<TPM2B_PUBLIC>(Make_TPM2B_PUBLIC(public_area));
}

std::unique_ptr<TPM2B_SENSITIVE_CREATE> TPM2B_SENSITIVE_CREATE_New(
    const std::string& user_auth, const std::string& data) {
  TPMS_SENSITIVE_CREATE sensitive;
  sensitive.user_auth = Make_TPM2B_DIGEST(user_auth);
  sensitive.data = Make_TPM2B_SENSITIVE_DATA(data);
  return std::make_unique<TPM2B_SENSITIVE_CREATE>(
      Make_TPM2B_SENSITIVE_CREATE(sensitive));
}

std::unique_ptr<TPML_PCR_SELECTION> EmptyPcrSelection() {
  TPML_PCR_SELECTION creation_pcrs = {};
  creation_pcrs.count = 0;
  return std::make_unique<TPML_PCR_SELECTION>(creation_pcrs);
}

std::unique_ptr<TPMT_TK_CREATION> TPMT_TK_CREATION_New() {
  return std::make_unique<TPMT_TK_CREATION>();
}

}  // namespace trunks
