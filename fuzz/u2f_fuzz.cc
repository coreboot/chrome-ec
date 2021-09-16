/* Copyright 2021 The Chromium OS Authors. All rights reserved.
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
int DCRYPTO_x509_gen_u2f_cert_name(const p256_int *d, const p256_int *pk_x,
				   const p256_int *pk_y, const p256_int *serial,
				   const char *name, uint8_t *cert, const int n)
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
		struct u2f_generate_resp *resp;
		struct u2f_generate_versioned_resp *versioned_resp;
		struct u2f_sign_req *sign_req;
		struct u2f_sign_versioned_req *sign_versioned_req;
		struct u2f_attest_req *attest_req;
		struct g2f_register_msg *g2f_msg;
		uint8_t appId[U2F_APPID_SIZE];
		uint8_t userSecret[U2F_USER_SECRET_SIZE];
		uint8_t flags;
		struct u2f_key_handle kh;
		struct u2f_versioned_key_handle versioned_kh;
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
			flags = generate_req->flags;
			response_size = 512;
			rc = u2f_generate_cmd(VENDOR_CC_U2F_GENERATE, buffer,
					      sizeof(struct u2f_generate_req),
					      &response_size);
			if (rc != VENDOR_RC_SUCCESS) {
				break;
			}
			kh_version = (response_size ==
				      sizeof(struct u2f_generate_resp)) ?
						   0 :
						   1;
			if (!kh_version) {
				resp = (u2f_generate_resp *)buffer;
				kh = resp->keyHandle;
				public_key = resp->pubKey;
				sign_req = (u2f_sign_req *)buffer;
				memcpy(sign_req->appId, appId, U2F_APPID_SIZE);
				memcpy(sign_req->userSecret, userSecret,
				       U2F_USER_SECRET_SIZE);
				sign_req->flags = flags;
				sign_req->keyHandle = kh;
				bytes = data_provider.ConsumeBytes<uint8_t>(
					U2F_P256_SIZE);
				memcpy(sign_req->hash, bytes.data(),
				       bytes.size());
				request_size = sizeof(struct u2f_sign_req);
			} else {
				versioned_resp =
					(u2f_generate_versioned_resp *)buffer;
				versioned_kh = versioned_resp->keyHandle;
				sign_versioned_req =
					(u2f_sign_versioned_req *)buffer;
				memcpy(sign_versioned_req->appId, appId,
				       U2F_APPID_SIZE);
				memcpy(sign_versioned_req->userSecret,
				       userSecret, U2F_USER_SECRET_SIZE);
				sign_versioned_req->flags = flags;
				sign_versioned_req->keyHandle = versioned_kh;
				bytes = data_provider.ConsumeBytes<uint8_t>(
					U2F_P256_SIZE);
				memcpy(sign_versioned_req->hash, bytes.data(),
				       bytes.size());
				request_size =
					sizeof(struct u2f_sign_versioned_req);
			}
			response_size = 512;
			u2f_sign_cmd(VENDOR_CC_U2F_SIGN, buffer, request_size,
				     &response_size);
			if (!kh_version) {
				attest_req = (u2f_attest_req *)buffer;
				attest_req->format = U2F_ATTEST_FORMAT_REG_RESP;
				attest_req->dataLen =
					sizeof(struct g2f_register_msg);
				memcpy(attest_req->userSecret, userSecret,
				       U2F_USER_SECRET_SIZE);
				g2f_msg = (g2f_register_msg *)attest_req->data;
				g2f_msg->reserved = 0;
				memcpy(g2f_msg->app_id, appId, U2F_APPID_SIZE);
				memcpy(g2f_msg->key_handle.hmac, kh.hmac,
				       sizeof(kh.hmac));
				memcpy(g2f_msg->key_handle.origin_seed,
				       kh.origin_seed, sizeof(kh.origin_seed));
				g2f_msg->public_key = public_key;
				bytes = data_provider.ConsumeBytes<uint8_t>(
					U2F_CHAL_SIZE);
				memcpy(g2f_msg->challenge, bytes.data(),
				       bytes.size());
				response_size = 512;
				u2f_attest_cmd(VENDOR_CC_U2F_ATTEST, buffer,
					       sizeof(struct u2f_attest_req),
					       &response_size);
			}
			break;
		case 1: {
			bool is_versioned = data_provider.ConsumeBool();
			request_size =
				is_versioned ?
					      sizeof(struct u2f_sign_versioned_req) :
					      sizeof(struct u2f_sign_req);
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
			attest_req->dataLen = sizeof(struct g2f_register_msg);
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