/*
 * Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "config.h"
#include "Global.h"
#include "board.h"
#include "closed_source_set1.h"
#include "console.h"
#include "dcrypto.h"
#include "extension.h"
#include "hooks.h"
#include "nvmem.h"
#include "system.h"
#include "timer.h"
#include "tpm_registers.h"
#include "tpm_vendor_cmds.h"

#define CPRINTS(format, args...) cprints(CC_EXTENSION, format, ## args)

static void disable_tpm(void)
{
	tpm_stop();
	DCRYPTO_ladder_revoke();
	nvmem_clear_cache();

	if (board_uses_closed_source_set1())
		close_source_set1_disable_tpm();
}
DECLARE_DEFERRED(disable_tpm);

/*
 * tpm_mode can be set only once after a hardware reset, to either
 * TPM_MODE_ENABLED or TPM_MODE_DISABLED.
 *
 * This allows the AP to make sure that TPM can't be disabled by setting mode
 * to TPM_MODE_ENABLED during start up.
 *
 * If mode is set to TPM_MODE_DISABLED, the AP loses the ability to
 * communicate with the TPM until next TPM reset (which will trigger the H1
 * hardware reset in that case).
 */
static enum tpm_modes s_tpm_mode;

static enum vendor_cmd_rc process_tpm_mode(struct vendor_cmd_params *p)
{
	uint8_t mode_val;
	uint8_t *buffer;

	p->out_size = 0;

	if (p->in_size > sizeof(uint8_t))
		return VENDOR_RC_NOT_ALLOWED;

	buffer = (uint8_t *)p->buffer;
	if (p->in_size == sizeof(uint8_t)) {

		if (s_tpm_mode != TPM_MODE_ENABLED_TENTATIVE)
			return VENDOR_RC_NOT_ALLOWED;

		mode_val = buffer[0];

		switch (mode_val) {
		case TPM_MODE_ENABLED:
			/*
			 * If Key ladder is disabled, then fail this request.
			 */
			if (!DCRYPTO_ladder_is_enabled())
				return VENDOR_RC_INTERNAL_ERROR;
			break;
		case TPM_MODE_DISABLED:
			/*
			 * If it is to be disabled, call disable_tpm() deferred
			 * so that this vendor command can be responded to
			 * before TPM stops.
			 */
			if (nvmem_enable_commits() != EC_SUCCESS)
				return VENDOR_RC_NVMEM_LOCKED;

			hook_call_deferred(&disable_tpm_data, 10 * MSEC);
			break;
		default:
			return VENDOR_RC_NO_SUCH_SUBCOMMAND;
		}
		s_tpm_mode = mode_val;
	} else {
		if (s_tpm_mode < TPM_MODE_DISABLED &&
		    !DCRYPTO_ladder_is_enabled())
			return VENDOR_RC_INTERNAL_ERROR;
	}

	p->out_size = sizeof(uint8_t);
	buffer[0] = (uint8_t) s_tpm_mode;

	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND_P(VENDOR_CC_TPM_MODE, process_tpm_mode);

enum tpm_modes get_tpm_mode(void)
{
	return s_tpm_mode;
}
