/* Copyright 2020 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * EC-EFS (Early Firmware Selection)
 */
#include "common.h"
#include "console.h"
#include "ec_comm.h"
#include "ccd_config.h"
#include "crc8.h"
#include "ec_commands.h"
#include "extension.h"
#include "hooks.h"
#ifndef BOARD_HOST
#include "Global.h"
#include "NV_fp.h"
#include "nvmem.h"
#endif
#include "registers.h"
#include "system.h"
#include "tpm_nvmem.h"
#include "tpm_nvmem_ops.h"
#include "vboot.h"

#ifdef CR50_DEV
#define CPRINTS(format, args...) cprints(CC_TASK, "EC-EFS: " format, ## args)
#else
#define CPRINTS(format, args...) do { } while (0)
#endif

#if CONFIG_EC_EFS2_VERSION == 0
#define EC_EFS_BOOT_MODE_INITIAL EC_EFS_BOOT_MODE_NORMAL
#elif CONFIG_EC_EFS2_VERSION == 1
#define EC_EFS_BOOT_MODE_INITIAL EC_EFS_BOOT_MODE_TRUSTED_RO
#else
#error "NOT SUPPORTED EFS2 VERSION"
#endif

/*
 * Context of EC-EFS
 */
static struct ec_efs_context_ {
	uint32_t boot_mode:8;	        /* enum ec_efs_boot_mode */
	uint32_t hash_is_loaded:1;	/* Is EC hash loaded from nvmem */
	uint32_t reserved:23;

	uint32_t secdata_error_code;

	uint8_t hash[SHA256_DIGEST_SIZE];	/* EC-RW digest */
} ec_efs_ctx = {
	.boot_mode = EC_EFS_BOOT_MODE_INITIAL,
	.hash_is_loaded = 0,
	.secdata_error_code = 0,
	.hash = { 0, },
};

static const char * const boot_mode_name_[] = {
#if CONFIG_EC_EFS2_VERSION == 0
	"NORMAL",
	"NO_BOOT",
#elif CONFIG_EC_EFS2_VERSION == 1
	"VERIFIED",
	"NO_BOOT",
	"TRUSTED_RO",
#endif
};

/*
 * Change the boot mode
 *
 * @param mode_val New boot mode value to change
 */
static void set_boot_mode_(uint8_t mode_val)
{
	if (ec_efs_ctx.boot_mode != mode_val)
		cprints(CC_SYSTEM, "boot_mode: 0x%02x -> 0x%02x",
			ec_efs_ctx.boot_mode, mode_val);

	ec_efs_ctx.boot_mode = mode_val;

	/* Backup some ec_efs context to scratch register */
	GREG32(PMU, PWRDN_SCRATCH20) &= ~0xff;
	GREG32(PMU, PWRDN_SCRATCH20) |= mode_val;
}

static int load_ec_hash_(uint8_t * const ec_hash)
{
	struct vb2_secdata_kernel secdata;
	const uint8_t secdata_size = sizeof(struct vb2_secdata_kernel);
	uint8_t size_to_crc;
	uint8_t struct_size;
	uint8_t crc;

	if (read_tpm_nvmem(NV_INDEX_KERNEL, secdata_size,
			   (void *)&secdata) != TPM_READ_SUCCESS)
		return EC_ERROR_VBOOT_DATA_UNDERSIZED;

	/*
	 * Check struct version. CRC offset may be different with old struct
	 * version
	 */
	if (secdata.struct_version < VB2_SECDATA_KERNEL_STRUCT_VERSION_MIN)
		return EC_ERROR_VBOOT_DATA_INCOMPATIBLE;

	/* Check struct size. */
	struct_size = secdata.struct_size;
	if (struct_size != secdata_size)
		return EC_ERROR_VBOOT_DATA;

	/* Check CRC */
	size_to_crc = struct_size -
		      offsetof(struct vb2_secdata_kernel, crc8) -
		      sizeof(secdata.crc8);
	crc = crc8((uint8_t *)&secdata.reserved0, size_to_crc);
	if (crc != secdata.crc8)
		return EC_ERROR_CRC;

	/* Read hash and copy to hash */
	memcpy(ec_hash, secdata.ec_hash, sizeof(secdata.ec_hash));

	return EC_SUCCESS;
}

/*
 * Initialize EC-EFS context.
 */
static void ec_efs_init_(void)
{
	/*
	 * If it is a wakeup from deep sleep, then recover some core EC-EFS
	 * context values, including the boot_mode value, from a PWRD_SCRATCH
	 * register. Otherwise, reset boot_mode.
	 */
	if (system_get_reset_flags() & EC_RESET_FLAG_HIBERNATE)
		set_boot_mode_(GREG32(PMU, PWRDN_SCRATCH20) & 0xff);
	else
		ec_efs_reset();

	/* Read an EC hash in kernel secdata (TPM kernel NV index). */
	if (ec_efs_ctx.hash_is_loaded)
		return;

	ec_efs_refresh();
}
DECLARE_HOOK(HOOK_INIT, ec_efs_init_, HOOK_PRIO_INIT_EC_EFS);

/**
 * TPM vendor command handler to respond with EC Boot Mode.
 *
 * @return VENDOR_RC_SUCCESS
 *
 */
static enum vendor_cmd_rc vc_get_boot_mode_(struct vendor_cmd_params *p)
{
	uint8_t *buffer;

	buffer = (uint8_t *)p->buffer;
	buffer[0] = (uint8_t)ec_efs_ctx.boot_mode;

	p->out_size = 1;

	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND_P(VENDOR_CC_GET_BOOT_MODE, vc_get_boot_mode_);

/**
 * TPM vendor command handler to reset EC.
 *
 * @return VEDOR_RC_SUCCESS
 */
static enum vendor_cmd_rc vc_reset_ec_(struct vendor_cmd_params *p)
{
	p->out_size = 0;

	/*
	 * Let's reset EC a little later so that CR50 can send a TPM command
	 * to AP.
	 */
	board_reboot_ec_deferred(50 * MSEC);

	return VENDOR_RC_SUCCESS;
}
DECLARE_VENDOR_COMMAND_P(VENDOR_CC_RESET_EC, vc_reset_ec_);

void ec_efs_reset(void)
{
	set_boot_mode_(
#if CONFIG_EC_EFS2_VERSION == 0
		EC_EFS_BOOT_MODE_NORMAL
#elif CONFIG_EC_EFS2_VERSION == 1
		EC_EFS_BOOT_MODE_TRUSTED_RO
#endif
	);
}

/*
 * Change the EC boot mode value.
 *
 * @param data Pointer to the EC-CR50 packet
 * @param size Data (payload) size in EC-CR50 packet
 * @return CR50_COMM_SUCCESS if the packet has been processed successfully,
 *         CR50_COMM_ERROR_SIZE if data size is not as expected, or
 *         0 if it does not respond to EC.
 */
uint16_t ec_efs_set_boot_mode(const char * const data, const uint8_t size)
{
	uint8_t boot_mode;

	if (size != 1)
		return CR50_COMM_ERROR_SIZE;

	boot_mode = data[0];

	switch (boot_mode) {
#if CONFIG_EC_EFS2_VERSION == 0
	case EC_EFS_BOOT_MODE_NORMAL:
		/*
		 * Per EC-EFS2 design, CR50 accepts the repeating commands
		 * as long as the result is the same. It is to be tolerant
		 * against CR50 response loss, so that EC can resend the
		 * same command.
		 */
		if (ec_efs_ctx.boot_mode == EC_EFS_BOOT_MODE_NORMAL)
			break;
		/*
		 * Once the boot mode is NO_BOOT, then it must not be
		 * set to NORMAL mode without resetting EC.
		 */
		board_reboot_ec_deferred(0);
		return 0;
#elif CONFIG_EC_EFS2_VERSION == 1
	case EC_EFS_BOOT_MODE_TRUSTED_RO:
		switch (ec_efs_ctx.boot_mode) {
		case EC_EFS_BOOT_MODE_TRUSTED_RO:
			/*
			 * Per EC-EFS2 design, CR50 accepts the repeating
			 * commands as long as the result is the same.
			 * It is to be tolerant against CR50 response loss,
			 * so that EC can resend the same command.
			 */
			return CR50_COMM_SUCCESS;
		case EC_EFS_BOOT_MODE_NO_BOOT:
			/* fall through */
		case EC_EFS_BOOT_MODE_VERIFIED:
			/*
			 * Once the boot mode is NO_BOOT, then it must not be
			 * set to RO mode without resetting EC.
			 */
			board_reboot_ec_deferred(0);
			return 0;
		default:
			/*
			 * This case should not happen.
			 * However, should it happen, reset EC and EFS-context.
			 */
			cprints(CC_SYSTEM,
				"ERROR: EFS boot_mode is corrupted: %x",
				ec_efs_ctx.boot_mode);
			board_reboot_ec_deferred(0);
			return 0;
		}
		break;
#else
#error "Not-supported EC-EFS2 version"
#endif /* CONFIG_EC_EFS2_VERSION == 1 */
	case EC_EFS_BOOT_MODE_NO_BOOT:
		break;

	default:
		/* Reject changing to all other BOOT_MODEs. */
		return CR50_COMM_ERROR_BAD_PARAM;
	}

	set_boot_mode_(boot_mode);
	return CR50_COMM_SUCCESS;
}

/*
 * Verify the given EC-FW hash against one in kernel secdata.
 *
 * @param data Pointer to the EC-CR50 packet
 * @param size Data (payload) size in EC-CR50 packet
 * @return CR50_COMM_SUCCESS if the packet has been processed successfully,
 *         CR50_COMM_ERROR_SIZE if data size is not as expected, or
 *         CR50_COMM_ERROR_BAD_PAYLOAD if the given hash and the hash in NVM
 *                                     are not same.
 *         0 if it deosn't have to respond to EC.
 */
uint16_t ec_efs_verify_hash(const char *hash_data, const uint8_t size)
{
	if (size != SHA256_DIGEST_SIZE)
		return CR50_COMM_ERROR_SIZE;

	if (!ec_efs_ctx.hash_is_loaded) {
		if (ec_efs_ctx.secdata_error_code == EC_SUCCESS)
			ec_efs_refresh();

		if (ec_efs_ctx.secdata_error_code != EC_SUCCESS)
			return CR50_COMM_ERROR_NVMEM;
	}

	if (safe_memcmp(hash_data, ec_efs_ctx.hash, SHA256_DIGEST_SIZE)) {
		/* Verification failed */
		set_boot_mode_(EC_EFS_BOOT_MODE_NO_BOOT);
		return CR50_COMM_ERROR_BAD_PAYLOAD;
	}

	/*
	 * Once the boot mode is NO_BOOT, then CR50
	 * should not approve the hash verification, but reset EC.
	 */
	if (ec_efs_ctx.boot_mode == EC_EFS_BOOT_MODE_NO_BOOT) {
		board_reboot_ec_deferred(0);
		return 0;
	}
#if CONFIG_EC_EFS2_VERSION == 1
	/*
	 * Changing BOOT_MODE_VERIFIED does not break a backward compatibility
	 * to EFS2.0 in AP-FW, because AP-FW accepts all boot_modes but NO_BOOT.
	 */
	set_boot_mode_(EC_EFS_BOOT_MODE_VERIFIED);
#endif
	return CR50_COMM_SUCCESS;
}

void ec_efs_refresh(void)
{
	int rv;

	rv = load_ec_hash_(ec_efs_ctx.hash);
	if (rv == EC_SUCCESS) {
		ec_efs_ctx.hash_is_loaded = 1;
	} else {
		ec_efs_ctx.hash_is_loaded = 0;
		cprints(CC_SYSTEM, "load_ec_hash error: 0x%x", rv);
	}
	ec_efs_ctx.secdata_error_code = rv;
}

void ec_efs_print_status(void)
{
	/* EC-EFS Context */
	ccprintf("ec_hash            : %sLOADED\n",
		 ec_efs_ctx.hash_is_loaded ? "" : "UN");
	ccprintf("secdata_error_code : 0x%08x\n",
		 ec_efs_ctx.secdata_error_code);
	ccprintf("boot_mode          : %s\n",
		 boot_mode_name_[ec_efs_ctx.boot_mode]);

#ifdef CR50_DEV
	ccprintf("ec_hash_secdata    : %ph\n",
		 HEX_BUF(ec_efs_ctx.hash, SHA256_DIGEST_SIZE));
#endif
}

enum ec_error_list ec_efs_corrupt_hash(void)
{
#ifndef BOARD_HOST
	struct vb2_secdata_kernel secdata;
	const uint8_t secdata_size = sizeof(struct vb2_secdata_kernel);
	TPM_HANDLE object_handle;
	NV_INDEX nvIndex;
	uint8_t size_to_crc;
	int i;

	/* Check CCD is opened */
	if (ccd_get_state() != CCD_STATE_OPENED) {
		ccprintf("CCD is not opened\n");
		return EC_ERROR_ACCESS_DENIED;
	}

	/* Read the kernel secdata */
	if (read_tpm_nvmem(NV_INDEX_KERNEL, secdata_size,
			   (void *)&secdata) != TPM_READ_SUCCESS)
		return EC_ERROR_VBOOT_DATA_UNDERSIZED;

	/* Modify hash */
	for (i = 0; i < SHA256_DIGEST_SIZE; i++)
		secdata.ec_hash[i] = ~ec_efs_ctx.hash[i] + 0x01;

	size_to_crc = secdata_size -
		      offsetof(struct vb2_secdata_kernel, crc8) -
		      sizeof(secdata.crc8);
	secdata.crc8 = crc8((uint8_t *)&secdata.reserved0, size_to_crc);

	/* Corrupt NV_INDEX_KERNEL in nvmem cache. */
	object_handle = HR_NV_INDEX + NV_INDEX_KERNEL;
	NvGetIndexInfo(object_handle, &nvIndex);
	NvWriteIndexData(object_handle, &nvIndex, 0, secdata_size, &secdata);
	nvmem_unlock_cache(1);

	/* Reload the corrupted ECRW-hash from kernel secdata. */
	ec_efs_refresh();
#endif
	return EC_SUCCESS;
}

#ifdef BOARD_HOST
uint8_t ec_efs_get_boot_mode(void)
{
	return ec_efs_ctx.boot_mode;
}
#endif
