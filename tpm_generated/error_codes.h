// Copyright 2014 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TRUNKS_ERROR_CODES_H_
#define TRUNKS_ERROR_CODES_H_

#include <string>

#include "trunks/tpm_generated.h"  // For TPM_RC.
#include "trunks/trunks_export.h"

namespace trunks {

// Use the TPM_RC type but with different layer bits (12 - 15). Choose the layer
// value arbitrarily. Currently TSS2 uses 9 for TCTI and 8 for SAPI.
// TCTI and SAPI error codes taken from
// http://www.trustedcomputinggroup.org/resources/
// tss_system_level_api_and_tpm_command_transmission_interface_specification
const TPM_RC kTrunksErrorBase = (7 << 12);
const TPM_RC kTctiErrorBase = (8 << 12);
const TPM_RC kSapiErrorBase = (9 << 12);
const TPM_RC kResourceManagerTpmErrorBase = (11 << 12);
const TPM_RC kResourceManagerErrorBase = (12 << 12);

// Note that the trunks error layer is shared with the unified error code in
// TPMError and related classes, see libhwsec/error/tpm_error.h for more info.
// From the perspective of trunks, trunks can only use kTrunksErrorBase + 0 up
// to kTrunksErrorBase + 1023. The other 3072 error code is reserved for use by
// the TPMError and related classes.
const TPM_RC TRUNKS_RC_AUTHORIZATION_FAILED = kTrunksErrorBase + 1;
const TPM_RC TRUNKS_RC_ENCRYPTION_FAILED = kTrunksErrorBase + 2;
const TPM_RC TRUNKS_RC_READ_ERROR = kTrunksErrorBase + 3;
const TPM_RC TRUNKS_RC_WRITE_ERROR = kTrunksErrorBase + 4;
const TPM_RC TRUNKS_RC_IPC_ERROR = kTrunksErrorBase + 5;
const TPM_RC TRUNKS_RC_SESSION_SETUP_ERROR = kTrunksErrorBase + 6;
const TPM_RC TRUNKS_RC_INVALID_TPM_CONFIGURATION = kTrunksErrorBase + 7;
const TPM_RC TRUNKS_RC_PARSE_ERROR = kTrunksErrorBase + 8;

const TPM_RC TCTI_RC_TRY_AGAIN = kTctiErrorBase + 1;
const TPM_RC TCTI_RC_GENERAL_FAILURE = kTctiErrorBase + 2;
const TPM_RC TCTI_RC_BAD_CONTEXT = kTctiErrorBase + 3;
const TPM_RC TCTI_RC_WRONG_ABI_VERSION = kTctiErrorBase + 4;
const TPM_RC TCTI_RC_NOT_IMPLEMENTED = kTctiErrorBase + 5;
const TPM_RC TCTI_RC_BAD_PARAMETER = kTctiErrorBase + 6;
const TPM_RC TCTI_RC_INSUFFICIENT_BUFFER = kTctiErrorBase + 7;
const TPM_RC TCTI_RC_NO_CONNECTION = kTctiErrorBase + 8;
const TPM_RC TCTI_RC_DRIVER_NOT_FOUND = kTctiErrorBase + 9;
const TPM_RC TCTI_RC_DRIVERINFO_NOT_FOUND = kTctiErrorBase + 10;
const TPM_RC TCTI_RC_NO_RESPONSE = kTctiErrorBase + 11;
const TPM_RC TCTI_RC_BAD_VALUE = kTctiErrorBase + 12;

const TPM_RC SAPI_RC_INVALID_SESSIONS = kSapiErrorBase + 1;
const TPM_RC SAPI_RC_ABI_MISMATCH = kSapiErrorBase + 2;
const TPM_RC SAPI_RC_INSUFFICIENT_BUFFER = kSapiErrorBase + 3;
const TPM_RC SAPI_RC_BAD_PARAMETER = kSapiErrorBase + 4;
const TPM_RC SAPI_RC_BAD_SEQUENCE = kSapiErrorBase + 5;
const TPM_RC SAPI_RC_NO_DECRYPT_PARAM = kSapiErrorBase + 6;
const TPM_RC SAPI_RC_NO_ENCRYPT_PARAM = kSapiErrorBase + 7;
const TPM_RC SAPI_RC_NO_RESPONSE_RECEIVED = kSapiErrorBase + 8;
const TPM_RC SAPI_RC_BAD_SIZE = kSapiErrorBase + 9;
const TPM_RC SAPI_RC_CORRUPTED_DATA = kSapiErrorBase + 10;
const TPM_RC SAPI_RC_INSUFFICIENT_CONTEXT = kSapiErrorBase + 11;
const TPM_RC SAPI_RC_INSUFFICIENT_RESPONSE = kSapiErrorBase + 12;
const TPM_RC SAPI_RC_INCOMPATIBLE_TCTI = kSapiErrorBase + 13;
const TPM_RC SAPI_RC_MALFORMED_RESPONSE = kSapiErrorBase + 14;
const TPM_RC SAPI_RC_BAD_TCTI_STRUCTURE = kSapiErrorBase + 15;
const TPM_RC SAPI_RC_NO_CONNECTION = kSapiErrorBase + 16;

// Returns a description of |error|.
TRUNKS_EXPORT std::string GetErrorString(TPM_RC error);

// Strips the P and N bits from a 'format one' error. If the given error code
// is not a format one error, it is returned as is. The error that is returned
// can be compared to TPM_RC_* constant values. See TPM 2.0 Part 2 Section 6.6
// for details on format one errors.
TRUNKS_EXPORT TPM_RC GetFormatOneError(TPM_RC error);

// Creates a well-formed response with the given |error_code|.
TRUNKS_EXPORT std::string CreateErrorResponse(TPM_RC error_code);

// Retrieves response code, |rc|, from the response string, |response|.
// Return TPM_RC_SUCCESS iff success.
TRUNKS_EXPORT TPM_RC GetResponseCode(const std::string& response, TPM_RC& rc);

}  // namespace trunks

#endif  // TRUNKS_ERROR_CODES_H_
