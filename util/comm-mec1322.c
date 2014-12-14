/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 *
 * LPC communication protocol for Host Command on MEC1322
 *
 * This module is used by ectool, coreboot, depthcharge, mosys, and flashrom.
 */

#include <stdint.h>

#ifndef __packed
#define __packed __attribute__((packed))
#endif

#define MEC1322_MEMMAP_BASE		0x200

#define MEC1322_MEMMAP_ADJUST_OFFSET	0x800

#define MEC1322_MEMMAP_DATA		(MEC1322_MEMMAP_BASE + 0x0)
#define  MEC1322_MEMMAP_READ_OP		1
#define  MEC1322_MEMMAP_WRITE_OP	0
#define MEC1322_MEMMAP_STATUS		(MEC1322_MEMMAP_BASE + 0x4)
#define  MEC1322_MEMMAP_OBF		0x01
#define  MEC1322_MEMMAP_IBF		0x02
#define  MEC1322_MEMMAP_BUSY		0x04

#define MEC1322_MEMMAP_DELAY		1000000

int mec1322_read_memmap(uint16_t addr, uint8_t *b);
int mec1322_write_memmap(uint8_t b, uint16_t addr);

struct mec1322_memmap {
	union {
		struct __packed {
			uint8_t reserved;	/* Reserved for Host Command */
			uint16_t offset:12;	/* Offset to mem_mapped[] */
			uint16_t op:4;		/* 0 = Read, 1 = Write */
			uint8_t data;		/* Data from/to host */
		};
		uint32_t reg;
	};
};

extern const struct {
	int8_t (*inb)(uint16_t);
	uint32_t (*inl)(uint16_t);
	void (*outl)(uint32_t, uint16_t);
	int (*usleep)(unsigned int);
} mec1322_host_func;

/*
 * Poll for MEC1322 status.
 *  bitmask : bits to check
 *  bitset  : check if bits are: 0 for cleared / 1 for set
 *  Returns -1 if timeout
 */
static int wait_for_mec1322(uint16_t bitmask, uint8_t bitset)
{
	int counter;
	uint16_t status;

	counter = MEC1322_MEMMAP_DELAY;
	do {
		status = mec1322_host_func.inb(MEC1322_MEMMAP_STATUS) & bitmask;
		if (bitset) {
			/* Check for bit set */
			if (status == bitmask)
				return 0;
		} else {
			/* Check for bit clear */
			if (status == 0)
				return 0;

			/*
			 * Special case for OBF which requires reading
			 * reading DATA register in order for to clear.
			 */
			if (bitmask == MEC1322_MEMMAP_OBF)
				mec1322_host_func.inl(MEC1322_MEMMAP_DATA+0x3);
		}
		mec1322_host_func.usleep(5);
		counter -= 5;
	} while (counter > 0);

	/* Timeout */
	return -1;
}

/*
 * Acquire access to MEC1322
 */
static int acquire_mec1322(void)
{
	/*
	 * Clear OBF since it should not be set
	 */
	if (wait_for_mec1322(MEC1322_MEMMAP_OBF, 0))
		return -1;

	/*
	 * Wait for MEC1322 to be ready
	 */
	return wait_for_mec1322((MEC1322_MEMMAP_IBF | MEC1322_MEMMAP_BUSY), 0);
}

int mec1322_read_memmap(uint16_t addr, uint8_t *b)
{
	struct mec1322_memmap memmap;

	/* Acquire access to MEC1322 */
	if (acquire_mec1322())
		return -1;

	/* Initialize Data */
	memmap.offset = addr - MEC1322_MEMMAP_ADJUST_OFFSET;
	memmap.op = MEC1322_MEMMAP_READ_OP;

	/* Send the data */
	mec1322_host_func.outl(memmap.reg, MEC1322_MEMMAP_DATA);

	/* Wait for reply from MEC1322 */
	if (wait_for_mec1322(MEC1322_MEMMAP_OBF, 1))
		return -1;

	/* Read the data */
	memmap.reg = mec1322_host_func.inl(MEC1322_MEMMAP_DATA);
	*b = memmap.data;

	/* Wait for MEC1322 to finish */
	if (acquire_mec1322())
		return -1;

	return 0;
}

int mec1322_write_memmap(uint8_t b, uint16_t addr)
{
	struct mec1322_memmap memmap;

	/* Acquire access to MEC1322 */
	if (acquire_mec1322())
		return -1;

	/* Initialize Data */
	memmap.offset = addr - MEC1322_MEMMAP_ADJUST_OFFSET;
	memmap.op = MEC1322_MEMMAP_WRITE_OP;
	memmap.data = b;

	/* Send the data */
	mec1322_host_func.outl(memmap.reg, MEC1322_MEMMAP_DATA);

	/* Wait for MEC1322 to finish */
	if (acquire_mec1322())
		return -1;

	return 0;
}
