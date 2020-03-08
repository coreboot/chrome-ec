/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* UART driver for emulator */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <termio.h>
#include <unistd.h>

#include "common.h"
#include "queue.h"
#include "task.h"
#include "test_util.h"
#include "uart.h"
#include "uartn.h"
#include "util.h"

static int stopped = 1;
static int init_done;

static pthread_t input_thread;

#define INPUT_BUFFER_SIZE 16
static int char_available;

static struct queue const cached_char = QUEUE_NULL(INPUT_BUFFER_SIZE, char);

#define CONSOLE_CAPTURE_SIZE 2048
static struct {
	char buf[CONSOLE_CAPTURE_SIZE];
	int size;
	int enabled;
} uart_capture[UART_COUNT];


static void putch_into_captured_buf(int uart, char c, int is_last)
{
	uart_capture[uart].buf[uart_capture[uart].size] = c;
	if (!is_last)
		uart_capture[uart].size++;
}

void test_capture_uartn(int uart, int enabled)
{
	if (uart >= UART_COUNT) {
		fprintf(stderr, "Unknown UART port accessed: %d\n", uart);
		exit(1);
	}

	if (enabled == uart_capture[uart].enabled)
		return;

	if (enabled)
		uart_capture[uart].size = 0;
	else
		putch_into_captured_buf(uart, '\0', 1);

	uart_capture[uart].enabled = enabled;
}

void test_capture_console(int enabled)
{
	test_capture_uartn(UART_DEFAULT, enabled);
}

static void test_capture_char(int uart, char c)
{
	if (uart >= UART_COUNT) {
		fprintf(stderr, "Unknown UART port accessed: %d\n", uart);
		exit(1);
	}

	if (uart_capture[uart].size == CONSOLE_CAPTURE_SIZE)
		return;

	putch_into_captured_buf(uart, c, 0);
}

const char *test_get_captured_uartn(int uart)
{
	if (uart >= UART_COUNT) {
		fprintf(stderr, "Unknown UART port accessed: %d\n", uart);
		exit(1);
	}

	return uart_capture[uart].buf;
}

const char *test_get_captured_console(void)
{
	return test_get_captured_uartn(UART_DEFAULT);
}

static void uart_interrupt(void)
{
	uart_process_input();
	uart_process_output();
}

int uart_init_done(void)
{
	return init_done;
}

void uart_tx_start(void)
{
	stopped = 0;
	task_trigger_test_interrupt(uart_interrupt);
}

void uart_tx_stop(void)
{
	stopped = 1;
}

int uart_tx_stopped(void)
{
	return stopped;
}

void uart_tx_flush(void)
{
	uartn_tx_flush(UART_DEFAULT);
}

int uart_tx_ready(void)
{
	return 1;
}

int uart_rx_available(void)
{
	return char_available;
}

void uart_write_char(char c)
{
	uartn_write_char(UART_DEFAULT, c);
}

int uart_read_char(void)
{
	char ret;
	ASSERT(in_interrupt_context());
	queue_remove_unit(&cached_char, &ret);
	--char_available;
	return ret;
}

void uart_inject_char(char *s, int sz)
{
	int i;
	int num_char;

	for (i = 0; i < sz; i += INPUT_BUFFER_SIZE - 1) {
		num_char = MIN(INPUT_BUFFER_SIZE - 1, sz - i);
		if (queue_space(&cached_char) < num_char)
			return;
		queue_add_units(&cached_char, s + i, num_char);
		char_available = num_char;
		task_trigger_test_interrupt(uart_interrupt);
	}
}

void *uart_monitor_stdin(void *d)
{
	struct termios org_settings, new_settings;
	char buf[INPUT_BUFFER_SIZE];
	int rv;

	tcgetattr(0, &org_settings);
	new_settings = org_settings;
	new_settings.c_lflag &= ~(ECHO | ICANON);
	new_settings.c_cc[VTIME] = 0;
	new_settings.c_cc[VMIN] = 1;

	printf("Console input initialized\n");
	while (1) {
		tcsetattr(0, TCSANOW, &new_settings);
		rv = read(0, buf, INPUT_BUFFER_SIZE);
		if (queue_space(&cached_char) >= rv) {
			queue_add_units(&cached_char, buf, rv);
			char_available = rv;
		}
		tcsetattr(0, TCSANOW, &org_settings);
		/*
		 * Trigger emulated interrupt to process input. Keyboard
		 * input while interrupt handler runs is queued by the
		 * system.
		 */
		task_trigger_test_interrupt(uart_interrupt);
	}

	return 0;
}

void uart_init(void)
{
	pthread_create(&input_thread, NULL, uart_monitor_stdin, NULL);
	stopped = 1;  /* Not transmitting yet */
	init_done = 1;
}

test_mockable void uartn_tx_flush(int uart_unused)
{
	/* Nothing */
}

test_mockable void uartn_write_char(int uart, char c)
{
	if (uart_capture[uart].enabled)
		test_capture_char(uart, c);

	if (uart == UART_DEFAULT) {
		printf("%c", c);
		fflush(stdout);
	}
}
