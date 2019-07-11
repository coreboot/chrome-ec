/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "console.h"
#include "hooks.h"
#include "tablet_mode.h"

#define CPRINTS(format, args...) cprints(CC_MOTION_LID, format, ## args)
#define CPRINTF(format, args...) cprintf(CC_MOTION_LID, format, ## args)

/* 1: in tablet mode; 0: notebook mode; -1: uninitialized  */
static int tablet_mode = -1;

/*
 * 1: all calls to tablet_set_mode are ignored and tablet_mode if forced to 0
 * 0: all calls to tablet_set_mode are honored
 */
static int disabled;

int tablet_get_mode(void)
{
	return !!tablet_mode;
}

void tablet_set_mode(int mode)
{
	if (tablet_mode == mode)
		return;

	if (disabled) {
		CPRINTS("Tablet mode set while disabled (ignoring)!");
		return;
	}

	tablet_mode = mode;
	CPRINTS("tablet mode %sabled", mode ? "en" : "dis");
	hook_notify(HOOK_TABLET_MODE_CHANGE);
}

void tablet_disable(void)
{
	tablet_mode = 0;
	disabled = 1;
}
