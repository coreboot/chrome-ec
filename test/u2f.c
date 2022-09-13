/* Copyright 2021 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "u2f_cmds.h"
#include "physical_presence.h"
#include "test_util.h"
#include "u2f_impl.h"

#include "internal.h"
#include "util.h"

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
int DCRYPTO_ladder_random(void *output)
{
	memset(output, 0, P256_NBYTES);
	/* Return 1 for success, 0 for error. */
	return 1;
}

bool fips_rand_bytes(void *buffer, size_t len)
{
	memset(buffer, 1, len);
	return true;
}

bool fips_trng_bytes(void *buffer, size_t len)
{
	memset(buffer, 2, len);
	return true;
}

size_t DCRYPTO_x509_gen_u2f_cert_name(const p256_int *d, const p256_int *pk_x,
				      const p256_int *pk_y,
				      const p256_int *serial, const char *name,
				      uint8_t *cert, const size_t n)
{
	/* Return the size of certificate, 0 means error. */
	return 0;
}

enum dcrypto_result DCRYPTO_p256_key_from_bytes(p256_int *x, p256_int *y,
		p256_int *d, const uint8_t key_bytes[P256_NBYTES])
{
	p256_int key;

	p256_from_bin(key_bytes, &key);

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
static int presence;

static struct u2f_state state;

struct u2f_state *u2f_get_state(void)
{
	state.drbg_entropy_size = 64;
	return &state;
}

struct u2f_state *u2f_get_state_no_commit(void)
{
	return u2f_get_state();
}

enum touch_state pop_check_presence(int consume)
{
	enum touch_state ret = presence ? POP_TOUCH_YES : POP_TOUCH_NO;

	if (consume)
		presence = 0;
	return ret;
}

/******************************************************************************/
/* Tests begin here.
 */
static uint8_t buffer[512];

test_static int test_u2f_generate_no_require_presence(void)
{
	struct u2f_generate_req *req = (struct u2f_generate_req *)buffer;
	size_t response_size = sizeof(struct u2f_generate_resp);
	int ret;

	memset(buffer, 0, sizeof(buffer));
	req->flags = 0;
	presence = 0;
	ret = u2f_generate_cmd(VENDOR_CC_U2F_GENERATE, &buffer,
			       sizeof(struct u2f_generate_req), &response_size);

	TEST_ASSERT(ret == VENDOR_RC_SUCCESS);
	return EC_SUCCESS;
}

test_static int test_u2f_generate_require_presence(void)
{
	struct u2f_generate_req *req = (struct u2f_generate_req *)buffer;
	size_t response_size = sizeof(struct u2f_generate_resp);
	int ret;

	memset(buffer, 0, sizeof(buffer));
	req->flags = U2F_AUTH_FLAG_TUP;
	presence = 0;
	ret = u2f_generate_cmd(VENDOR_CC_U2F_GENERATE, &buffer,
			       sizeof(struct u2f_generate_req), &response_size);
	TEST_ASSERT(ret == VENDOR_RC_NOT_ALLOWED);

	memset(buffer, 0, sizeof(buffer));
	req->flags = U2F_AUTH_FLAG_TUP;
	response_size = sizeof(struct u2f_generate_resp);
	presence = 1;
	ret = u2f_generate_cmd(VENDOR_CC_U2F_GENERATE, &buffer,
			       sizeof(struct u2f_generate_req), &response_size);
	TEST_ASSERT(ret == VENDOR_RC_SUCCESS);

	return EC_SUCCESS;
}

void run_test(void)
{
	RUN_TEST(test_u2f_generate_no_require_presence);
	RUN_TEST(test_u2f_generate_require_presence);

	test_print_result();
}
