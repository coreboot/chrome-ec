/*
 * Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __GSC_UTILS_BOOT_PARAM_CBOR_BASIC_H
#define __GSC_UTILS_BOOT_PARAM_CBOR_BASIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CBOR_MAJOR_UINT         ((uint8_t)(0 << 5))
#define CBOR_MAJOR_NINT         ((uint8_t)(1 << 5))
#define CBOR_MAJOR_BSTR         ((uint8_t)(2 << 5))
#define CBOR_MAJOR_TSTR         ((uint8_t)(3 << 5))
#define CBOR_MAJOR_ARR          ((uint8_t)(4 << 5))
#define CBOR_MAJOR_MAP          ((uint8_t)(5 << 5))
#define CBOR_MAJOR_TAG          ((uint8_t)(6 << 5))
#define CBOR_MAJOR_SIMPLE       ((uint8_t)(7 << 5))

#define CBOR_HDR1(major, value) ((uint8_t)(major) | (uint8_t)((value) & 0x1f))

#define CBOR_FALSE CBOR_HDR1(CBOR_MAJOR_SIMPLE, 20)
#define CBOR_TRUE  CBOR_HDR1(CBOR_MAJOR_SIMPLE, 21)
#define CBOR_NULL  CBOR_HDR1(CBOR_MAJOR_SIMPLE, 22)

#define CBOR_BYTES1 24
#define CBOR_BYTES2 25
#define CBOR_BYTES4 26
#define CBOR_BYTES8 27

/* NINTs in [-24..-1] range ("0 bytes") */
#define CBOR_NINT0_LEN	  1
#define CBOR_NINT0(label) CBOR_HDR1(CBOR_MAJOR_NINT, -(label) - 1)

/* UINTs in [0..23] range ("0 bytes") */
#define CBOR_UINT0_LEN	  1
#define CBOR_UINT0(label) CBOR_HDR1(CBOR_MAJOR_UINT, (label))

/* Building block for 32 bit (4 bytes) integer representations */
#define CBOR_INT32_LEN (1 + 4)
#define CBOR_INT32(major, value)                         \
	{                                                \
		CBOR_HDR1(major, CBOR_BYTES4),           \
		(uint8_t)(((value) & 0xFF000000) >> 24), \
		(uint8_t)(((value) & 0x00FF0000) >> 16), \
		(uint8_t)(((value) & 0x0000FF00) >> 8),  \
		(uint8_t)((value) & 0x000000FF)          \
	}

/* 32 bit (4 bytes) negative integers */
#define CBOR_NINT32_LEN	   CBOR_INT32_LEN
#define CBOR_NINT32(value) CBOR_INT32(CBOR_MAJOR_NINT, (-(value) - 1))

/* 32 bit (4 bytes) positive integers */
#define CBOR_UINT32_LEN	   CBOR_INT32_LEN
#define CBOR_UINT32(value) CBOR_INT32(CBOR_MAJOR_UINT, value)

/* BSTR with 1 byte size */
#define CBOR_BSTR_HDR8(size)                              \
	{ CBOR_HDR1(CBOR_MAJOR_BSTR, CBOR_BYTES1), size }

/* TSTR with 1 byte size */
#define CBOR_TSTR_HDR8(size)                              \
	{ CBOR_HDR1(CBOR_MAJOR_TSTR, CBOR_BYTES1), size }

/* BSTR with 2 byte size */
#define CBOR_BSTR_HDR16(size)                            \
	{                                                \
		CBOR_HDR1(CBOR_MAJOR_BSTR, CBOR_BYTES2), \
		(uint8_t)(((size) & 0xFF00) >> 8),       \
		(uint8_t)((size) & 0x00FF)               \
	}

/* BSTR of length 1 */
struct cbor_bstr1_s {
	uint8_t cbor_hdr;
	uint8_t value;
};
#define CBOR_BSTR1_HDR CBOR_HDR1(CBOR_MAJOR_BSTR, 1)
#define CBOR_BSTR1_EMPTY { CBOR_BSTR1_HDR, 0 }

/* BSTR of length 32: UDS, CDI, digest */
struct cbor_bstr32_s {
	uint8_t cbor_hdr[2];
	uint8_t value[32];
};
#define CBOR_BSTR32_HDR CBOR_BSTR_HDR8(32)
#define CBOR_BSTR32_EMPTY                       \
	{                                       \
		CBOR_BSTR32_HDR,                \
		{                               \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0  \
		}                               \
	}

/* TSTR of length 2*20 = 40: UDS_ID, CDI_ID as hex */
struct cbor_tstr40_s {
	uint8_t cbor_hdr[2];
	uint8_t value[40];
};
#define CBOR_TSTR40_HDR CBOR_TSTR_HDR8(40)
#define CBOR_TSTR40_EMPTY                       \
	{                                       \
		CBOR_TSTR40_HDR,                \
		{                               \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0  \
		}                               \
	}

/* BSTR of length 64: signature */
struct cbor_bstr64_s {
	uint8_t cbor_hdr[2];
	uint8_t value[64];
};
#define CBOR_BSTR64_HDR CBOR_BSTR_HDR8(64)
#define CBOR_BSTR64_EMPTY                       \
	{                                       \
		CBOR_BSTR64_HDR,                \
		{                               \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0  \
		}                               \
	}

/* UINT32 */
struct cbor_uint32_s {
	uint8_t cbor_hdr;
	uint8_t value[4];
};
#define CBOR_UINT32_HDR CBOR_HDR1(CBOR_MAJOR_UINT, CBOR_BYTES4)
#define CBOR_UINT32_ZERO           \
	{                          \
		CBOR_UINT32_HDR,   \
		{                  \
			0, 0, 0, 0 \
		}                  \
	}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __CGSC_UTILS_BOOT_PARAM_CBOR_BASIC_H */
