/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* CCD factory enable */

#include "ccd_config.h"
#include "console.h"
#include "extension.h"
#include "hooks.h"
#include "system.h"
#include "tpm_registers.h"
#include "tpm_vendor_cmds.h"

#define CPRINTS(format, args...) cprints(CC_CCD, format, ## args)

static uint8_t ccd_hook_active;
static uint8_t reset_required_;

static void ccd_config_changed(void)
{
	if (!ccd_hook_active)
		return;

	ccd_hook_active = 0;

	if (!reset_required_)
		return;

	CPRINTS("%s: saved, rebooting\n", __func__);
	cflush();
	system_reset(SYSTEM_RESET_HARD);
}
DECLARE_HOOK(HOOK_CCD_CHANGE, ccd_config_changed, HOOK_PRIO_LAST);

static void factory_enable_failed(void)
{
	ccd_hook_active = 0;
	CPRINTS("factory enable failed");

	if (reset_required_) {
		reset_required_ = 0;
		deassert_ec_rst();
	}
}
DECLARE_DEFERRED(factory_enable_failed);

static void factory_enable_deferred(void)
{
	int rv;

	CPRINTS("%s: reset TPM\n", __func__);
	if (reset_required_)
		assert_ec_rst();

	if (tpm_reset_request(1, 1) != EC_SUCCESS) {
		CPRINTS("%s: TPM reset failed\n", __func__);
		/*
		 * Attempt to reset TPM failed, let's reboot the device just
		 * in case.
		 */
		if (!reset_required_)
			assert_ec_rst();
		deassert_ec_rst();
		return;
	}

	/*
	 * TPM was wiped out successfully, let's prevent further
	 * communications from the AP until next reboot.
	 */
	if (!reset_required_)
		tpm_stop();

	/*
	 * Need this to make sure that CCD state changes are saved in the
	 * NVMEM before reboot.
	 */
	tpm_reinstate_nvmem_commits();

	CPRINTS("%s: TPM reset done, enabling factory mode", __func__);

	ccd_hook_active = 1;
	rv = ccd_reset_config(CCD_RESET_FACTORY);
	if (rv != EC_SUCCESS)
		factory_enable_failed();

	if (reset_required_) {
		/*
		 * Make sure we never end up with the EC held in reset, no
		 * matter what prevents the proper factory reset flow from
		 * succeeding.
		 */
		hook_call_deferred(&factory_enable_failed_data, TPM_RESET_TIME);
	}
}
DECLARE_DEFERRED(factory_enable_deferred);

void enable_ccd_factory_mode(int reset_required)
{
	/*
	 * Wiping the TPM may take a while. Delay sleep long enough for the
	 * factory enable process to finish.
	 */
	delay_sleep_by(DISABLE_SLEEP_TIME_TPM_WIPE);

	reset_required_ |= !!reset_required;
	hook_call_deferred(&factory_enable_deferred_data,
		TPM_PROCESSING_TIME);
}
