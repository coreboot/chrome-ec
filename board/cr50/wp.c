/* Copyright 2016 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "ccd_config.h"
#include "console.h"
#include "crc8.h"
#include "ec_commands.h"
#include "extension.h"
#include "flash_log.h"
#include "gpio.h"
#include "hooks.h"
#include "registers.h"
#include "scratch_reg1.h"
#include "system.h"
#include "system_chip.h"
#include "tpm_nvmem.h"
#include "tpm_nvmem_ops.h"
#include "tpm_registers.h"
#include "util.h"

#define CPRINTS(format, args...) cprints(CC_RBOX, format, ## args)
#define CPRINTF(format, args...) cprintf(CC_RBOX, format, ## args)

enum fwmp_controlled_action_t {
	CCD_UNLOCK,
	BOOT_POLICY_UPDATE,
};

uint8_t bp_connect;
uint8_t bp_forced;
/**
 * Return non-zero if battery is present
 */
int board_battery_is_present(void)
{
	/* Invert because battery-present signal is active low */
	return bp_forced ? bp_connect : !gpio_get_level(GPIO_BATT_PRES_L);
}

/**
 * Return non-zero if the FWMP is enabling write protect.
 */
static int board_fwmp_force_wp_en(void)
{
	return !!(GREG32(PMU, LONG_LIFE_SCRATCH1) & BOARD_FWMP_FORCE_WP_EN);
}

/**
 * Return non-zero if the wp state is being overridden.
 */
static int board_forcing_wp(void)
{
	return GREG32(PMU, LONG_LIFE_SCRATCH1) & BOARD_FORCING_WP;
}

/**
 * Set the current write protect state in RBOX and long life scratch register.
 *
 * @param asserted: 0 to disable write protect, otherwise enable write protect.
 */
static void set_wp_state(int asserted)
{
	/* This shouldn't be possible. Ensure nothing can disable wp. */
	if (board_fwmp_force_wp_en() && !asserted) {
		CPRINTS("FWMP: WP ovrd");
		asserted = 1;
	}
	/* Enable writing to the long life register */
	GWRITE_FIELD(PMU, LONG_LIFE_SCRATCH_WR_EN, REG1, 1);

	if (asserted) {
		GREG32(PMU, LONG_LIFE_SCRATCH1) |= BOARD_WP_ASSERTED;
		GREG32(RBOX, EC_WP_L) = 0;
	} else {
		GREG32(PMU, LONG_LIFE_SCRATCH1) &= ~BOARD_WP_ASSERTED;
		GREG32(RBOX, EC_WP_L) = 1;
	}

	/* Disable writing to the long life register */
	GWRITE_FIELD(PMU, LONG_LIFE_SCRATCH_WR_EN, REG1, 0);
}

/**
 * Return the current WP state
 *
 * @return 0 if WP deasserted, 1 if WP asserted
 */
int wp_is_asserted(void)
{
	/* Signal is active low, so invert */
	return !GREG32(RBOX, EC_WP_L);
}

static void check_wp_battery_presence(void)
{
	int bp = board_battery_is_present();

	/* If we're forcing WP, ignore battery detect */
	if (board_fwmp_force_wp_en() || board_forcing_wp())
		return;

	/* Otherwise, mirror battery */
	if (bp != wp_is_asserted()) {
		CPRINTS("WP %d", bp);
		set_wp_state(bp);
	}
}
DECLARE_HOOK(HOOK_SECOND, check_wp_battery_presence, HOOK_PRIO_DEFAULT);

/**
 * Force write protect state or follow battery presence.
 *
 * @param force: Force write protect to wp_en if non-zero, otherwise use battery
 *               presence as the source.
 * @param wp_en: 0: Deassert WP. 1: Assert WP.
 */
static void force_write_protect(int force, int wp_en)
{
	/* Enable writing to the long life register */
	GWRITE_FIELD(PMU, LONG_LIFE_SCRATCH_WR_EN, REG1, 1);

	if (force) {
		/* Force WP regardless of battery presence. */
		GREG32(PMU, LONG_LIFE_SCRATCH1) |= BOARD_FORCING_WP;
	} else {
		/* Stop forcing write protect. */
		GREG32(PMU, LONG_LIFE_SCRATCH1) &= ~BOARD_FORCING_WP;
		/* Use battery presence as the value for write protect. */
		wp_en = board_battery_is_present();
	}

	/* Disable writing to the long life register */
	GWRITE_FIELD(PMU, LONG_LIFE_SCRATCH_WR_EN, REG1, 0);

	/* Update the WP state. */
	set_wp_state(wp_en);
}

#ifdef CONFIG_CMD_WP
static enum vendor_cmd_rc vc_set_wp(enum vendor_cmd_cc code,
				    void *buf,
				    size_t input_size,
				    size_t *response_size)
{
	uint8_t response = 0;

	*response_size = 0;
	/* There shouldn't be any args */
	if (input_size > 1)
		return VENDOR_RC_BOGUS_ARGS;

	if (input_size == 1) {
		uint8_t *cmd = buf;

		if (*cmd != WP_ENABLE)
			return VENDOR_RC_BOGUS_ARGS;

		set_wp_state(1);
	}

	/* Get current wp settings */
	if (board_fwmp_force_wp_en()) {
		response |= WPV_FWMP_FORCE_WP_EN;
		/*
		 * Setting WPV_ATBOOT_SET and WPV_ATBOOT_ENABLE isn't completely
		 * necessary. It just makes cr50 more compatible with old
		 * gsctool versions. These flags show WP is enabled at boot
		 * which is essentially what the FWMP override does.
		 */
		response |= WPV_ATBOOT_SET;
		response |= WPV_ATBOOT_ENABLE;
	}
	if (board_forcing_wp())
		response |= WPV_FORCE;
	if (wp_is_asserted())
		response |= WPV_ENABLE;
	/* Get atboot wp settings */
	if (ccd_get_flag(CCD_FLAG_OVERRIDE_WP_AT_BOOT)) {
		response |= WPV_ATBOOT_SET;
		if (ccd_get_flag(CCD_FLAG_OVERRIDE_WP_STATE_ENABLED))
			response |= WPV_ATBOOT_ENABLE;
	}
	((uint8_t *)buf)[0] = response;
	*response_size = sizeof(response);
	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_WP, vc_set_wp);

static int command_bpforce(int argc, char **argv)
{
	int val = 1;
	int forced = 1;

	if (argc > 1) {
		/* Make sure we're allowed to override battery presence */
		if (!ccd_is_cap_enabled(CCD_CAP_OVERRIDE_BATT_STATE))
			return EC_ERROR_ACCESS_DENIED;

		/* Update BP */
		if (!strncasecmp(argv[1], "follow", 6))
			forced = 0;
		else if (!strncasecmp(argv[1], "dis", 3))
			val = 0;
		else if (strncasecmp(argv[1], "con", 3))
			return EC_ERROR_PARAM2;

		bp_forced = forced;
		bp_connect = val;

		if (argc > 2 && !strcasecmp(argv[2], "atboot")) {
			/* Change override at boot to match */
			ccd_set_flag(CCD_FLAG_OVERRIDE_BATT_AT_BOOT, bp_forced);
			ccd_set_flag(CCD_FLAG_OVERRIDE_BATT_STATE_CONNECT,
				     bp_connect);
		}
		/* Update the WP state based on new battery presence setting */
		check_wp_battery_presence();
	}

	ccprintf("batt pres: %s%sconnect\n", bp_forced ? "forced " : "",
		 board_battery_is_present() ? "" : "dis");
	ccprintf("  at boot: ");
	if (ccd_get_flag(CCD_FLAG_OVERRIDE_BATT_AT_BOOT))
		ccprintf("forced %sconnect\n",
			 ccd_get_flag(CCD_FLAG_OVERRIDE_BATT_STATE_CONNECT) ? ""
			 : "dis");
	else
		ccprintf("follow_batt_pres\n");
	return EC_SUCCESS;
}
DECLARE_SAFE_CONSOLE_COMMAND(bpforce, command_bpforce,
			     "[connect|disconnect|follow_batt_pres [atboot]]",
			     "Get/set BATT_PRES_L signal override");

static int command_wp(int argc, char **argv)
{
	int val;
	int forced;

	if (argc > 1) {
		/* Make sure we're allowed to override WP settings */
		if (!ccd_is_cap_enabled(CCD_CAP_OVERRIDE_WP))
			return EC_ERROR_ACCESS_DENIED;

		/* It's not possible to change WP when the FWMP enables it. */
		if (board_fwmp_force_wp_en()) {
			ccprintf("FWMP enabled WP\n");
			return EC_ERROR_ACCESS_DENIED;
		}
		/* Update WP */
		if (!strncasecmp(argv[1], "follow", 6))
			forced = 0;
		else if (parse_bool(argv[1], &val))
			forced = 1;
		else
			return EC_ERROR_PARAM1;

		force_write_protect(forced, val);

		if (argc > 2 && !strcasecmp(argv[2], "atboot")) {
			/* Change override at boot to match */
			ccd_set_flag(CCD_FLAG_OVERRIDE_WP_AT_BOOT, forced);
			ccd_set_flag(CCD_FLAG_OVERRIDE_WP_STATE_ENABLED, val);
		}
	}

	ccprintf("Flash WP: %s%s%sabled\n",
		board_fwmp_force_wp_en() ? "fwmp " : "",
		board_forcing_wp() ? "forced " : "",
		wp_is_asserted() ? "en" : "dis");
	ccprintf(" at boot: ");
	if (board_fwmp_force_wp_en())
		ccprintf("fwmp enabled\n");
	else if (ccd_get_flag(CCD_FLAG_OVERRIDE_WP_AT_BOOT))
		ccprintf("forced %sabled\n",
			 ccd_get_flag(CCD_FLAG_OVERRIDE_WP_STATE_ENABLED)
			 ? "en" : "dis");
	else
		ccprintf("follow_batt_pres\n");

	return EC_SUCCESS;
}
DECLARE_SAFE_CONSOLE_COMMAND(wp, command_wp,
			     "[<BOOLEAN>/follow_batt_pres [atboot]]",
			     "Get/set the flash HW write-protect signal");
#endif /* CONFIG_CMD_WP */

void set_bp_follow_ccd_config(void)
{
	if (ccd_get_flag(CCD_FLAG_OVERRIDE_BATT_AT_BOOT)) {
		/* Reset to at-boot state specified by CCD */
		bp_forced = 1;
		bp_connect = ccd_get_flag(CCD_FLAG_OVERRIDE_BATT_STATE_CONNECT);
	} else {
		bp_forced = 0;
	}
}

static void set_wp_follow_ccd_config(void)
{
	if (board_fwmp_force_wp_en()) {
		force_write_protect(1, 1);
	} else if (ccd_get_flag(CCD_FLAG_OVERRIDE_WP_AT_BOOT)) {
		/* Reset to at-boot state specified by CCD */
		force_write_protect(1, ccd_get_flag
				    (CCD_FLAG_OVERRIDE_WP_STATE_ENABLED));
	} else {
		/* Reset to WP based on battery-present (val is ignored) */
		force_write_protect(0, 1);
	}
}

void board_wp_follow_ccd_config(void)
{
	/*
	 * Battery presence can be overridden using CCD. Get that setting before
	 * configuring write protect.
	 */
	set_bp_follow_ccd_config();

	/* Update write protect setting based on ccd config */
	set_wp_follow_ccd_config();
}

static int board_fwmp_check_wp_policy(void)
{
	int fwmp_force_wp = !board_fwmp_allows_unlock();
	int update_brdprop = fwmp_force_wp != board_fwmp_force_wp_en();

	if (update_brdprop) {
		CPRINTS("%s", __func__);
		board_write_prop(BOARD_FWMP_FORCE_WP_EN, fwmp_force_wp);
	}

	if (fwmp_force_wp && (!wp_is_asserted() || update_brdprop)) {
		CPRINTS("%s: force en", __func__);
		force_write_protect(1, 1);
		return 0;
	}
	return update_brdprop;
}

void init_wp_state(void)
{
	/*
	 * Battery presence can be overridden using CCD. Get that setting before
	 * configuring write protect.
	 */
	set_bp_follow_ccd_config();

	/*
	 * If the FWMP is forcing WP, enable write protect. It overrides other
	 * configs.
	 */
	board_fwmp_check_wp_policy();
	if (board_fwmp_force_wp_en()) {
		set_wp_state(1);
		return;
	}

	/* Check system reset flags after CCD config is initially loaded */
	if ((system_get_reset_flags() & EC_RESET_FLAG_HIBERNATE) &&
	    !system_rollback_detected()) {
		/*
		 * Deep sleep resume without rollback, so reload the WP state
		 * that was saved to the long-life registers before the deep
		 * sleep instead of going back to the at-boot default.
		 */
		if (board_forcing_wp()) {
			/* Temporarily forcing WP */
			set_wp_state(GREG32(PMU, LONG_LIFE_SCRATCH1) &
				     BOARD_WP_ASSERTED);
		} else {
			/* Write protected if battery is present */
			set_wp_state(board_battery_is_present());
		}
	} else {
		set_wp_follow_ccd_config();
	}
}

/**
 * Wipe the TPM
 *
 * @param reset_required: reset the system after wiping the TPM.
 *
 * @return EC_SUCCESS, or non-zero if error.
 */
enum ec_error_list board_wipe_tpm(int reset_required)
{
	enum ec_error_list rc;

	/* Wipe the TPM's memory and reset the TPM task. */
	rc = tpm_reset_request(true, true);
	if (rc != EC_SUCCESS) {
		flash_log_add_event(FE_LOG_TPM_WIPE_ERROR, 0, NULL);
		/*
		 * If anything goes wrong (which is unlikely), we REALLY don't
		 * want to unlock the console. It's possible to fail without
		 * the TPM task ever running, so rebooting is probably our best
		 * bet for fixing the problem.
		 */
		CPRINTS("%s: Couldn't wipe nvmem! (rc %d)", __func__, rc);
		cflush();
		system_reset(SYSTEM_RESET_MANUALLY_TRIGGERED |
			     SYSTEM_RESET_HARD);

		/*
		 * That should never return, but if it did, reset the EC and
		 * through the error we got.
		 */
		board_reboot_ec();
		return rc;
	}

	/*
	 * TPM was wiped out successfully, let's prevent further communications
	 * from the AP until next reboot. The reboot will be triggered below if
	 * a reset is requested. If we aren't resetting the system now, the TPM
	 * will stay disabled until the user resets the system.
	 * This should be done as soon as possible after tpm_reset_request
	 * completes.
	 */
	tpm_stop();

	CPRINTS("TPM is erased");

	/* Tell the TPM task to re-enable NvMem commits. */
	tpm_reinstate_nvmem_commits();

	/* Update the WP policies after the tpm is wiped. */
	board_fwmp_update_policies();
	/*
	 * Use board_reboot_ec to ensure the system resets instead of
	 * deassert_ec_reset. Some boards don't reset immediately when EC_RST_L
	 * is asserted. board_reboot_ec will ensure the system has actually
	 * reset before releasing it. If the system has a normal reset scheme,
	 * EC reset will be released immediately.
	 */
	if (reset_required) {
		CPRINTS("%s: reset EC", __func__);
		board_reboot_ec();
	}
	return EC_SUCCESS;
}

/****************************************************************************/
/* Verified boot TPM NVRAM space support */

#ifndef CR50_DEV
static int lock_enforced(const struct RollbackSpaceFwmp *fwmp,
			 enum fwmp_controlled_action_t action)
{
	uint8_t crc;

	/* Let's verify that the FWMP structure makes sense. */
	if (fwmp->struct_size != sizeof(*fwmp)) {
		CPRINTS("%s: fwmp size mismatch (%d)", __func__,
			fwmp->struct_size);
		return 1;
	}

	crc = crc8(&fwmp->struct_version, sizeof(struct RollbackSpaceFwmp) -
		   offsetof(struct RollbackSpaceFwmp, struct_version));
	if (fwmp->crc != crc) {
		CPRINTS("%s: fwmp crc mismatch", __func__);
		return 1;
	}

	switch (action) {
	case CCD_UNLOCK:
		return !!(fwmp->flags & FWMP_DEV_DISABLE_CCD_UNLOCK);
	case BOOT_POLICY_UPDATE:
		return !!(fwmp->flags & FWMP_DEV_DISABLE_BOOT);
	}
	return 0;
}
#endif

static int fwmp_allows(enum fwmp_controlled_action_t action)
{
#ifdef CR50_DEV
	return 1;
#else
	/* Let's see if FWMP allows the requested action. */
	struct RollbackSpaceFwmp fwmp;
	int allows;

	switch (read_tpm_nvmem(NV_INDEX_FWMP,
			       sizeof(struct RollbackSpaceFwmp), &fwmp)) {
	default:
		/* Something is messed up, let's not allow. */
		allows = 0;
		break;

	case TPM_READ_NOT_FOUND:
		allows = 1;
		break;

	case TPM_READ_SUCCESS:
		allows = !lock_enforced(&fwmp, action);
		break;
	}

	return allows;
#endif
}

int board_fwmp_allows_unlock(void)
{
	int allows_unlock = fwmp_allows(CCD_UNLOCK);
#ifndef CR50_DEV
	CPRINTS("Console unlock %sallowed", allows_unlock ? "" : "not ");
#endif
	return allows_unlock;
}

int board_fwmp_allows_boot_policy_update(void)
{
	return fwmp_allows(BOOT_POLICY_UPDATE);
}

void board_fwmp_update_policies(void)
{
	if (board_fwmp_check_wp_policy())
		set_wp_follow_ccd_config();
}

int board_vboot_dev_mode_enabled(void)
{
	struct RollbackSpaceFirmware fw;

	if (TPM_READ_SUCCESS ==
	    read_tpm_nvmem(NV_INDEX_FIRMWARE, sizeof(fw), &fw)) {
		return !!(fw.flags & FIRMWARE_FLAG_DEV_MODE);
	}

	/* If not found or other error, assume dev mode is disabled */
	return 0;
}

/****************************************************************************/
/* TPM vendor-specific commands */

static enum vendor_cmd_rc vc_lock(enum vendor_cmd_cc code,
				  void *buf,
				  size_t input_size,
				  size_t *response_size)
{
	uint8_t *buffer = buf;

	if (code == VENDOR_CC_GET_LOCK) {
		/*
		 * Get the state of the console lock.
		 *
		 *   Args: none
		 *   Returns: one byte; true (locked) or false (unlocked)
		 */
		if (input_size != 0) {
			*response_size = 0;
			return VENDOR_RC_BOGUS_ARGS;
		}

		buffer[0] = console_is_restricted() ? 0x01 : 0x00;
		*response_size = 1;
		return VENDOR_RC_SUCCESS;
	}

	/* I have no idea what you're talking about */
	*response_size = 0;
	return VENDOR_RC_NO_SUCH_COMMAND;
}
DECLARE_VENDOR_COMMAND(VENDOR_CC_GET_LOCK, vc_lock);

/*
 * TODO(rspangler): The old concept of 'lock the console' really meant
 * something closer to 'reset CCD config', not the CCD V1 meaning of 'ccdlock'.
 * This command is no longer supported, so will fail.  It was defined this
 * way:
 *
 * DECLARE_VENDOR_COMMAND(VENDOR_CC_SET_LOCK, vc_lock);
 */
