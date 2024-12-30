// Copyright 2024 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TPM_GENERATED_FFI_H_
#define TPM_GENERATED_FFI_H_

// The cxx Rust library cannot invoke all C++ methods -- for example, it cannot
// invoke static methods, and there are many types it cannot pass by value.
// These functions provide cxx-compatible access to the functionality of other
// code in this directory.
//
// Because this entire library (libtpmgenerated) is a temporary measure
// (long-term, we want to replace it with tpm-rs), these bindings are added
// as-needed and do not always expose all the functionality that they could.

#include <memory>
#include <string>

#include "authorization_delegate.h"
#include "tpm_generated.h"

namespace trunks {

// Organization: each subsection is a type, ordered alphabetically.

// -----------------------------------------------------------------------------
// PasswordAuthorizationDelegate
// -----------------------------------------------------------------------------

// Wraps the PasswordAuthorizationDelegate constructor. Returns an
// AuthorizationDelegate pointer rather than a PasswordAuthorizationDelegate
// pointer because Rust code doesn't know how to convert a
// PasswordAuthorizationDelegate pointer into an AuthorizationDelegate pointer.
std::unique_ptr<AuthorizationDelegate> PasswordAuthorizationDelegate_New(
    const std::string& password);

// -----------------------------------------------------------------------------
// Tpm
// -----------------------------------------------------------------------------

// Wraps Tpm::SerializeCommand_Create. Serializes the TPM2_Create command.
// authorization_delegate is nullable.
TPM_RC SerializeCommand_Create(
    const TPMI_DH_OBJECT& parent_handle, const std::string& parent_handle_name,
    const TPM2B_SENSITIVE_CREATE& in_sensitive, const TPM2B_PUBLIC& in_public,
    const TPM2B_DATA& outside_info, const TPML_PCR_SELECTION& creation_pcr,
    std::string& serialized_command,
    const std::unique_ptr<AuthorizationDelegate>& authorization_delegate);

// Wraps Tpm::SerializeCommand_CreatePrimary. Serializes the TPM2_CreatePrimary
// command.
// authorization_delegate is nullable.
TPM_RC SerializeCommand_CreatePrimary(
    const TPMI_RH_HIERARCHY& primary_handle,
    const std::string& primary_handle_name,
    const TPM2B_SENSITIVE_CREATE& in_sensitive, const TPM2B_PUBLIC& in_public,
    const TPM2B_DATA& outside_info, const TPML_PCR_SELECTION& creation_pcr,
    std::string& serialized_command,
    const std::unique_ptr<AuthorizationDelegate>& authorization_delegate);

// Wraps Tpm::ParseResponse_CreatePrimary. Parses the response from a
// TPM2_CreatePrimary command.
// authorization_delegate is nullable.
TPM_RC ParseResponse_CreatePrimary(
    const std::string& response, TPM_HANDLE& object_handle,
    TPM2B_PUBLIC& out_public, TPM2B_CREATION_DATA& creation_data,
    TPM2B_DIGEST& creation_hash, TPMT_TK_CREATION& creation_ticket,
    std::string& name,
    const std::unique_ptr<AuthorizationDelegate>& authorization_delegate);

// Wraps Tpm::SerializeCommand_NV_ReadPublic. Serializes the TPM2_NV_ReadPublic
// command.
// authorization_delegate is nullable.
TPM_RC SerializeCommand_NV_ReadPublic(
    const TPMI_RH_NV_INDEX& nv_index, const std::string& nv_index_name,
    std::string& serialized_command,
    const std::unique_ptr<AuthorizationDelegate>& authorization_delegate);

// -----------------------------------------------------------------------------
// TPM_HANDLE
// -----------------------------------------------------------------------------

// Returns a serialized representation of the unmodified handle. This is useful
// for predefined handle values, like TPM_RH_OWNER. For details on what types of
// handles use this name formula see Table 3 in the TPM 2.0 Library Spec Part 1
// (Section 16 - Names).
std::unique_ptr<std::string> NameFromHandle(const TPM_HANDLE& handle);

// -----------------------------------------------------------------------------
// TPM2B_CREATION_DATA
// -----------------------------------------------------------------------------

// Creates a new empty TPM2B_CREATION_DATA.
std::unique_ptr<TPM2B_CREATION_DATA> TPM2B_CREATION_DATA_New();

// -----------------------------------------------------------------------------
// TPM2B_DATA
// -----------------------------------------------------------------------------

// Creates a TPM2B_DATA with the given data.
std::unique_ptr<TPM2B_DATA> TPM2B_DATA_New(const std::string& bytes);

// -----------------------------------------------------------------------------
// TPM2B_DIGEST
// -----------------------------------------------------------------------------

// Creates a new empty TPM2B_DIGEST.
std::unique_ptr<TPM2B_DIGEST> TPM2B_DIGEST_New();

// -----------------------------------------------------------------------------
// TPM2B_PUBLIC
// -----------------------------------------------------------------------------

// Returns the public area template for the Attestation Identity Key.
std::unique_ptr<TPM2B_PUBLIC> AttestationIdentityKeyTemplate();

// Returns the public area template for the Storage Root Key.
std::unique_ptr<TPM2B_PUBLIC> StorageRootKeyTemplate();

// -----------------------------------------------------------------------------
// TPM2B_SENSITIVE_CREATE
// -----------------------------------------------------------------------------

// Creates a TPM2B_SENSITIVE_CREATE with the given auth and data values.
std::unique_ptr<TPM2B_SENSITIVE_CREATE> TPM2B_SENSITIVE_CREATE_New(
    const std::string& user_auth, const std::string& data);

// -----------------------------------------------------------------------------
// TPML_PCR_SELECTION
// -----------------------------------------------------------------------------

// Returns an empty PCR selection list.
std::unique_ptr<TPML_PCR_SELECTION> EmptyPcrSelection();

// -----------------------------------------------------------------------------
// TPMT_TK_CREATION
// -----------------------------------------------------------------------------

// Creates a new, empty TPMT_TK_CREATION.
std::unique_ptr<TPMT_TK_CREATION> TPMT_TK_CREATION_New();

}  // namespace trunks

#endif  // TPM_GENERATED_FFI_H_
