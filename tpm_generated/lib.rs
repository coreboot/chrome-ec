// Copyright 2024 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! TPM command encoding/decoding library.

use cxx::{let_cxx_string, UniquePtr};
use std::fmt::{self, Display, Formatter, Write};
use std::num::NonZeroU32;

pub use trunks::*;

#[allow(clippy::too_many_arguments)]
#[cxx::bridge(namespace = "trunks")]
pub mod trunks {
    unsafe extern "C++" {
        include!("authorization_delegate.h");

        type AuthorizationDelegate;

        include!("tpm_generated.h");

        type TPM2B_CREATION_DATA;
        type TPM2B_DATA;
        type TPM2B_DIGEST;
        type TPM2B_PUBLIC;
        type TPM2B_SENSITIVE_CREATE;
        type TPML_PCR_SELECTION;
        type TPMT_TK_CREATION;

        include!("ffi.h");

        /// Constructs a new PasswordAuthorizationDelegate with the given
        /// password.
        fn PasswordAuthorizationDelegate_New(
            password: &CxxString,
        ) -> UniquePtr<AuthorizationDelegate>;

        /// See Tpm::SerializeCommand_Create for docs.
        fn SerializeCommand_Create(
            parent_handle: &u32,
            parent_handle_name: &CxxString,
            in_sensitive: &TPM2B_SENSITIVE_CREATE,
            in_public: &TPM2B_PUBLIC,
            outside_info: &TPM2B_DATA,
            creation_pcr: &TPML_PCR_SELECTION,
            serialized_command: Pin<&mut CxxString>,
            authorization_delegate: &UniquePtr<AuthorizationDelegate>,
        ) -> u32;

        /// See Tpm::SerializeCommand_CreatePrimary for docs.
        fn SerializeCommand_CreatePrimary(
            primary_handle: &u32,
            primary_handle_name: &CxxString,
            in_sensitive: &TPM2B_SENSITIVE_CREATE,
            in_public: &TPM2B_PUBLIC,
            outside_info: &TPM2B_DATA,
            creation_pcr: &TPML_PCR_SELECTION,
            serialized_command: Pin<&mut CxxString>,
            authorization_delegate: &UniquePtr<AuthorizationDelegate>,
        ) -> u32;

        /// See Tpm::ParseResponse_CreatePrimary for docs.
        fn ParseResponse_CreatePrimary(
            response: &CxxString,
            object_handle: Pin<&mut u32>,
            out_public: Pin<&mut TPM2B_PUBLIC>,
            creation_data: Pin<&mut TPM2B_CREATION_DATA>,
            creation_hash: Pin<&mut TPM2B_DIGEST>,
            creation_ticket: Pin<&mut TPMT_TK_CREATION>,
            name: Pin<&mut CxxString>,
            authorization_delegate: &UniquePtr<AuthorizationDelegate>,
        ) -> u32;

        /// See Tpm::SerializeCommand_NV_ReadPublic for docs.
        fn SerializeCommand_NV_ReadPublic(
            nv_index: &u32,
            nv_index_name: &CxxString,
            serialized_command: Pin<&mut CxxString>,
            authorization_delegate: &UniquePtr<AuthorizationDelegate>,
        ) -> u32;

        /// Returns a serialized representation of the unmodified handle. This
        /// is useful for predefined handle values, like TPM_RH_OWNER. For
        /// details on what types of handles use this name formula see Table 3
        /// in the TPM 2.0 Library Spec Part 1 (Section 16 - Names).
        fn NameFromHandle(handle: &u32) -> UniquePtr<CxxString>;

        /// Creates a new empty TPM2B_CREATION_DATA.
        fn TPM2B_CREATION_DATA_New() -> UniquePtr<TPM2B_CREATION_DATA>;

        /// Creates a TPM2B_DATA with the given data.
        fn TPM2B_DATA_New(bytes: &CxxString) -> UniquePtr<TPM2B_DATA>;

        /// Creates a new empty TPM2B_DIGEST.
        fn TPM2B_DIGEST_New() -> UniquePtr<TPM2B_DIGEST>;

        /// Returns the public area template for the Attestation Identity Key.
        fn AttestationIdentityKeyTemplate() -> UniquePtr<TPM2B_PUBLIC>;

        /// Returns the public area template for the Storage Root Key.
        fn StorageRootKeyTemplate() -> UniquePtr<TPM2B_PUBLIC>;

        /// Creates a new TPM2B_SENSITIVE_CREATE with the given auth and data
        /// values.
        fn TPM2B_SENSITIVE_CREATE_New(
            user_auth: &CxxString,
            data: &CxxString,
        ) -> UniquePtr<TPM2B_SENSITIVE_CREATE>;

        /// Returns an empty PCR selection list.
        fn EmptyPcrSelection() -> UniquePtr<TPML_PCR_SELECTION>;

        /// Makes an empty TPMT_TK_CREATION;
        fn TPMT_TK_CREATION_New() -> UniquePtr<TPMT_TK_CREATION>;
    }
}

/// An error code returned by a Tpm method.
#[derive(Debug)]
pub struct TpmError {
    return_code: NonZeroU32,
}

impl TpmError {
    /// Creates a TpmError for the given return code, or None if this is a
    /// successful code.
    pub fn from_tpm_return_code(return_code: u32) -> Option<TpmError> {
        Some(TpmError { return_code: NonZeroU32::new(return_code)? })
    }
}

impl Display for TpmError {
    fn fmt(&self, f: &mut Formatter) -> Result<(), fmt::Error> {
        write!(f, "{:#x}", self.return_code)
    }
}

/// Converts the given byte array into a hex string (intended for debug prints).
pub fn bytes_to_hex(bytes: &[u8]) -> String {
    let expected_len = 2 * bytes.len();
    let mut out = String::with_capacity(expected_len);
    for b in bytes {
        write!(out, "{:02x}", b).unwrap();
    }
    out
}
