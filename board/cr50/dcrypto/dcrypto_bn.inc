/* AUTO-GENERATED.  DO NOT MODIFY. */
/* clang-format off */
static const uint32_t IMEM_dcrypto_bn[] = {
/* @0x0: function tag[1] { */
#define CF_tag_adr 0
0xf8000001,	/* sigini #1 */
/* } */
/* @0x1: function d0inv[14] { */
#define CF_d0inv_adr 1
0x4c000000,	/* xor r0, r0, r0 */
0x80000001,	/* movi r0.0l, #1 */
0x7c740000,	/* mov r29, r0 */
0x05100008,	/* loop #256 ( */
0x5807bc00,	/* mul128 r1, r28l, r29l */
0x588bbc00,	/* mul128 r2, r28u, r29l */
0x50044110,	/* add r1, r1, r2 << 128 */
0x590bbc00,	/* mul128 r2, r28l, r29u */
0x50044110,	/* add r1, r1, r2 << 128 */
0x40040100,	/* and r1, r1, r0 */
0x44743d00,	/* or r29, r29, r1 */
0x50000000,	/* add r0, r0, r0 */
/*		   ) */
0x5477bf00,	/* sub r29, r31, r29 */
0x0c000000,	/* ret */
/* } */
/* @0xf: function selcxSub[25] { */
#define CF_selcxSub_adr 15
0x97800100,	/* ldrfp r1 */
0x95800000,	/* lddmp r0 */
0x99100000,	/* strnd r4 */
0x5013e400,	/* add r4, r4, r31 */
0x1000101e,	/* bl selcxSub_invsel */
0x528c8402,	/* addcx r3, r4, r4 << 16 */
0x0600c007,	/* loop *6 ( */
0x8c081800,	/* ld *2, *0++ */
0x7c8c0000,	/* ldr *3, *0 */
0x7c800400,	/* ldr *0, *4 */
0x54906200,	/* subb r4, r2, r3 */
0x990c0000,	/* strnd r3 */
0x660c4401,	/* sellx r3, r4, r2 */
0x7ca00200,	/* ldr *0++, *2 */
/*		   ) */
0x0c000000,	/* ret */
/*selcxSub_invsel: */
0x528c8402,	/* addcx r3, r4, r4 << 16 */
0x0600c007,	/* loop *6 ( */
0x8c081800,	/* ld *2, *0++ */
0x7c8c0000,	/* ldr *3, *0 */
0x7c800400,	/* ldr *0, *4 */
0x54906200,	/* subb r4, r2, r3 */
0x990c0000,	/* strnd r3 */
0x660c8201,	/* sellx r3, r2, r4 */
0x7ca00200,	/* ldr *0++, *2 */
/*		   ) */
0x0c000000,	/* ret */
/* } */
/* @0x28: function computeRR[41] { */
#define CF_computeRR_adr 40
0x4c7fff00,	/* xor r31, r31, r31 */
0x84004000,	/* ldi r0, [#0] */
0x95800000,	/* lddmp r0 */
0x4c0c6300,	/* xor r3, r3, r3 */
0x800cffff,	/* movi r3.0l, #65535 */
0x40040398,	/* and r1, r3, r0 >> 192 */
0x480c6000,	/* not r3, r3 */
0x400c0300,	/* and r3, r3, r0 */
0x500c2301,	/* add r3, r3, r1 << 8 */
0x94800300,	/* ldlc r3 */
0x80040005,	/* movi r1.0l, #5 */
0x81040003,	/* movi r1.2l, #3 */
0x81840002,	/* movi r1.3l, #2 */
0x82040004,	/* movi r1.4l, #4 */
0x97800100,	/* ldrfp r1 */
0x4c0c6300,	/* xor r3, r3, r3 */
0x0600c001,	/* loop *6 ( */
0x7ca00200,	/* ldr *0++, *2 */
/*		   ) */
0x560c1f00,	/* subx r3, r31, r0 */
0x0800000f,	/* call &selcxSub */
0x06000010,	/* loop *0 ( */
0x97800100,	/* ldrfp r1 */
0x560c6300,	/* subx r3, r3, r3 */
0x0600c003,	/* loop *6 ( */
0x7c8c0000,	/* ldr *3, *0 */
0x52884200,	/* addcx r2, r2, r2 */
0x7ca00300,	/* ldr *0++, *3 */
/*		   ) */
0x0800000f,	/* call &selcxSub */
0x97800100,	/* ldrfp r1 */
0x95800000,	/* lddmp r0 */
0x560c6300,	/* subx r3, r3, r3 */
0x0600c003,	/* loop *6 ( */
0x8c081800,	/* ld *2, *0++ */
0x7c8c0800,	/* ldr *3, *0++ */
0x5e804300,	/* cmpbx r3, r2 */
/*		   ) */
0x0800000f,	/* call &selcxSub */
0xfc000000,	/* nop */
/*		   ) */
0x97800100,	/* ldrfp r1 */
0x0600c001,	/* loop *6 ( */
0x90680800,	/* st *0++, *2++ */
/*		   ) */
0x0c000000,	/* ret */
/* } */
/* @0x51: function dmXd0[9] { */
#define CF_dmXd0_adr 81
0x586f3e00,	/* mul128 r27, r30l, r25l */
0x59eb3e00,	/* mul128 r26, r30u, r25u */
0x58df3e00,	/* mul128 r23, r30u, r25l */
0x506efb10,	/* add r27, r27, r23 << 128 */
0x50eafa90,	/* addc r26, r26, r23 >> 128 */
0x595f3e00,	/* mul128 r23, r30l, r25u */
0x506efb10,	/* add r27, r27, r23 << 128 */
0x50eafa90,	/* addc r26, r26, r23 >> 128 */
0x0c000000,	/* ret */
/* } */
/* @0x5a: function dmXa[9] { */
#define CF_dmXa_adr 90
0x586c5e00,	/* mul128 r27, r30l, r2l */
0x59e85e00,	/* mul128 r26, r30u, r2u */
0x58dc5e00,	/* mul128 r23, r30u, r2l */
0x506efb10,	/* add r27, r27, r23 << 128 */
0x50eafa90,	/* addc r26, r26, r23 >> 128 */
0x595c5e00,	/* mul128 r23, r30l, r2u */
0x506efb10,	/* add r27, r27, r23 << 128 */
0x50eafa90,	/* addc r26, r26, r23 >> 128 */
0x0c000000,	/* ret */
/* } */
/* @0x63: function mma_sub_cx[23] { */
#define CF_mma_sub_cx_adr 99
0x99700000,	/* strnd r28 */
0x5073fc00,	/* add r28, r28, r31 */
0x10001070,	/* bl mma_invsel */
0x52f39c02,	/* addcx r28, r28, r28 << 16 */
0x0600c007,	/* loop *6 ( */
0x8c141800,	/* ld *5, *0++ */
0x7c900000,	/* ldr *4, *0 */
0x54f71e00,	/* subb r29, r30, r24 */
0x99600000,	/* strnd r24 */
0x7c800500,	/* ldr *0, *5 */
0x6663dd01,	/* sellx r24, r29, r30 */
0x7ca00500,	/* ldr *0++, *5 */
/*		   ) */
0x0c000000,	/* ret */
/*mma_invsel: */
0x52f39c02,	/* addcx r28, r28, r28 << 16 */
0x0600c007,	/* loop *6 ( */
0x8c141800,	/* ld *5, *0++ */
0x7c900000,	/* ldr *4, *0 */
0x54f71e00,	/* subb r29, r30, r24 */
0x99600000,	/* strnd r24 */
0x7c800500,	/* ldr *0, *5 */
0x6663be01,	/* sellx r24, r30, r29 */
0x7ca00500,	/* ldr *0++, *5 */
/*		   ) */
0x0c000000,	/* ret */
/* } */
/* @0x7a: function mma[39] { */
#define CF_mma_adr 122
0x8204001e,	/* movi r1.4l, #30 */
0x82840018,	/* movi r1.5l, #24 */
0x97800100,	/* ldrfp r1 */
0x8c101b00,	/* ld *4, *3++ */
0x0800005a,	/* call &dmXa */
0x7c940800,	/* ldr *5, *0++ */
0x507b1b00,	/* add r30, r27, r24 */
0x50f7fa00,	/* addc r29, r26, r31 */
0x7c640300,	/* mov r25, r3 */
0x08000051,	/* call &dmXd0 */
0x7c641b00,	/* mov r25, r27 */
0x7c701a00,	/* mov r28, r26 */
0x7c601e00,	/* mov r24, r30 */
0x8c101800,	/* ld *4, *0++ */
0x08000051,	/* call &dmXd0 */
0x506f1b00,	/* add r27, r27, r24 */
0x50f3fa00,	/* addc r28, r26, r31 */
0x0600e00e,	/* loop *7 ( */
0x8c101b00,	/* ld *4, *3++ */
0x0800005a,	/* call &dmXa */
0x7c940800,	/* ldr *5, *0++ */
0x506f1b00,	/* add r27, r27, r24 */
0x50ebfa00,	/* addc r26, r26, r31 */
0x5063bb00,	/* add r24, r27, r29 */
0x50f7fa00,	/* addc r29, r26, r31 */
0x8c101800,	/* ld *4, *0++ */
0x08000051,	/* call &dmXd0 */
0x506f1b00,	/* add r27, r27, r24 */
0x50ebfa00,	/* addc r26, r26, r31 */
0x52639b00,	/* addx r24, r27, r28 */
0x7ca80500,	/* ldr *2++, *5 */
0x52f3fa00,	/* addcx r28, r26, r31 */
/*		   ) */
0x52e39d00,	/* addcx r24, r29, r28 */
0x7ca80500,	/* ldr *2++, *5 */
0x95800000,	/* lddmp r0 */
0x97800100,	/* ldrfp r1 */
0x08000063,	/* call &mma_sub_cx */
0xfc000000,	/* nop */
0x0c000000,	/* ret */
/* } */
/* @0xa1: function setupPtrs[11] { */
#define CF_setupPtrs_adr 161
0x847c4000,	/* ldi r31, [#0] */
0x4c7fff00,	/* xor r31, r31, r31 */
0x95800000,	/* lddmp r0 */
0x94800000,	/* ldlc r0 */
0x7c041f00,	/* mov r1, r31 */
0x80040004,	/* movi r1.0l, #4 */
0x80840003,	/* movi r1.1l, #3 */
0x81040004,	/* movi r1.2l, #4 */
0x81840002,	/* movi r1.3l, #2 */
0x97800100,	/* ldrfp r1 */
0x0c000000,	/* ret */
/* } */
/* @0xac: function mulx[19] { */
#define CF_mulx_adr 172
0x84004000,	/* ldi r0, [#0] */
0x080000a1,	/* call &setupPtrs */
0x8c041100,	/* ld *1, *1 */
0x7c081f00,	/* mov r2, r31 */
0x0600c001,	/* loop *6 ( */
0x7ca80300,	/* ldr *2++, *3 */
/*		   ) */
0x97800100,	/* ldrfp r1 */
0x0600c004,	/* loop *6 ( */
0x8c0c1c00,	/* ld *3, *4++ */
0x95000000,	/* stdmp r0 */
0x0800007a,	/* call &mma */
0x95800000,	/* lddmp r0 */
/*		   ) */
0x97800100,	/* ldrfp r1 */
0x95800000,	/* lddmp r0 */
0x0600c001,	/* loop *6 ( */
0x90740800,	/* st *0++, *5++ */
/*		   ) */
0x97800100,	/* ldrfp r1 */
0x95800000,	/* lddmp r0 */
0x0c000000,	/* ret */
/* } */
/* @0xbf: function mm1_sub_cx[22] { */
#define CF_mm1_sub_cx_adr 191
0x990c0000,	/* strnd r3 */
0x500fe300,	/* add r3, r3, r31 */
0x100010cc,	/* bl mm1_invsel */
0x528c6302,	/* addcx r3, r3, r3 << 16 */
0x0600c006,	/* loop *6 ( */
0x8c041800,	/* ld *1, *0++ */
0x7c8c0800,	/* ldr *3, *0++ */
0x548c6200,	/* subb r3, r2, r3 */
0x66084301,	/* sellx r2, r3, r2 */
0x90740300,	/* st *3, *5++ */
0xfc000000,	/* nop */
/*		   ) */
0x0c000000,	/* ret */
0xfc000000,	/* nop */
/*mm1_invsel: */
0x528c6302,	/* addcx r3, r3, r3 << 16 */
0x0600c006,	/* loop *6 ( */
0x8c041800,	/* ld *1, *0++ */
0x7c8c0800,	/* ldr *3, *0++ */
0x548c6200,	/* subb r3, r2, r3 */
0x66086201,	/* sellx r2, r2, r3 */
0x90740300,	/* st *3, *5++ */
0xfc000000,	/* nop */
/*		   ) */
0x0c000000,	/* ret */
/* } */
/* @0xd5: function mul1_exp[23] { */
#define CF_mul1_exp_adr 213
0x8c041100,	/* ld *1, *1 */
0x7c081f00,	/* mov r2, r31 */
0x0600c001,	/* loop *6 ( */
0x7ca80300,	/* ldr *2++, *3 */
/*		   ) */
0x97800100,	/* ldrfp r1 */
0x80080001,	/* movi r2.0l, #1 */
0x0600c003,	/* loop *6 ( */
0x95800000,	/* lddmp r0 */
0x0800007a,	/* call &mma */
0x7c081f00,	/* mov r2, r31 */
/*		   ) */
0x97800100,	/* ldrfp r1 */
0x95800000,	/* lddmp r0 */
0x56084200,	/* subx r2, r2, r2 */
0x0600c003,	/* loop *6 ( */
0x8c041800,	/* ld *1, *0++ */
0x7c8c0800,	/* ldr *3, *0++ */
0x5e804300,	/* cmpbx r3, r2 */
/*		   ) */
0x97800100,	/* ldrfp r1 */
0x95800000,	/* lddmp r0 */
0x080000bf,	/* call &mm1_sub_cx */
0x97800100,	/* ldrfp r1 */
0x95800000,	/* lddmp r0 */
0x0c000000,	/* ret */
/* } */
/* @0xec: function mul1[4] { */
#define CF_mul1_adr 236
0x84004000,	/* ldi r0, [#0] */
0x080000a1,	/* call &setupPtrs */
0x080000d5,	/* call &mul1_exp */
0x0c000000,	/* ret */
/* } */
/* @0xf0: function sqrx_exp[19] { */
#define CF_sqrx_exp_adr 240
0x84004020,	/* ldi r0, [#1] */
0x95800000,	/* lddmp r0 */
0x8c041100,	/* ld *1, *1 */
0x7c081f00,	/* mov r2, r31 */
0x0600c001,	/* loop *6 ( */
0x7ca80300,	/* ldr *2++, *3 */
/*		   ) */
0x97800100,	/* ldrfp r1 */
0x0600c004,	/* loop *6 ( */
0x8c0c1c00,	/* ld *3, *4++ */
0x95000000,	/* stdmp r0 */
0x0800007a,	/* call &mma */
0x95800000,	/* lddmp r0 */
/*		   ) */
0x97800100,	/* ldrfp r1 */
0x95800000,	/* lddmp r0 */
0x0600c001,	/* loop *6 ( */
0x90740800,	/* st *0++, *5++ */
/*		   ) */
0x97800100,	/* ldrfp r1 */
0x95800000,	/* lddmp r0 */
0x0c000000,	/* ret */
/* } */
/* @0x103: function mulx_exp[14] { */
#define CF_mulx_exp_adr 259
0x84004040,	/* ldi r0, [#2] */
0x95800000,	/* lddmp r0 */
0x8c041100,	/* ld *1, *1 */
0x7c081f00,	/* mov r2, r31 */
0x0600c001,	/* loop *6 ( */
0x7ca80300,	/* ldr *2++, *3 */
/*		   ) */
0x97800100,	/* ldrfp r1 */
0x0600c004,	/* loop *6 ( */
0x8c0c1c00,	/* ld *3, *4++ */
0x95000000,	/* stdmp r0 */
0x0800007a,	/* call &mma */
0x95800000,	/* lddmp r0 */
/*		   ) */
0x97800100,	/* ldrfp r1 */
0x0c000000,	/* ret */
/* } */
/* @0x111: function selOutOrC[30] { */
#define CF_selOutOrC_adr 273
0x990c0000,	/* strnd r3 */
0x440c6300,	/* or r3, r3, r3 */
0x10001122,	/* bl selOutOrC_invsel */
0x508c6302,	/* addc r3, r3, r3 << 16 */
0x0600c00a,	/* loop *6 ( */
0x990c0000,	/* strnd r3 */
0x99080000,	/* strnd r2 */
0x8c041500,	/* ld *1, *5 */
0x90540300,	/* st *3, *5 */
0x7c8c0800,	/* ldr *3, *0++ */
0x99000000,	/* strnd r0 */
0x7c000200,	/* mov r0, r2 */
0x99080000,	/* strnd r2 */
0x64086001,	/* sell r2, r0, r3 */
0x90740300,	/* st *3, *5++ */
/*		   ) */
0x0c000000,	/* ret */
0xfc000000,	/* nop */
/*selOutOrC_invsel: */
0x508c6302,	/* addc r3, r3, r3 << 16 */
0x0600c00a,	/* loop *6 ( */
0x990c0000,	/* strnd r3 */
0x99080000,	/* strnd r2 */
0x8c041500,	/* ld *1, *5 */
0x90540300,	/* st *3, *5 */
0x7c8c0800,	/* ldr *3, *0++ */
0x99000000,	/* strnd r0 */
0x7c000200,	/* mov r0, r2 */
0x99080000,	/* strnd r2 */
0x64080301,	/* sell r2, r3, r0 */
0x90740300,	/* st *3, *5++ */
/*		   ) */
0x0c000000,	/* ret */
/* } */
/* @0x12f: function modexp[35] { */
#define CF_modexp_adr 303
0x080000ac,	/* call &mulx */
0x84004060,	/* ldi r0, [#3] */
0x95800000,	/* lddmp r0 */
0x54084200,	/* sub r2, r2, r2 */
0x0600c004,	/* loop *6 ( */
0xfc000000,	/* nop */
0x8c0c1800,	/* ld *3, *0++ */
0x54885f00,	/* subb r2, r31, r2 */
0x90740300,	/* st *3, *5++ */
/*		   ) */
0xfc000000,	/* nop */
0x7c081f00,	/* mov r2, r31 */
0x8008ffff,	/* movi r2.0l, #65535 */
0x400c0298,	/* and r3, r2, r0 >> 192 */
0x48084000,	/* not r2, r2 */
0x40080200,	/* and r2, r2, r0 */
0x50086201,	/* add r2, r2, r3 << 8 */
0x94800200,	/* ldlc r2 */
0x0600000d,	/* loop *0 ( */
0x080000f0,	/* call &sqrx_exp */
0x08000103,	/* call &mulx_exp */
0x84004060,	/* ldi r0, [#3] */
0x95800000,	/* lddmp r0 */
0x99080000,	/* strnd r2 */
0x50084200,	/* add r2, r2, r2 */
0x0600c004,	/* loop *6 ( */
0x99080000,	/* strnd r2 */
0x8c0c1400,	/* ld *3, *4 */
0x50884200,	/* addc r2, r2, r2 */
0x90700300,	/* st *3, *4++ */
/*		   ) */
0x08000111,	/* call &selOutOrC */
0xfc000000,	/* nop */
/*		   ) */
0x84004060,	/* ldi r0, [#3] */
0x95800000,	/* lddmp r0 */
0x080000d5,	/* call &mul1_exp */
0x0c000000,	/* ret */
/* } */
/* @0x152: function modexp_blinded[76] { */
#define CF_modexp_blinded_adr 338
0x080000ac,	/* call &mulx */
0x84004060,	/* ldi r0, [#3] */
0x95800000,	/* lddmp r0 */
0x54084200,	/* sub r2, r2, r2 */
0x0600c004,	/* loop *6 ( */
0xfc000000,	/* nop */
0x8c0c1800,	/* ld *3, *0++ */
0x54885f00,	/* subb r2, r31, r2 */
0x90740300,	/* st *3, *5++ */
/*		   ) */
0xfc000000,	/* nop */
0x8c0c1900,	/* ld *3, *1++ */
0x8c0c1100,	/* ld *3, *1 */
0x521c5f90,	/* addx r7, r31, r2 >> 128 */
0x590c4200,	/* mul128 r3, r2l, r2u */
0x7c181f00,	/* mov r6, r31 */
0x0600c011,	/* loop *6 ( */
0x99080000,	/* strnd r2 */
0x8c0c1400,	/* ld *3, *4 */
0x58106200,	/* mul128 r4, r2l, r3l */
0x59946200,	/* mul128 r5, r2u, r3u */
0x58806200,	/* mul128 r0, r2u, r3l */
0x50100410,	/* add r4, r4, r0 << 128 */
0x50940590,	/* addc r5, r5, r0 >> 128 */
0x59006200,	/* mul128 r0, r2l, r3u */
0x50100410,	/* add r4, r4, r0 << 128 */
0x50940590,	/* addc r5, r5, r0 >> 128 */
0x5010c400,	/* add r4, r4, r6 */
0x5097e500,	/* addc r5, r5, r31 */
0x50088200,	/* add r2, r2, r4 */
0x509be500,	/* addc r6, r5, r31 */
0x5688e200,	/* subbx r2, r2, r7 */
0x90700300,	/* st *3, *4++ */
0x541ce700,	/* sub r7, r7, r7 */
/*		   ) */
0x7c080600,	/* mov r2, r6 */
0x5688e200,	/* subbx r2, r2, r7 */
0x90500300,	/* st *3, *4 */
0xfc000000,	/* nop */
0x84004060,	/* ldi r0, [#3] */
0x7c081f00,	/* mov r2, r31 */
0x8008ffff,	/* movi r2.0l, #65535 */
0x400c0298,	/* and r3, r2, r0 >> 192 */
0x48084000,	/* not r2, r2 */
0x40080200,	/* and r2, r2, r0 */
0x510c0301,	/* addi r3, r3, #1 */
0x50086201,	/* add r2, r2, r3 << 8 */
0x94800200,	/* ldlc r2 */
0x06000019,	/* loop *0 ( */
0x080000f0,	/* call &sqrx_exp */
0x08000103,	/* call &mulx_exp */
0x84004060,	/* ldi r0, [#3] */
0x95800000,	/* lddmp r0 */
0x99080000,	/* strnd r2 */
0x54084200,	/* sub r2, r2, r2 */
0x0600c004,	/* loop *6 ( */
0x99080000,	/* strnd r2 */
0x8c0c1400,	/* ld *3, *4 */
0x50884200,	/* addc r2, r2, r2 */
0x90700300,	/* st *3, *4++ */
/*		   ) */
0x99080000,	/* strnd r2 */
0x8c0c1400,	/* ld *3, *4 */
0x50884200,	/* addc r2, r2, r2 */
0x90700300,	/* st *3, *4++ */
0x0600c008,	/* loop *6 ( */
0x99080000,	/* strnd r2 */
0x8c041500,	/* ld *1, *5 */
0x90540300,	/* st *3, *5 */
0x7c8c0800,	/* ldr *3, *0++ */
0x7c000200,	/* mov r0, r2 */
0x99080000,	/* strnd r2 */
0x64086008,	/* selc r2, r0, r3 */
0x90740300,	/* st *3, *5++ */
/*		   ) */
0xfc000000,	/* nop */
/*		   ) */
0x84004060,	/* ldi r0, [#3] */
0x95800000,	/* lddmp r0 */
0x080000d5,	/* call &mul1_exp */
0x0c000000,	/* ret */
/* } */
/* @0x19e: function modload[12] { */
#define CF_modload_adr 414
0x4c7fff00,	/* xor r31, r31, r31 */
0x84004000,	/* ldi r0, [#0] */
0x95800000,	/* lddmp r0 */
0x94800000,	/* ldlc r0 */
0x8000001c,	/* movi r0.0l, #28 */
0x8080001d,	/* movi r0.1l, #29 */
0x97800000,	/* ldrfp r0 */
0x8c001000,	/* ld *0, *0 */
0x08000001,	/* call &d0inv */
0x90440100,	/* st *1, *1 */
0x08000028,	/* call &computeRR */
0x0c000000,	/* ret */
/* } */
#ifdef CONFIG_DCRYPTO_RSA_SPEEDUP
/* @0x1aa: function selA0orC4[16] { */
#define CF_selA0orC4_adr 426
0x99000000,	/* strnd r0 */
0x44000000,	/* or r0, r0, r0 */
0x100011b4,	/* bl selA0orC4_invsel */
0x50840002,	/* addc r1, r0, r0 << 16 */
0x6458da01,	/* sell r22, r26, r6 */
0x645cfb01,	/* sell r23, r27, r7 */
0x64611c01,	/* sell r24, r28, r8 */
0x64653d01,	/* sell r25, r29, r9 */
0x0c000000,	/* ret */
0xfc000000,	/* nop */
/*selA0orC4_invsel: */
0x50840002,	/* addc r1, r0, r0 << 16 */
0x645b4601,	/* sell r22, r6, r26 */
0x645f6701,	/* sell r23, r7, r27 */
0x64638801,	/* sell r24, r8, r28 */
0x6467a901,	/* sell r25, r9, r29 */
0x0c000000,	/* ret */
/* } */
/* @0x1ba: function mul4[169] { */
#define CF_mul4_adr 442
0x58594600,	/* mul128 r22, r6l, r10l */
0x59dd4600,	/* mul128 r23, r6u, r10u */
0x58894600,	/* mul128 r2, r6u, r10l */
0x50585610,	/* add r22, r22, r2 << 128 */
0x50dc5790,	/* addc r23, r23, r2 >> 128 */
0x59094600,	/* mul128 r2, r6l, r10u */
0x50585610,	/* add r22, r22, r2 << 128 */
0x50dc5790,	/* addc r23, r23, r2 >> 128 */
0x58616700,	/* mul128 r24, r7l, r11l */
0x59e56700,	/* mul128 r25, r7u, r11u */
0x58896700,	/* mul128 r2, r7u, r11l */
0x50605810,	/* add r24, r24, r2 << 128 */
0x50e45990,	/* addc r25, r25, r2 >> 128 */
0x59096700,	/* mul128 r2, r7l, r11u */
0x50605810,	/* add r24, r24, r2 << 128 */
0x50e45990,	/* addc r25, r25, r2 >> 128 */
0x58698800,	/* mul128 r26, r8l, r12l */
0x59ed8800,	/* mul128 r27, r8u, r12u */
0x58898800,	/* mul128 r2, r8u, r12l */
0x50685a10,	/* add r26, r26, r2 << 128 */
0x50ec5b90,	/* addc r27, r27, r2 >> 128 */
0x59098800,	/* mul128 r2, r8l, r12u */
0x50685a10,	/* add r26, r26, r2 << 128 */
0x50ec5b90,	/* addc r27, r27, r2 >> 128 */
0x5871a900,	/* mul128 r28, r9l, r13l */
0x59f5a900,	/* mul128 r29, r9u, r13u */
0x5889a900,	/* mul128 r2, r9u, r13l */
0x50705c10,	/* add r28, r28, r2 << 128 */
0x50f45d90,	/* addc r29, r29, r2 >> 128 */
0x5909a900,	/* mul128 r2, r9l, r13u */
0x50705c10,	/* add r28, r28, r2 << 128 */
0x50f45d90,	/* addc r29, r29, r2 >> 128 */
0x58016600,	/* mul128 r0, r6l, r11l */
0x59856600,	/* mul128 r1, r6u, r11u */
0x58896600,	/* mul128 r2, r6u, r11l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x59096600,	/* mul128 r2, r6l, r11u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x505c1700,	/* add r23, r23, r0 */
0x50e03800,	/* addc r24, r24, r1 */
0x508fff00,	/* addc r3, r31, r31 */
0x58014700,	/* mul128 r0, r7l, r10l */
0x59854700,	/* mul128 r1, r7u, r10u */
0x58894700,	/* mul128 r2, r7u, r10l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x59094700,	/* mul128 r2, r7l, r10u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x505c1700,	/* add r23, r23, r0 */
0x50e03800,	/* addc r24, r24, r1 */
0x50e47900,	/* addc r25, r25, r3 */
0x508fff00,	/* addc r3, r31, r31 */
0x58018600,	/* mul128 r0, r6l, r12l */
0x59858600,	/* mul128 r1, r6u, r12u */
0x58898600,	/* mul128 r2, r6u, r12l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x59098600,	/* mul128 r2, r6l, r12u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x50601800,	/* add r24, r24, r0 */
0x50e43900,	/* addc r25, r25, r1 */
0x508fe300,	/* addc r3, r3, r31 */
0x58014800,	/* mul128 r0, r8l, r10l */
0x59854800,	/* mul128 r1, r8u, r10u */
0x58894800,	/* mul128 r2, r8u, r10l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x59094800,	/* mul128 r2, r8l, r10u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x50601800,	/* add r24, r24, r0 */
0x50e43900,	/* addc r25, r25, r1 */
0x50e87a00,	/* addc r26, r26, r3 */
0x508fff00,	/* addc r3, r31, r31 */
0x5801a600,	/* mul128 r0, r6l, r13l */
0x5985a600,	/* mul128 r1, r6u, r13u */
0x5889a600,	/* mul128 r2, r6u, r13l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x5909a600,	/* mul128 r2, r6l, r13u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x50641900,	/* add r25, r25, r0 */
0x50e83a00,	/* addc r26, r26, r1 */
0x508fe300,	/* addc r3, r3, r31 */
0x58018700,	/* mul128 r0, r7l, r12l */
0x59858700,	/* mul128 r1, r7u, r12u */
0x58898700,	/* mul128 r2, r7u, r12l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x59098700,	/* mul128 r2, r7l, r12u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x50641900,	/* add r25, r25, r0 */
0x50e83a00,	/* addc r26, r26, r1 */
0x508fe300,	/* addc r3, r3, r31 */
0x58014900,	/* mul128 r0, r9l, r10l */
0x59854900,	/* mul128 r1, r9u, r10u */
0x58894900,	/* mul128 r2, r9u, r10l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x59094900,	/* mul128 r2, r9l, r10u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x50641900,	/* add r25, r25, r0 */
0x50e83a00,	/* addc r26, r26, r1 */
0x508fe300,	/* addc r3, r3, r31 */
0x58016800,	/* mul128 r0, r8l, r11l */
0x59856800,	/* mul128 r1, r8u, r11u */
0x58896800,	/* mul128 r2, r8u, r11l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x59096800,	/* mul128 r2, r8l, r11u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x50641900,	/* add r25, r25, r0 */
0x50e83a00,	/* addc r26, r26, r1 */
0x50ec7b00,	/* addc r27, r27, r3 */
0x508fff00,	/* addc r3, r31, r31 */
0x5801a700,	/* mul128 r0, r7l, r13l */
0x5985a700,	/* mul128 r1, r7u, r13u */
0x5889a700,	/* mul128 r2, r7u, r13l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x5909a700,	/* mul128 r2, r7l, r13u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x50681a00,	/* add r26, r26, r0 */
0x50ec3b00,	/* addc r27, r27, r1 */
0x508fe300,	/* addc r3, r3, r31 */
0x58016900,	/* mul128 r0, r9l, r11l */
0x59856900,	/* mul128 r1, r9u, r11u */
0x58896900,	/* mul128 r2, r9u, r11l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x59096900,	/* mul128 r2, r9l, r11u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x50681a00,	/* add r26, r26, r0 */
0x50ec3b00,	/* addc r27, r27, r1 */
0x50f07c00,	/* addc r28, r28, r3 */
0x50f7fd00,	/* addc r29, r29, r31 */
0x5801a800,	/* mul128 r0, r8l, r13l */
0x5985a800,	/* mul128 r1, r8u, r13u */
0x5889a800,	/* mul128 r2, r8u, r13l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x5909a800,	/* mul128 r2, r8l, r13u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x506c1b00,	/* add r27, r27, r0 */
0x50f03c00,	/* addc r28, r28, r1 */
0x50f7fd00,	/* addc r29, r29, r31 */
0x58018900,	/* mul128 r0, r9l, r12l */
0x59858900,	/* mul128 r1, r9u, r12u */
0x58898900,	/* mul128 r2, r9u, r12l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x59098900,	/* mul128 r2, r9l, r12u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x506c1b00,	/* add r27, r27, r0 */
0x50f03c00,	/* addc r28, r28, r1 */
0x50f7fd00,	/* addc r29, r29, r31 */
0x0c000000,	/* ret */
/* } */
/* @0x263: function sqr4[117] { */
#define CF_sqr4_adr 611
0x5858c600,	/* mul128 r22, r6l, r6l */
0x59dcc600,	/* mul128 r23, r6u, r6u */
0x5888c600,	/* mul128 r2, r6u, r6l */
0x50585610,	/* add r22, r22, r2 << 128 */
0x50dc5790,	/* addc r23, r23, r2 >> 128 */
0x50585610,	/* add r22, r22, r2 << 128 */
0x50dc5790,	/* addc r23, r23, r2 >> 128 */
0x5860e700,	/* mul128 r24, r7l, r7l */
0x59e4e700,	/* mul128 r25, r7u, r7u */
0x5888e700,	/* mul128 r2, r7u, r7l */
0x50605810,	/* add r24, r24, r2 << 128 */
0x50e45990,	/* addc r25, r25, r2 >> 128 */
0x50605810,	/* add r24, r24, r2 << 128 */
0x50e45990,	/* addc r25, r25, r2 >> 128 */
0x58690800,	/* mul128 r26, r8l, r8l */
0x59ed0800,	/* mul128 r27, r8u, r8u */
0x58890800,	/* mul128 r2, r8u, r8l */
0x50685a10,	/* add r26, r26, r2 << 128 */
0x50ec5b90,	/* addc r27, r27, r2 >> 128 */
0x50685a10,	/* add r26, r26, r2 << 128 */
0x50ec5b90,	/* addc r27, r27, r2 >> 128 */
0x58712900,	/* mul128 r28, r9l, r9l */
0x59f52900,	/* mul128 r29, r9u, r9u */
0x58892900,	/* mul128 r2, r9u, r9l */
0x50705c10,	/* add r28, r28, r2 << 128 */
0x50f45d90,	/* addc r29, r29, r2 >> 128 */
0x50705c10,	/* add r28, r28, r2 << 128 */
0x50f45d90,	/* addc r29, r29, r2 >> 128 */
0x5800e600,	/* mul128 r0, r6l, r7l */
0x5984e600,	/* mul128 r1, r6u, r7u */
0x5888e600,	/* mul128 r2, r6u, r7l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x5908e600,	/* mul128 r2, r6l, r7u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x505c1700,	/* add r23, r23, r0 */
0x50e03800,	/* addc r24, r24, r1 */
0x508fff00,	/* addc r3, r31, r31 */
0x505c1700,	/* add r23, r23, r0 */
0x50e03800,	/* addc r24, r24, r1 */
0x50e47900,	/* addc r25, r25, r3 */
0x508fff00,	/* addc r3, r31, r31 */
0x58010600,	/* mul128 r0, r6l, r8l */
0x59850600,	/* mul128 r1, r6u, r8u */
0x58890600,	/* mul128 r2, r6u, r8l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x59090600,	/* mul128 r2, r6l, r8u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x50601800,	/* add r24, r24, r0 */
0x50e43900,	/* addc r25, r25, r1 */
0x508fe300,	/* addc r3, r3, r31 */
0x50601800,	/* add r24, r24, r0 */
0x50e43900,	/* addc r25, r25, r1 */
0x50e87a00,	/* addc r26, r26, r3 */
0x508fff00,	/* addc r3, r31, r31 */
0x58012600,	/* mul128 r0, r6l, r9l */
0x59852600,	/* mul128 r1, r6u, r9u */
0x58892600,	/* mul128 r2, r6u, r9l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x59092600,	/* mul128 r2, r6l, r9u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x50641900,	/* add r25, r25, r0 */
0x50e83a00,	/* addc r26, r26, r1 */
0x508fe300,	/* addc r3, r3, r31 */
0x50641900,	/* add r25, r25, r0 */
0x50e83a00,	/* addc r26, r26, r1 */
0x508fe300,	/* addc r3, r3, r31 */
0x58010700,	/* mul128 r0, r7l, r8l */
0x59850700,	/* mul128 r1, r7u, r8u */
0x58890700,	/* mul128 r2, r7u, r8l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x59090700,	/* mul128 r2, r7l, r8u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x50641900,	/* add r25, r25, r0 */
0x50e83a00,	/* addc r26, r26, r1 */
0x508fe300,	/* addc r3, r3, r31 */
0x50641900,	/* add r25, r25, r0 */
0x50e83a00,	/* addc r26, r26, r1 */
0x50ec7b00,	/* addc r27, r27, r3 */
0x508fff00,	/* addc r3, r31, r31 */
0x58012700,	/* mul128 r0, r7l, r9l */
0x59852700,	/* mul128 r1, r7u, r9u */
0x58892700,	/* mul128 r2, r7u, r9l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x59092700,	/* mul128 r2, r7l, r9u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x50681a00,	/* add r26, r26, r0 */
0x50ec3b00,	/* addc r27, r27, r1 */
0x508fe300,	/* addc r3, r3, r31 */
0x50681a00,	/* add r26, r26, r0 */
0x50ec3b00,	/* addc r27, r27, r1 */
0x50f07c00,	/* addc r28, r28, r3 */
0x50f7fd00,	/* addc r29, r29, r31 */
0x58012800,	/* mul128 r0, r8l, r9l */
0x59852800,	/* mul128 r1, r8u, r9u */
0x58892800,	/* mul128 r2, r8u, r9l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x59092800,	/* mul128 r2, r8l, r9u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x506c1b00,	/* add r27, r27, r0 */
0x50f03c00,	/* addc r28, r28, r1 */
0x50f7fd00,	/* addc r29, r29, r31 */
0x506c1b00,	/* add r27, r27, r0 */
0x50f03c00,	/* addc r28, r28, r1 */
0x50f7fd00,	/* addc r29, r29, r31 */
0x0c000000,	/* ret */
/* } */
/* @0x2d8: function dod0[15] { */
#define CF_dod0_adr 728
0x8c0c1100,	/* ld *3, *1 */
0x58140100,	/* mul128 r5, r1l, r0l */
0x58880100,	/* mul128 r2, r1u, r0l */
0x50144510,	/* add r5, r5, r2 << 128 */
0x59080100,	/* mul128 r2, r1l, r0u */
0x50144510,	/* add r5, r5, r2 << 128 */
0x5801c500,	/* mul128 r0, r5l, r14l */
0x5985c500,	/* mul128 r1, r5u, r14u */
0x5889c500,	/* mul128 r2, r5u, r14l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x5909c500,	/* mul128 r2, r5l, r14u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x0c000000,	/* ret */
/* } */
/* @0x2e7: function dod1[9] { */
#define CF_dod1_adr 743
0x5801e500,	/* mul128 r0, r5l, r15l */
0x5985e500,	/* mul128 r1, r5u, r15u */
0x5889e500,	/* mul128 r2, r5u, r15l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x5909e500,	/* mul128 r2, r5l, r15u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x0c000000,	/* ret */
/* } */
/* @0x2f0: function dod2[9] { */
#define CF_dod2_adr 752
0x58020500,	/* mul128 r0, r5l, r16l */
0x59860500,	/* mul128 r1, r5u, r16u */
0x588a0500,	/* mul128 r2, r5u, r16l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x590a0500,	/* mul128 r2, r5l, r16u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x0c000000,	/* ret */
/* } */
/* @0x2f9: function dod3[9] { */
#define CF_dod3_adr 761
0x58022500,	/* mul128 r0, r5l, r17l */
0x59862500,	/* mul128 r1, r5u, r17u */
0x588a2500,	/* mul128 r2, r5u, r17l */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x590a2500,	/* mul128 r2, r5l, r17u */
0x50004010,	/* add r0, r0, r2 << 128 */
0x50844190,	/* addc r1, r1, r2 >> 128 */
0x0c000000,	/* ret */
/* } */
/* @0x302: function redc4[97] { */
#define CF_redc4_adr 770
0x7c001600,	/* mov r0, r22 */
0x080002d8,	/* call &dod0 */
0x50581600,	/* add r22, r22, r0 */
0x50dc3700,	/* addc r23, r23, r1 */
0x50e3f800,	/* addc r24, r24, r31 */
0x5093ff00,	/* addc r4, r31, r31 */
0x080002e7,	/* call &dod1 */
0x505c1700,	/* add r23, r23, r0 */
0x50e03800,	/* addc r24, r24, r1 */
0x50e49900,	/* addc r25, r25, r4 */
0x5093ff00,	/* addc r4, r31, r31 */
0x080002f0,	/* call &dod2 */
0x50601800,	/* add r24, r24, r0 */
0x50e43900,	/* addc r25, r25, r1 */
0x50e89a00,	/* addc r26, r26, r4 */
0x5093ff00,	/* addc r4, r31, r31 */
0x080002f9,	/* call &dod3 */
0x50641900,	/* add r25, r25, r0 */
0x50e83a00,	/* addc r26, r26, r1 */
0x50ec9b00,	/* addc r27, r27, r4 */
0x508fff00,	/* addc r3, r31, r31 */
0x7c001700,	/* mov r0, r23 */
0x080002d8,	/* call &dod0 */
0x505c1700,	/* add r23, r23, r0 */
0x50e03800,	/* addc r24, r24, r1 */
0x50e7f900,	/* addc r25, r25, r31 */
0x5093ff00,	/* addc r4, r31, r31 */
0x080002e7,	/* call &dod1 */
0x50601800,	/* add r24, r24, r0 */
0x50e43900,	/* addc r25, r25, r1 */
0x50e89a00,	/* addc r26, r26, r4 */
0x5093ff00,	/* addc r4, r31, r31 */
0x080002f0,	/* call &dod2 */
0x50641900,	/* add r25, r25, r0 */
0x50e83a00,	/* addc r26, r26, r1 */
0x50ec9b00,	/* addc r27, r27, r4 */
0x508fff00,	/* addc r3, r31, r31 */
0x080002f9,	/* call &dod3 */
0x50681a00,	/* add r26, r26, r0 */
0x50ec3b00,	/* addc r27, r27, r1 */
0x50f07c00,	/* addc r28, r28, r3 */
0x508fff00,	/* addc r3, r31, r31 */
0x7c001800,	/* mov r0, r24 */
0x080002d8,	/* call &dod0 */
0x50601800,	/* add r24, r24, r0 */
0x50e43900,	/* addc r25, r25, r1 */
0x50ebfa00,	/* addc r26, r26, r31 */
0x5093ff00,	/* addc r4, r31, r31 */
0x080002e7,	/* call &dod1 */
0x50641900,	/* add r25, r25, r0 */
0x50e83a00,	/* addc r26, r26, r1 */
0x50ec9b00,	/* addc r27, r27, r4 */
0x5093ff00,	/* addc r4, r31, r31 */
0x080002f0,	/* call &dod2 */
0x50681a00,	/* add r26, r26, r0 */
0x50ec3b00,	/* addc r27, r27, r1 */
0x50f09c00,	/* addc r28, r28, r4 */
0x5093e300,	/* addc r4, r3, r31 */
0x080002f9,	/* call &dod3 */
0x506c1b00,	/* add r27, r27, r0 */
0x50f03c00,	/* addc r28, r28, r1 */
0x50f49d00,	/* addc r29, r29, r4 */
0x508fff00,	/* addc r3, r31, r31 */
0x7c001900,	/* mov r0, r25 */
0x080002d8,	/* call &dod0 */
0x50641900,	/* add r25, r25, r0 */
0x50d83a00,	/* addc r22, r26, r1 */
0x50dffb00,	/* addc r23, r27, r31 */
0x5093ff00,	/* addc r4, r31, r31 */
0x080002e7,	/* call &dod1 */
0x50581600,	/* add r22, r22, r0 */
0x50dc3700,	/* addc r23, r23, r1 */
0x50e09c00,	/* addc r24, r28, r4 */
0x5093ff00,	/* addc r4, r31, r31 */
0x080002f0,	/* call &dod2 */
0x505c1700,	/* add r23, r23, r0 */
0x50e03800,	/* addc r24, r24, r1 */
0x50e49d00,	/* addc r25, r29, r4 */
0x508fe300,	/* addc r3, r3, r31 */
0x080002f9,	/* call &dod3 */
0x50601800,	/* add r24, r24, r0 */
0x50e43900,	/* addc r25, r25, r1 */
0x508fe300,	/* addc r3, r3, r31 */
0x56007f00,	/* subx r0, r31, r3 */
0x99680000,	/* strnd r26 */
0x996c0000,	/* strnd r27 */
0x99700000,	/* strnd r28 */
0x99740000,	/* strnd r29 */
0x5409d600,	/* sub r2, r22, r14 */
0x54e9f700,	/* subb r26, r23, r15 */
0x54ee1800,	/* subb r27, r24, r16 */
0x54f23900,	/* subb r28, r25, r17 */
0x66773c08,	/* selcx r29, r28, r25 */
0x66731b08,	/* selcx r28, r27, r24 */
0x666efa08,	/* selcx r27, r26, r23 */
0x666ac208,	/* selcx r26, r2, r22 */
0x0c000000,	/* ret */
/* } */
/* @0x363: function modexp_1024[101] { */
#define CF_modexp_1024_adr 867
0x7c081f00,	/* mov r2, r31 */
0x80080006,	/* movi r2.0l, #6 */
0x8088000a,	/* movi r2.1l, #10 */
0x81880001,	/* movi r2.3l, #1 */
0x8208000e,	/* movi r2.4l, #14 */
0x82880016,	/* movi r2.5l, #22 */
0x83080012,	/* movi r2.6l, #18 */
0x97800200,	/* ldrfp r2 */
0x7c001f00,	/* mov r0, r31 */
0x8180ffff,	/* movi r0.3l, #65535 */
0x84044000,	/* ldi r1, [#0] */
0x40040100,	/* and r1, r1, r0 */
0x48000000,	/* not r0, r0 */
0x84084060,	/* ldi r2, [#3] */
0x40080200,	/* and r2, r2, r0 */
0x44082200,	/* or r2, r2, r1 */
0x95800200,	/* lddmp r2 */
0x05004004,	/* loop #4 ( */
0x8c201b00,	/* ld *0++, *3++ */
0x8c241a00,	/* ld *1++, *2++ */
0x8c301800,	/* ld *4++, *0++ */
0x8c381c00,	/* ld *6++, *4++ */
/*		   ) */
0x99780000,	/* strnd r30 */
0x507bde00,	/* add r30, r30, r30 */
0x080001ba,	/* call &mul4 */
0x08000302,	/* call &redc4 */
0x7c281a00,	/* mov r10, r26 */
0x7c2c1b00,	/* mov r11, r27 */
0x7c301c00,	/* mov r12, r28 */
0x7c341d00,	/* mov r13, r29 */
0x99180000,	/* strnd r6 */
0x991c0000,	/* strnd r7 */
0x99200000,	/* strnd r8 */
0x99240000,	/* strnd r9 */
0x05400033,	/* loop #1024 ( */
0x08000263,	/* call &sqr4 */
0x08000302,	/* call &redc4 */
0x99180000,	/* strnd r6 */
0x991c0000,	/* strnd r7 */
0x99200000,	/* strnd r8 */
0x99240000,	/* strnd r9 */
0x7c181a00,	/* mov r6, r26 */
0x7c1c1b00,	/* mov r7, r27 */
0x7c201c00,	/* mov r8, r28 */
0x7c241d00,	/* mov r9, r29 */
0x080001ba,	/* call &mul4 */
0x08000302,	/* call &redc4 */
0x99000000,	/* strnd r0 */
0x5002b500,	/* add r0, r21, r21 */
0x99000000,	/* strnd r0 */
0x50825200,	/* addc r0, r18, r18 */
0x99480000,	/* strnd r18 */
0x7c480000,	/* mov r18, r0 */
0x99000000,	/* strnd r0 */
0x50827300,	/* addc r0, r19, r19 */
0x994c0000,	/* strnd r19 */
0x7c4c0000,	/* mov r19, r0 */
0x99000000,	/* strnd r0 */
0x50829400,	/* addc r0, r20, r20 */
0x99500000,	/* strnd r20 */
0x7c500000,	/* mov r20, r0 */
0x99000000,	/* strnd r0 */
0x5082b500,	/* addc r0, r21, r21 */
0x99540000,	/* strnd r21 */
0x7c540000,	/* mov r21, r0 */
0x99580000,	/* strnd r22 */
0x995c0000,	/* strnd r23 */
0x99600000,	/* strnd r24 */
0x99640000,	/* strnd r25 */
0x080001aa,	/* call &selA0orC4 */
0x99180000,	/* strnd r6 */
0x991c0000,	/* strnd r7 */
0x99200000,	/* strnd r8 */
0x99240000,	/* strnd r9 */
0x99000000,	/* strnd r0 */
0x50000000,	/* add r0, r0, r0 */
0x4c001e00,	/* xor r0, r30, r0 */
0x99780000,	/* strnd r30 */
0x507bde00,	/* add r30, r30, r30 */
0x4c781e00,	/* xor r30, r30, r0 */
0x447a5e00,	/* or r30, r30, r18 */
0x4c03c000,	/* xor r0, r0, r30 */
0x641aca01,	/* sell r6, r10, r22 */
0x641eeb01,	/* sell r7, r11, r23 */
0x64230c01,	/* sell r8, r12, r24 */
0x64272d01,	/* sell r9, r13, r25 */
/*		   ) */
0x7c281f00,	/* mov r10, r31 */
0x80280001,	/* movi r10.0l, #1 */
0x7c2c1f00,	/* mov r11, r31 */
0x7c301f00,	/* mov r12, r31 */
0x7c341f00,	/* mov r13, r31 */
0x080001ba,	/* call &mul4 */
0x08000302,	/* call &redc4 */
0x5419da00,	/* sub r6, r26, r14 */
0x549dfb00,	/* subb r7, r27, r15 */
0x54a21c00,	/* subb r8, r28, r16 */
0x54a63d00,	/* subb r9, r29, r17 */
0x080001aa,	/* call &selA0orC4 */
0x05004001,	/* loop #4 ( */
0x90740d00,	/* st *5++, *5++ */
/*		   ) */
0x0c000000,	/* ret */
/* } */
#endif // CONFIG_DCRYPTO_RSA_SPEEDUP
};
/* clang-format on */
