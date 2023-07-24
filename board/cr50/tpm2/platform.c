/* Copyright 2015 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "Global.h"
#include "Platform.h"
#include "TPM_Types.h"

#include "ccd_config.h"
#include "console.h"
#include "nvmem_vars.h"
#include "pinweaver.h"
#include "pinweaver_eal.h"
#include "tpm_nvmem.h"
#include "tpm_nvmem_ops.h"
#include "dcrypto.h"
#include "u2f_impl.h"
#include "util.h"
#include "version.h"

#define CPRINTF(format, args...) cprintf(CC_EXTENSION, format, ## args)

/*
 * PCR0 values for the states when FWMP/antirollback updates are allowed.
 * For the source of the specific values see b/140958855#comment25.
 */
static const uint8_t pcr_boot_policy_update_allowed[][SHA256_DIGEST_SIZE] = {
	/* normal mode (rec=0, dev=0) */
	{
		0x89, 0xEA, 0xF3, 0x51, 0x34, 0xB4, 0xB3, 0xC6,
		0x49, 0xF4, 0x4C, 0x0C, 0x76, 0x5B, 0x96, 0xAE,
		0xAB, 0x8B, 0xB3, 0x4E, 0xE8, 0x3C, 0xC7, 0xA6,
		0x83, 0xC4, 0xE5, 0x3D, 0x15, 0x81, 0xC8, 0xC7
	},
	/* dev mode (rec=0, dev=1) */
	{
		0x23, 0xE1, 0x4D, 0xD9, 0xBB, 0x51, 0xA5, 0x0E,
		0x16, 0x91, 0x1F, 0x7E, 0x11, 0xDF, 0x1E, 0x1A,
		0xAF, 0x0B, 0x17, 0x13, 0x4D, 0xC7, 0x39, 0xC5,
		0x65, 0x36, 0x07, 0xA1, 0xEC, 0x8D, 0xD3, 0x7A
	},
	/* recovery mode (rec=1, dev=0) */
	{
		0x9F, 0x9E, 0xA8, 0x66, 0xD3, 0xF3, 0x4F, 0xE3,
		0xA3, 0x11, 0x2A, 0xE9, 0xCB, 0x1F, 0xBA, 0xBC,
		0x6F, 0xFE, 0x8C, 0xD2, 0x61, 0xD4, 0x24, 0x93,
		0xBC, 0x68, 0x42, 0xA9, 0xE4, 0xF9, 0x3B, 0x3D
	},
	/* initial PCR0 state at reset - all zeroes */
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	}
};

uint16_t _cpri__GenerateRandom(int32_t random_size,
			uint8_t *buffer)
{
	if (!fips_rand_bytes(buffer, random_size))
		return 0;
	return random_size;
}

/*
 * Return the pointer to the character immediately after the first dash
 * encountered in the passed in string, or NULL if there is no dashes in the
 * string.
 */
static const char *char_after_dash(const char *str)
{
	char c;

	do {
		c = *str++;

		if (c == '-')
			return str;
	} while (c);

	return NULL;
}

/*
 * The properly formatted build_info string has the ec code SHA1 after the
 * first dash, and tpm2 code sha1 after the second dash.
 */

void   _plat__GetFwVersion(uint32_t *firmwareV1, uint32_t *firmwareV2)
{
	const char *ver_str = char_after_dash(build_info);

	/* Just in case the build_info string is misformatted. */
	*firmwareV1 = 0;
	*firmwareV2 = 0;

	if (!ver_str)
		return;

	*firmwareV1 = strtoi(ver_str, NULL, 16);

	ver_str = char_after_dash(ver_str);
	if (!ver_str)
		return;

	*firmwareV2 = strtoi(ver_str, NULL, 16);
}

void _plat__StartupCallback(void)
{
	pinweaver_init();

	/*
	 * Eventually, we'll want to allow CCD unlock with no password, so
	 * enterprise policy can set a password to block CCD instead of locking
	 * it out via the FWMP.
	 *
	 * When we do that, we'll allow unlock without password between a real
	 * TPM startup (not just a resume) - which is this callback - and
	 * explicit disabling of that feature via a to-be-created vendor
	 * command.  That vendor command will be called after enterprize policy
	 * is updated, or the device is determined not to be enrolled.
	 *
	 * But for now, we'll just block unlock entirely if no password is set,
	 * so we don't yet need to tell CCD that a real TPM startup has
	 * occurred.
	 */

	/* TODO(b/262324344). Remove when zero sized EPS fixed. */
	if (gp.EPSeed.t.size == 0) {
		CPRINTF("%s: EPS seed length is zero!\n", __func__);
		cflush();
	}
}

BOOL _plat__ShallSurviveOwnerClear(uint32_t  index)
{
	return index == HR_NV_INDEX + FWMP_NV_INDEX;
}

void _plat__OwnerClearCallback(void)
{
	int result;
	enum ec_error_list rv;

	/* Invalidate existing biometrics pairing secrets. */
	result = setvar(PW_FP_PK, sizeof(PW_FP_PK) - 1, NULL, 0);
	if (result)
		CPRINTF("%s: failed (%d)\n", __func__, result);
	/* Invalidate existing u2f registrations. */
	rv = u2f_gen_kek_seed();
	if (rv != EC_SUCCESS)
		CPRINTF("%s: failed (%d)\n", __func__, rv);
}

/* Returns TRUE if FWMP/antirollback is allowed to be updated
 * with the current PCR state.
 */
static BOOL pcr_allows_boot_policy_update(void)
{
	uint8_t pcr0_value[SHA256_DIGEST_SIZE];
	int i;

	if (!get_tpm_pcr_value(0, pcr0_value))
		return FALSE;  /* something went wrong, let's be strict */

	for (i = 0; i < ARRAY_SIZE(pcr_boot_policy_update_allowed); ++i) {
		if (memcmp(pcr0_value,
				pcr_boot_policy_update_allowed[i],
				SHA256_DIGEST_SIZE) == 0)
			return TRUE;
	}

	return FALSE;
}

BOOL _plat__NvUpdateAllowed(uint32_t handle)
{
	switch (handle) {
	case HR_NV_INDEX + FWMP_NV_INDEX:
		return pcr_allows_boot_policy_update();
	case HR_NV_INDEX + FIRMWARE_NV_INDEX:
	case HR_NV_INDEX + KERNEL_NV_INDEX:
		return pcr_allows_boot_policy_update()
			|| board_fwmp_allows_boot_policy_update();
	}

	return TRUE;
}
