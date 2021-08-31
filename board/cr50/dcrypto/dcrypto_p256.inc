/* AUTO-GENERATED @7059d2df78.  DO NOT MODIFY. */
/* clang-format off */
static const uint32_t IMEM_dcrypto_p256[] = {
/* @0x0: function tag[1] { */
#define CF_tag_adr 0
	0xf8000002, /* sigini #2 */
/* } */
/* @0x1: function alignment[3] { */
#define CF_alignment_adr 1
	0xfc7c0000, /* nop */
	0xfc7c0000, /* nop */
	0x0c000000, /* ret */
/* } */
/* @0x4: function SetupP256PandMuLow[21] { */
#define CF_SetupP256PandMuLow_adr 4
	0x55741f01, /* subi r29, r31, #1 */
	0x83750000, /* movi r29.6h, #0 */
	0x83740001, /* movi r29.6l, #1 */
	0x82f50000, /* movi r29.5h, #0 */
	0x82f40000, /* movi r29.5l, #0 */
	0x82750000, /* movi r29.4h, #0 */
	0x82740000, /* movi r29.4l, #0 */
	0x81f50000, /* movi r29.3h, #0 */
	0x81f40000, /* movi r29.3l, #0 */
	0x98801d00, /* ldmod r29 */
	0x55701f01, /* subi r28, r31, #1 */
	0x83f10000, /* movi r28.7h, #0 */
	0x83f00000, /* movi r28.7l, #0 */
	0x82f0fffe, /* movi r28.5l, #65534 */
	0x8270fffe, /* movi r28.4l, #65534 */
	0x81f0fffe, /* movi r28.3l, #65534 */
	0x80f10000, /* movi r28.1h, #0 */
	0x80f00000, /* movi r28.1l, #0 */
	0x80710000, /* movi r28.0h, #0 */
	0x80700003, /* movi r28.0l, #3 */
	0x0c000000, /* ret */
/* } */
/* @0x19: function p256init[22] { */
#define CF_p256init_adr 25
	0x847c4000, /* ldi r31, [#0] */
	0x4c7fff00, /* xor r31, r31, r31 */
	0x51781f01, /* addi r30, r31, #1 */
	0x08000004, /* call &SetupP256PandMuLow */
	0x7c6c1f00, /* mov r27, r31 */
	0x83ed5ac6, /* movi r27.7h, #23238 */
	0x83ec35d8, /* movi r27.7l, #13784 */
	0x836daa3a, /* movi r27.6h, #43578 */
	0x836c93e7, /* movi r27.6l, #37863 */
	0x82edb3eb, /* movi r27.5h, #46059 */
	0x82ecbd55, /* movi r27.5l, #48469 */
	0x826d7698, /* movi r27.4h, #30360 */
	0x826c86bc, /* movi r27.4l, #34492 */
	0x81ed651d, /* movi r27.3h, #25885 */
	0x81ec06b0, /* movi r27.3l, #1712 */
	0x816dcc53, /* movi r27.2h, #52307 */
	0x816cb0f6, /* movi r27.2l, #45302 */
	0x80ed3bce, /* movi r27.1h, #15310 */
	0x80ec3c3e, /* movi r27.1l, #15422 */
	0x806d27d2, /* movi r27.0h, #10194 */
	0x806c604b, /* movi r27.0l, #24651 */
	0x0c000000, /* ret */
/* } */
/* @0x2f: function MulMod[38] { */
#define CF_MulMod_adr 47
	0x584f3800, /* mul128 r19, r24l, r25l */
	0x59d33800, /* mul128 r20, r24u, r25u */
	0x58d73800, /* mul128 r21, r24u, r25l */
	0x504eb310, /* add r19, r19, r21 << 128 */
	0x50d2b490, /* addc r20, r20, r21 >> 128 */
	0x59573800, /* mul128 r21, r24l, r25u */
	0x504eb310, /* add r19, r19, r21 << 128 */
	0x50d2b490, /* addc r20, r20, r21 >> 128 */
	0x645bfc02, /* selm r22, r28, r31 */
	0x685693ff, /* rshi r21, r19, r20 >> 255 */
	0x585f9500, /* mul128 r23, r21l, r28l */
	0x59e39500, /* mul128 r24, r21u, r28u */
	0x58e79500, /* mul128 r25, r21u, r28l */
	0x505f3710, /* add r23, r23, r25 << 128 */
	0x50e33890, /* addc r24, r24, r25 >> 128 */
	0x59679500, /* mul128 r25, r21l, r28u */
	0x505f3710, /* add r23, r23, r25 << 128 */
	0x50e33890, /* addc r24, r24, r25 >> 128 */
	0x6867f4ff, /* rshi r25, r20, r31 >> 255 */
	0x5062b800, /* add r24, r24, r21 */
	0x50e7f900, /* addc r25, r25, r31 */
	0x5062d800, /* add r24, r24, r22 */
	0x50e7f900, /* addc r25, r25, r31 */
	0x68573801, /* rshi r21, r24, r25 >> 1 */
	0x585abd00, /* mul128 r22, r29l, r21l */
	0x59debd00, /* mul128 r23, r29u, r21u */
	0x58e2bd00, /* mul128 r24, r29u, r21l */
	0x505b1610, /* add r22, r22, r24 << 128 */
	0x50df1790, /* addc r23, r23, r24 >> 128 */
	0x5962bd00, /* mul128 r24, r29l, r21u */
	0x505b1610, /* add r22, r22, r24 << 128 */
	0x50df1790, /* addc r23, r23, r24 >> 128 */
	0x545ad300, /* sub r22, r19, r22 */
	0x54d2f400, /* subb r20, r20, r23 */
	0x6457fd01, /* sell r21, r29, r31 */
	0x5456b600, /* sub r21, r22, r21 */
	0x9c4ff500, /* addm r19, r21, r31 */
	0x0c000000, /* ret */
/* } */
/* @0x55: function p256isoncurve[24] { */
#define CF_p256isoncurve_adr 85
	0x84004000, /* ldi r0, [#0] */
	0x95800000, /* lddmp r0 */
	0x82800018, /* movi r0.5l, #24 */
	0x83000018, /* movi r0.6l, #24 */
	0x80000000, /* movi r0.0l, #0 */
	0x97800000, /* ldrfp r0 */
	0x8c181600, /* ld *6, *6 */
	0x9c67f800, /* addm r25, r24, r31 */
	0x0800002f, /* call &MulMod */
	0x7c001300, /* mov r0, r19 */
	0x8c141500, /* ld *5, *5 */
	0x9c67f800, /* addm r25, r24, r31 */
	0x0800002f, /* call &MulMod */
	0x8c141500, /* ld *5, *5 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0x8c141500, /* ld *5, *5 */
	0xa04f1300, /* subm r19, r19, r24 */
	0xa04f1300, /* subm r19, r19, r24 */
	0xa04f1300, /* subm r19, r19, r24 */
	0x9c637300, /* addm r24, r19, r27 */
	0x904c0500, /* st *5, *3 */
	0x90500000, /* st *0, *4 */
	0x0c000000, /* ret */
/* } */
/* @0x6d: function ProjAdd[80] { */
#define CF_ProjAdd_adr 109
	0x7c600b00, /* mov r24, r11 */
	0x7c640800, /* mov r25, r8 */
	0x0800002f, /* call &MulMod */
	0x7c381300, /* mov r14, r19 */
	0x7c600c00, /* mov r24, r12 */
	0x7c640900, /* mov r25, r9 */
	0x0800002f, /* call &MulMod */
	0x7c3c1300, /* mov r15, r19 */
	0x7c600d00, /* mov r24, r13 */
	0x7c640a00, /* mov r25, r10 */
	0x0800002f, /* call &MulMod */
	0x7c401300, /* mov r16, r19 */
	0x9c458b00, /* addm r17, r11, r12 */
	0x9c492800, /* addm r18, r8, r9 */
	0x7c601100, /* mov r24, r17 */
	0x7c641200, /* mov r25, r18 */
	0x0800002f, /* call &MulMod */
	0x9c49ee00, /* addm r18, r14, r15 */
	0xa0465300, /* subm r17, r19, r18 */
	0x9c49ac00, /* addm r18, r12, r13 */
	0x9c4d4900, /* addm r19, r9, r10 */
	0x7c601200, /* mov r24, r18 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0x7c481300, /* mov r18, r19 */
	0x9c4e0f00, /* addm r19, r15, r16 */
	0xa04a7200, /* subm r18, r18, r19 */
	0x9c4dab00, /* addm r19, r11, r13 */
	0x9c314800, /* addm r12, r8, r10 */
	0x7c601300, /* mov r24, r19 */
	0x7c640c00, /* mov r25, r12 */
	0x0800002f, /* call &MulMod */
	0x7c2c1300, /* mov r11, r19 */
	0x9c320e00, /* addm r12, r14, r16 */
	0xa0318b00, /* subm r12, r11, r12 */
	0x7c601b00, /* mov r24, r27 */
	0x7c641000, /* mov r25, r16 */
	0x0800002f, /* call &MulMod */
	0xa02e6c00, /* subm r11, r12, r19 */
	0x9c356b00, /* addm r13, r11, r11 */
	0x9c2dab00, /* addm r11, r11, r13 */
	0xa0356f00, /* subm r13, r15, r11 */
	0x9c2d6f00, /* addm r11, r15, r11 */
	0x7c601b00, /* mov r24, r27 */
	0x7c640c00, /* mov r25, r12 */
	0x0800002f, /* call &MulMod */
	0x9c3e1000, /* addm r15, r16, r16 */
	0x9c420f00, /* addm r16, r15, r16 */
	0xa0321300, /* subm r12, r19, r16 */
	0xa031cc00, /* subm r12, r12, r14 */
	0x9c3d8c00, /* addm r15, r12, r12 */
	0x9c318f00, /* addm r12, r15, r12 */
	0x9c3dce00, /* addm r15, r14, r14 */
	0x9c39cf00, /* addm r14, r15, r14 */
	0xa03a0e00, /* subm r14, r14, r16 */
	0x7c601200, /* mov r24, r18 */
	0x7c640c00, /* mov r25, r12 */
	0x0800002f, /* call &MulMod */
	0x7c3c1300, /* mov r15, r19 */
	0x7c600e00, /* mov r24, r14 */
	0x7c640c00, /* mov r25, r12 */
	0x0800002f, /* call &MulMod */
	0x7c401300, /* mov r16, r19 */
	0x7c600b00, /* mov r24, r11 */
	0x7c640d00, /* mov r25, r13 */
	0x0800002f, /* call &MulMod */
	0x9c321300, /* addm r12, r19, r16 */
	0x7c601100, /* mov r24, r17 */
	0x7c640b00, /* mov r25, r11 */
	0x0800002f, /* call &MulMod */
	0xa02df300, /* subm r11, r19, r15 */
	0x7c601200, /* mov r24, r18 */
	0x7c640d00, /* mov r25, r13 */
	0x0800002f, /* call &MulMod */
	0x7c341300, /* mov r13, r19 */
	0x7c601100, /* mov r24, r17 */
	0x7c640e00, /* mov r25, r14 */
	0x0800002f, /* call &MulMod */
	0x9c366d00, /* addm r13, r13, r19 */
	0x0c000000, /* ret */
/* } */
/* @0xbd: function ProjToAffine[116] { */
#define CF_ProjToAffine_adr 189
	0x9c2bea00, /* addm r10, r10, r31 */
	0x7c600a00, /* mov r24, r10 */
	0x7c640a00, /* mov r25, r10 */
	0x0800002f, /* call &MulMod */
	0x7c601300, /* mov r24, r19 */
	0x7c640a00, /* mov r25, r10 */
	0x0800002f, /* call &MulMod */
	0x7c301300, /* mov r12, r19 */
	0x7c601300, /* mov r24, r19 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0x7c601300, /* mov r24, r19 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0x7c601300, /* mov r24, r19 */
	0x7c640c00, /* mov r25, r12 */
	0x0800002f, /* call &MulMod */
	0x7c341300, /* mov r13, r19 */
	0x05004004, /* loop #4 ( */
	0x7c601300, /* mov r24, r19 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0xfc7c0000, /* nop */
/*		   ) */
	0x7c601300, /* mov r24, r19 */
	0x7c640d00, /* mov r25, r13 */
	0x0800002f, /* call &MulMod */
	0x7c381300, /* mov r14, r19 */
	0x05008004, /* loop #8 ( */
	0x7c601300, /* mov r24, r19 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0xfc7c0000, /* nop */
/*		   ) */
	0x7c601300, /* mov r24, r19 */
	0x7c640e00, /* mov r25, r14 */
	0x0800002f, /* call &MulMod */
	0x7c3c1300, /* mov r15, r19 */
	0x05010004, /* loop #16 ( */
	0x7c601300, /* mov r24, r19 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0xfc7c0000, /* nop */
/*		   ) */
	0x7c601300, /* mov r24, r19 */
	0x7c640f00, /* mov r25, r15 */
	0x0800002f, /* call &MulMod */
	0x7c401300, /* mov r16, r19 */
	0x05020004, /* loop #32 ( */
	0x7c601300, /* mov r24, r19 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0xfc7c0000, /* nop */
/*		   ) */
	0x7c441300, /* mov r17, r19 */
	0x7c600a00, /* mov r24, r10 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0x050c0004, /* loop #192 ( */
	0x7c601300, /* mov r24, r19 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0xfc7c0000, /* nop */
/*		   ) */
	0x7c481300, /* mov r18, r19 */
	0x7c601100, /* mov r24, r17 */
	0x7c641000, /* mov r25, r16 */
	0x0800002f, /* call &MulMod */
	0x05010004, /* loop #16 ( */
	0x7c601300, /* mov r24, r19 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0xfc7c0000, /* nop */
/*		   ) */
	0x7c600f00, /* mov r24, r15 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0x05008004, /* loop #8 ( */
	0x7c601300, /* mov r24, r19 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0xfc7c0000, /* nop */
/*		   ) */
	0x7c600e00, /* mov r24, r14 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0x05004004, /* loop #4 ( */
	0x7c601300, /* mov r24, r19 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0xfc7c0000, /* nop */
/*		   ) */
	0x7c600d00, /* mov r24, r13 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0x05002004, /* loop #2 ( */
	0x7c601300, /* mov r24, r19 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0xfc7c0000, /* nop */
/*		   ) */
	0x7c600c00, /* mov r24, r12 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0x05002004, /* loop #2 ( */
	0x7c601300, /* mov r24, r19 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0xfc7c0000, /* nop */
/*		   ) */
	0x7c600a00, /* mov r24, r10 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0x7c601300, /* mov r24, r19 */
	0x7c641200, /* mov r25, r18 */
	0x0800002f, /* call &MulMod */
	0x7c381300, /* mov r14, r19 */
	0x7c600800, /* mov r24, r8 */
	0x7c640e00, /* mov r25, r14 */
	0x0800002f, /* call &MulMod */
	0x7c2c1300, /* mov r11, r19 */
	0x7c600900, /* mov r24, r9 */
	0x7c640e00, /* mov r25, r14 */
	0x0800002f, /* call &MulMod */
	0x7c301300, /* mov r12, r19 */
	0x0c000000, /* ret */
/* } */
/* @0x131: function ModInv[17] { */
#define CF_ModInv_adr 305
	0x98100000, /* stmod r4 */
	0x55100402, /* subi r4, r4, #2 */
	0x7c081e00, /* mov r2, r30 */
	0x0510000c, /* loop #256 ( */
	0x7c600200, /* mov r24, r2 */
	0x7c640200, /* mov r25, r2 */
	0x0800002f, /* call &MulMod */
	0x7c141300, /* mov r5, r19 */
	0x50108400, /* add r4, r4, r4 */
	0x6408a208, /* selc r2, r2, r5 */
	0x10088140, /* bnc nomul */
	0x7c600500, /* mov r24, r5 */
	0x7c640000, /* mov r25, r0 */
	0x0800002f, /* call &MulMod */
	0x7c081300, /* mov r2, r19 */
/*nomul: */
	0xfc7c0000, /* nop */
/*		   ) */
	0x0c000000, /* ret */
/* } */
/* @0x142: function FetchBandRandomize[20] { */
#define CF_FetchBandRandomize_adr 322
	0x99680000, /* strnd r26 */
	0x9c6bfa00, /* addm r26, r26, r31 */
	0x486b4000, /* not r26, r26 */
	0x546b5f00, /* sub r26, r31, r26 */
	0x50ebfa00, /* addc r26, r26, r31 */
	0x546b5f00, /* sub r26, r31, r26 */
	0x841c41c0, /* ldi r7, [#14] */
	0x7c600700, /* mov r24, r7 */
	0x7c641a00, /* mov r25, r26 */
	0x0800002f, /* call &MulMod */
	0x7c1c1300, /* mov r7, r19 */
	0x8c081500, /* ld *2, *5 */
	0x7c641a00, /* mov r25, r26 */
	0x0800002f, /* call &MulMod */
	0x7c141300, /* mov r5, r19 */
	0x8c081600, /* ld *2, *6 */
	0x7c641a00, /* mov r25, r26 */
	0x0800002f, /* call &MulMod */
	0x7c181300, /* mov r6, r19 */
	0x0c000000, /* ret */
/* } */
/* @0x156: function ProjDouble[5] { */
#define CF_ProjDouble_adr 342
	0x7c2c0800, /* mov r11, r8 */
	0x7c300900, /* mov r12, r9 */
	0x7c340a00, /* mov r13, r10 */
	0x0800006d, /* call &ProjAdd */
	0x0c000000, /* ret */
/* } */
/* @0x15b: function SetupP256NandMuLow[25] { */
#define CF_SetupP256NandMuLow_adr 347
	0x55741f01, /* subi r29, r31, #1 */
	0x83750000, /* movi r29.6h, #0 */
	0x83740000, /* movi r29.6l, #0 */
	0x81f5bce6, /* movi r29.3h, #48358 */
	0x81f4faad, /* movi r29.3l, #64173 */
	0x8175a717, /* movi r29.2h, #42775 */
	0x81749e84, /* movi r29.2l, #40580 */
	0x80f5f3b9, /* movi r29.1h, #62393 */
	0x80f4cac2, /* movi r29.1l, #51906 */
	0x8075fc63, /* movi r29.0h, #64611 */
	0x80742551, /* movi r29.0l, #9553 */
	0x55701f01, /* subi r28, r31, #1 */
	0x83f10000, /* movi r28.7h, #0 */
	0x83f00000, /* movi r28.7l, #0 */
	0x82f0fffe, /* movi r28.5l, #65534 */
	0x81f14319, /* movi r28.3h, #17177 */
	0x81f00552, /* movi r28.3l, #1362 */
	0x8171df1a, /* movi r28.2h, #57114 */
	0x81706c21, /* movi r28.2l, #27681 */
	0x80f1012f, /* movi r28.1h, #303 */
	0x80f0fd85, /* movi r28.1l, #64901 */
	0x8071eedf, /* movi r28.0h, #61151 */
	0x80709bfe, /* movi r28.0l, #39934 */
	0x98801d00, /* ldmod r29 */
	0x0c000000, /* ret */
/* } */
/* @0x174: function ScalarMult_internal[82] { */
#define CF_ScalarMult_internal_adr 372
	0xf8efb341, /* sigini #15708993 */
	0x0800015b, /* call &SetupP256NandMuLow */
	0x4c002000, /* xor r0, r0, r1 */
	0x9c07e100, /* addm r1, r1, r31 */
	0xa0002000, /* subm r0, r0, r1 */
	0x5007a100, /* add r1, r1, r29 */
	0x646bbf08, /* selc r26, r31, r29 */
	0x50074100, /* add r1, r1, r26 */
	0x08000004, /* call &SetupP256PandMuLow */
	0x08000142, /* call &FetchBandRandomize */
	0x7c200500, /* mov r8, r5 */
	0x7c240600, /* mov r9, r6 */
	0x7c280700, /* mov r10, r7 */
	0x08000156, /* call &ProjDouble */
	0x7c080b00, /* mov r2, r11 */
	0x7c0c0c00, /* mov r3, r12 */
	0x7c100d00, /* mov r4, r13 */
	0xf91fda16, /* sigchk #2087446 */
	0x0510003e, /* loop #256 ( */
	0xf8123456, /* sigini #1193046 */
	0x08000156, /* call &ProjDouble */
	0x08000142, /* call &FetchBandRandomize */
	0x4c204500, /* xor r8, r5, r2 */
	0x4c246600, /* xor r9, r6, r3 */
	0x4c288700, /* xor r10, r7, r4 */
	0xf97d2764, /* sigchk #8202084 */
	0x4c682000, /* xor r26, r0, r1 */
	0x10002194, /* bm inv_b2b */
	0x4c20a800, /* xor r8, r8, r5 */
	0x4c24c900, /* xor r9, r9, r6 */
	0x4c28ea00, /* xor r10, r10, r7 */
	0x10080198, /* b inv_b2b_done */
/*inv_b2b: */
	0x4c204800, /* xor r8, r8, r2 */
	0x4c246900, /* xor r9, r9, r3 */
	0x4c288a00, /* xor r10, r10, r4 */
	0x10080198, /* b inv_b2b_done */
/*inv_b2b_done: */
	0xf8895efc, /* sigini #9002748 */
	0x7c140b00, /* mov r5, r11 */
	0x7c180c00, /* mov r6, r12 */
	0x7c1c0d00, /* mov r7, r13 */
	0x0800006d, /* call &ProjAdd */
	0x4c216500, /* xor r8, r5, r11 */
	0x4c258600, /* xor r9, r6, r12 */
	0xf9856147, /* sigchk #8741191 */
	0x44282000, /* or r10, r0, r1 */
	0xfc7c0000, /* nop */
	0x100021a8, /* bm inv_012 */
	0x4c29a700, /* xor r10, r7, r13 */
	0x4c216800, /* xor r8, r8, r11 */
	0x4c258900, /* xor r9, r9, r12 */
	0x4c29aa00, /* xor r10, r10, r13 */
	0x100801ad, /* b inv_012_done */
/*inv_012: */
	0x4c29a700, /* xor r10, r7, r13 */
	0x4c20a800, /* xor r8, r8, r5 */
	0x4c24c900, /* xor r9, r9, r6 */
	0x4c28ea00, /* xor r10, r10, r7 */
	0x100801ad, /* b inv_012_done */
/*inv_012_done: */
	0xf883423f, /* sigini #8602175 */
	0x680000ff, /* rshi r0, r0, r0 >> 255 */
	0x680421ff, /* rshi r1, r1, r1 >> 255 */
	0x992c0000, /* strnd r11 */
	0x99300000, /* strnd r12 */
	0x99340000, /* strnd r13 */
	0x99680000, /* strnd r26 */
	0x48674000, /* not r25, r26 */
	0x546b5f00, /* sub r26, r31, r26 */
	0x50e7f900, /* addc r25, r25, r31 */
	0x546b3f00, /* sub r26, r31, r25 */
	0x7c600200, /* mov r24, r2 */
	0x7c641a00, /* mov r25, r26 */
	0x0800002f, /* call &MulMod */
	0x7c081300, /* mov r2, r19 */
	0x7c600300, /* mov r24, r3 */
	0x7c641a00, /* mov r25, r26 */
	0x0800002f, /* call &MulMod */
	0x7c0c1300, /* mov r3, r19 */
	0x7c600400, /* mov r24, r4 */
	0x7c641a00, /* mov r25, r26 */
	0x0800002f, /* call &MulMod */
	0x7c101300, /* mov r4, r19 */
	0xf98e4420, /* sigchk #9323552 */
/*		   ) */
	0x0c000000, /* ret */
/* } */
/* @0x1c6: function get_P256B[35] { */
#define CF_get_P256B_adr 454
	0x7c201f00, /* mov r8, r31 */
	0x83a16b17, /* movi r8.7h, #27415 */
	0x83a0d1f2, /* movi r8.7l, #53746 */
	0x8321e12c, /* movi r8.6h, #57644 */
	0x83204247, /* movi r8.6l, #16967 */
	0x82a1f8bc, /* movi r8.5h, #63676 */
	0x82a0e6e5, /* movi r8.5l, #59109 */
	0x822163a4, /* movi r8.4h, #25508 */
	0x822040f2, /* movi r8.4l, #16626 */
	0x81a17703, /* movi r8.3h, #30467 */
	0x81a07d81, /* movi r8.3l, #32129 */
	0x81212deb, /* movi r8.2h, #11755 */
	0x812033a0, /* movi r8.2l, #13216 */
	0x80a1f4a1, /* movi r8.1h, #62625 */
	0x80a03945, /* movi r8.1l, #14661 */
	0x8021d898, /* movi r8.0h, #55448 */
	0x8020c296, /* movi r8.0l, #49814 */
	0x7c241f00, /* mov r9, r31 */
	0x83a54fe3, /* movi r9.7h, #20451 */
	0x83a442e2, /* movi r9.7l, #17122 */
	0x8325fe1a, /* movi r9.6h, #65050 */
	0x83247f9b, /* movi r9.6l, #32667 */
	0x82a58ee7, /* movi r9.5h, #36583 */
	0x82a4eb4a, /* movi r9.5l, #60234 */
	0x82257c0f, /* movi r9.4h, #31759 */
	0x82249e16, /* movi r9.4l, #40470 */
	0x81a52bce, /* movi r9.3h, #11214 */
	0x81a43357, /* movi r9.3l, #13143 */
	0x81256b31, /* movi r9.2h, #27441 */
	0x81245ece, /* movi r9.2l, #24270 */
	0x80a5cbb6, /* movi r9.1h, #52150 */
	0x80a44068, /* movi r9.1l, #16488 */
	0x802537bf, /* movi r9.0h, #14271 */
	0x802451f5, /* movi r9.0l, #20981 */
	0x0c000000, /* ret */
/* } */
/* @0x1e9: function p256sign[76] { */
#define CF_p256sign_adr 489
	0xfc7c0000, /* nop */
	0xf8fa73bc, /* sigini #16413628 */
	0x84004000, /* ldi r0, [#0] */
	0x95800000, /* lddmp r0 */
	0x80000000, /* movi r0.0l, #0 */
	0x80800001, /* movi r0.1l, #1 */
	0x81000018, /* movi r0.2l, #24 */
	0x82000008, /* movi r0.4l, #8 */
	0x82800009, /* movi r0.5l, #9 */
	0x83000002, /* movi r0.6l, #2 */
	0x97800000, /* ldrfp r0 */
	0x8c001000, /* ld *0, *0 */
	0x8c041100, /* ld *1, *1 */
	0x99800100, /* ldrnd r1 */
	0x080001c6, /* call &get_P256B */
	0x90540400, /* st *4, *5 */
	0x90580500, /* st *5, *6 */
	0x887841c0, /* sti r30, [#14] */
	0xf9fc9dbf, /* sigchk #16555455 */
	0x08000174, /* call &ScalarMult_internal */
	0xf92598c3, /* sigchk #2463939 */
	0x080000bd, /* call &ProjToAffine */
	0x7c600b00, /* mov r24, r11 */
	0x7c640b00, /* mov r25, r11 */
	0x0800002f, /* call &MulMod */
	0x7c600b00, /* mov r24, r11 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0xa04d7300, /* subm r19, r19, r11 */
	0xa04d7300, /* subm r19, r19, r11 */
	0xa04d7300, /* subm r19, r19, r11 */
	0x9c0b7300, /* addm r2, r19, r27 */
	0x7c600c00, /* mov r24, r12 */
	0x7c640c00, /* mov r25, r12 */
	0x0800002f, /* call &MulMod */
	0x5c005300, /* cmp r19, r2 */
	0x10084232, /* bnz sign_fail */
	0x0800015b, /* call &SetupP256NandMuLow */
	0x8c081000, /* ld *2, *0 */
	0x8c181100, /* ld *6, *1 */
	0x4c605800, /* xor r24, r24, r2 */
	0x480c4000, /* not r3, r2 */
	0x54085f00, /* sub r2, r31, r2 */
	0x508fe300, /* addc r3, r3, r31 */
	0x7c640300, /* mov r25, r3 */
	0x0800002f, /* call &MulMod */
	0x7c001300, /* mov r0, r19 */
	0x08000131, /* call &ModInv */
	0x7c600200, /* mov r24, r2 */
	0x7c640300, /* mov r25, r3 */
	0x0800002f, /* call &MulMod */
	0x7c081300, /* mov r2, r19 */
	0x99600000, /* strnd r24 */
	0x8c081700, /* ld *2, *7 */
	0x8c001100, /* ld *0, *1 */
	0x4c601800, /* xor r24, r24, r0 */
	0x7c640200, /* mov r25, r2 */
	0x0800002f, /* call &MulMod */
	0x9c63eb00, /* addm r24, r11, r31 */
	0x7c041800, /* mov r1, r24 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0x7c001300, /* mov r0, r19 */
	0x8c081200, /* ld *2, *2 */
	0x7c640200, /* mov r25, r2 */
	0x0800002f, /* call &MulMod */
	0x9c001300, /* addm r0, r19, r0 */
	0x5c03e000, /* cmp r0, r31 */
	0x64043f04, /* selz r1, r31, r1 */
	0x5c03e100, /* cmp r1, r31 */
	0x64001f04, /* selz r0, r31, r0 */
	0x90500000, /* st *0, *4 */
	0x904c0100, /* st *1, *3 */
/*sign_fail: */
	0x08000004, /* call &SetupP256PandMuLow */
	0xf9901a43, /* sigchk #9443907 */
	0x0c000000, /* ret */
/* } */
/* @0x235: function p256scalarbasemult[25] { */
#define CF_p256scalarbasemult_adr 565
	0xf8123456, /* sigini #1193046 */
	0x84004000, /* ldi r0, [#0] */
	0x95800000, /* lddmp r0 */
	0x80000000, /* movi r0.0l, #0 */
	0x80800001, /* movi r0.1l, #1 */
	0x81000018, /* movi r0.2l, #24 */
	0x8180000b, /* movi r0.3l, #11 */
	0x82000008, /* movi r0.4l, #8 */
	0x82800009, /* movi r0.5l, #9 */
	0x97800000, /* ldrfp r0 */
	0x8c001700, /* ld *0, *7 */
	0x8c041100, /* ld *1, *1 */
	0x99800100, /* ldrnd r1 */
	0x080001c6, /* call &get_P256B */
	0x90540400, /* st *4, *5 */
	0x90580500, /* st *5, *6 */
	0x887841c0, /* sti r30, [#14] */
	0xf9328977, /* sigchk #3311991 */
	0x08000174, /* call &ScalarMult_internal */
	0xf92598c3, /* sigchk #2463939 */
	0x080000bd, /* call &ProjToAffine */
	0x90540b00, /* st *3++, *5 */
	0x90580b00, /* st *3++, *6 */
	0xf987c02a, /* sigchk #8896554 */
	0x0c000000, /* ret */
/* } */
/* @0x24e: function P256RecoverY[46] { */
#define CF_P256RecoverY_adr 590
	0x7c600200, /* mov r24, r2 */
	0x7c640200, /* mov r25, r2 */
	0x0800002f, /* call &MulMod */
	0x7c181300, /* mov r6, r19 */
	0x7c600600, /* mov r24, r6 */
	0x7c640200, /* mov r25, r2 */
	0x0800002f, /* call &MulMod */
	0x7c181300, /* mov r6, r19 */
	0xa0184600, /* subm r6, r6, r2 */
	0xa0184600, /* subm r6, r6, r2 */
	0xa0184600, /* subm r6, r6, r2 */
	0x9c1b6600, /* addm r6, r6, r27 */
	0x7c0c1e00, /* mov r3, r30 */
	0x05020008, /* loop #32 ( */
	0x7c600300, /* mov r24, r3 */
	0x7c640300, /* mov r25, r3 */
	0x0800002f, /* call &MulMod */
	0x7c0c1300, /* mov r3, r19 */
	0x7c600300, /* mov r24, r3 */
	0x7c640600, /* mov r25, r6 */
	0x0800002f, /* call &MulMod */
	0x7c0c1300, /* mov r3, r19 */
/*		   ) */
	0x05020004, /* loop #32 ( */
	0x7c600300, /* mov r24, r3 */
	0x7c640300, /* mov r25, r3 */
	0x0800002f, /* call &MulMod */
	0x7c0c1300, /* mov r3, r19 */
/*		   ) */
	0x7c600300, /* mov r24, r3 */
	0x7c640600, /* mov r25, r6 */
	0x0800002f, /* call &MulMod */
	0x7c0c1300, /* mov r3, r19 */
	0x05060004, /* loop #96 ( */
	0x7c600300, /* mov r24, r3 */
	0x7c640300, /* mov r25, r3 */
	0x0800002f, /* call &MulMod */
	0x7c0c1300, /* mov r3, r19 */
/*		   ) */
	0x7c600300, /* mov r24, r3 */
	0x7c640600, /* mov r25, r6 */
	0x0800002f, /* call &MulMod */
	0x7c0c1300, /* mov r3, r19 */
	0x0505e004, /* loop #94 ( */
	0x7c600300, /* mov r24, r3 */
	0x7c640300, /* mov r25, r3 */
	0x0800002f, /* call &MulMod */
	0x7c0c1300, /* mov r3, r19 */
/*		   ) */
	0x0c000000, /* ret */
/* } */
/* @0x27c: function P256GenerateRandomPoint[34] { */
#define CF_P256GenerateRandomPoint_adr 636
/*genX: */
	0x99080000, /* strnd r2 */
	0x9c0be200, /* addm r2, r2, r31 */
	0x0800024e, /* call &P256RecoverY */
	0x7c600300, /* mov r24, r3 */
	0x7c640300, /* mov r25, r3 */
	0x0800002f, /* call &MulMod */
	0x7c101300, /* mov r4, r19 */
	0x7c600200, /* mov r24, r2 */
	0x7c640200, /* mov r25, r2 */
	0x0800002f, /* call &MulMod */
	0x7c141300, /* mov r5, r19 */
	0x7c600500, /* mov r24, r5 */
	0x7c640200, /* mov r25, r2 */
	0x0800002f, /* call &MulMod */
	0x7c141300, /* mov r5, r19 */
	0xa0144500, /* subm r5, r5, r2 */
	0xa0144500, /* subm r5, r5, r2 */
	0xa0144500, /* subm r5, r5, r2 */
	0x9c176500, /* addm r5, r5, r27 */
	0x5c00a400, /* cmp r4, r5 */
	0x1008427c, /* bnz genX */
	0x99100000, /* strnd r4 */
	0x7c600200, /* mov r24, r2 */
	0x7c640400, /* mov r25, r4 */
	0x0800002f, /* call &MulMod */
	0x7c081300, /* mov r2, r19 */
	0x7c600300, /* mov r24, r3 */
	0x7c640400, /* mov r25, r4 */
	0x0800002f, /* call &MulMod */
	0x7c0c1300, /* mov r3, r19 */
	0x88084100, /* sti r2, [#8] */
	0x880c4120, /* sti r3, [#9] */
	0x88104140, /* sti r4, [#10] */
	0x0c000000, /* ret */
/* } */
/* @0x29e: function p256scalarmult[56] { */
#define CF_p256scalarmult_adr 670
	0xf8123546, /* sigini #1193286 */
	0x84004000, /* ldi r0, [#0] */
	0x95800000, /* lddmp r0 */
	0x80000000, /* movi r0.0l, #0 */
	0x80800001, /* movi r0.1l, #1 */
	0x81000018, /* movi r0.2l, #24 */
	0x82800008, /* movi r0.5l, #8 */
	0x83000009, /* movi r0.6l, #9 */
	0x8180000b, /* movi r0.3l, #11 */
	0x8200000c, /* movi r0.4l, #12 */
	0x97800000, /* ldrfp r0 */
	0x8c001000, /* ld *0, *0 */
	0x8c041100, /* ld *1, *1 */
	0x99800100, /* ldrnd r1 */
	0x8c141500, /* ld *5, *5 */
	0x8c181600, /* ld *6, *6 */
	0x7c281e00, /* mov r10, r30 */
	0xf94d6eff, /* sigchk #5074687 */
	0x0800027c, /* call &P256GenerateRandomPoint */
	0xf8123456, /* sigini #1193046 */
	0x842c4100, /* ldi r11, [#8] */
	0x84304120, /* ldi r12, [#9] */
	0x84344140, /* ldi r13, [#10] */
	0x0800006d, /* call &ProjAdd */
	0x90540300, /* st *3, *5 */
	0x90580400, /* st *4, *6 */
	0x883441c0, /* sti r13, [#14] */
	0xf94f6304, /* sigchk #5202692 */
	0x08000174, /* call &ScalarMult_internal */
	0x842c4100, /* ldi r11, [#8] */
	0x84304120, /* ldi r12, [#9] */
	0x84344140, /* ldi r13, [#10] */
	0x90540300, /* st *3, *5 */
	0x90580400, /* st *4, *6 */
	0x883441c0, /* sti r13, [#14] */
	0x88204160, /* sti r8, [#11] */
	0x88244180, /* sti r9, [#12] */
	0x882841a0, /* sti r10, [#13] */
	0xfc7c0000, /* nop */
	0x8c001000, /* ld *0, *0 */
	0x8c041100, /* ld *1, *1 */
	0x08000174, /* call &ScalarMult_internal */
	0xa0253f00, /* subm r9, r31, r9 */
	0x842c4160, /* ldi r11, [#11] */
	0x84304180, /* ldi r12, [#12] */
	0x843441a0, /* ldi r13, [#13] */
	0x0800006d, /* call &ProjAdd */
	0x7c200b00, /* mov r8, r11 */
	0x7c240c00, /* mov r9, r12 */
	0x7c280d00, /* mov r10, r13 */
	0x080000bd, /* call &ProjToAffine */
	0xf903f90e, /* sigchk #260366 */
	0x90540300, /* st *3, *5 */
	0x90580400, /* st *4, *6 */
	0xf9e02e49, /* sigchk #14691913 */
	0x0c000000, /* ret */
/* } */
/* @0x2d6: function ModInvVar[38] { */
#define CF_ModInvVar_adr 726
	0x7c081f00, /* mov r2, r31 */
	0x5017e000, /* add r5, r0, r31 */
	0x100042fa, /* bz invert-zero */
	0x7c0c1e00, /* mov r3, r30 */
	0x98100000, /* stmod r4 */
	0x981c0000, /* stmod r7 */
/*impvt_Loop: */
	0x44108400, /* or r4, r4, r4 */
	0x100012e7, /* bl impvt_Uodd */
	0x6813e401, /* rshi r4, r4, r31 >> 1 */
	0x44084200, /* or r2, r2, r2 */
	0x100012e3, /* bl impvt_Rodd */
	0x680be201, /* rshi r2, r2, r31 >> 1 */
	0x100802dc, /* b impvt_Loop */
/*impvt_Rodd: */
	0x50084700, /* add r2, r7, r2 */
	0x509bff00, /* addc r6, r31, r31 */
	0x6808c201, /* rshi r2, r2, r6 >> 1 */
	0x100802dc, /* b impvt_Loop */
/*impvt_Uodd: */
	0x4414a500, /* or r5, r5, r5 */
	0x100012f2, /* bl impvt_UVodd */
	0x6817e501, /* rshi r5, r5, r31 >> 1 */
	0x440c6300, /* or r3, r3, r3 */
	0x100012ee, /* bl impvt_Sodd */
	0x680fe301, /* rshi r3, r3, r31 >> 1 */
	0x100802dc, /* b impvt_Loop */
/*impvt_Sodd: */
	0x500c6700, /* add r3, r7, r3 */
	0x509bff00, /* addc r6, r31, r31 */
	0x680cc301, /* rshi r3, r3, r6 >> 1 */
	0x100802dc, /* b impvt_Loop */
/*impvt_UVodd: */
	0x5c008500, /* cmp r5, r4 */
	0x100882f7, /* bnc impvt_V>=U */
	0xa0086200, /* subm r2, r2, r3 */
	0x5410a400, /* sub r4, r4, r5 */
	0x100802dc, /* b impvt_Loop */
/*impvt_V>=U: */
	0xa00c4300, /* subm r3, r3, r2 */
	0x54148500, /* sub r5, r5, r4 */
	0x100842dc, /* bnz impvt_Loop */
/*invert-zero: */
	0x9c07e200, /* addm r1, r2, r31 */
	0x0c000000, /* ret */
/* } */
/* @0x2fc: function p256verify[97] { */
#define CF_p256verify_adr 764
	0x84184000, /* ldi r6, [#0] */
	0x95800600, /* lddmp r6 */
	0x81980018, /* movi r6.3l, #24 */
	0x82180000, /* movi r6.4l, #0 */
	0x82980008, /* movi r6.5l, #8 */
	0x83180009, /* movi r6.6l, #9 */
	0x8018000b, /* movi r6.0l, #11 */
	0x8398000c, /* movi r6.7l, #12 */
	0x81180018, /* movi r6.2l, #24 */
	0x97800600, /* ldrfp r6 */
	0x8c081600, /* ld *2, *6 */
	0x9c67f800, /* addm r25, r24, r31 */
	0x0800002f, /* call &MulMod */
	0x7c181300, /* mov r6, r19 */
	0x8c081500, /* ld *2, *5 */
	0x9c67f800, /* addm r25, r24, r31 */
	0x0800002f, /* call &MulMod */
	0x8c081500, /* ld *2, *5 */
	0x7c641300, /* mov r25, r19 */
	0x0800002f, /* call &MulMod */
	0x8c081500, /* ld *2, *5 */
	0xa04f1300, /* subm r19, r19, r24 */
	0xa04f1300, /* subm r19, r19, r24 */
	0xa04f1300, /* subm r19, r19, r24 */
	0x9c637300, /* addm r24, r19, r27 */
	0x5c030600, /* cmp r6, r24 */
	0x8c0c1300, /* ld *3, *3 */
	0x7c181800, /* mov r6, r24 */
	0x4a630000, /* notx r24, r24 */
	0x1008435b, /* bnz fail */
	0x0800015b, /* call &SetupP256NandMuLow */
	0x5c03e600, /* cmp r6, r31 */
	0x1000435b, /* bz fail */
	0x5c03a600, /* cmp r6, r29 */
	0x1008835b, /* bnc fail */
	0x8c101400, /* ld *4, *4 */
	0x5c03e000, /* cmp r0, r31 */
	0x1000435b, /* bz fail */
	0x5c03a000, /* cmp r0, r29 */
	0x1008835b, /* bnc fail */
	0x080002d6, /* call &ModInvVar */
	0x8c0c1300, /* ld *3, *3 */
	0x7c640100, /* mov r25, r1 */
	0x0800002f, /* call &MulMod */
	0x7c001300, /* mov r0, r19 */
	0x8c081200, /* ld *2, *2 */
	0x7c640100, /* mov r25, r1 */
	0x0800002f, /* call &MulMod */
	0x7c041300, /* mov r1, r19 */
	0x08000004, /* call &SetupP256PandMuLow */
	0x8c001500, /* ld *0, *5 */
	0x8c1c1600, /* ld *7, *6 */
	0x7c341e00, /* mov r13, r30 */
	0x080001c6, /* call &get_P256B */
	0x7c281e00, /* mov r10, r30 */
	0x0800006d, /* call &ProjAdd */
	0x7c0c0b00, /* mov r3, r11 */
	0x7c100c00, /* mov r4, r12 */
	0x7c140d00, /* mov r5, r13 */
	0x40082000, /* and r2, r0, r1 */
	0x7c2c1f00, /* mov r11, r31 */
	0x7c301e00, /* mov r12, r30 */
	0x7c341f00, /* mov r13, r31 */
	0x05100018, /* loop #256 ( */
	0x7c200b00, /* mov r8, r11 */
	0x7c240c00, /* mov r9, r12 */
	0x7c280d00, /* mov r10, r13 */
	0x0800006d, /* call &ProjAdd */
	0x50084200, /* add r2, r2, r2 */
	0x10088347, /* bnc noBoth */
	0x7c200300, /* mov r8, r3 */
	0x7c240400, /* mov r9, r4 */
	0x7c280500, /* mov r10, r5 */
	0x0800006d, /* call &ProjAdd */
	0x10080352, /* b noY */
/*noBoth: */
	0x50180000, /* add r6, r0, r0 */
	0x1008834d, /* bnc noG */
	0x8c141500, /* ld *5, *5 */
	0x8c181600, /* ld *6, *6 */
	0x7c281e00, /* mov r10, r30 */
	0x0800006d, /* call &ProjAdd */
/*noG: */
	0x50182100, /* add r6, r1, r1 */
	0x10088352, /* bnc noY */
	0x080001c6, /* call &get_P256B */
	0x7c281e00, /* mov r10, r30 */
	0x0800006d, /* call &ProjAdd */
/*noY: */
	0x50000000, /* add r0, r0, r0 */
	0x50042100, /* add r1, r1, r1 */
/*		   ) */
	0x7c000d00, /* mov r0, r13 */
	0x080002d6, /* call &ModInvVar */
	0x7c600100, /* mov r24, r1 */
	0x7c640b00, /* mov r25, r11 */
	0x0800002f, /* call &MulMod */
	0x0800015b, /* call &SetupP256NandMuLow */
	0x9c63f300, /* addm r24, r19, r31 */
/*fail: */
	0x90440300, /* st *3, *1 */
	0x0c000000, /* ret */
/* } */
/* @0x35d: function p256verisign[4] { */
#define CF_p256verisign_adr 861
	0x080001e9, /* call &p256sign */
	0x08000235, /* call &p256scalarbasemult */
	0x080002fc, /* call &p256verify */
	0x0c000000, /* ret */
/* } */
};
/* clang-format on */
