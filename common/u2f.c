/* Copyright 2017 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* APDU dispatcher and U2F command handlers. */

#include "console.h"

#include "dcrypto.h"
#include "extension.h"
#include "nvmem_vars.h"
#include "physical_presence.h"
#include "system.h"
#include "tpm_nvmem_ops.h"
#include "tpm_vendor_cmds.h"
#include "u2f.h"
#include "u2f_cmds.h"
#include "u2f_impl.h"
#include "util.h"

#define CPRINTF(format, args...) cprintf(CC_EXTENSION, format, ##args)

size_t g2f_attestation_cert(uint8_t *buf)
{
	uint8_t *serial;

	const struct u2f_state *state = u2f_get_state_no_commit();

	if (!state)
		return 0;

	if (system_get_chip_unique_id(&serial) != P256_NBYTES)
		return 0;

#ifdef CHIP_G
	/**
	 * chip/g implementation of system_get_chip_unique_id() always
	 * returns 32-bit aligned pointer, but host mock-ups do no guarantee
	 * that, so copy data to aligned location.
	 */
	return g2f_attestation_cert_serial(state, (p256_int *)serial, buf);
#else
	{
		p256_int p256_serial;

		memcpy(&p256_serial, serial, sizeof(p256_serial));
		return g2f_attestation_cert_serial(state, &p256_serial, buf);
	}
#endif
}

/* U2F GENERATE command  */
enum vendor_cmd_rc u2f_generate_cmd(enum vendor_cmd_cc code, void *buf,
				    size_t input_size, size_t *response_size)
{
	struct u2f_generate_req *req = buf;
	union u2f_generate_response *resp = buf;
	struct u2f_ec_point *pubKey = NULL;

	const struct u2f_state *state = u2f_get_state();
	uint8_t kh_version = 0; /* Fall back version of key handle. */

	static const size_t kh_version_response_size[] = {
		sizeof(struct u2f_generate_resp),
		sizeof(struct u2f_generate_versioned_resp),
		sizeof(struct u2f_generate_versioned_resp_v2)
	};

	/**
	 * Buffer for generating key handle as part of response. Note, it
	 * overlaps with authTimeSecret in response since request and response
	 * shares same buffer.
	 */
	union u2f_key_handle_variant *kh_buf = NULL;

	uint8_t authTimeSecretHash[U2F_AUTH_TIME_SECRET_SIZE];

	size_t response_buf_size = *response_size;
	enum ec_error_list result;

	*response_size = 0;

	if (input_size != sizeof(struct u2f_generate_req))
		return VENDOR_RC_BOGUS_ARGS;

	if (state == NULL)
		return VENDOR_RC_INTERNAL_ERROR;

	if ((req->flags & U2F_V2_KH_MASK) == U2F_V2_KH_MASK)
		kh_version = U2F_KH_VERSION_2;
	else if (req->flags & U2F_UV_ENABLED_KH)
		kh_version = U2F_KH_VERSION_1;

	/* Check there is enough room for response in response buffer. */
	if (response_buf_size < kh_version_response_size[kh_version])
		return VENDOR_RC_BOGUS_ARGS;

	/* Copy to avoid overwriting data before use. */
	memcpy(authTimeSecretHash, req->authTimeSecretHash,
	       sizeof(authTimeSecretHash));

	switch (kh_version) {
	case U2F_KH_VERSION_2:
		pubKey = &resp->v2.pubKey;
		kh_buf = (union u2f_key_handle_variant *)&resp->v2.keyHandle;
		break;
	case U2F_KH_VERSION_1:
		pubKey = &resp->v1.pubKey;
		kh_buf = (union u2f_key_handle_variant *)&resp->v1.keyHandle;
		break;
	default: /* Version 0 */
		pubKey = &resp->v0.pubKey;
		kh_buf = (union u2f_key_handle_variant *)&resp->v0.keyHandle;
		break;
	}

	/* Maybe enforce user presence, w/ optional consume */
	if (pop_check_presence(req->flags & G2F_CONSUME) != POP_TOUCH_YES &&
	    (req->flags & U2F_AUTH_FLAG_TUP) != 0)
		return VENDOR_RC_NOT_ALLOWED;

	/**
	 * req->userSecret and req->appId are consumed  by u2f_generate() before
	 * being overwritten.
	 */
	result = u2f_generate(state, req->userSecret, req->appId,
			      authTimeSecretHash, kh_buf, kh_version, pubKey);

	always_memset(authTimeSecretHash, 0, sizeof(authTimeSecretHash));

	if (result != EC_SUCCESS)
		return VENDOR_RC_INTERNAL_ERROR;

	/*
	 * From this point: the request 'req' content is invalid as it is
	 * overridden by the response we are building in the same buffer.
	 */
	*response_size = kh_version_response_size[kh_version];
	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_U2F_GENERATE, u2f_generate_cmd);

/* Below, we depend on the response not being larger than than the request. */
BUILD_ASSERT(sizeof(struct u2f_sign_resp) <= sizeof(struct u2f_sign_req));


/* U2F SIGN command */
enum vendor_cmd_rc u2f_sign_cmd(enum vendor_cmd_cc code, void *buf,
				size_t input_size, size_t *response_size)
{
	const union u2f_sign_request *req = buf;
	union u2f_key_handle_variant *kh;

	const struct u2f_state *state = u2f_get_state();

	const uint8_t *hash, *user, *origin;
	uint8_t *authTimeSecret = NULL;
	uint8_t flags;
	struct u2f_sign_resp *resp;

	/* Version of KH; 0 if KH is not versioned. */
	uint8_t kh_version;

	enum ec_error_list result;

	/* Response is smaller than request, so no need to check this. */
	*response_size = 0;

	if (!state)
		return VENDOR_RC_INTERNAL_ERROR;

	/**
	 * Request can be in old (non-versioned) and new (versioned) formats,
	 * which differs in size. Use request size to distinguish it.
	 */
	if (input_size == sizeof(struct u2f_sign_req)) {
		kh_version = 0;
		kh = (union u2f_key_handle_variant *)&req->v0.keyHandle;
		hash = req->v0.hash;
		flags = req->v0.flags;
		user = req->v0.userSecret;
		origin = req->v0.appId;
	} else if (input_size == sizeof(struct u2f_sign_versioned_req)) {
		kh = (union u2f_key_handle_variant *)&req->v1.keyHandle;
		kh_version = U2F_KH_VERSION_1;
		hash = req->v1.hash;
		flags = req->v1.flags;
		user = req->v1.userSecret;
		origin = req->v1.appId;
		/**
		 * TODO(b/184393647): Enforce user verification if no user
		 * presence check is requested. Set
		 *    authTimeSecret = req->v1.authTimeSecret;
		 * unconditionally or if (flags & U2F_AUTH_FLAG_TUP) == 0
		 */
		authTimeSecret = NULL;
	} else if (input_size == sizeof(struct u2f_sign_versioned_req_v2)) {
		kh = (union u2f_key_handle_variant *)&req->v2.keyHandle;
		kh_version = U2F_KH_VERSION_2;
		hash = req->v2.hash;
		flags = req->v2.flags;
		user = req->v2.userSecret;
		origin = req->v2.appId;
		authTimeSecret = (uint8_t *)req->v2.authTimeSecret;
	} else
		return VENDOR_RC_BOGUS_ARGS;

	if (authTimeSecret) {
		/**
		 * U2F_Sign receives a pre-hashed version of credential which
		 * was used by U2F_Generate, so perform hash before use.
		 */
		if (!DCRYPTO_SHA256_hash(authTimeSecret,
					 U2F_AUTH_TIME_SECRET_SIZE,
					 authTimeSecret))
			return VENDOR_RC_INTERNAL_ERROR;
	}
	result = u2f_authorize_keyhandle(state, kh, kh_version, user, origin,
					 authTimeSecret);
	if (result == EC_ERROR_ACCESS_DENIED)
		return VENDOR_RC_PASSWORD_REQUIRED;
	if (result != EC_SUCCESS)
		return VENDOR_RC_INTERNAL_ERROR;

	/* We might not actually need to sign anything. */
	if ((flags & U2F_AUTH_CHECK_ONLY) == U2F_AUTH_CHECK_ONLY)
		return VENDOR_RC_SUCCESS;

	/*
	 * Enforce user presence for version 0 KHs, with optional consume.
	 * TODO(b/184393647): update logic for version 2 if needed.
	 */
	if (pop_check_presence(flags & G2F_CONSUME) != POP_TOUCH_YES) {
		if (kh_version != U2F_KH_VERSION_1)
			return VENDOR_RC_NOT_ALLOWED;
		if ((flags & U2F_AUTH_FLAG_TUP) != 0)
			return VENDOR_RC_NOT_ALLOWED;
	}

	/*
	 * u2f_sign first consume all data from request 'req', and compute
	 * result in temporary storage. Once accomplished, it stores it in
	 * provided buffer. This allows overlap between input and output
	 * parameters.
	 * The response is smaller than the request, so we have the space.
	 */
	resp = buf;

	result = u2f_sign(state, kh, kh_version, user, origin, authTimeSecret,
			  hash, (struct u2f_signature *)resp);

	if (result == EC_ERROR_ACCESS_DENIED)
		return VENDOR_RC_PASSWORD_REQUIRED;
	if (result != EC_SUCCESS)
		return VENDOR_RC_INTERNAL_ERROR;

	*response_size = sizeof(*resp);

	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_U2F_SIGN, u2f_sign_cmd);

static inline size_t u2f_attest_format_size(uint8_t format)
{
	switch (format) {
	case U2F_ATTEST_FORMAT_REG_RESP:
		return sizeof(struct g2f_register_msg_v0);
	case CORP_ATTEST_FORMAT_REG_RESP:
		return sizeof(struct corp_attest_data);
	default:
		return 0;
	}
}

/* U2F ATTEST command */
enum vendor_cmd_rc u2f_attest_cmd(enum vendor_cmd_cc code, void *buf,
				  size_t input_size, size_t *response_size)
{
	const struct u2f_attest_req *req = buf;
	struct u2f_attest_resp *resp;
	union u2f_attest_msg_variant *msg = (void *)req->data;
	enum ec_error_list result;

	const union u2f_key_handle_variant *kh;
	const uint8_t *origin;
	const struct u2f_ec_point *pubKey;

	size_t response_buf_size = *response_size;

	const struct u2f_state *state = u2f_get_state();

	*response_size = 0;

	if (!state)
		return VENDOR_RC_INTERNAL_ERROR;

	if (input_size < offsetof(struct u2f_attest_req, data) ||
	    input_size <
		    (offsetof(struct u2f_attest_req, data) + req->dataLen) ||
	    input_size > sizeof(struct u2f_attest_req) ||
	    response_buf_size < sizeof(*resp))
		return VENDOR_RC_BOGUS_ARGS;

	/*
	 * Two formats are supported, U2F Attest format and Corp Attest format,
	 * both with key handle version 0.
	 */
	if (req->format == U2F_ATTEST_FORMAT_REG_RESP) {
		if (req->dataLen != sizeof(struct g2f_register_msg_v0))
			return VENDOR_RC_NOT_ALLOWED;

		if (msg->g2f.reserved != 0)
			return VENDOR_RC_NOT_ALLOWED;

		kh = (union u2f_key_handle_variant *)&msg->g2f.key_handle;
		origin = msg->g2f.app_id;
		pubKey = &msg->g2f.public_key;
	} else if (req->format == CORP_ATTEST_FORMAT_REG_RESP) {
		if (req->dataLen != sizeof(struct corp_register_msg_v0))
			return VENDOR_RC_NOT_ALLOWED;

		kh = (union u2f_key_handle_variant *)&msg->corp.key_handle;
		origin = msg->corp.app_id;
		pubKey = &msg->corp.data.public_key;
	} else {
		return VENDOR_RC_NOT_ALLOWED;
	}

	/*
	 * u2f_attest first consume all data from request 'req', and compute
	 * result in temporary storage. Once accomplished, it stores it in
	 * provided buffer. This allows overlap between input and output
	 * parameters.
	 * The response is smaller than the request, so we have the space.
	 */
	resp = buf;

	/* TODO: If WebAuthn support is needed, pass AuthTimeSecret. */
	result = u2f_attest(state, kh, 0, req->userSecret, origin, NULL, pubKey,
			    req->data, u2f_attest_format_size(req->format),
			    (struct u2f_signature *)resp);

	if (result == EC_ERROR_ACCESS_DENIED)
		return VENDOR_RC_NOT_ALLOWED;

	if (result != EC_SUCCESS) {
		CPRINTF("Attestation failed");
		return VENDOR_RC_INTERNAL_ERROR;
	}

	*response_size = sizeof(*resp);
	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_U2F_ATTEST, u2f_attest_cmd);
