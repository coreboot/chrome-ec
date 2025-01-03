/* Copyright 2023 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Test battery info in CBI
 */

#include "battery_fuel_gauge.h"
#include "common.h"
#include "console.h"
#include "cros_board_info.h"
#include "ec_commands.h"
#include "test_util.h"
#include "util.h"
#include "write_protect.h"

const struct board_batt_params *get_batt_params(void);

const struct batt_conf_embed board_battery_info[] = {
	[BATTERY_C214] = {
		.manuf_name = "AS1GUXd3KB",
		.device_name = "C214-43",
		.config = {
			.fuel_gauge = {
				.ship_mode = {
					.reg_addr = 0x0,
					.reg_data = { 0x10, 0x10 },
				},
				.fet = {
					.reg_addr = 0x00,
					.reg_mask = 0x2000,
					.disconnect_val = 0x2000,
				},
				.flags = FUEL_GAUGE_FLAG_MFGACC,
			},
			.batt_info = {
				.voltage_max = 13200,
				.voltage_normal = 11550,
				.voltage_min = 9000,
				.precharge_current = 256,
				.start_charging_min_c = 0,
				.start_charging_max_c = 45,
				.charging_min_c = 0,
				.discharging_min_c = 0,
				.discharging_max_c = 60,
			},
		},
	},
};

static struct board_batt_params conf_in_cbi = {
	.fuel_gauge = {
		.ship_mode = {
			.reg_addr = 0xaa,
			.reg_data = {
				[0] = 0x89ab,
				[1] = 0xcdef,
			},
		},
	},
	.batt_info = {
		.voltage_max = 8400,
		.voltage_normal = 7400,
		.voltage_min = 6000,
		.precharge_current = 64, /* mA */
		.start_charging_min_c = 0,
		.start_charging_max_c = 50,
		.charging_min_c = 0,
		.charging_max_c = 50,
		.discharging_min_c = -20,
		.discharging_max_c = 60,
	},
};

static const char *manuf_in_batt = "AS1GUXd3KB";
static const char *device_in_batt = "C214-43";

int battery_manufacturer_name(char *dest, int size)
{
	if (!manuf_in_batt)
		return EC_ERROR_UNKNOWN;
	strncpy(dest, manuf_in_batt, size);
	return EC_SUCCESS;
}

int battery_device_name(char *dest, int size)
{
	if (!device_in_batt)
		return EC_ERROR_UNKNOWN;
	strncpy(dest, device_in_batt, size);
	return EC_SUCCESS;
}

const enum battery_type DEFAULT_BATTERY_TYPE = BATTERY_C214;

static void test_setup(void)
{
	/* Make sure that write protect is disabled */
	write_protect_set(0);

	cbi_create();
	cbi_write();

	manuf_in_batt = "AS1GUXd3KB";
	device_in_batt = "C214-43";
}

static void test_teardown(void)
{
}

static void cbi_set_batt_conf(const struct board_batt_params *conf,
			      const char *manuf_name, const char *device_name)
{
	uint8_t buf[BATT_CONF_MAX_SIZE];
	struct batt_conf_header *head = (void *)buf;
	void *p = buf;
	uint8_t size;

	head->struct_version = 0;
	head->manuf_name_size = strlen(manuf_name);
	head->device_name_size = strlen(device_name);

	/* Copy names. Don't copy the terminating null. */
	p += sizeof(*head);
	memcpy(p, manuf_name, head->manuf_name_size);
	p += head->manuf_name_size;
	memcpy(p, device_name, head->device_name_size);
	p += head->device_name_size;
	memcpy(p, conf, sizeof(*conf));

	size = sizeof(*head) + head->manuf_name_size + head->device_name_size +
	       sizeof(*conf);
	cbi_set_board_info(CBI_TAG_BATTERY_CONFIG, buf, size);
	ccprintf("CBI conf is set to '%s,%s'\n", manuf_name, device_name);
}

DECLARE_EC_TEST(test_power_on_reset)
{
	ccprintf("\ntest_power_on_reset\n");
	/* On POR, no config in CBI. Legacy mode should choose conf[0]. */
	zassert_equal_ptr(get_batt_params(), &board_battery_info[0].config);

	return EC_SUCCESS;
}

DECLARE_EC_TEST(test_manuf_name_mismatch)
{
	/*
	 * manuf_name != manuf_name
	 */
	ccprintf("\ntest_manuf_name_mismatch\n");
	manuf_in_batt = "xyz";
	cbi_set_batt_conf(&conf_in_cbi, "foo", "");
	init_battery_type();
	zassert_equal_ptr(get_batt_params(), &board_battery_info[0].config);

	return EC_SUCCESS;
}

DECLARE_EC_TEST(test_empty_device_name)
{
	const struct board_batt_params *conf;

	/*
	 * manuf_name == manuf_name && device_name == ""
	 */
	ccprintf("\ntest_empty_device_name\n");
	manuf_in_batt = "xyz";
	cbi_set_batt_conf(&conf_in_cbi, manuf_in_batt, "");
	init_battery_type();
	conf = get_batt_params();
	zassert_equal(memcmp(conf, &conf_in_cbi, sizeof(*conf)), 0);
	zassert_equal(strcmp(get_batt_conf()->manuf_name, manuf_in_batt), 0);

	return EC_SUCCESS;
}

DECLARE_EC_TEST(test_device_name_mismatch)
{
	/*
	 * manuf_name == manuf_name && device_name != device_name
	 */
	ccprintf("\ntest_device_name_mismatch\n");
	manuf_in_batt = "xyz";
	cbi_set_batt_conf(&conf_in_cbi, manuf_in_batt, "foo");
	init_battery_type();
	zassert_equal_ptr(get_batt_params(), &board_battery_info[0].config);

	return EC_SUCCESS;
}

DECLARE_EC_TEST(test_match_in_cbi)
{
	const struct board_batt_params *conf;

	/*
	 * manuf_name == manuf_name && device_name == device_name
	 */
	ccprintf("\ntest_match_in_cbi\n");
	manuf_in_batt = "xyz";
	cbi_set_batt_conf(&conf_in_cbi, manuf_in_batt, device_in_batt);
	init_battery_type();
	conf = get_batt_params();
	zassert_equal(memcmp(conf, &conf_in_cbi, sizeof(*conf)), 0);
	zassert_equal(strcmp(get_batt_conf()->manuf_name, manuf_in_batt), 0);
	zassert_equal(strcmp(get_batt_conf()->device_name, device_in_batt), 0);

	return EC_SUCCESS;
}

DECLARE_EC_TEST(test_search_fw_first)
{
	ccprintf("\ntest_search_fw_first\n");
	/* Create duplicate entry in CBI. */
	cbi_set_batt_conf(&conf_in_cbi, manuf_in_batt, device_in_batt);
	init_battery_type();
	/* Verify config_fw[0] is selected. */
	zassert_equal_ptr(get_batt_params(), &board_battery_info[0].config);

	return EC_SUCCESS;
}

DECLARE_EC_TEST(test_device_name_as_prefix)
{
	const struct board_batt_params *conf;

	/*
	 * Battery's device name contains extra chars.
	 */
	ccprintf("\ntest_device_name_as_prefix\n");
	manuf_in_batt = "xyz";
	cbi_set_batt_conf(&conf_in_cbi, manuf_in_batt, "C214-43");
	device_in_batt = "C214-43 123";
	init_battery_type();
	conf = get_batt_params();
	zassert_equal(memcmp(conf, &conf_in_cbi, sizeof(*conf)), 0);
	zassert_equal(strcmp(get_batt_conf()->manuf_name, manuf_in_batt), 0);
	zassert_equal(strcmp(get_batt_conf()->device_name, "C214-43"), 0);

	return EC_SUCCESS;
}

DECLARE_EC_TEST(test_manuf_name_not_found)
{
	/*
	 * Manuf name not found in battery.
	 */
	ccprintf("\ntest_manuf_name_not_found\n");
	manuf_in_batt = NULL;
	init_battery_type();
	zassert_equal_ptr(get_batt_params(), &board_battery_info[0].config);
	manuf_in_batt = "AS1GUXd3KB";

	return EC_SUCCESS;
}

DECLARE_EC_TEST(test_device_name_not_found)
{
	/*
	 * Device name not found in battery.
	 */
	ccprintf("\ntest_device_name_not_found\n");
	device_in_batt = NULL;
	init_battery_type();
	zassert_equal_ptr(get_batt_params(), &board_battery_info[0].config);
	device_in_batt = "C214-43";

	return EC_SUCCESS;
}

DECLARE_EC_TEST(test_batt_conf_main_invalid)
{
	struct batt_conf_header head;

	/*
	 * Version mismatch
	 */
	ccprintf("\nVersion mismatch\n");
	head.struct_version = EC_BATTERY_CONFIG_STRUCT_VERSION + 1;
	cbi_set_board_info(CBI_TAG_BATTERY_CONFIG, (void *)&head, sizeof(head));
	init_battery_type();
	zassert_equal_ptr(get_batt_params(), &board_battery_info[0].config);
	head.struct_version = EC_BATTERY_CONFIG_STRUCT_VERSION;

	/*
	 * Size mismatch
	 */
	ccprintf("\nSize mismatch\n");
	head.manuf_name_size = 0xff;
	cbi_set_board_info(CBI_TAG_BATTERY_CONFIG, (void *)&head, sizeof(head));
	init_battery_type();
	zassert_equal_ptr(get_batt_params(), &board_battery_info[0].config);

	return EC_SUCCESS;
}

TEST_SUITE(test_suite_battery_config)
{
	ztest_test_suite(
		test_battery_config,
		ztest_unit_test_setup_teardown(test_power_on_reset, test_setup,
					       test_teardown),
		ztest_unit_test_setup_teardown(test_manuf_name_mismatch,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_empty_device_name,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_device_name_mismatch,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_match_in_cbi, test_setup,
					       test_teardown),
		ztest_unit_test_setup_teardown(test_search_fw_first, test_setup,
					       test_teardown),
		ztest_unit_test_setup_teardown(test_device_name_as_prefix,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_manuf_name_not_found,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_device_name_not_found,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_batt_conf_main_invalid,
					       test_setup, test_teardown));
	ztest_run_test_suite(test_battery_config);
}
