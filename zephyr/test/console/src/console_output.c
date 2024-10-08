/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.h"
#include "console.h"
#include "ec_app_main.h"
#include "zephyr/kernel.h"

#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/interrupt_util.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_backend.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(test_console_out, LOG_LEVEL_DBG);

#define EMUL_UART_NODE DT_NODELABEL(euart0)
#define EMUL_UART_RX_FIFO_SIZE DT_PROP(EMUL_UART_NODE, rx_fifo_size)
#define EMUL_UART_TX_FIFO_SIZE DT_PROP(EMUL_UART_NODE, tx_fifo_size)

#define SAMPLE_DATA_SIZE MIN(EMUL_UART_RX_FIFO_SIZE, EMUL_UART_TX_FIFO_SIZE) - 1

#ifdef CONFIG_SHELL_BACKEND_SERIAL_API_INTERRUPT_DRIVEN
#define SHELL_SLEEP() k_sleep(K_MSEC(5))
#else
#define SHELL_SLEEP()
#endif

struct console_output_fixture {
	const struct device *dev;
};

static void *setup(void)
{
	static struct console_output_fixture fixture = {
		.dev = DEVICE_DT_GET(EMUL_UART_NODE),
	};

	zassert_not_null(fixture.dev);

	return &fixture;
}

static void before(void *f)
{
	struct console_output_fixture *fixture = f;

	uart_emul_flush_tx_data(fixture->dev);

	uart_irq_tx_enable(fixture->dev);
	uart_irq_rx_enable(fixture->dev);

	zassert_ok(uart_err_check(fixture->dev));

	console_channel_enable("system");
	console_channel_enable("zephyr_log");
}

ZTEST_SUITE(console_output, NULL, setup, before, NULL, NULL);

static const char *cputs_message = "cputs() test output";
static const char *cprints_message = "cprints() test output";
static const char *cprintf_message = "cprintf() test output";

/**
 * @brief Test non-shell output from the legacy EC.
 */
ZTEST_F(console_output, test_legacy_debug_output)
{
	uint8_t tx_content[SAMPLE_DATA_SIZE];
	uint32_t tx_bytes;

	/* All legacy output is sent to the shell backend which inserts
	 * a prompt and other control characters.
	 * Just look for our substring in the output.
	 */
	uart_emul_flush_tx_data(fixture->dev);
	memset(tx_content, 0, sizeof(tx_content));
	cputs(CC_SYSTEM, cputs_message);
	SHELL_SLEEP();
	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));
	zassert_true(tx_bytes >= strlen(cputs_message));
	zassert_not_null(strstr((char *)tx_content, cputs_message));

	uart_emul_flush_tx_data(fixture->dev);
	memset(tx_content, 0, sizeof(tx_content));
	cprints(CC_SYSTEM, "%s", cprints_message);
	SHELL_SLEEP();
	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));
	zassert_true(tx_bytes >= strlen(cprints_message));
	zassert_not_null(strstr((char *)tx_content, cprints_message));

	uart_emul_flush_tx_data(fixture->dev);
	memset(tx_content, 0, sizeof(tx_content));
	cprintf(CC_SYSTEM, "%s", cprintf_message);
	SHELL_SLEEP();
	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));
	zassert_true(tx_bytes >= strlen(cprintf_message));
	zassert_not_null(strstr((char *)tx_content, cprintf_message));

	/* Filter out CC_SYSTEM, no output should be generated. */
	uart_emul_flush_tx_data(fixture->dev);
	memset(tx_content, 0, sizeof(tx_content));

	console_channel_disable("system");
	cputs(CC_SYSTEM, cputs_message);
	cprints(CC_SYSTEM, "%s", cprints_message);
	cprintf(CC_SYSTEM, "%s", cprintf_message);
	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));
	zassert_equal(tx_bytes, 0);
}

static const char *cputs_system_message = "cputs(CC_SYSTEM) test output";
static const char *shell_invalid_param = "Parameter 1 invalid";
static const char *shell_help_output = "Usage:";

/* Verify that filtering Zephyr log messages still allows legacy
 * EC output through.
 */
ZTEST_F(console_output, test_legacy_output_with_log_filtered)
{
	uint8_t tx_content[SAMPLE_DATA_SIZE];
	uint32_t tx_bytes;

	/* Disable all legacy channels to simulate how FAFT is typically run. */
	zassert_ok(shell_execute_cmd(get_ec_shell(), "chan 0"));
	k_sleep(K_MSEC(1));

	/* Enable just the CC_SYSTEM channel. */
	console_channel_enable("system");

	uart_emul_flush_tx_data(fixture->dev);
	memset(tx_content, 0, sizeof(tx_content));
	cputs(CC_SYSTEM, cputs_system_message);
	SHELL_SLEEP();
	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));
	zassert_true(tx_bytes >= strlen(cputs_system_message));
	zassert_not_null(strstr((char *)tx_content, cputs_system_message));

	/* Running a legacy shell command with a parameter error should still
	 * generate output.
	 */
	uart_emul_flush_tx_data(fixture->dev);
	memset(tx_content, 0, sizeof(tx_content));

	zassert_equal(shell_execute_cmd(get_ec_shell(),
					"chan not_a_valid_name"),
		      EC_ERROR_PARAM1);
	k_sleep(K_MSEC(1));

	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));
	zassert_true(tx_bytes >= strlen(shell_invalid_param));
	zassert_not_null(strstr((char *)tx_content, shell_invalid_param));

	/* Running help on a legacy shell command should generate output. */
	uart_emul_flush_tx_data(fixture->dev);
	memset(tx_content, 0, sizeof(tx_content));
	zassert_ok(shell_execute_cmd(get_ec_shell(), "chan help"));
	k_sleep(K_MSEC(1));
	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));
	zassert_true(tx_bytes >= strlen(shell_help_output));
	zassert_not_null(strstr((char *)tx_content, shell_help_output));

	zassert_ok(shell_execute_cmd(get_ec_shell(), "chan restore"));
	k_sleep(K_MSEC(1));
}

ZTEST_F(console_output, test_legacy_shell_output)
{
	uint8_t tx_content[SAMPLE_DATA_SIZE];
	uint32_t tx_bytes;

	/* Disable "all" legacy channels.  The CC_COMMAND channel
	 * Should remain enabled.
	 */
	zassert_ok(shell_execute_cmd(get_ec_shell(), "chan 0"));
	k_sleep(K_MSEC(1));

	/* The shell backend inserts a prompt and other control characters.
	 * Just look for our substring in the output.
	 */
	uart_emul_flush_tx_data(fixture->dev);
	memset(tx_content, 0, sizeof(tx_content));
	cputs(CC_COMMAND, cputs_message);
	SHELL_SLEEP();
	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));
	zassert_true(tx_bytes >= strlen(cputs_message));
	zassert_not_null(strstr((char *)tx_content, cputs_message));

	/* Test cprints() to the shell. */
	uart_emul_flush_tx_data(fixture->dev);
	memset(tx_content, 0, sizeof(tx_content));
	cprints(CC_COMMAND, "%s", cprints_message);
	SHELL_SLEEP();
	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));
	zassert_true(tx_bytes >= strlen(cprints_message));
	zassert_not_null(strstr((char *)tx_content, cprints_message));

	/* Test cprintf() to the shell. */
	uart_emul_flush_tx_data(fixture->dev);
	memset(tx_content, 0, sizeof(tx_content));
	cprintf(CC_COMMAND, "%s", cprintf_message);
	SHELL_SLEEP();
	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));
	zassert_true(tx_bytes >= strlen(cprintf_message));
	zassert_not_null(strstr((char *)tx_content, cprintf_message));

	zassert_ok(shell_execute_cmd(get_ec_shell(), "chan restore"));
	k_sleep(K_MSEC(1));
}

static const char *log_raw_message = "LOG_RAW test output";
static const char *log_err_message = "LOG_ERR test output";
static const char *log_inf_message = "LOG_INF test output";
static const char *log_dbg_message = "LOG_DBG test output";

ZTEST_F(console_output, test_log_output)
{
	uint8_t tx_content[SAMPLE_DATA_SIZE];
	uint32_t tx_bytes;

	/* We expect an exact match with LOG_RAW() output. */
	uart_emul_flush_tx_data(fixture->dev);
	memset(tx_content, 0, sizeof(tx_content));
	LOG_RAW("%s", log_raw_message);
	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));
	zassert_equal(tx_bytes, strlen(log_raw_message));
	zassert_mem_equal(tx_content, log_raw_message, strlen(tx_content));

	/* LOG_ERR prepends the output, so an exact match isn't possible. */
	uart_emul_flush_tx_data(fixture->dev);
	memset(tx_content, 0, sizeof(tx_content));
	LOG_ERR("%s", log_err_message);
	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));
	zassert_true(tx_bytes >= strlen(log_err_message));
	zassert_not_null(strstr((char *)tx_content, log_err_message));

	/* LOG_INF prepends the output, so an exact match isn't possible. */
	uart_emul_flush_tx_data(fixture->dev);
	memset(tx_content, 0, sizeof(tx_content));
	LOG_INF("%s", log_inf_message);
	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));
	zassert_true(tx_bytes >= strlen(log_inf_message));
	zassert_not_null(strstr((char *)tx_content, log_inf_message));

	/* LOG_DBG prepends the output, so an exact match isn't possible. */
	uart_emul_flush_tx_data(fixture->dev);
	memset(tx_content, 0, sizeof(tx_content));
	LOG_DBG("%s", log_dbg_message);
	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));
	zassert_true(tx_bytes >= strlen(log_dbg_message));
	zassert_not_null(strstr((char *)tx_content, log_dbg_message));

	/* Filter out CC_ZEPHYR_LOG, no output should be generated. */
	console_channel_disable("zephyr_log");
	LOG_RAW("%s", log_raw_message);
	LOG_ERR("%s", log_err_message);
	LOG_INF("%s", log_inf_message);
	LOG_DBG("%s", log_dbg_message);
	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));
	zassert_equal(tx_bytes, 0);
}

static const char *shell_message = "Zephyr shell test output";

ZTEST_F(console_output, test_shell_output)
{
	uint8_t tx_content[SAMPLE_DATA_SIZE];
	uint32_t tx_bytes;
	const struct shell *ec_sh;

	/* Disable "all" legacy channels.  Output from the shell subsystem
	 * should remain enabled.
	 */
	ec_sh = get_ec_shell();
	zassert_ok(shell_execute_cmd(ec_sh, "chan 0"));
	k_sleep(K_MSEC(1));

	uart_emul_flush_tx_data(fixture->dev);
	memset(tx_content, 0, sizeof(tx_content));
	shell_fprintf(ec_sh, SHELL_NORMAL, "%s", shell_message);
	k_sleep(K_MSEC(1));

	/* The shell backend inserts a prompt and other control characters.
	 * Just look for our substring in the output.
	 */
	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));
	zassert_true(tx_bytes >= strlen(shell_message));
	zassert_not_null(strstr((char *)tx_content, shell_message));

	zassert_ok(shell_execute_cmd(get_ec_shell(), "chan restore"));
	k_sleep(K_MSEC(1));
}

#define TEST_IRQ_LINE_1 27
#define TEST_IRQ_LINE_2 28
#define TEST_IRQ_LINE_3 29

#define TEST_IRQ_PRIO 2

enum output_type {
	OUTPUT_CPUTS,
	OUTPUT_CPRINTS,
	OUTPUT_CPRINTF,
};

static const char *isr_cputs = "cputs from ISR";
static const char *isr_cprints = "cprints from ISR";
static const char *isr_cprintf = "cprintf from ISR";

static bool cputs_isr_called;
static bool cprints_isr_called;
static bool cprintf_isr_called;

void test_isr(const void *param)
{
	enum output_type type = (enum output_type)param;

	switch (type) {
	case OUTPUT_CPUTS:
		cputs_isr_called = true;
		cputs(CC_SYSTEM, isr_cputs);
		break;
	case OUTPUT_CPRINTS:
		cprints_isr_called = true;
		cprints(CC_SYSTEM, "%s", isr_cprints);
		break;
	case OUTPUT_CPRINTF:
		cprintf_isr_called = true;
		cprintf(CC_SYSTEM, "%s", isr_cprintf);
		break;
	}
}

ZTEST_F(console_output, test_isr_output)
{
	uint8_t tx_content[SAMPLE_DATA_SIZE];
	uint32_t tx_bytes;

	IRQ_CONNECT(TEST_IRQ_LINE_1, TEST_IRQ_PRIO, test_isr,
		    (void *)OUTPUT_CPUTS, 0);
	IRQ_CONNECT(TEST_IRQ_LINE_2, TEST_IRQ_PRIO, test_isr,
		    (void *)OUTPUT_CPRINTS, 0);
	IRQ_CONNECT(TEST_IRQ_LINE_3, TEST_IRQ_PRIO, test_isr,
		    (void *)OUTPUT_CPRINTF, 0);

	irq_enable(TEST_IRQ_LINE_1);
	irq_enable(TEST_IRQ_LINE_2);
	irq_enable(TEST_IRQ_LINE_3);

	SHELL_SLEEP();
	uart_emul_flush_tx_data(fixture->dev);

	trigger_irq(TEST_IRQ_LINE_1);
	zassert_true(cputs_isr_called);
	SHELL_SLEEP();

	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));

	if (IS_ENABLED(CONFIG_PLATFORM_EC_ISR_CONSOLE_OUTPUT)) {
		zassert_true(tx_bytes >= strlen(isr_cputs));
		zassert_not_null(strstr((char *)tx_content, isr_cputs));
	} else {
		zassert_true(tx_bytes == 0);
	}

	uart_emul_flush_tx_data(fixture->dev);
	trigger_irq(TEST_IRQ_LINE_2);
	zassert_true(cprints_isr_called);
	SHELL_SLEEP();

	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));

	if (IS_ENABLED(CONFIG_PLATFORM_EC_ISR_CONSOLE_OUTPUT)) {
		zassert_true(tx_bytes >= strlen(isr_cprints));
		zassert_not_null(strstr((char *)tx_content, isr_cprints));
	} else {
		zassert_true(tx_bytes == 0);
	}

	uart_emul_flush_tx_data(fixture->dev);
	trigger_irq(TEST_IRQ_LINE_3);
	zassert_true(cprintf_isr_called);
	SHELL_SLEEP();

	tx_bytes = uart_emul_get_tx_data(fixture->dev, tx_content,
					 sizeof(tx_content));

	if (IS_ENABLED(CONFIG_PLATFORM_EC_ISR_CONSOLE_OUTPUT)) {
		zassert_true(tx_bytes >= strlen(isr_cprintf));
		zassert_not_null(strstr((char *)tx_content, isr_cprintf));
	} else {
		zassert_true(tx_bytes == 0);
	}
}

void test_main(void)
{
	ec_app_main();

	/* Allow Zephyr defined threads a chance to run. */
	k_sleep(K_MSEC(10));

	/* Run all the suites that depend on main being called */
	ztest_run_test_suites(NULL, false, 1, 1);

	/* Check that every suite ran */
	ztest_verify_all_test_suites_ran();
}
