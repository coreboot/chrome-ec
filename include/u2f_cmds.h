/* Copyright 2021 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __U2F_CMDS_H_INCLUDED__
#define __U2F_CMDS_H_INCLUDED__

/* Load platform hooks/definitions */
#include "common.h"
#include "tpm_vendor_cmds.h"
#include "u2f.h"

/* Until u2fd migrates to new structs, check they are compatible. */
BUILD_ASSERT(sizeof(struct u2f_key_handle_v0) == sizeof(struct u2f_key_handle));

BUILD_ASSERT(sizeof(struct u2f_key_handle_v1) ==
	     sizeof(struct u2f_versioned_key_handle));

BUILD_ASSERT(sizeof(struct u2f_signature) == sizeof(struct u2f_sign_resp));
BUILD_ASSERT(sizeof(struct u2f_signature) == sizeof(struct u2f_attest_resp));

BUILD_ASSERT(sizeof(struct u2f_key_handle_v0) == U2F_V0_KH_SIZE);
BUILD_ASSERT(sizeof(struct u2f_key_handle_v1) == U2F_V1_KH_SIZE);
BUILD_ASSERT(sizeof(struct u2f_key_handle_v2) == U2F_V2_KH_SIZE);

BUILD_ASSERT(sizeof(struct g2f_register_msg_v0) <= U2F_MAX_ATTEST_SIZE);
/**
 * U2F_GENERATE command handler. Generates a key handle according to input
 * parameters.
 */
enum vendor_cmd_rc u2f_generate_cmd(enum vendor_cmd_cc code, void *buf,
				    size_t input_size, size_t *response_size);

/**
 * U2F_SIGN command handler. Verifies a key handle is owned and signs data with
 * it.
 */
enum vendor_cmd_rc u2f_sign_cmd(enum vendor_cmd_cc code, void *buf,
				size_t input_size, size_t *response_size);

/**
 *  U2F_ATTEST command handler.
 */
enum vendor_cmd_rc u2f_attest_cmd(enum vendor_cmd_cc code, void *buf,
				  size_t input_size, size_t *response_size);

/* Maximum size in bytes of G2F attestation certificate. */
#define G2F_ATTESTATION_CERT_MAX_LEN 315

/**
 * Gets the x509 certificate for the attestation key pair returned
 * by g2f_individual_keypair().
 *
 * @param buf pointer to a buffer that must be at least
 * G2F_ATTESTATION_CERT_MAX_LEN bytes.
 * @return size of certificate written to buf, 0 on error.
 */
size_t g2f_attestation_cert(uint8_t *buf);

#endif /* __U2F_CMDS_H_INCLUDED__ */
