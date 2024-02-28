/* Copyright 2020 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Test ec_comm
 */

#include "common.h"
#include "crc8.h"
#include "ec_comm.h"
#include "test_util.h"
#include "timer.h"
#include "tpm_nvmem.h"
#include "tpm_nvmem_ops.h"
#include "uart.h"
#include "util.h"
#include "vboot.h"

const uint8_t sample_ec_hash[SHA256_DIGEST_SIZE] = {
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
};

struct vb2_secdata_kernel test_secdata = {
	.struct_version = VB2_SECDATA_KERNEL_STRUCT_VERSION_MIN,
	.struct_size = sizeof(struct vb2_secdata_kernel),
	.reserved0 = 0,
	.kernel_versions = VB2_SECDATA_KERNEL_UID,
};

union cr50_test_packet {
	struct cr50_comm_packet ph;
	uint8_t packet[CR50_COMM_MAX_PACKET_SIZE * 2];
};

union cr50_test_packet sample_packet_cmd_set_mode = {
	.ph.magic = CR50_COMM_MAGIC_WORD,
	.ph.version = CR50_COMM_VERSION,
	.ph.crc = 0,
	.ph.cmd = CR50_COMM_CMD_SET_BOOT_MODE,
	.ph.size = 1,
};

union cr50_test_packet sample_packet_cmd_verify_hash = {
	.ph.magic = CR50_COMM_MAGIC_WORD,
	.ph.version = CR50_COMM_VERSION,
	.ph.crc = 0,
	.ph.cmd = CR50_COMM_CMD_VERIFY_HASH,
	.ph.size = SHA256_DIGEST_SIZE,
};

#if CONFIG_EC_EFS2_VERSION == 0
#define EXPECTED_BOOT_MODE_AFTER_EC_RST EC_EFS_BOOT_MODE_NORMAL
#define EXPECTED_BOOT_MODE_AFTER_VERIFY EC_EFS_BOOT_MODE_NORMAL
#elif CONFIG_EC_EFS2_VERSION == 1
#define EXPECTED_BOOT_MODE_AFTER_EC_RST EC_EFS_BOOT_MODE_TRUSTED_RO
#define EXPECTED_BOOT_MODE_AFTER_VERIFY EC_EFS_BOOT_MODE_VERIFIED
#endif

/* EC Reset Count. It is used to see if ec has been reset. */
static int ec_reset_count_;

/*
 * Return 1 if EC has been reset since the last call of this function, or
 *        0 otherwise.
 */
static int ec_has_reset(void)
{
	static int prev_ec_reset_count;

	if (prev_ec_reset_count == ec_reset_count_)
		return 0;

	prev_ec_reset_count = ec_reset_count_;
	return 1;
}

void board_reboot_ec_deferred(int usec_delay_used)
{
	/* ec_reset */
	ec_reset_count_++;
	ec_efs_reset();
}

enum tpm_read_rv read_tpm_nvmem(uint16_t object_index, uint16_t object_size,
				void *obj_value)
{
	/* Check the input parameter */
	if (object_index != NV_INDEX_KERNEL)
		return TPM_READ_NOT_FOUND;
	if (object_size != test_secdata.struct_size)
		return TPM_READ_TOO_SMALL;

	/*
	 * Copy the test_secdata to obj_value as if it was loaded
	 * from NVMEM.
	 */
	memcpy(obj_value, (void *)&test_secdata, object_size);

	return TPM_READ_SUCCESS;
}

/*
 * Return 1 if the byte is found in buf string.
 *        0 otherwise
 */
static int find_byte(const char *buf, uint8_t byte)
{
	int i = strlen(buf);

	while (i--) {
		if (*buf == byte)
			return 1;
		buf++;
	}
	return 0;
}

/*
 * Calculate CRC8 of the the given CR50 packet and fill it in
 * the packet.
 */
static void calculate_crc8(union cr50_test_packet *pk)
{
	const int offset_cmd = offsetof(struct cr50_comm_packet, cmd);

	/* Calculate the sample EC-CR50 packet.	*/
	pk->ph.crc = crc8((uint8_t *)&pk->ph.cmd,
			  (sizeof(struct cr50_comm_packet)
			  + pk->ph.size - offset_cmd));
}

/*
 * Test EC CR50 communication with the given EC-CR50 packet.
 *
 * @param pk         an ec_cr50 comm packet to test
 * @param preambles  the number of preambles
 * @param resp_exp   the expected response in two bytes as in
 *                   CR50_COMM_RESPONSE.
 *                   if it is zero, then no response is expected.
 */
static int test_ec_comm(const union cr50_test_packet *pk, int preambles,
			uint16_t resp_exp)
{
	uint8_t *buf;
	int leng;
	int i;
	const char *resp;

	leng = sizeof(struct cr50_comm_packet) + pk->ph.size;

	/* Prepare the input packet. */
	buf = (uint8_t *)pk;

	/* Start the test */
	ec_comm_packet_mode_en(1);
	TEST_ASSERT(ec_comm_is_uart_in_packet_mode(UART_EC));

	for (i = 0; i < preambles; i++)
		ec_comm_process_packet(CR50_COMM_PREAMBLE);

	test_capture_uartn(UART_EC, 1);

	for (i = 0;  i < leng; i++)
		ec_comm_process_packet(buf[i]);

	resp = test_get_captured_uartn(UART_EC);

	if (resp_exp)
		TEST_ASSERT(*(uint16_t *)resp == resp_exp);
	else
		/* Check if there was any EC-CR50-comm response. */
		TEST_ASSERT(find_byte(resp, '\xec') == 0);

	test_capture_uartn(UART_EC, 0);

	ec_comm_packet_mode_dis(1);
	TEST_ASSERT(!ec_comm_is_uart_in_packet_mode(UART_EC));

	return EC_SUCCESS;
}

/*
 * Test the failure case for packet errors.
 */
static int test_ec_comm_packet_failure(void)
{
	/* Copy the sample packet to buffer. */
	union cr50_test_packet pk = sample_packet_cmd_verify_hash;
	int preambles = MIN_LENGTH_PREAMBLE;

	ec_has_reset();

	TEST_ASSERT(ec_efs_get_boot_mode() == EXPECTED_BOOT_MODE_AFTER_EC_RST);

	/* Test 1: Test with less preambles than required. */
	calculate_crc8(&pk);
	TEST_ASSERT(!test_ec_comm(&pk, 1, 0));

	/* Test 2: Test a wrong magic */
	pk.ph.magic = 0x1234;
	calculate_crc8(&pk);
	TEST_ASSERT(!test_ec_comm(&pk, preambles, CR50_COMM_ERROR_MAGIC));

	/* Test 3: Test with a wrong CRC */
	pk = sample_packet_cmd_verify_hash;
	calculate_crc8(&pk);
	pk.ph.crc += 0x01;	/* corrupt the CRC */
	TEST_ASSERT(!test_ec_comm(&pk, preambles, CR50_COMM_ERROR_CRC));

	/* Test 4: Test with too large payload */
	pk = sample_packet_cmd_verify_hash;
	pk.ph.size = SHA256_DIGEST_SIZE + 1;
	pk.ph.data[SHA256_DIGEST_SIZE] = 0xff;
	calculate_crc8(&pk);

	TEST_ASSERT(!test_ec_comm(&pk, preambles, CR50_COMM_ERROR_SIZE));

	/* Test 5: Test with a undefined command */
	pk = sample_packet_cmd_verify_hash;
	pk.ph.cmd = 0x1000;
	calculate_crc8(&pk);
	TEST_ASSERT(!test_ec_comm(&pk, preambles,
				  CR50_COMM_ERROR_UNDEFINED_CMD));

	/* Test 6: Test with a wrong struct version */
	pk = sample_packet_cmd_verify_hash;
	pk.ph.version = CR50_COMM_VERSION + 0x01;
	calculate_crc8(&pk);
	TEST_ASSERT(!test_ec_comm(&pk, preambles,
				  CR50_COMM_ERROR_STRUCT_VERSION));

	/* Check if ec has ever been reset during these tests */
	TEST_ASSERT(!ec_has_reset());

	TEST_ASSERT(ec_efs_get_boot_mode() == EXPECTED_BOOT_MODE_AFTER_EC_RST);

	return EC_SUCCESS;
}

/*
 * Test cases for set_boot_mode command.
 */
static int test_ec_comm_set_boot_mode(void)
{
	/* Copy the sample packet to buffer. */
	union cr50_test_packet pk = sample_packet_cmd_set_mode;
	int preambles;
	uint8_t boot_mode_expected;

	ec_has_reset();

	boot_mode_expected = EXPECTED_BOOT_MODE_AFTER_EC_RST;
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

	/*
	 * Test 1-1: Attempt to set boot mode to EXPECTED_BOOT_MODE_AFTER_EC_RST
	 *           NORMAL -> NORMAL (in 2.0)  or RO -> RO (in 2.1)
	 */
	boot_mode_expected = EXPECTED_BOOT_MODE_AFTER_EC_RST;
	pk.ph.data[0] = boot_mode_expected;
	calculate_crc8(&pk);
	preambles = MIN_LENGTH_PREAMBLE * 2;
	TEST_ASSERT(!test_ec_comm(&pk, preambles, CR50_COMM_SUCCESS));
	TEST_ASSERT(!ec_has_reset());	/* EC must not be reset. */
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

	/*
	 * Test 1-1: Attempt to set boot mode to EXPECTED_BOOT_MODE_AFTER_EC_RST
	 *           NORMAL -> NORMAL (in 2.0)  or RO -> RO (in 2.1)
	 */
	boot_mode_expected = EXPECTED_BOOT_MODE_AFTER_EC_RST;
	preambles = MIN_LENGTH_PREAMBLE;
	TEST_ASSERT(!test_ec_comm(&pk, preambles, CR50_COMM_SUCCESS));
	TEST_ASSERT(!ec_has_reset());	/* EC must not be reset. */
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

#if CONFIG_EC_EFS2_VERSION == 1
	/*
	 * Test 1-3: Attempt to set boot mode to BOOT_MODE_VERIFIED.
	 *           It should fail.
	 *           RO -> VERIFIED (x) RO (o)
	 */
	boot_mode_expected = EC_EFS_BOOT_MODE_TRUSTED_RO;
	pk.ph.data[0] = EC_EFS_BOOT_MODE_VERIFIED;
	calculate_crc8(&pk);
	TEST_ASSERT(!test_ec_comm(&pk, preambles, CR50_COMM_ERROR_BAD_PARAM));
	TEST_ASSERT(!ec_has_reset());	/* EC must not be reset. */
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);
#endif

	/*
	 * Test 2-1: Attempt to set boot mode to NO BOOT.
	 *           EC should not be reset with this boot mode change from
	 *           BOOT_MODE_TRUSTED_RO to BOOT_MODE_NO_BOOT.
	 *           INITIAL(RO or NORMAL) -> NO_BOOT
	 */
	boot_mode_expected = EC_EFS_BOOT_MODE_NO_BOOT;
	pk.ph.data[0] = boot_mode_expected;
	calculate_crc8(&pk);
	TEST_ASSERT(!test_ec_comm(&pk, preambles, CR50_COMM_SUCCESS));
	TEST_ASSERT(!ec_has_reset());	/* EC must not be reset. */
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

	/*
	 * Test 2-2: Attempt to set boot mode to NO BOOT again.
	 *           EC should not be reset since it is a repeating command.
	 *           NO_BOOT -> NO_BOOT
	 */
	boot_mode_expected = EC_EFS_BOOT_MODE_NO_BOOT;
	TEST_ASSERT(!test_ec_comm(&pk, preambles, CR50_COMM_SUCCESS));
	TEST_ASSERT(!ec_has_reset());	/* EC must not be reset. */
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

#if CONFIG_EC_EFS2_VERSION == 1
	/*
	 * Test 2-3: Attempt to set boot mode to VERIFIED. It should fail.
	 *           NO_BOOT -> VERIFIED (x) NO_BOOT(o)
	 */
	boot_mode_expected = EC_EFS_BOOT_MODE_NO_BOOT;
	pk.ph.data[0] = EC_EFS_BOOT_MODE_VERIFIED;
	calculate_crc8(&pk);
	TEST_ASSERT(!test_ec_comm(&pk, preambles, CR50_COMM_ERROR_BAD_PARAM));
	TEST_ASSERT(!ec_has_reset());	/* EC must not be reset. */
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);
#endif

	/*
	 * Test 2-4: Attempt to set boot mode to BOOT_MODE_TRUSTED_RO.
	 *           EC should be reset with this boot mode change from
	 *           BOOT_MODE_NO_BOOT to BOOT_MODE_TRUSTED_RO.
	 *           NO_BOOT -> NORMAL (in 2.0) or RO (in 2.1)
	 */
	boot_mode_expected = EXPECTED_BOOT_MODE_AFTER_EC_RST;
	pk.ph.data[0] = EXPECTED_BOOT_MODE_AFTER_EC_RST;
	calculate_crc8(&pk);
	TEST_ASSERT(!test_ec_comm(&pk, preambles, 0));
	TEST_ASSERT(ec_has_reset());	/* EC must be reset. */
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

	return EC_SUCCESS;
}

/*
 * Test cases for verify_hash command.
 */
static int test_ec_comm_verify_hash(void)
{
	/* Copy the sample packet to buffer. */
	union cr50_test_packet pk = sample_packet_cmd_verify_hash;
	int preambles = MIN_LENGTH_PREAMBLE;
	uint8_t boot_mode_expected;

	ec_has_reset();

	boot_mode_expected = EXPECTED_BOOT_MODE_AFTER_EC_RST;
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

	/*
	 * Test 1: Attempt to verify EC Hash.
	 *         NORMAL -> NORMAL (in 2.0)
	 *         RO -> VERIFIED (in 2.1)
	 */
	boot_mode_expected = EXPECTED_BOOT_MODE_AFTER_VERIFY;
	calculate_crc8(&pk);
	preambles = MIN_LENGTH_PREAMBLE * 2;
	TEST_ASSERT(!test_ec_comm(&pk, preambles, CR50_COMM_SUCCESS));
	TEST_ASSERT(!ec_has_reset());
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

	/*
	 * Test 2: Attempt to verify EC Hash again.
	 *         NORMAL -> NORMAL (in 2.0)
	 *         VERIFIED -> VERIFIED (in 2.1)
	 */
	boot_mode_expected = EXPECTED_BOOT_MODE_AFTER_VERIFY;
	preambles = MIN_LENGTH_PREAMBLE;
	TEST_ASSERT(!test_ec_comm(&pk, preambles, CR50_COMM_SUCCESS));
	TEST_ASSERT(!ec_has_reset());
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

	/*
	 * Test 3: Attempt to verify a wrong EC Hash.
	 *         NORMAL -> NO_BOOT (in 2.0)
	 *         VERIFIED -> NO_BOOT (in 2.1)
	 */
	boot_mode_expected = EC_EFS_BOOT_MODE_NO_BOOT;
	pk.ph.data[0] ^= 0xff;	/* corrupt the payload */
	calculate_crc8(&pk);
	TEST_ASSERT(!test_ec_comm(&pk, preambles, CR50_COMM_ERROR_BAD_PAYLOAD));
	TEST_ASSERT(!ec_has_reset());	/* EC should not be reset though. */
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

	/*
	 * Test 4: Attempt to verify a wrong EC Hash again.
	 *         NO_BOOT -> NO_BOOT
	 */
	boot_mode_expected = EC_EFS_BOOT_MODE_NO_BOOT;
	TEST_ASSERT(!test_ec_comm(&pk, preambles,
		CR50_COMM_ERROR_BAD_PAYLOAD));
	TEST_ASSERT(!ec_has_reset());	/* EC should not be reset though. */
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

	/*
	 * Test 5: Attempt to verify the correct EC Hash.
	 *         EC should be reset because EC Boot mode is NO BOOT.
	 *         NO_BOOT -> INITIAL
	 */
	boot_mode_expected = EXPECTED_BOOT_MODE_AFTER_EC_RST;
	pk = sample_packet_cmd_verify_hash;
	calculate_crc8(&pk);
	preambles = MIN_LENGTH_PREAMBLE * 2;
	TEST_ASSERT(!test_ec_comm(&pk, preambles, 0));
	TEST_ASSERT(ec_has_reset());	/* EC must be reset. */
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

	/* Check if ec has ever been reset during these tests */
	return EC_SUCCESS;
}

/*
 * Test cases for verify_hash command failure case.
 */
static int test_ec_comm_verify_hash_fail(void)
{
	/* Copy the sample packet to buffer. */
	union cr50_test_packet pk = sample_packet_cmd_verify_hash;
	int preambles = MIN_LENGTH_PREAMBLE;
	uint8_t boot_mode_expected;

	ec_has_reset();

	boot_mode_expected = EXPECTED_BOOT_MODE_AFTER_EC_RST;
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

	/*
	 * Test 1: Attempt to verify a wrong EC Hash.
	 *         RO -> NO_BOOT
	 */
	boot_mode_expected = EC_EFS_BOOT_MODE_NO_BOOT;
	pk.ph.data[0] ^= 0xff;	/* corrupt the payload */
	calculate_crc8(&pk);
	TEST_ASSERT(!test_ec_comm(&pk, preambles, CR50_COMM_ERROR_BAD_PAYLOAD));
	TEST_ASSERT(!ec_has_reset());	/* EC should not be reset though. */
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);


	/* Check if ec has ever been reset during these tests */
	return EC_SUCCESS;
}

#if CONFIG_EC_EFS2_VERSION == 1
/*
 * Test cases for EC-EFS 2.1 protocols
 */
static int test_ec_comm_verify_efs2_1(void)
{
	/* Copy the sample packet to buffer. */
	union cr50_test_packet pk_verify = sample_packet_cmd_verify_hash;
	union cr50_test_packet pk_set_bootmode = sample_packet_cmd_set_mode;
	int preambles = MIN_LENGTH_PREAMBLE;
	uint8_t boot_mode_expected;

	ec_has_reset();

	boot_mode_expected = EC_EFS_BOOT_MODE_TRUSTED_RO;
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

	/*
	 * Test 1-1: Attempt to verify EC Hash while RO.
	 *           RO -> VERIFIED
	 */
	boot_mode_expected = EC_EFS_BOOT_MODE_VERIFIED;
	calculate_crc8(&pk_verify);
	TEST_ASSERT(!test_ec_comm(&pk_verify, preambles, CR50_COMM_SUCCESS));
	TEST_ASSERT(!ec_has_reset());
	TEST_ASSERT(ec_efs_get_boot_mode() == EC_EFS_BOOT_MODE_VERIFIED);

	/*
	 * Test 1-2: Attempt to change BOOT_MODE to RO. Should fail.
	 *           VERIFIED -> RO, EC Reset
	 */
	boot_mode_expected = EC_EFS_BOOT_MODE_TRUSTED_RO;
	pk_set_bootmode.ph.data[0] = boot_mode_expected;
	calculate_crc8(&pk_set_bootmode);
	TEST_ASSERT(!test_ec_comm(&pk_set_bootmode, preambles, 0));
	TEST_ASSERT(ec_has_reset());	/* EC must not be reset. */
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

	/*
	 * Test 1-3: Attempt to change BOOT_MODE to VERIFIED.
	 *           Should fail, since VERIFIED is not allowed
	 *           as a parameter of SET_BOOT_MODE command.
	 *           RO -> VERIFIED (x) RO(o)
	 */
	boot_mode_expected = EC_EFS_BOOT_MODE_TRUSTED_RO;
	pk_set_bootmode.ph.data[0] = EC_EFS_BOOT_MODE_VERIFIED;
	calculate_crc8(&pk_set_bootmode);
	TEST_ASSERT(!test_ec_comm(&pk_set_bootmode, preambles,
		CR50_COMM_ERROR_BAD_PARAM));
	TEST_ASSERT(!ec_has_reset());	/* EC must not be reset. */
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

	/*
	 * Test 1-4: Attempt to change BOOT_MODE to NO_BOOT.
	 *           Should succeed
	 *           VERIFIED -> NO_BOOT
	 */
	boot_mode_expected = EC_EFS_BOOT_MODE_NO_BOOT;
	pk_set_bootmode.ph.data[0] = boot_mode_expected;
	calculate_crc8(&pk_set_bootmode);
	TEST_ASSERT(!test_ec_comm(&pk_set_bootmode, preambles,
		CR50_COMM_SUCCESS));
	TEST_ASSERT(!ec_has_reset());	/* EC must not be reset. */
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

	/*
	 * Test 1-5: Attempt to verify EC Hash while NO_BOOT
	 *           NO_BOOT -> RO
	 */
	boot_mode_expected = EC_EFS_BOOT_MODE_TRUSTED_RO;
	TEST_ASSERT(!test_ec_comm(&pk_verify, preambles, 0));
	TEST_ASSERT(ec_has_reset());
	TEST_ASSERT(ec_efs_get_boot_mode() == boot_mode_expected);

	/* Check if ec has ever been reset during these tests */
	return EC_SUCCESS;
}
#endif /* CONFIG_EC_EFS2_VERSION == 1 */

void run_test(void)
{
	uint8_t size_to_crc;

	/* Prepare the sample kernel secdata and a sample packet. */
	memcpy(test_secdata.ec_hash, sample_ec_hash, sizeof(sample_ec_hash));
	memcpy(sample_packet_cmd_verify_hash.ph.data,
	       sample_ec_hash, sizeof(sample_ec_hash));

	/* Calculate the CRC8 for the sample kernel secdata. */
	size_to_crc = test_secdata.struct_size -
		      offsetof(struct vb2_secdata_kernel, crc8) -
		      sizeof(test_secdata.crc8);
	test_secdata.crc8 = crc8((uint8_t *)&test_secdata.reserved0,
				  size_to_crc);


	/* Start test */
	test_reset();

	board_reboot_ec_deferred(0);
	ec_efs_refresh();
	RUN_TEST(test_ec_comm_packet_failure);

	board_reboot_ec_deferred(0);
	ec_efs_refresh();
	RUN_TEST(test_ec_comm_set_boot_mode);

	board_reboot_ec_deferred(0);
	ec_efs_refresh();
	RUN_TEST(test_ec_comm_verify_hash);

	board_reboot_ec_deferred(0);
	ec_efs_refresh();
	RUN_TEST(test_ec_comm_verify_hash_fail);

#if CONFIG_EC_EFS2_VERSION == 1
	board_reboot_ec_deferred(0);
	ec_efs_refresh();
	RUN_TEST(test_ec_comm_verify_efs2_1);
#endif

	test_print_result();
}
