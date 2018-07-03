/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Tune the MP2949A IMVP8 parameters for atlas */

#include "console.h"
#include "hooks.h"
#include "i2c.h"
#include "timer.h"
#include "util.h"

#define MP2949_PAGE			0x00
#define MP2949_STORE_USER_ALL		0x15
#define MP2949_RESTORE_USER_ALL		0x16
#define MP2949_MFR_VOUT_TRIM		0x22
#define MP2949_STATUS_CML		0x7e

#define MP2949_PRODUCT_REV_DATA		0xc0
#define MP2949_CODE_REV_PROTOCOL_RB_RA	0xc1
#define MP2949_MINOFF_BLANK_TIME	0xce

#define MP2949_MFR_SLOPE_CNT_1P		0xdb
#define MP2949_MFR_TRIM_2_1_DCM		0xde

#define MP2949_MFR_FS_VBOOT		0xe5
#define MP2949_VFB_TRIM_DCLL		0xe7
#define MP2949_MFR_OCP_SET_LEVEL	0xee
#define MP2949_OC_LIMIT_ICC_MAX		0xef

static int mp2949_write8(uint8_t reg, uint8_t value)
{
	uint8_t buf[2] = { reg, value };

	return i2c_xfer(I2C_PORT_POWER, I2C_ADDR_MP2949,
			buf, sizeof(buf), NULL, 0, I2C_XFER_SINGLE);
}

static int mp2949_read8(uint8_t reg, uint8_t *value)
{
	uint8_t buf[2] = { reg };
	int status;

	status = i2c_xfer(I2C_PORT_POWER, I2C_ADDR_MP2949,
			  buf, 1, buf + 1, 1, I2C_XFER_SINGLE);
	if (status != EC_SUCCESS)
		return status;
	*value = buf[1];
	return EC_SUCCESS;
}

static void mp2949_read16(uint8_t reg, uint16_t *value)
{
	uint8_t buf[3] = { reg };

	i2c_xfer(I2C_PORT_POWER, I2C_ADDR_MP2949,
		 buf, 1, buf + 1, 2, I2C_XFER_SINGLE);
	*value = (buf[2] << 8) | buf[1];
}

static void mp2949_write16(uint8_t reg, uint16_t value)
{
	uint8_t buf[3] = { reg, value & 0xff, value >> 8 };

	i2c_xfer(I2C_PORT_POWER, I2C_ADDR_MP2949,
		 buf, sizeof(buf), NULL, 0, I2C_XFER_SINGLE);
}

struct reg_val16 {
	uint8_t reg;
	uint16_t val;
};

static int mp2949_select_page(uint8_t page)
{
	int status;

	if (page > 2)
		return EC_ERROR_INVAL;

	status = mp2949_write8(MP2949_PAGE, page);
	if (status != EC_SUCCESS) {
		ccprintf("%s: could not select page 0x%02x, error %d\n",
			 __func__, page, status);
	}
	return status;
}

static void mp2949_write_vec16(const struct reg_val16 *init_list, int count,
			       int *delta)
{
	const struct reg_val16 *reg_val;
	uint16_t oval;
	int i;

	reg_val = init_list;
	for (i = 0; i < count; ++i, ++reg_val) {
		mp2949_read16(reg_val->reg, &oval);
		if (oval == reg_val->val) {
			ccprintf("mp2949: reg 0x%02x already 0x%04x\n",
				 reg_val->reg, oval);
			continue;
		}
		ccprintf("mp2949: tuning reg 0x%02x from 0x%04x to %04x\n",
			 reg_val->reg, oval, reg_val->val);
		mp2949_write16(reg_val->reg, reg_val->val);
		*delta += 1;
	}
}

static int mp2949_store_user_all(void)
{
	const uint8_t wr = MP2949_STORE_USER_ALL;
	const uint8_t rd = MP2949_RESTORE_USER_ALL;
	int status;
	uint8_t val;

	ccprintf("%s: updating persistent settings\n", __func__);

	status = i2c_xfer(I2C_PORT_POWER, I2C_ADDR_MP2949,
			  &wr, sizeof(wr), NULL, 0, I2C_XFER_SINGLE);
	if (status != EC_SUCCESS)
		return status;
	usleep(1000 * MSEC);

	status = i2c_xfer(I2C_PORT_POWER, I2C_ADDR_MP2949,
			  &rd, sizeof(rd), NULL, 0, I2C_XFER_SINGLE);
	if (status != EC_SUCCESS)
		return status;
	usleep(2 * MSEC);

	mp2949_select_page(0);
	status = mp2949_read8(MP2949_STATUS_CML, &val);
	if ((val & 0x09) != 0) {
		ccprintf("%s: store seems to have failed with status %02x\n",
			 __func__, val);
		return status;
	}
	return EC_SUCCESS;
}

static void board_patch_rail1(int *delta)
{
	const static struct reg_val16 rail1[] = {
		{ MP2949_MFR_VOUT_TRIM,		  0x0012 },
		{ MP2949_PRODUCT_REV_DATA,	  0x0053 },
		{ MP2949_CODE_REV_PROTOCOL_RB_RA, 0x0155 },
		{ MP2949_MINOFF_BLANK_TIME,	  0xc28a },
		{ MP2949_MFR_SLOPE_CNT_1P,	  0x008c },
		{ MP2949_MFR_TRIM_2_1_DCM,	  0x20c7 },
		{ MP2949_MFR_FS_VBOOT,		  0x1400 },
	};

	if (mp2949_select_page(0x00) != EC_SUCCESS)
		return;
	mp2949_write_vec16(rail1, ARRAY_SIZE(rail1), delta);
}

static void board_patch_rail2(int *delta)
{
	const static struct reg_val16 rail2[] = {
		{ MP2949_MFR_VOUT_TRIM,     0x00ff },
		{ MP2949_MFR_TRIM_2_1_DCM,  0x20c7 },
		{ MP2949_VFB_TRIM_DCLL,     0x0028 },
		{ MP2949_MFR_OCP_SET_LEVEL, 0x0024 },
		{ MP2949_OC_LIMIT_ICC_MAX,  0xb11c },
	};

	if (mp2949_select_page(0x01) != EC_SUCCESS)
		return;
	mp2949_write_vec16(rail2, ARRAY_SIZE(rail2), delta);
}

static void board_patch_rail3(int *delta)
{
	const static struct reg_val16 rail3[] = {
		{ MP2949_MFR_VOUT_TRIM,    0x00ff },
		{ MP2949_MFR_TRIM_2_1_DCM, 0x00c7 },
	};

	if (mp2949_select_page(0x02) != EC_SUCCESS)
		return;
	mp2949_write_vec16(rail3, ARRAY_SIZE(rail3), delta);
}

/*
 *    1 ms - page0 fails
 *   10 ms - OK
 *   50 ms - OK
 *  100 ms - OK
 *  500 ms - OK
 * 1000 ms - OK
 * 2000 ms - watchdog
 */

static void mp2949_on_startup(void)
{
	int delta = 0;
	int status;

	ccprintf("%s: attempting to tune PMIC\n", __func__);
	udelay(50 * MSEC);
	i2c_lock(I2C_PORT_POWER, 1);
	board_patch_rail1(&delta);
	board_patch_rail2(&delta);
	board_patch_rail3(&delta);
	if (delta > 0) {
		status = mp2949_store_user_all();
		if (status != EC_SUCCESS)
			ccprintf("%s: could not store settings\n", __func__);
	}
	i2c_lock(I2C_PORT_POWER, 0);
}
DECLARE_HOOK(HOOK_CHIPSET_STARTUP, mp2949_on_startup,
	     HOOK_PRIO_FIRST);

/*
 * these are the before/after tuning parameters
 *
 * mp2949_on_startup: attempting to tune PMIC
 *
 * mp2949_write8: returning on count 0
 * mp2949: tuning reg 0x22 from 0x0000 to 0012
 * mp2949: tuning reg 0xc0 from 0x0050 to 0053
 * mp2949: tuning reg 0xc1 from 0x0855 to 0155
 * mp2949: tuning reg 0xce from 0xc64a to c28a
 * mp2949: tuning reg 0xdb from 0x00aa to 008c
 * mp2949: tuning reg 0xde from 0x2128 to 20c7
 * mp2949: tuning reg 0xe5 from 0x1000 to 1400
 *
 * mp2949_write8: returning on count 0
 * mp2949: tuning reg 0x22 from 0x0011 to 00ff
 * mp2949: tuning reg 0xde from 0x2107 to 20c7
 * mp2949: tuning reg 0xe7 from 0x2230 to 0028
 * mp2949: tuning reg 0xee from 0x001d to 0024
 * mp2949: tuning reg 0xef from 0xab18 to b11c
 *
 * mp2949_write8: returning on count 0
 * mp2949: tuning reg 0x22 from 0x00ee to 00ff
 * mp2949: tuning reg 0xde from 0x0128 to 00c7
 *
 */
