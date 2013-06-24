/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>
#include <time.h>
#include <unistd.h>

#include "comm-host.h"
#include "compile_time_macros.h"
#include "ectool.h"
#include "lock/gec_lock.h"

#define GEC_LOCK_TIMEOUT_SECS	30  /* 30 secs */
#define BUF_SIZE 0x40


struct ec_params_i2c_read pRead;
struct ec_response_i2c_read rRead;

struct ec_params_i2c_write pWrite;

int fet1_write(uint8_t d)
{
	pWrite.write_size = 8;
	pWrite.port = 0;
	pWrite.addr = 0x90;
	pWrite.offset = 0xf;
	pWrite.data = d;

	return ec_command(EC_CMD_I2C_WRITE, 0, &pWrite, sizeof(pWrite),
			  NULL, 0);
}

uint8_t fet1_read(void)
{
	pRead.read_size = 8;
	pRead.port = 0;
	pRead.addr = 0x90;
	pRead.offset = 0xf;

	ec_command(EC_CMD_I2C_READ, 0, &pRead, sizeof(pRead),
		   &rRead, sizeof(rRead));

	return rRead.data;
}

int cmd_cure_fet1(int argc, char *argv[])
{
	int iter, on_usec, off_usec;
	int i;
	char *e;

	char buf[BUF_SIZE + 1];

	if (argc != 4) {
		fprintf(stderr,
			"Usage: %s <iterations> <on_usec> <off_usec>\n",
			argv[0]);
		return -1;
	}

	iter = strtol(argv[1], &e, 0);
	if ((e && *e) || iter < 0) {
		fprintf(stderr, "Bad iteration.\n");
		return -1;
	}

	on_usec = strtol(argv[2], &e, 0);
	if ((e && *e) || on_usec < 0) {
		fprintf(stderr, "Bad on-time.\n");
		return -1;
	}

	off_usec = strtol(argv[3], &e, 0);
	if ((e && *e) || off_usec < 0) {
		fprintf(stderr, "Bad off-time.\n");
		return -1;
	}

	for (i = 0; i < iter; ++i) {
		fet1_write(0xf);
		usleep(on_usec);
		buf[i & (BUF_SIZE - 1)] = (fet1_read() & 0x80) ? '1' : '0';
		if ((i & (BUF_SIZE - 1)) == BUF_SIZE - 1) {
			buf[BUF_SIZE] = '\0';
			printf("%s", buf);
		}
		fet1_write(0xe);
		usleep(off_usec);
	}
	buf[iter & (BUF_SIZE - 1)] = '\0';
	printf("%s\n", buf);
	fet1_write(0xf);

	return 0;
}


int main(int argc, char *argv[])
{
	int rv = 1;

	if (acquire_gec_lock(GEC_LOCK_TIMEOUT_SECS) < 0) {
		fprintf(stderr, "Could not acquire GEC lock.\n");
		exit(1);
	}

	if (comm_init()) {
		fprintf(stderr, "Couldn't find EC\n");
		goto out;
	}

	rv = cmd_cure_fet1(argc, argv);

out:
	release_gec_lock();
	return !!rv;
}
