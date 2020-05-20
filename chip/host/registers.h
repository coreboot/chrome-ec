/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Empty registers header for emulator */

/*
 * There is no register for emulator, but this file exists to prevent
 * compilation failure if any file includes registers.h
 */

#define GNAME(mname, rname)  "GC_ ## mname ## _ ## rname ## _NAME"
#define GREG32(mname, rname)	REG32(get_reg_addr(GNAME(mname, rname)))

void *get_reg_addr(const char * const reg_name);
