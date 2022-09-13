/* Copyright 2013 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Persistence module for emulator */

#include <assert.h>
#include <linux/limits.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

static void get_storage_path(char *out)
{
	char buf[PATH_MAX];
	int sz;
	char *current;

	sz = readlink("/proc/self/exe", buf, PATH_MAX - 1);
	buf[sz] = '\0';

	/* replace / by underscores in the path to get the shared memory name */
	current = strchr(buf, '/');
	while (current) {
		*current = '_';
		current = strchr(current, '/');
	}

	assert(snprintf(out, PATH_MAX - 1, "/dev/shm/EC_persist_%s", buf) > 0);
	out[PATH_MAX - 1] = '\0';
}

FILE *get_persistent_storage(const char *tag, const char *mode)
{
	char buf[PATH_MAX];
	char path[PATH_MAX];

	/*
	 * The persistent storage with tag 'foo' for test 'bar' would
	 * be named 'bar_persist_foo'
	 */
	get_storage_path(buf);
	assert(snprintf(path, PATH_MAX - 1, "%s_%s", buf, tag) > 0);
	path[PATH_MAX - 1] = '\0';

	return fopen(path, mode);
}

void release_persistent_storage(FILE *ps)
{
	fclose(ps);
}

void remove_persistent_storage(const char *tag)
{
	char buf[PATH_MAX];
	char path[PATH_MAX];

	get_storage_path(buf);
	assert(snprintf(path, PATH_MAX - 1, "%s_%s", buf, tag) > 0);
	path[PATH_MAX - 1] = '\0';

	unlink(path);
}
