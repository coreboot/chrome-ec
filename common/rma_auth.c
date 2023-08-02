/* Copyright 2017 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* RMA authorization challenge-response */

#include "common.h"
#include "base32.h"
#include "byteorder.h"
#include "ccd_config.h"
#include "chip/g/board_id.h"
#include "console.h"
#ifdef CONFIG_CURVE25519
#include "curve25519.h"
#endif
#include "extension.h"
#include "hooks.h"
#include "rma_auth.h"
#include "shared_mem.h"
#include "system.h"
#include "timer.h"
#include "tpm_registers.h"
#include "tpm_vendor_cmds.h"
#ifdef CONFIG_RMA_AUTH_USE_P256
#include "dcrypto.h"
#endif
#include "util.h"

#ifndef TEST_BUILD
#include "rma_key_from_blob.h"
#else
/* Cryptoc library is not available to the test layer. */
#define always_memset memset
#endif

#if defined(CONFIG_DCRYPTO) || defined(CONFIG_DCRYPTO_BOARD)
#include "dcrypto.h"
#define USE_DCRYPTO
#else
#include "sha256.h"
#endif

#define CPRINTF(format, args...) cprintf(CC_EXTENSION, format, ## args)

/* Minimum time since system boot or last challenge before making a new one */
#define CHALLENGE_INTERVAL (10 * SECOND)

/* Number of tries to properly enter auth code */
#define MAX_AUTHCODE_TRIES 3

#ifdef CONFIG_RMA_AUTH_USE_P256
#define RMA_SERVER_PUB_KEY_SZ 65
#else
#define RMA_SERVER_PUB_KEY_SZ 32
#endif

/* Server public key and key ID */
static const struct  {
	union {
		uint8_t raw_blob[RMA_SERVER_PUB_KEY_SZ + 1];
		struct {
			uint8_t server_pub_key[RMA_SERVER_PUB_KEY_SZ];
			volatile uint8_t server_key_id;
		};
	};
} __packed rma_key_blob = {
	.raw_blob = RMA_KEY_BLOB
};

BUILD_ASSERT(sizeof(rma_key_blob) == (RMA_SERVER_PUB_KEY_SZ + 1));

static char challenge[RMA_CHALLENGE_BUF_SIZE];
static char authcode[RMA_AUTHCODE_BUF_SIZE];
static int tries_left;
static uint64_t last_challenge_time;

static enum ec_error_list get_hmac_sha256(void *hmac_out, const uint8_t *secret,
			    size_t secret_size, const void *ch_ptr,
			    size_t ch_size)
{
#ifdef USE_DCRYPTO
	struct hmac_sha256_ctx hmac;

	if (DCRYPTO_hw_hmac_sha256_init(&hmac, secret, secret_size) !=
	    DCRYPTO_OK)
		return EC_ERROR_HW_INTERNAL;
	HMAC_SHA256_update(&hmac, ch_ptr, ch_size);
	memcpy(hmac_out, HMAC_SHA256_final(&hmac), 32);
#else
	hmac_SHA256(hmac_out, secret, secret_size, ch_ptr, ch_size);
#endif
	return EC_SUCCESS;
}

static enum ec_error_list hash_buffer(void *dest, size_t dest_size,
				      const void *buffer, size_t buf_size)
{
	/* We know that the destination is no larger than 32 bytes. */
	uint8_t temp[32];
	enum ec_error_list ret;

	ret = get_hmac_sha256(temp, buffer, buf_size, buffer, buf_size);
	if (ret)
		return ret;

	/* Or should we do XOR of the temp modulo dest size? */
	memcpy(dest, temp, dest_size);
	return EC_SUCCESS;
}

#ifdef CONFIG_RMA_AUTH_USE_P256
/*
 * Generate a p256 key pair, such that Y coordinate component of the public
 * key is an odd value. Use the X component value as the compressed public key
 * to be sent to the server. Multiply server public key by our private key to
 * generate the shared secret.
 *
 * @pub_key - array to return 32 bytes of the X coordinate public key
 *	      component.
 * @secet - array to return the X coordinate of the product of the server
 *            public key multiplied by our private key.
 */
static int p256_get_pub_key_and_secret(uint8_t pub_key[P256_NBYTES],
					uint8_t secret[P256_NBYTES])
{
	uint8_t buf[SHA256_DIGEST_SIZE];
	p256_int d;
	p256_int pk_x;
	p256_int pk_y;

	/* Get some noise for private key. */
	if (!fips_rand_bytes(buf, sizeof(buf)))
		return EC_ERROR_HW_INTERNAL;

	/*
	 * By convention with the RMA server the Y coordinate of the Cr50
	 * public key component is required to be an odd value. Keep trying
	 * until the genreated bublic key has the compliant Y coordinate.
	 */
	while (1) {
		struct sha256_ctx sha;
		enum dcrypto_result result;

		result = DCRYPTO_p256_key_from_bytes(&pk_x, &pk_y, &d, buf);

		if (result == DCRYPTO_FAIL)
			return EC_ERROR_HW_INTERNAL;

		if (result == DCRYPTO_OK) {

			/* Is Y coordinate an odd value? */
			if (p256_is_odd(&pk_y))
				break; /* Yes it is, got a good key. */
		}

		/* Did not succeed, rehash the private key and try again. */
		if (DCRYPTO_hw_sha256_init(&sha) != DCRYPTO_OK)
			return EC_ERROR_HW_INTERNAL;
		SHA256_update(&sha, buf, sizeof(buf));
		memcpy(buf, SHA256_final(&sha), sizeof(buf));
	}

	/* X coordinate is passed to the server as the public key. */
	p256_to_bin(&pk_x, pub_key);

	/*
	 * Now let's calculate the secret as a the server pub key multiplied
	 * by our private key.
	 */
	p256_from_bin(rma_key_blob.raw_blob + 1, &pk_x);
	p256_from_bin(rma_key_blob.raw_blob + 1 + P256_NBYTES, &pk_y);

	/* Use input space for storing multiplication results. */
	if (DCRYPTO_p256_point_mul(&pk_x, &pk_y, &d, &pk_x, &pk_y) !=
	    DCRYPTO_OK)
		return EC_ERROR_HW_INTERNAL;

	/* X value is the seed for the shared secret. */
	p256_to_bin(&pk_x, secret);

	/* Wipe out the private key just in case. */
	always_memset(&d, 0, sizeof(d));
	return EC_SUCCESS;
}
#endif

int get_rma_device_id(uint8_t rma_device_id[RMA_DEVICE_ID_SIZE])
{
	uint8_t *chip_unique_id;
	int chip_unique_id_size = system_get_chip_unique_id(&chip_unique_id);
	enum ec_error_list ret;

	if (chip_unique_id_size < 0)
		chip_unique_id_size = 0;
	/* Smaller unique chip IDs will fill rma_device_id only partially. */
	if (chip_unique_id_size <= RMA_DEVICE_ID_SIZE) {
		/* The size matches, let's just copy it as is. */
		memcpy(rma_device_id, chip_unique_id, chip_unique_id_size);
		if (chip_unique_id_size < RMA_DEVICE_ID_SIZE) {
			memset(rma_device_id + chip_unique_id_size, 0,
				RMA_DEVICE_ID_SIZE - chip_unique_id_size);
		}
	} else {
		/*
		 * The unique chip ID size exceeds space allotted in
		 * rma_challenge:device_id, let's use first few bytes of
		 * its hash.
		 */
		ret = hash_buffer(rma_device_id, RMA_DEVICE_ID_SIZE,
				  chip_unique_id, chip_unique_id_size);
		if (ret != EC_SUCCESS)
			return ret;
	}
	return EC_SUCCESS;
}

/**
 * Create a new RMA challenge/response
 *
 * @return EC_SUCCESS, EC_ERROR_TIMEOUT if too soon since the last challenge,
 * or other non-zero error code.
 */
int rma_create_challenge(void)
{
	uint8_t temp[32];	/* Private key or HMAC */
	uint8_t secret[32];
	struct rma_challenge c;
	struct board_id bid;
	uint8_t *cptr = (uint8_t *)&c;
	uint64_t t;
	int ret;

	/* Clear the current challenge and authcode, if any */
	memset(challenge, 0, sizeof(challenge));
	memset(authcode, 0, sizeof(authcode));

	/* Rate limit challenges */
	t = get_time().val;
	if (t - last_challenge_time < CHALLENGE_INTERVAL)
		return EC_ERROR_TIMEOUT;
	last_challenge_time = t;

	memset(&c, 0, sizeof(c));
	c.version_key_id = RMA_CHALLENGE_VKID_BYTE(
	    RMA_CHALLENGE_VERSION, rma_key_blob.server_key_id);

	if (read_board_id(&bid))
		return EC_ERROR_UNKNOWN;

	memcpy(c.board_id, &bid.type, sizeof(c.board_id));
	ret = get_rma_device_id(c.device_id);
	if (ret != EC_SUCCESS)
		return ret;

		/* Calculate a new ephemeral key pair and the shared secret. */
#ifdef CONFIG_RMA_AUTH_USE_P256
	if (p256_get_pub_key_and_secret(c.device_pub_key, secret) != EC_SUCCESS)
		return EC_ERROR_UNKNOWN;
#endif
#ifdef CONFIG_CURVE25519
	X25519_keypair(c.device_pub_key, temp);
	X25519(secret, temp, rma_key_blob.server_pub_key);
#endif
	/* Encode the challenge */
	if (base32_encode(challenge, sizeof(challenge), cptr, 8 * sizeof(c), 9))
		return EC_ERROR_UNKNOWN;

	/*
	 * Auth code is a truncated HMAC of the ephemeral public key, BoardID,
	 * and DeviceID.  Those are all in the right order in the challenge
	 * struct, after the version/key id byte.
	 */
	ret = get_hmac_sha256(temp, secret, sizeof(secret), cptr + 1,
			      sizeof(c) - 1);
	if (ret != EC_SUCCESS)
		return ret;
	if (base32_encode(authcode, sizeof(authcode), temp,
			  RMA_AUTHCODE_CHARS * 5, 0))
		return EC_ERROR_UNKNOWN;

	tries_left = MAX_AUTHCODE_TRIES;
	return EC_SUCCESS;
}

const char *rma_get_challenge(void)
{
	return challenge;
}

int rma_try_authcode(const char *code)
{
	int rv = EC_ERROR_INVAL;

	/* Fail if out of tries */
	if (!tries_left)
		return EC_ERROR_ACCESS_DENIED;

	/* Fail if auth code has not been calculated yet. */
	if (!*authcode)
		return EC_ERROR_ACCESS_DENIED;

	if (safe_memcmp(authcode, code, RMA_AUTHCODE_CHARS)) {
		/* Mismatch */
		tries_left--;
	} else {
		rv = EC_SUCCESS;
		tries_left = 0;
	}

	/* Clear challenge and response if out of tries */
	if (!tries_left) {
		memset(challenge, 0, sizeof(challenge));
		memset(authcode, 0, sizeof(authcode));
	}

	return rv;
}

#ifndef TEST_BUILD
/*
 * Trigger generating of the new challenge/authcode pair. If successful, store
 * the challenge in the vendor command response buffer and send it to the
 * sender. If not successful - return the error value to the sender.
 */
static enum vendor_cmd_rc get_challenge(uint8_t *buf, size_t *buf_size)
{
	int rv;
	size_t i;

	if (*buf_size < sizeof(challenge)) {
		*buf_size = 1;
		buf[0] = VENDOR_RC_RESPONSE_TOO_BIG;
		return buf[0];
	}

	rv = rma_create_challenge();
	if (rv != EC_SUCCESS) {
		*buf_size = 1;
		buf[0] = rv;
		return buf[0];
	}

	*buf_size = sizeof(challenge) - 1;
	memcpy(buf, rma_get_challenge(), *buf_size);


	CPRINTF("generated challenge:\n\n");
	for (i = 0; i < *buf_size; i++)
		CPRINTF("%c", ((uint8_t *)buf)[i]);
	CPRINTF("\n\n");

#ifdef CR50_DEV

	CPRINTF("expected authcode: ");
	for (i = 0; i < RMA_AUTHCODE_CHARS; i++)
		CPRINTF("%c", authcode[i]);
	CPRINTF("\n");
#endif
	return VENDOR_RC_SUCCESS;
}
/*
 * Compare response sent by the operator with the pre-compiled auth code.
 * Return error code or success depending on the comparison results.
 */
static enum vendor_cmd_rc process_response(uint8_t *buf,
					   size_t input_size,
					   size_t *response_size)
{
	int rv;

	*response_size = 1; /* Just in case there is an error. */

	if (input_size != RMA_AUTHCODE_CHARS) {
		CPRINTF("%s: authcode size %d\n",
			__func__, input_size);
		buf[0] = VENDOR_RC_BOGUS_ARGS;
		return buf[0];
	}

	rv = rma_try_authcode(buf);

	if (rv == EC_SUCCESS) {
		CPRINTF("%s: success!\n", __func__);
		*response_size = 0;
		enable_ccd_factory_mode(0);
		return VENDOR_RC_SUCCESS;
	}

	CPRINTF("%s: authcode mismatch\n", __func__);
	buf[0] = VENDOR_RC_INTERNAL_ERROR;
	return buf[0];
}

/*
 * Handle the VENDOR_CC_RMA_CHALLENGE_RESPONSE command. When received with
 * empty payload - this is a request to generate a new challenge, when
 * received with a payload, this is a request to check if the payload matches
 * the previously calculated auth code.
 */
static enum vendor_cmd_rc rma_challenge_response(enum vendor_cmd_cc code,
						 void *buf,
						 size_t input_size,
						 size_t *response_size)
{
	if (!input_size)
		/*
		 * This is a request for the challenge, get it and send it
		 * back.
		 */
		return get_challenge(buf, response_size);

	return process_response(buf, input_size, response_size);
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_RMA_CHALLENGE_RESPONSE,
		       rma_challenge_response);


#define RMA_CMD_BUF_SIZE (sizeof(struct tpm_cmd_header) + \
			  RMA_CHALLENGE_BUF_SIZE)
static int rma_auth_cmd(int argc, char **argv)
{
	struct tpm_cmd_header *tpmh;
	int rv;

	if (argc > 2) {
		ccprintf("Error: the only accepted parameter is"
			 " the auth code to check\n");
		return EC_ERROR_PARAM_COUNT;
	}

	rv = shared_mem_acquire(RMA_CMD_BUF_SIZE, (char **)&tpmh);
	if (rv != EC_SUCCESS)
		return rv;

	/* Common fields of the RMA AUTH challenge/response vendor command. */
	tpmh->tag = htobe16(0x8001); /* TPM_ST_NO_SESSIONS */
	tpmh->command_code = htobe32(TPM_CC_VENDOR_BIT_MASK);
	tpmh->subcommand_code = htobe16(VENDOR_CC_RMA_CHALLENGE_RESPONSE);

	if (argc == 2) {
		/*
		 * The user entered a value, must be the auth code, build and
		 * send vendor command to check it.
		 */
		const char *authcode = argv[1];

		if (strlen(authcode) != RMA_AUTHCODE_CHARS) {
			ccprintf("Wrong auth code size.\n");
			return EC_ERROR_PARAM1;
		}

		tpmh->size = htobe32(sizeof(struct tpm_cmd_header) +
				     RMA_AUTHCODE_CHARS);

		memcpy(tpmh + 1, authcode, RMA_AUTHCODE_CHARS);

		tpm_alt_extension(tpmh, RMA_CMD_BUF_SIZE);

		if (tpmh->command_code) {
			ccprintf("Auth code does not match.\n");
			return EC_ERROR_PARAM1;
		}
		ccprintf("Auth code match, reboot might be coming!\n");
		return EC_SUCCESS;
	}

	/* Prepare and send the request to get RMA auth challenge. */
	tpmh->size = htobe32(sizeof(struct tpm_cmd_header));
	tpm_alt_extension(tpmh, RMA_CMD_BUF_SIZE);

	/* Return status in the command code field now. */
	if (tpmh->command_code) {
		ccprintf("RMA Auth error 0x%x\n", be32toh(tpmh->command_code));
		rv = EC_ERROR_UNKNOWN;
	}

	shared_mem_release(tpmh);
	return EC_SUCCESS;
}

DECLARE_SAFE_CONSOLE_COMMAND(rma_auth, rma_auth_cmd, NULL,
			     "rma_auth [auth code] - "
			"Generate RMA challenge or check auth code match\n");
#endif
