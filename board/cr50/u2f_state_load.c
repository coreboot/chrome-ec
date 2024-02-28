/* Copyright 2021 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "console.h"
#include "new_nvmem.h"
#include "nvmem.h"
#include "nvmem_vars.h"
#include "tpm_nvmem.h"
#include "tpm_nvmem_ops.h"
#include "tpm_vendor_cmds.h"
#include "u2f_impl.h"
#include "util.h"

/* For test/u2f.c we provide a mock-up implementation of u2f_get_state(). */
#ifndef U2F_TEST
static const uint8_t k_salt = NVMEM_VAR_G2F_SALT;
static const uint8_t k_salt_deprecated = NVMEM_VAR_U2F_SALT;

#define CPRINTF(format, args...) cprintf(CC_EXTENSION, format, ##args)

bool u2f_load_or_create_state(struct u2f_state *state, bool force_create,
			      bool commit)
{
	bool g2f_secret_was_created = false;

	const struct tuple *t_salt = NULL;

	t_salt = getvar(&k_salt, sizeof(k_salt));

	if (force_create && t_salt) {
		/* Remove k_salt variable first. */
		freevar(t_salt);
		setvar(&k_salt, sizeof(k_salt), NULL, 0);
		t_salt = NULL;
	}

	/* Load or create G2F secret. */
	if (!t_salt) {
		g2f_secret_was_created = true;
		if (u2f_generate_g2f_secret(state) != EC_SUCCESS)
			return false;

		/* Delete the old salt if present, no-op if not. */
		if (setvar(&k_salt_deprecated, sizeof(k_salt_deprecated), NULL,
			   0) != EC_SUCCESS)
			return false;
		if (setvar(&k_salt, sizeof(k_salt),
			   (const uint8_t *)state->salt,
			   sizeof(state->salt)) != EC_SUCCESS)
			return false;
	} else {
		memcpy(state->salt, tuple_val(t_salt), sizeof(state->salt));
		freevar(t_salt);
	}

	/* Load or create HMAC key. Force creation if G2F wasn't loaded. */
	if (g2f_secret_was_created ||
	    read_tpm_nvmem_hidden(TPM_HIDDEN_U2F_KEK, sizeof(state->hmac_key),
				  state->hmac_key) != TPM_READ_SUCCESS) {
		if (u2f_generate_hmac_key(state) != EC_SUCCESS)
			return false;

		if (write_tpm_nvmem_hidden(
			    TPM_HIDDEN_U2F_KEK, sizeof(state->hmac_key),
			    state->hmac_key, commit) == TPM_WRITE_FAIL)
			return false;
	}

	/* Load or create DRBG entropy. Force creation if G2F wasn't loaded. */
	state->drbg_entropy_size = read_tpm_nvmem_size(TPM_HIDDEN_U2F_KH_SALT);

	if (g2f_secret_was_created ||
	    ((state->drbg_entropy_size != sizeof(state->drbg_entropy)) &&
	     (state->drbg_entropy_size != 32)) ||
	    (read_tpm_nvmem_hidden(TPM_HIDDEN_U2F_KH_SALT,
				   state->drbg_entropy_size,
				   state->drbg_entropy) != TPM_READ_SUCCESS)) {

		if (u2f_generate_drbg_entropy(state) != EC_SUCCESS)
			return false;

		/**
		 * We are in the inconsistent state with only G2F valid.
		 * This could be a result of very old platform being updated.
		 * In such case continue to use old, non FIPS path which is
		 * indicated by 'old' DRBG entropy size.
		 *
		 * Note, that if keys weren't properly created all at once it
		 * will continue in non-FIPS mode until keys are deleted and
		 * properly created again.
		 */
		if (!g2f_secret_was_created)
			state->drbg_entropy_size = 32;

		if (write_tpm_nvmem_hidden(
			    TPM_HIDDEN_U2F_KH_SALT, state->drbg_entropy_size,
			    state->drbg_entropy, commit) == TPM_WRITE_FAIL) {
			state->drbg_entropy_size = 0;
			return false;
		}
	}

	/**
	 * If we loaded G2F secrets, but failed to load U2F secrets, it means
	 * we should continue in non FIPS mode until all keys will be recreated
	 * properly.
	 *
	 * On first run after update:
	 *  1. Load G2F key
	 *  2. Failed or succeeded to load HMAC. Failing at this point means
	 *     DRBG load will also fail.
	 *  3. Failed to load DRBG, created DRBG with size = 32 as
	 *     g2f_secret_was_created == false
	 *
	 * On subsequent runs it will load DRBG size == 32 until keys would be
	 * removed and recreated.
	 */

	return true;
}

/**
 * Get the current u2f state from the board.
 */
static bool u2f_state_loaded;
static struct u2f_state u2f_state;

static struct u2f_state *u2f_get_state_common(bool commit)
{
	if (!u2f_state_loaded) {
		u2f_state_loaded =
			u2f_load_or_create_state(&u2f_state, false, commit);
	}
	return u2f_state_loaded ? &u2f_state : NULL;
}

struct u2f_state *u2f_get_state(void)
{
	return u2f_get_state_common(true);
}

struct u2f_state *u2f_get_state_no_commit(void)
{
	return u2f_get_state_common(false);
}

enum ec_error_list u2f_gen_kek_seed(void)
{
	/**
	 * If U2F state is loaded, update HMAC key in memory, otherwise this
	 * is just temporary storage and will be updated (to the same value)
	 * in u2f_load_or_create_state() when u2f_get_state() will be called
	 * upon use of U2F.
	 */
	if (u2f_generate_hmac_key(&u2f_state) != EC_SUCCESS)
		return EC_ERROR_HW_INTERNAL;

	/* Store new U2F HMAC key in nvmem */
	if (write_tpm_nvmem_hidden(TPM_HIDDEN_U2F_KEK,
				   sizeof(u2f_state.hmac_key),
				   u2f_state.hmac_key, 0) == TPM_WRITE_FAIL) {
		/**
		 * Failure to write means we now have inconsistent state
		 * between u2f_state and nvmem, so mark it as not loaded.
		 */
		u2f_state_loaded = false;
		return EC_ERROR_UNKNOWN;
	}

	return EC_SUCCESS;
}

/* Can't include TPM2 headers, so just define constant locally. */
#define TPM_HT_HIDDEN ((uint8_t)0xfe)
#define HR_SHIFT      24
#define HR_HIDDEN     (TPM_HT_HIDDEN << HR_SHIFT)

enum ec_error_list u2f_zeroize_keys(void)
{
	const uint32_t u2fobjs[] = { TPM_HIDDEN_U2F_KEK | HR_HIDDEN,
				     TPM_HIDDEN_U2F_KH_SALT | HR_HIDDEN, 0 };

	enum ec_error_list result1, result2;

	/* Delete NVMEM_VAR_G2F_SALT. */
	result1 = setvar(&k_salt, sizeof(k_salt), NULL, 0);

	/* Remove U2F keys and wipe all deleted objects. */
	result2 = nvmem_erase_tpm_data_selective(u2fobjs);

	always_memset(&u2f_state, 0, sizeof(u2f_state));
	u2f_state_loaded = false;
	if ((result1 == EC_SUCCESS) && (result2 != EC_SUCCESS))
		result1 = result2;

	return result1;
}

bool u2f_keys_are_fips(void)
{
	struct u2f_state *state = u2f_get_state();

	/* if we couldn't load state or state is not representing new keys */
	if (!state || state->drbg_entropy_size != sizeof(state->drbg_entropy))
		return false;

	return true;
}

enum ec_error_list u2f_update_keys(void)
{
	struct u2f_state *state = u2f_get_state();
	enum ec_error_list result = EC_SUCCESS;

	/* if we couldn't load state or state is not representing new keys */
	if (!state || state->drbg_entropy_size != sizeof(state->drbg_entropy)) {
		result = u2f_zeroize_keys();
		/* Force creation of new keys. */
		u2f_state_loaded =
			u2f_load_or_create_state(&u2f_state, true, true);

		/* try to load again */
		state = u2f_get_state();
	}
	if (!state || state->drbg_entropy_size != sizeof(state->drbg_entropy))
		result = EC_ERROR_HW_INTERNAL;

	return result;
}

#endif /* U2F_TEST */
