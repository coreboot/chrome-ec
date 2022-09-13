/* Copyright 2021 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <fuzzer/FuzzedDataProvider.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#define HIDE_EC_STDLIB
extern "C" {
#include "physical_presence.h"
#include "u2f_cmds.h"
#include "u2f_impl.h"
#include "internal.h"
}

extern "C" {
/******************************************************************************/
/* Mock implementations of cr50 board.
 */
int system_get_chip_unique_id(uint8_t **id)
{
	return P256_NBYTES;
}

/******************************************************************************/
/* Mock implementations of Dcrypto functionality.
 */
size_t DCRYPTO_x509_gen_u2f_cert_name(const p256_int *d, const p256_int *pk_x,
				      const p256_int *pk_y,
				      const p256_int *serial, const char *name,
				      uint8_t *cert, const size_t n)
{
	memset(cert, 1, n);
	return n;
}

enum dcrypto_result DCRYPTO_p256_key_from_bytes(
	p256_int *x, p256_int *y, p256_int *d,
	const uint8_t key_bytes[P256_NBYTES])
{
	p256_int key;

	p256_from_bin(key_bytes, &key);

	// The actual condition for this function to fail happens rarely,
	// and not able to to control. So we assume it fails for some inputs
	// for fuzz purpose.
	if (P256_DIGIT(&key, 0) % 10 == 0) {
		return DCRYPTO_RETRY;
	}

	if (p256_lt_blinded(&key, &SECP256r1_nMin2) >= 0)
		return DCRYPTO_RETRY;
	p256_add_d(&key, 1, d);
	if (x == NULL || y == NULL)
		return DCRYPTO_OK;
	memset(x, 0, P256_NBYTES);
	memset(y, 0, P256_NBYTES);
	return DCRYPTO_OK;
}

enum dcrypto_result dcrypto_p256_ecdsa_sign(struct drbg_ctx *drbg,
					    const p256_int *key,
					    const p256_int *message,
					    p256_int *r, p256_int *s)
{
	memset(r, 0, sizeof(p256_int));
	memset(s, 0, sizeof(p256_int));
	return DCRYPTO_OK;
}

/******************************************************************************/
/* Mock implementations of U2F functionality.
 */
static struct u2f_state ustate;
struct u2f_state *u2f_get_state(void)
{
	return &ustate;
}

struct u2f_state *u2f_get_state_no_commit(void)
{
	return &ustate;
}

static enum touch_state tstate;
enum touch_state pop_check_presence(int consume)
{
	return tstate;
}

uint8_t buffer[512];

int test_fuzz_one_input(const uint8_t *data, unsigned int size)
{
	FuzzedDataProvider data_provider(data, size);

	if (data_provider.ConsumeBool()) {
		ustate.drbg_entropy_size = 64;
	} else {
		ustate.drbg_entropy_size = 32;
	}

	if (data_provider.ConsumeBool()) {
		tstate = POP_TOUCH_YES;
	} else {
		tstate = POP_TOUCH_NO;
	}

	while (data_provider.remaining_bytes() > 0) {
		std::vector<uint8_t> bytes;
		int command_num = data_provider.ConsumeIntegralInRange(0, 2);

		size_t request_size, response_size;
		struct u2f_generate_req *generate_req;
		struct u2f_generate_resp *generate_resp_v0;
		struct u2f_generate_versioned_resp *generate_resp_v1;
		struct u2f_generate_versioned_resp_v2 *generate_resp_v2;
		struct u2f_sign_req *sign_req_v0;
		struct u2f_sign_versioned_req *sign_req_v1;
		struct u2f_sign_versioned_req_v2 *sign_req_v2;
		struct u2f_attest_req *attest_req;
		struct g2f_register_msg_v0 *g2f_msg_v0;
		uint8_t appId[U2F_APPID_SIZE];
		uint8_t userSecret[U2F_USER_SECRET_SIZE];
		uint8_t authTimeSecretHash[U2F_AUTH_TIME_SECRET_SIZE];
		uint8_t flags;
		struct u2f_key_handle kh_0;
		struct u2f_versioned_key_handle kh_1;
		struct u2f_key_handle_v2 kh_2;
		struct u2f_ec_point public_key;
		int kh_version;
		vendor_cmd_rc rc;

		switch (command_num) {
		case 0:
			bytes = data_provider.ConsumeBytes<uint8_t>(
				sizeof(struct u2f_generate_req));
			memcpy(buffer, bytes.data(), bytes.size());
			generate_req = (u2f_generate_req *)buffer;
			memcpy(appId, generate_req->appId, U2F_APPID_SIZE);
			memcpy(userSecret, generate_req->userSecret,
			       U2F_USER_SECRET_SIZE);
			memcpy(authTimeSecretHash,
			       generate_req->authTimeSecretHash,
			       U2F_AUTH_TIME_SECRET_SIZE);
			flags = generate_req->flags;
			response_size = 512;
			rc = u2f_generate_cmd(VENDOR_CC_U2F_GENERATE, buffer,
					      sizeof(struct u2f_generate_req),
					      &response_size);
			if (rc != VENDOR_RC_SUCCESS) {
				break;
			}
			if (response_size == sizeof(struct u2f_generate_resp)) {
				kh_version = 0;
			} else if (response_size ==
				   sizeof(struct u2f_generate_versioned_resp)) {
				kh_version = 1;
			} else if (response_size ==
				   sizeof(struct u2f_generate_versioned_resp_v2)) {
				kh_version = 2;
			} else {
				exit(1);
			}
			if (kh_version == 0) {
				generate_resp_v0 = (u2f_generate_resp *)buffer;
				kh_0 = generate_resp_v0->keyHandle;
				public_key = generate_resp_v0->pubKey;
				sign_req_v0 = (u2f_sign_req *)buffer;
				memcpy(sign_req_v0->appId, appId,
				       U2F_APPID_SIZE);
				memcpy(sign_req_v0->userSecret, userSecret,
				       U2F_USER_SECRET_SIZE);
				sign_req_v0->flags = flags;
				sign_req_v0->keyHandle = kh_0;
				bytes = data_provider.ConsumeBytes<uint8_t>(
					U2F_P256_SIZE);
				memcpy(sign_req_v0->hash, bytes.data(),
				       bytes.size());
				request_size = sizeof(struct u2f_sign_req);
			} else if (kh_version == 1) {
				generate_resp_v1 =
					(u2f_generate_versioned_resp *)buffer;
				kh_1 = generate_resp_v1->keyHandle;
				sign_req_v1 = (u2f_sign_versioned_req *)buffer;
				memcpy(sign_req_v1->appId, appId,
				       U2F_APPID_SIZE);
				memcpy(sign_req_v1->userSecret, userSecret,
				       U2F_USER_SECRET_SIZE);
				sign_req_v1->flags = flags;
				sign_req_v1->keyHandle = kh_1;
				bytes = data_provider.ConsumeBytes<uint8_t>(
					U2F_P256_SIZE);
				memcpy(sign_req_v1->hash, bytes.data(),
				       bytes.size());
				request_size =
					sizeof(struct u2f_sign_versioned_req);
			} else {
				generate_resp_v2 =
					(u2f_generate_versioned_resp_v2 *)buffer;
				kh_2 = generate_resp_v2->keyHandle;
				sign_req_v2 =
					(u2f_sign_versioned_req_v2 *)buffer;
				memcpy(sign_req_v2->appId, appId,
				       U2F_APPID_SIZE);
				memcpy(sign_req_v2->userSecret, userSecret,
				       U2F_USER_SECRET_SIZE);
				memcpy(sign_req_v2->authTimeSecret,
				       authTimeSecretHash,
				       U2F_AUTH_TIME_SECRET_SIZE);
				sign_req_v2->flags = flags;
				sign_req_v2->keyHandle = kh_2;
				bytes = data_provider.ConsumeBytes<uint8_t>(
					U2F_P256_SIZE);
				memcpy(sign_req_v2->hash, bytes.data(),
				       bytes.size());
				request_size = sizeof(
					struct u2f_sign_versioned_req_v2);
			}
			response_size = 512;
			u2f_sign_cmd(VENDOR_CC_U2F_SIGN, buffer, request_size,
				     &response_size);
			if (kh_version == 0) {
				attest_req = (u2f_attest_req *)buffer;
				attest_req->format = U2F_ATTEST_FORMAT_REG_RESP;
				attest_req->dataLen =
					sizeof(struct g2f_register_msg_v0);
				memcpy(attest_req->userSecret, userSecret,
				       U2F_USER_SECRET_SIZE);
				g2f_msg_v0 =
					(g2f_register_msg_v0 *)attest_req->data;
				g2f_msg_v0->reserved = 0;
				memcpy(g2f_msg_v0->app_id, appId,
				       U2F_APPID_SIZE);
				memcpy(g2f_msg_v0->key_handle.hmac, kh_0.hmac,
				       sizeof(kh_0.hmac));
				memcpy(g2f_msg_v0->key_handle.origin_seed,
				       kh_0.origin_seed,
				       sizeof(kh_0.origin_seed));
				g2f_msg_v0->public_key = public_key;
				bytes = data_provider.ConsumeBytes<uint8_t>(
					U2F_CHAL_SIZE);
				memcpy(g2f_msg_v0->challenge, bytes.data(),
				       bytes.size());
				response_size = 512;
				u2f_attest_cmd(VENDOR_CC_U2F_ATTEST, buffer,
					       sizeof(struct u2f_attest_req),
					       &response_size);
			}
			break;
		case 1: {
			int version =
				data_provider.ConsumeIntegralInRange(0, 2);
			request_size =
				(version == 0) ?
					      sizeof(struct u2f_sign_req) :
				(version == 1) ?
					      sizeof(struct u2f_sign_versioned_req) :
					      sizeof(struct u2f_sign_versioned_req_v2);
			bytes = data_provider.ConsumeBytes<uint8_t>(
				request_size);
			memcpy(buffer, bytes.data(), bytes.size());
			response_size = 512;
			u2f_sign_cmd(VENDOR_CC_U2F_SIGN, buffer, request_size,
				     &response_size);
			break;
		}
		case 2:
			auto str = data_provider.ConsumeRandomLengthString(256);
			memcpy(buffer, str.data(), str.size());
			attest_req = (u2f_attest_req *)buffer;
			attest_req->dataLen =
				sizeof(struct g2f_register_msg_v0);
			response_size = 512;
			u2f_attest_cmd(VENDOR_CC_U2F_ATTEST, buffer, str.size(),
				       &response_size);
			break;
		}
		break;
	}
	return 0;
}
}