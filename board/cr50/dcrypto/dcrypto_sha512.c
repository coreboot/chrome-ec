/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "dcrypto.h"
#include "internal.h"
#include "registers.h"

#ifdef CRYPTO_TEST_SETUP

/* test and benchmark */
#include "common.h"
#include "console.h"
#include "hooks.h"
#include "task.h"
#include "watchdog.h"

#define cyclecounter() GREG32(M3, DWT_CYCCNT)
#define START_PROFILE(x)                                                       \
	{                                                                      \
		x -= cyclecounter();                                           \
	}
#define END_PROFILE(x)                                                         \
	{                                                                      \
		x += cyclecounter();                                           \
	}
static uint32_t t_sw;
static uint32_t t_hw;
static uint32_t t_transform;
static uint32_t t_dcrypto;

#else /* CRYPTO_TEST_SETUP */

#define START_PROFILE(x)
#define END_PROFILE(x)

#endif /* CRYPTO_TEST_SETUP */

/* auto-generated from go test haven -test.run=TestSha512 -test.v */
/* clang-format off */
static const uint32_t IMEM_dcrypto[] = {
/* @0x0: function tag[1] { */
#define CF_tag_adr 0
	0xf8000003, /* sigini #3 */
/* } */
/* @0x1: function expandw[84] { */
#define CF_expandw_adr 1
	0x4c3def00, /* xor r15, r15, r15 */
	0x803c0013, /* movi r15.0l, #19 */
	0x80bc0016, /* movi r15.1l, #22 */
	0x97800f00, /* ldrfp r15 */
	0x05004003, /* loop #4 ( */
	0x8c001800, /* ld *0, *0++ */
	0x906c0800, /* st *0++, *3++ */
	0xfc000000, /* nop */
	/*		   ) */
	0x0501004a, /* loop #16 ( */
	0x684a6080, /* rshi r18, r0, r19 >> 128 */
	0x68443340, /* rshi r17, r19, r1 >> 64 */
	0x683e3201, /* rshi r15, r18, r17 >> 1 */
	0x68423208, /* rshi r16, r18, r17 >> 8 */
	0x4c3e0f00, /* xor r15, r15, r16 */
	0x6843f207, /* rshi r16, r18, r31 >> 7 */
	0x4c3e0f00, /* xor r15, r15, r16 */
	0x505df398, /* add r23, r19, r15 >> 192 */
	0x505eb788, /* add r23, r23, r21 >> 64 */
	0x684ac0c0, /* rshi r18, r0, r22 >> 192 */
	0x68443680, /* rshi r17, r22, r1 >> 128 */
	0x683e3213, /* rshi r15, r18, r17 >> 19 */
	0x6842323d, /* rshi r16, r18, r17 >> 61 */
	0x4c3e0f00, /* xor r15, r15, r16 */
	0x6843f206, /* rshi r16, r18, r31 >> 6 */
	0x4c3e0f00, /* xor r15, r15, r16 */
	0x505df798, /* add r23, r23, r15 >> 192 */
	0x684a60c0, /* rshi r18, r0, r19 >> 192 */
	0x68443380, /* rshi r17, r19, r1 >> 128 */
	0x683e3201, /* rshi r15, r18, r17 >> 1 */
	0x68423208, /* rshi r16, r18, r17 >> 8 */
	0x4c3e0f00, /* xor r15, r15, r16 */
	0x6843f207, /* rshi r16, r18, r31 >> 7 */
	0x4c3e0f00, /* xor r15, r15, r16 */
	0x50627f88, /* add r24, r31, r19 >> 64 */
	0x5061f898, /* add r24, r24, r15 >> 192 */
	0x5062b890, /* add r24, r24, r21 >> 128 */
	0x684416c0, /* rshi r17, r22, r0 >> 192 */
	0x683e3613, /* rshi r15, r22, r17 >> 19 */
	0x6842363d, /* rshi r16, r22, r17 >> 61 */
	0x4c3e0f00, /* xor r15, r15, r16 */
	0x6843f606, /* rshi r16, r22, r31 >> 6 */
	0x4c3e0f00, /* xor r15, r15, r16 */
	0x5061f898, /* add r24, r24, r15 >> 192 */
	0x684433c0, /* rshi r17, r19, r1 >> 192 */
	0x683e3301, /* rshi r15, r19, r17 >> 1 */
	0x68423308, /* rshi r16, r19, r17 >> 8 */
	0x4c3e0f00, /* xor r15, r15, r16 */
	0x6843f307, /* rshi r16, r19, r31 >> 7 */
	0x4c3e0f00, /* xor r15, r15, r16 */
	0x50667f90, /* add r25, r31, r19 >> 128 */
	0x5065f998, /* add r25, r25, r15 >> 192 */
	0x5066b998, /* add r25, r25, r21 >> 192 */
	0x684ae040, /* rshi r18, r0, r23 >> 64 */
	0x683ef213, /* rshi r15, r18, r23 >> 19 */
	0x6842f23d, /* rshi r16, r18, r23 >> 61 */
	0x4c3e0f00, /* xor r15, r15, r16 */
	0x6843f206, /* rshi r16, r18, r31 >> 6 */
	0x4c3e0f00, /* xor r15, r15, r16 */
	0x5065f998, /* add r25, r25, r15 >> 192 */
	0x684a8040, /* rshi r18, r0, r20 >> 64 */
	0x683e9201, /* rshi r15, r18, r20 >> 1 */
	0x68429208, /* rshi r16, r18, r20 >> 8 */
	0x4c3e0f00, /* xor r15, r15, r16 */
	0x6843f207, /* rshi r16, r18, r31 >> 7 */
	0x4c3e0f00, /* xor r15, r15, r16 */
	0x506a7f98, /* add r26, r31, r19 >> 192 */
	0x5069fa98, /* add r26, r26, r15 >> 192 */
	0x506ada00, /* add r26, r26, r22 */
	0x684b0040, /* rshi r18, r0, r24 >> 64 */
	0x683f1213, /* rshi r15, r18, r24 >> 19 */
	0x6843123d, /* rshi r16, r18, r24 >> 61 */
	0x4c3e0f00, /* xor r15, r15, r16 */
	0x6843f206, /* rshi r16, r18, r31 >> 6 */
	0x4c3e0f00, /* xor r15, r15, r16 */
	0x5069fa98, /* add r26, r26, r15 >> 192 */
	0x7c4c1400, /* mov r19, r20 */
	0x7c501500, /* mov r20, r21 */
	0x7c541600, /* mov r21, r22 */
	0x685af640, /* rshi r22, r22, r23 >> 64 */
	0x685b1640, /* rshi r22, r22, r24 >> 64 */
	0x685b3640, /* rshi r22, r22, r25 >> 64 */
	0x685b5640, /* rshi r22, r22, r26 >> 64 */
	0x906c0100, /* st *1, *3++ */
	/*		   ) */
	0x0c000000, /* ret */
/* } */
/* @0x55: function Sha512_a[125] { */
#define CF_Sha512_a_adr 85
	0x68580c40, /* rshi r22, r12, r0 >> 64 */
	0x683c161c, /* rshi r15, r22, r0 >> 28 */
	0x68541622, /* rshi r21, r22, r0 >> 34 */
	0x4c3eaf00, /* xor r15, r15, r21 */
	0x68541627, /* rshi r21, r22, r0 >> 39 */
	0x4c3eaf00, /* xor r15, r15, r21 */
	0x40402000, /* and r16, r0, r1 */
	0x40544000, /* and r21, r0, r2 */
	0x4c42b000, /* xor r16, r16, r21 */
	0x40544100, /* and r21, r1, r2 */
	0x4c42b000, /* xor r16, r16, r21 */
	0x68458fc0, /* rshi r17, r15, r12 >> 192 */
	0x50461100, /* add r17, r17, r16 */
	0x68588d40, /* rshi r22, r13, r4 >> 64 */
	0x6848960e, /* rshi r18, r22, r4 >> 14 */
	0x68549612, /* rshi r21, r22, r4 >> 18 */
	0x4c4ab200, /* xor r18, r18, r21 */
	0x684c9629, /* rshi r19, r22, r4 >> 41 */
	0x4c4a7200, /* xor r18, r18, r19 */
	0x404ca400, /* and r19, r4, r5 */
	0x48548000, /* not r21, r4 */
	0x4054d500, /* and r21, r21, r6 */
	0x4c4eb300, /* xor r19, r19, r21 */
	0x6851b2c0, /* rshi r20, r18, r13 >> 192 */
	0x5050f400, /* add r20, r20, r7 */
	0x50515480, /* add r20, r20, r10 >> 0 */
	0x68558b00, /* rshi r21, r11, r12 >> 0 */
	0x50567500, /* add r21, r21, r19 */
	0x5052b400, /* add r20, r20, r21 */
	0x500e8300, /* add r3, r3, r20 */
	0x501e3400, /* add r7, r20, r17 */
	0x6858ec40, /* rshi r22, r12, r7 >> 64 */
	0x683cf61c, /* rshi r15, r22, r7 >> 28 */
	0x6854f622, /* rshi r21, r22, r7 >> 34 */
	0x4c3eaf00, /* xor r15, r15, r21 */
	0x6854f627, /* rshi r21, r22, r7 >> 39 */
	0x4c3eaf00, /* xor r15, r15, r21 */
	0x40400700, /* and r16, r7, r0 */
	0x40542700, /* and r21, r7, r1 */
	0x4c42b000, /* xor r16, r16, r21 */
	0x40542000, /* and r21, r0, r1 */
	0x4c42b000, /* xor r16, r16, r21 */
	0x68458fc0, /* rshi r17, r15, r12 >> 192 */
	0x50461100, /* add r17, r17, r16 */
	0x68586d40, /* rshi r22, r13, r3 >> 64 */
	0x6848760e, /* rshi r18, r22, r3 >> 14 */
	0x68547612, /* rshi r21, r22, r3 >> 18 */
	0x4c4ab200, /* xor r18, r18, r21 */
	0x684c7629, /* rshi r19, r22, r3 >> 41 */
	0x4c4a7200, /* xor r18, r18, r19 */
	0x404c8300, /* and r19, r3, r4 */
	0x48546000, /* not r21, r3 */
	0x4054b500, /* and r21, r21, r5 */
	0x4c4eb300, /* xor r19, r19, r21 */
	0x6851b2c0, /* rshi r20, r18, r13 >> 192 */
	0x5050d400, /* add r20, r20, r6 */
	0x50515488, /* add r20, r20, r10 >> 64 */
	0x68558b40, /* rshi r21, r11, r12 >> 64 */
	0x50567500, /* add r21, r21, r19 */
	0x5052b400, /* add r20, r20, r21 */
	0x500a8200, /* add r2, r2, r20 */
	0x501a3400, /* add r6, r20, r17 */
	0x6858cc40, /* rshi r22, r12, r6 >> 64 */
	0x683cd61c, /* rshi r15, r22, r6 >> 28 */
	0x6854d622, /* rshi r21, r22, r6 >> 34 */
	0x4c3eaf00, /* xor r15, r15, r21 */
	0x6854d627, /* rshi r21, r22, r6 >> 39 */
	0x4c3eaf00, /* xor r15, r15, r21 */
	0x4040e600, /* and r16, r6, r7 */
	0x40540600, /* and r21, r6, r0 */
	0x4c42b000, /* xor r16, r16, r21 */
	0x40540700, /* and r21, r7, r0 */
	0x4c42b000, /* xor r16, r16, r21 */
	0x68458fc0, /* rshi r17, r15, r12 >> 192 */
	0x50461100, /* add r17, r17, r16 */
	0x68584d40, /* rshi r22, r13, r2 >> 64 */
	0x6848560e, /* rshi r18, r22, r2 >> 14 */
	0x68545612, /* rshi r21, r22, r2 >> 18 */
	0x4c4ab200, /* xor r18, r18, r21 */
	0x684c5629, /* rshi r19, r22, r2 >> 41 */
	0x4c4a7200, /* xor r18, r18, r19 */
	0x404c6200, /* and r19, r2, r3 */
	0x48544000, /* not r21, r2 */
	0x40549500, /* and r21, r21, r4 */
	0x4c4eb300, /* xor r19, r19, r21 */
	0x6851b2c0, /* rshi r20, r18, r13 >> 192 */
	0x5050b400, /* add r20, r20, r5 */
	0x50515490, /* add r20, r20, r10 >> 128 */
	0x68558b80, /* rshi r21, r11, r12 >> 128 */
	0x50567500, /* add r21, r21, r19 */
	0x5052b400, /* add r20, r20, r21 */
	0x50068100, /* add r1, r1, r20 */
	0x50163400, /* add r5, r20, r17 */
	0x6858ac40, /* rshi r22, r12, r5 >> 64 */
	0x683cb61c, /* rshi r15, r22, r5 >> 28 */
	0x6854b622, /* rshi r21, r22, r5 >> 34 */
	0x4c3eaf00, /* xor r15, r15, r21 */
	0x6854b627, /* rshi r21, r22, r5 >> 39 */
	0x4c3eaf00, /* xor r15, r15, r21 */
	0x4040c500, /* and r16, r5, r6 */
	0x4054e500, /* and r21, r5, r7 */
	0x4c42b000, /* xor r16, r16, r21 */
	0x4054e600, /* and r21, r6, r7 */
	0x4c42b000, /* xor r16, r16, r21 */
	0x68458fc0, /* rshi r17, r15, r12 >> 192 */
	0x50461100, /* add r17, r17, r16 */
	0x68582d40, /* rshi r22, r13, r1 >> 64 */
	0x6848360e, /* rshi r18, r22, r1 >> 14 */
	0x68543612, /* rshi r21, r22, r1 >> 18 */
	0x4c4ab200, /* xor r18, r18, r21 */
	0x684c3629, /* rshi r19, r22, r1 >> 41 */
	0x4c4a7200, /* xor r18, r18, r19 */
	0x404c4100, /* and r19, r1, r2 */
	0x48542000, /* not r21, r1 */
	0x40547500, /* and r21, r21, r3 */
	0x4c4eb300, /* xor r19, r19, r21 */
	0x6851b2c0, /* rshi r20, r18, r13 >> 192 */
	0x50509400, /* add r20, r20, r4 */
	0x50515498, /* add r20, r20, r10 >> 192 */
	0x68558bc0, /* rshi r21, r11, r12 >> 192 */
	0x50567500, /* add r21, r21, r19 */
	0x5052b400, /* add r20, r20, r21 */
	0x50028000, /* add r0, r0, r20 */
	0x50123400, /* add r4, r20, r17 */
	0x0c000000, /* ret */
/* } */
/* @0xd2: function Sha512_b[125] { */
#define CF_Sha512_b_adr 210
	0x68588d40, /* rshi r22, r13, r4 >> 64 */
	0x683c961c, /* rshi r15, r22, r4 >> 28 */
	0x68549622, /* rshi r21, r22, r4 >> 34 */
	0x4c3eaf00, /* xor r15, r15, r21 */
	0x68549627, /* rshi r21, r22, r4 >> 39 */
	0x4c3eaf00, /* xor r15, r15, r21 */
	0x4040a400, /* and r16, r4, r5 */
	0x4054c400, /* and r21, r4, r6 */
	0x4c42b000, /* xor r16, r16, r21 */
	0x4054c500, /* and r21, r5, r6 */
	0x4c42b000, /* xor r16, r16, r21 */
	0x6845afc0, /* rshi r17, r15, r13 >> 192 */
	0x50461100, /* add r17, r17, r16 */
	0x68580c40, /* rshi r22, r12, r0 >> 64 */
	0x6848160e, /* rshi r18, r22, r0 >> 14 */
	0x68541612, /* rshi r21, r22, r0 >> 18 */
	0x4c4ab200, /* xor r18, r18, r21 */
	0x684c1629, /* rshi r19, r22, r0 >> 41 */
	0x4c4a7200, /* xor r18, r18, r19 */
	0x404c2000, /* and r19, r0, r1 */
	0x48540000, /* not r21, r0 */
	0x40545500, /* and r21, r21, r2 */
	0x4c4eb300, /* xor r19, r19, r21 */
	0x685192c0, /* rshi r20, r18, r12 >> 192 */
	0x50507400, /* add r20, r20, r3 */
	0x50515480, /* add r20, r20, r10 >> 0 */
	0x6855ab00, /* rshi r21, r11, r13 >> 0 */
	0x50567500, /* add r21, r21, r19 */
	0x5052b400, /* add r20, r20, r21 */
	0x501e8700, /* add r7, r7, r20 */
	0x500e3400, /* add r3, r20, r17 */
	0x68586d40, /* rshi r22, r13, r3 >> 64 */
	0x683c761c, /* rshi r15, r22, r3 >> 28 */
	0x68547622, /* rshi r21, r22, r3 >> 34 */
	0x4c3eaf00, /* xor r15, r15, r21 */
	0x68547627, /* rshi r21, r22, r3 >> 39 */
	0x4c3eaf00, /* xor r15, r15, r21 */
	0x40408300, /* and r16, r3, r4 */
	0x4054a300, /* and r21, r3, r5 */
	0x4c42b000, /* xor r16, r16, r21 */
	0x4054a400, /* and r21, r4, r5 */
	0x4c42b000, /* xor r16, r16, r21 */
	0x6845afc0, /* rshi r17, r15, r13 >> 192 */
	0x50461100, /* add r17, r17, r16 */
	0x6858ec40, /* rshi r22, r12, r7 >> 64 */
	0x6848f60e, /* rshi r18, r22, r7 >> 14 */
	0x6854f612, /* rshi r21, r22, r7 >> 18 */
	0x4c4ab200, /* xor r18, r18, r21 */
	0x684cf629, /* rshi r19, r22, r7 >> 41 */
	0x4c4a7200, /* xor r18, r18, r19 */
	0x404c0700, /* and r19, r7, r0 */
	0x4854e000, /* not r21, r7 */
	0x40543500, /* and r21, r21, r1 */
	0x4c4eb300, /* xor r19, r19, r21 */
	0x685192c0, /* rshi r20, r18, r12 >> 192 */
	0x50505400, /* add r20, r20, r2 */
	0x50515488, /* add r20, r20, r10 >> 64 */
	0x6855ab40, /* rshi r21, r11, r13 >> 64 */
	0x50567500, /* add r21, r21, r19 */
	0x5052b400, /* add r20, r20, r21 */
	0x501a8600, /* add r6, r6, r20 */
	0x500a3400, /* add r2, r20, r17 */
	0x68584d40, /* rshi r22, r13, r2 >> 64 */
	0x683c561c, /* rshi r15, r22, r2 >> 28 */
	0x68545622, /* rshi r21, r22, r2 >> 34 */
	0x4c3eaf00, /* xor r15, r15, r21 */
	0x68545627, /* rshi r21, r22, r2 >> 39 */
	0x4c3eaf00, /* xor r15, r15, r21 */
	0x40406200, /* and r16, r2, r3 */
	0x40548200, /* and r21, r2, r4 */
	0x4c42b000, /* xor r16, r16, r21 */
	0x40548300, /* and r21, r3, r4 */
	0x4c42b000, /* xor r16, r16, r21 */
	0x6845afc0, /* rshi r17, r15, r13 >> 192 */
	0x50461100, /* add r17, r17, r16 */
	0x6858cc40, /* rshi r22, r12, r6 >> 64 */
	0x6848d60e, /* rshi r18, r22, r6 >> 14 */
	0x6854d612, /* rshi r21, r22, r6 >> 18 */
	0x4c4ab200, /* xor r18, r18, r21 */
	0x684cd629, /* rshi r19, r22, r6 >> 41 */
	0x4c4a7200, /* xor r18, r18, r19 */
	0x404ce600, /* and r19, r6, r7 */
	0x4854c000, /* not r21, r6 */
	0x40541500, /* and r21, r21, r0 */
	0x4c4eb300, /* xor r19, r19, r21 */
	0x685192c0, /* rshi r20, r18, r12 >> 192 */
	0x50503400, /* add r20, r20, r1 */
	0x50515490, /* add r20, r20, r10 >> 128 */
	0x6855ab80, /* rshi r21, r11, r13 >> 128 */
	0x50567500, /* add r21, r21, r19 */
	0x5052b400, /* add r20, r20, r21 */
	0x50168500, /* add r5, r5, r20 */
	0x50063400, /* add r1, r20, r17 */
	0x68582d40, /* rshi r22, r13, r1 >> 64 */
	0x683c361c, /* rshi r15, r22, r1 >> 28 */
	0x68543622, /* rshi r21, r22, r1 >> 34 */
	0x4c3eaf00, /* xor r15, r15, r21 */
	0x68543627, /* rshi r21, r22, r1 >> 39 */
	0x4c3eaf00, /* xor r15, r15, r21 */
	0x40404100, /* and r16, r1, r2 */
	0x40546100, /* and r21, r1, r3 */
	0x4c42b000, /* xor r16, r16, r21 */
	0x40546200, /* and r21, r2, r3 */
	0x4c42b000, /* xor r16, r16, r21 */
	0x6845afc0, /* rshi r17, r15, r13 >> 192 */
	0x50461100, /* add r17, r17, r16 */
	0x6858ac40, /* rshi r22, r12, r5 >> 64 */
	0x6848b60e, /* rshi r18, r22, r5 >> 14 */
	0x6854b612, /* rshi r21, r22, r5 >> 18 */
	0x4c4ab200, /* xor r18, r18, r21 */
	0x684cb629, /* rshi r19, r22, r5 >> 41 */
	0x4c4a7200, /* xor r18, r18, r19 */
	0x404cc500, /* and r19, r5, r6 */
	0x4854a000, /* not r21, r5 */
	0x4054f500, /* and r21, r21, r7 */
	0x4c4eb300, /* xor r19, r19, r21 */
	0x685192c0, /* rshi r20, r18, r12 >> 192 */
	0x50501400, /* add r20, r20, r0 */
	0x50515498, /* add r20, r20, r10 >> 192 */
	0x6855abc0, /* rshi r21, r11, r13 >> 192 */
	0x50567500, /* add r21, r21, r19 */
	0x5052b400, /* add r20, r20, r21 */
	0x50128400, /* add r4, r4, r20 */
	0x50023400, /* add r0, r20, r17 */
	0x0c000000, /* ret */
/* } */
/* @0x14f: function compress[70] { */
#define CF_compress_adr 335
	0xfc000000, /* nop */
	0x4c7fff00, /* xor r31, r31, r31 */
	0x4c000000, /* xor r0, r0, r0 */
	0x4c042100, /* xor r1, r1, r1 */
	0x55000001, /* subi r0, r0, #1 */
	0x55040101, /* subi r1, r1, #1 */
	0x84204100, /* ldi r8, [#8] */
	0x94800800, /* ldlc r8 */
	0x4c3def00, /* xor r15, r15, r15 */
	0x803c000a, /* movi r15.0l, #10 */
	0x95800f00, /* lddmp r15 */
	0x06000039, /* loop *0 ( */
	0x953c0000, /* stdmp r15 */
	0x81bc002a, /* movi r15.3l, #42 */
	0x95800f00, /* lddmp r15 */
	0x08000001, /* call &expandw */
	0x84004000, /* ldi r0, [#0] */
	0x84044020, /* ldi r1, [#1] */
	0x84084040, /* ldi r2, [#2] */
	0x840c4060, /* ldi r3, [#3] */
	0x84104080, /* ldi r4, [#4] */
	0x841440a0, /* ldi r5, [#5] */
	0x841840c0, /* ldi r6, [#6] */
	0x841c40e0, /* ldi r7, [#7] */
	0x4c3def00, /* xor r15, r15, r15 */
	0x803c0060, /* movi r15.0l, #96 */
	0x80bc000a, /* movi r15.1l, #10 */
	0x813c000b, /* movi r15.2l, #11 */
	0x96800f00, /* lddrp r15 */
	0x97800f00, /* ldrfp r15 */
	0x953c0000, /* stdmp r15 */
	0x81bc002a, /* movi r15.3l, #42 */
	0x95800f00, /* lddmp r15 */
	0x4c318c00, /* xor r12, r12, r12 */
	0x4c35ad00, /* xor r13, r13, r13 */
	0x55300c01, /* subi r12, r12, #1 */
	0x55340d01, /* subi r13, r13, #1 */
	0x0500a007, /* loop #10 ( */
	0x8c440800, /* ldc *1, *0++ */
	0x8c081b00, /* ld *2, *3++ */
	0x08000055, /* call &Sha512_a */
	0x8c440800, /* ldc *1, *0++ */
	0x8c081b00, /* ld *2, *3++ */
	0x080000d2, /* call &Sha512_b */
	0xfc000000, /* nop */
	/*		   ) */
	0x843c4000, /* ldi r15, [#0] */
	0x5001e000, /* add r0, r0, r15 */
	0x843c4020, /* ldi r15, [#1] */
	0x5005e100, /* add r1, r1, r15 */
	0x843c4040, /* ldi r15, [#2] */
	0x5009e200, /* add r2, r2, r15 */
	0x843c4060, /* ldi r15, [#3] */
	0x500de300, /* add r3, r3, r15 */
	0x843c4080, /* ldi r15, [#4] */
	0x5011e400, /* add r4, r4, r15 */
	0x843c40a0, /* ldi r15, [#5] */
	0x5015e500, /* add r5, r5, r15 */
	0x843c40c0, /* ldi r15, [#6] */
	0x5019e600, /* add r6, r6, r15 */
	0x843c40e0, /* ldi r15, [#7] */
	0x501de700, /* add r7, r7, r15 */
	0x88004000, /* sti r0, [#0] */
	0x88044020, /* sti r1, [#1] */
	0x88084040, /* sti r2, [#2] */
	0x880c4060, /* sti r3, [#3] */
	0x88104080, /* sti r4, [#4] */
	0x881440a0, /* sti r5, [#5] */
	0x881840c0, /* sti r6, [#6] */
	0x881c40e0, /* sti r7, [#7] */
	/*		   ) */
	0x0c000000, /* ret */
	/* } */
};
/* clang-format on */

struct DMEM_sha512 {
	uint64_t H0[4];
	uint64_t H1[4];
	uint64_t H2[4];
	uint64_t H3[4];
	uint64_t H4[4];
	uint64_t H5[4];
	uint64_t H6[4];
	uint64_t H7[4];
	uint32_t nblocks;
	uint32_t unused[2 * 8 - 1];
	uint32_t input[4 * 8 * 8]; // dmem[10..41]
};

static void copy_words(const void *in, uint32_t *dst, size_t nwords)
{
	const uint32_t *src = (const uint32_t *) in;

	do {
		uint32_t w1 = __builtin_bswap32(*src++);
		uint32_t w2 = __builtin_bswap32(*src++);
		*dst++ = w2;
		*dst++ = w1;
	} while (nwords -= 2);
}

static void dcrypto_SHA512_setup(void)
{
	dcrypto_imem_load(0, IMEM_dcrypto, ARRAY_SIZE(IMEM_dcrypto));
}

static void dcrypto_SHA512_Transform(struct sha512_ctx *ctx,
				     const uint32_t *buf, size_t nwords)
{
	int result = 0;
	struct DMEM_sha512 *p512 =
	    (struct DMEM_sha512 *) GREG32_ADDR(CRYPTO, DMEM_DUMMY);

	START_PROFILE(t_transform)

	/* Pass in H[] */
	p512->H0[0] = ctx->state[0];
	p512->H1[0] = ctx->state[1];
	p512->H2[0] = ctx->state[2];
	p512->H3[0] = ctx->state[3];
	p512->H4[0] = ctx->state[4];
	p512->H5[0] = ctx->state[5];
	p512->H6[0] = ctx->state[6];
	p512->H7[0] = ctx->state[7];

	p512->nblocks = nwords / 32;

	/* Pass in buf[] */
	copy_words(buf, p512->input, nwords);

	START_PROFILE(t_dcrypto)
	result |= dcrypto_call(CF_compress_adr);
	END_PROFILE(t_dcrypto)

	/* Retrieve new H[] */
	ctx->state[0] = p512->H0[0];
	ctx->state[1] = p512->H1[0];
	ctx->state[2] = p512->H2[0];
	ctx->state[3] = p512->H3[0];
	ctx->state[4] = p512->H4[0];
	ctx->state[5] = p512->H5[0];
	ctx->state[6] = p512->H6[0];
	ctx->state[7] = p512->H7[0];

	/* TODO: errno or such to capture errors */
	(void) (result == 0);

	END_PROFILE(t_transform)
}

void SHA512_hw_update(struct sha512_ctx *ctx, const void *data, size_t len)
{
	int i = (int)(ctx->count & (sizeof(ctx->b8) - 1));
	const uint8_t *p = (const uint8_t *)data;
	uint8_t *d = &ctx->b8[i];

	ctx->count += len;

	dcrypto_init_and_lock();
	dcrypto_SHA512_setup();

	/* Take fast path for 32-bit aligned 1KB inputs */
	if (i == 0 && len == 1024 && (((intptr_t)data) & 3) == 0) {
		dcrypto_SHA512_Transform(ctx, (const uint32_t *)data, 8 * 32);
	} else {
		if (len <= sizeof(ctx->b8) - i) {
			memcpy(d, p, len);
			if (len == sizeof(ctx->b8) - i) {
				dcrypto_SHA512_Transform(
					ctx, (uint32_t *)(ctx->b8), 32);
			}
		} else {
			memcpy(d, p, sizeof(ctx->b8) - i);
			dcrypto_SHA512_Transform(ctx, ctx->b32, 32);
			d = ctx->b8;
			len -= (sizeof(ctx->b8) - i);
			p += (sizeof(ctx->b8) - i);
			while (len >= sizeof(ctx->b8)) {
				memcpy(d, p, sizeof(ctx->b8));
				p += sizeof(ctx->b8);
				len -= sizeof(ctx->b8);
				dcrypto_SHA512_Transform(
					ctx, (uint32_t *)(ctx->b8), 32);
			}
			/* Leave remainder in ctx->buf */
			memcpy(d, p, len);
		}
	}
	dcrypto_unlock();
}

struct sha512_digest *SHA512_hw_final(struct sha512_ctx *ctx)
{
	uint64_t cnt = ctx->count * 8;
	int i = (int) (ctx->count & (sizeof(ctx->b8) - 1));
	uint8_t *p = &ctx->b8[i];

	*p++ = 0x80;
	i++;

	dcrypto_init_and_lock();
	dcrypto_SHA512_setup();

	if (i > sizeof(ctx->b8) - 16) {
		memset(p, 0, sizeof(ctx->b8) - i);
		dcrypto_SHA512_Transform(ctx, ctx->b32, 32);
		i = 0;
		p = ctx->b8;
	}

	memset(p, 0, sizeof(ctx->b8) - 8 - i);
	p += sizeof(ctx->b8) - 8 - i;

	for (i = 0; i < 8; ++i) {
		uint8_t tmp = (uint8_t)(cnt >> 56);
		cnt <<= 8;
		*p++ = tmp;
	}

	dcrypto_SHA512_Transform(ctx, ctx->b32, 32);

	p = ctx->b8;
	for (i = 0; i < 8; i++) {
		uint64_t tmp = ctx->state[i];
		*p++ = (uint8_t)(tmp >> 56);
		*p++ = (uint8_t)(tmp >> 48);
		*p++ = (uint8_t)(tmp >> 40);
		*p++ = (uint8_t)(tmp >> 32);
		*p++ = (uint8_t)(tmp >> 24);
		*p++ = (uint8_t)(tmp >> 16);
		*p++ = (uint8_t)(tmp >> 8);
		*p++ = (uint8_t)(tmp >> 0);
	}

	dcrypto_unlock();
	return &ctx->digest;
}

BUILD_ASSERT(sizeof(union hash_ctx) >= sizeof(struct sha512_ctx));
BUILD_ASSERT(sizeof(union hash_ctx) >= sizeof(struct sha384_ctx));

static void SHA512_hw_init_as_hash(union hash_ctx *const ctx)
	__alias(SHA512_hw_init);

static void SHA384_hw_init_as_hash(union hash_ctx *const ctx)
	__alias(SHA384_hw_init);

static void SHA512_hw_update_as_hash(union hash_ctx *const ctx,
				     const void *data, size_t len)
	__alias(SHA512_hw_update);

static const union sha_digests *SHA512_final_as_hash(union hash_ctx *const ctx)
	__alias(SHA512_hw_final);

void SHA384_hw_update(struct sha384_ctx *const ctx, const void *data,
		      size_t len) __alias(SHA512_hw_update);

const struct sha384_digest *SHA384_hw_final(struct sha384_ctx *const ctx)
	__alias(SHA512_hw_final);

const struct sha512_digest *SHA512_hw_hash(const void *data, size_t len,
					   struct sha512_digest *digest)
{
	struct sha512_ctx ctx;

	SHA512_hw_init(&ctx);
	SHA512_hw_update(&ctx, data, len);
	memcpy(digest, SHA512_hw_final(&ctx), SHA512_DIGEST_SIZE);

	return digest;
}

void SHA512_hw_init(struct sha512_ctx *ctx)
{
	static const struct hash_vtable dcrypto_SHA512_VTAB = {
		SHA512_hw_init_as_hash,	  SHA512_hw_update_as_hash,
		SHA512_final_as_hash,	  HMAC_sw_final,
		SHA512_DIGEST_SIZE,	  SHA512_BLOCK_SIZE,
		sizeof(struct sha512_ctx)
	};
	static const uint64_t sha512_init[SHA512_DIGEST_DWORDS] = {
		0x6a09e667f3bcc908ll, 0xbb67ae8584caa73bll,
		0x3c6ef372fe94f82bll, 0xa54ff53a5f1d36f1ll,
		0x510e527fade682d1ll, 0x9b05688c2b3e6c1fll,
		0x1f83d9abfb41bd6bll, 0x5be0cd19137e2179ll
	};

	memcpy(ctx->state, sha512_init, sizeof(ctx->state));
	ctx->count = 0;
	ctx->f = &dcrypto_SHA512_VTAB;
}

const struct sha384_digest *SHA384_hw_hash(const void *data, size_t len,
			      struct sha384_digest *digest)
{
	struct sha384_ctx ctx;

	SHA384_hw_init(&ctx);
	SHA384_hw_update(&ctx, data, len);
	memcpy(digest->b8, SHA384_hw_final(&ctx)->b8, SHA384_DIGEST_SIZE);
	return digest;
}

void SHA384_hw_init(struct sha512_ctx *ctx)
{
	static const struct hash_vtable dcrypto_SHA384_VTAB = {
		SHA384_hw_init_as_hash,	  SHA512_hw_update_as_hash,
		SHA512_final_as_hash,	  HMAC_sw_final,
		SHA384_DIGEST_SIZE,	  SHA512_BLOCK_SIZE,
		sizeof(struct sha384_ctx)
	};
	static const uint64_t sha384_init[SHA512_DIGEST_DWORDS] = {
		0xcbbb9d5dc1059ed8ll, 0x629a292a367cd507ll,
		0x9159015a3070dd17ll, 0x152fecd8f70e5939ll,
		0x67332667ffc00b31ll, 0x8eb44a8768581511ll,
		0xdb0c2e0d64f98fa7ll, 0x47b5481dbefa4fa4ll
	};

	memcpy(ctx->state, sha384_init, sizeof(ctx->state));

	ctx->count = 0;
	ctx->f = &dcrypto_SHA384_VTAB;
}

#if defined(CONFIG_SHA512_HW_EQ_SW)
/**
 * Make sw version equal to hw. Unlike SHA2-256, dcrypto implementation
 * of SHA2-512/384 allows to save context, so can fully replace software
 * implementation.
 */
const struct sha512_digest *SHA512_sw_hash(const void *data, size_t len,
					   struct sha512_digest *digest)
	__alias(SHA512_hw_hash);


void SHA512_sw_init(struct sha512_ctx *const ctx) __alias(SHA512_hw_init);

void SHA512_sw_update(struct sha512_ctx *const ctx, const void *data,
		      size_t len) __alias(SHA512_hw_update);

const struct sha512_digest *SHA512_sw_final(struct sha512_ctx *const ctx)
	__alias(SHA512_hw_final);


const struct sha384_digest *SHA384_sw_hash(const void *data, size_t len,
					   struct sha384_digest *digest)
	__alias(SHA384_hw_hash);

void SHA384_sw_init(struct sha384_ctx *const ctx) __alias(SHA384_hw_init);

void SHA384_sw_update(struct sha384_ctx *const ctx, const void *data,
		      size_t len) __alias(SHA384_hw_update);

const struct sha384_digest *SHA384_sw_final(struct sha384_ctx *const ctx)
	__alias(SHA384_hw_final);

#endif

#ifdef CRYPTO_TEST_SETUP

static uint32_t msg[256]; // 1KB
static int msg_len;
static int msg_loops;
static struct sha512_ctx sw;
static struct sha512_ctx hw;
static const struct sha512_digest *sw_digest;
static const struct sha512_digest *hw_digest;
static uint32_t t_sw;
static uint32_t t_hw;

static void run_sha512_cmd(void)
{
	int i;

	t_transform = 0;
	t_dcrypto = 0;
	t_sw = 0;
	t_hw = 0;

	START_PROFILE(t_sw)
	SHA512_sw_init(&sw);
	for (i = 0; i < msg_loops; ++i) {
		SHA512_update(&sw, msg, msg_len);
	}
	sw_digest = SHA512_final(&sw);
	END_PROFILE(t_sw)

	watchdog_reload();

	START_PROFILE(t_hw)
	SHA512_hw_init(&hw);
	for (i = 0; i < msg_loops; ++i) {
		SHA512_update(&hw, msg, msg_len);
	}
	hw_digest = SHA512_final(&hw);
	END_PROFILE(t_hw)

	watchdog_reload();

	ccprintf("sw(%u):\n", t_sw);
	for (i = 0; i < SHA512_DIGEST_SIZE; ++i)
		ccprintf("%02x", sw_digest->b8[i]);
	ccprintf("\n");

	ccprintf("hw(%u/%u/%u):\n", t_hw, t_transform, t_dcrypto);
	for (i = 0; i < SHA512_DIGEST_SIZE; ++i)
		ccprintf("%02x", hw_digest->b8[i]);
	ccprintf("\n");
}

static int cmd_sha512_bench(int argc, char *argv[])
{
	memset(msg, '!', sizeof(msg));

	if (argc > 1) {
		msg_loops = 1;
		msg_len = strlen(argv[1]);
		memcpy(msg, argv[1], msg_len);
	} else {
		msg_loops = 64; // benchmark 64K
		msg_len = sizeof(msg);
	}

	run_sha512_cmd();

	return EC_SUCCESS;
}
DECLARE_SAFE_CONSOLE_COMMAND(sha512_bench, cmd_sha512_bench, NULL, NULL);

static int cmd_sha512_test(int argc, char *argv[])
{
	size_t i;

	ccprintf("sha512 self-test started!\n");

	for (i = 0; i < 129; ++i) {
		memset(msg, i, i);
		watchdog_reload();
		SHA512_sw_init(&sw);
		SHA512_update(&sw, msg, i);
		sw_digest = SHA512_final(&sw);

		SHA512_hw_init(&hw);
		SHA512_update(&hw, msg, i);
		hw_digest = SHA512_final(&hw);

		if (memcmp(sw_digest, hw_digest, SHA512_DIGEST_SIZE) != 0) {
			ccprintf("sha512 self-test fail at %d!\n", i);
			cflush();
		}
	}

	ccprintf("sha512 self-test PASS!\n");

	return EC_SUCCESS;
}
DECLARE_SAFE_CONSOLE_COMMAND(sha512_test, cmd_sha512_test, NULL, NULL);

#endif /* CRYPTO_TEST_SETUP */
