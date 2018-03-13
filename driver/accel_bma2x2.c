/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Bosch Accelerometer driver for Chrome EC
 *
 * Supported: BMA255
 */

#include "accelgyro.h"
#include "common.h"
#include "console.h"
#include "driver/accel_bma2x2.h"
#include "i2c.h"
#include "math_util.h"
#include "spi.h"
#include "task.h"
#include "util.h"

#define CPUTS(outstr) cputs(CC_ACCEL, outstr)
#define CPRINTF(format, args...) cprintf(CC_ACCEL, format, ## args)

/* Number of times to attempt to enable sensor before giving up. */
#define SENSOR_ENABLE_ATTEMPTS 5

/**
 * Read register from accelerometer.
 */
static inline int raw_read8(const int port, const int addr, const int reg,
			    int *data_ptr)
{
	return i2c_read8(port, addr, reg, data_ptr);
}

/**
 * Write register from accelerometer.
 */
static inline int raw_write8(const int port, const int addr, const int reg,
			     int data)
{
	return i2c_write8(port, addr, reg, data);
}

static int raw_read_multi(const int port, int addr, uint8_t reg,
			  uint8_t *rxdata, int rxlen)
{
	int rv;

	i2c_lock(port, 1);
	rv = i2c_xfer(port, addr, &reg, 1, rxdata, rxlen,
		      I2C_XFER_SINGLE);
	i2c_lock(port, 0);

	return rv;
}

static int set_range(const struct motion_sensor_t *s, int range, int rnd)
{
	int ret,  range_val, reg_val, range_reg_val;
	struct bma2x2_accel_data *data = s->drv_data;

	range_val = BMA2x2_RANGE_TO_REG(range);
	if ((BMA2x2_RANGE_TO_REG(range_val) < range) && rnd)
		range_val = BMA2x2_RANGE_TO_REG(range * 2);

	mutex_lock(s->mutex);

	/* Determine the new value of control reg and attempt to write it. */
	ret = raw_read8(s->port, s->addr, BMA2x2_RANGE_SELECT_ADDR,
			&range_reg_val);
	if (ret != EC_SUCCESS) {
		mutex_unlock(s->mutex);
		return ret;
	}
	reg_val = (range_reg_val & ~BMA2x2_RANGE_SELECT_MSK) | range_val;
	ret = raw_write8(s->port, s->addr, BMA2x2_RANGE_SELECT_ADDR, reg_val);

	/* If successfully written, then save the range. */
	if (ret == EC_SUCCESS)
		data->sensor_range = BMA2x2_REG_TO_RANGE(range_val);

	mutex_unlock(s->mutex);

	return ret;
}

static int get_range(const struct motion_sensor_t *s)
{
	struct bma2x2_accel_data *data = s->drv_data;

	return data->sensor_range;
}

static int get_resolution(const struct motion_sensor_t *s)
{
	return BMA2x2_RESOLUTION;
}

static int set_data_rate(const struct motion_sensor_t *s, int rate, int rnd)
{
	int ret, odr_val, odr_reg_val, reg_val;
	struct bma2x2_accel_data *data = s->drv_data;

	odr_val = BMA2x2_BW_TO_REG(rate);
	if ((BMA2x2_REG_TO_BW(odr_val) < rate) && rnd)
		odr_val = BMA2x2_BW_TO_REG(rate * 2);

	mutex_lock(s->mutex);

	/* Determine the new value of control reg and attempt to write it. */
	ret = raw_read8(s->port, s->addr, BMA2x2_BW_SELECT_ADDR, &odr_reg_val);
	if (ret != EC_SUCCESS) {
		mutex_unlock(s->mutex);
		return ret;
	}
	reg_val = (odr_reg_val & ~BMA2x2_BW_MSK) | odr_val;
	/* Set output data rate. */
	ret = raw_write8(s->port, s->addr, BMA2x2_BW_SELECT_ADDR, reg_val);

	/* If successfully written, then save the new data rate. */
	if (ret == EC_SUCCESS)
		data->sensor_datarate = BMA2x2_REG_TO_BW(odr_val);

	mutex_unlock(s->mutex);
	return ret;
}

static int get_data_rate(const struct motion_sensor_t *s)
{
	struct bma2x2_accel_data *data = s->drv_data;

	return data->sensor_datarate;
}

static int set_offset(const struct motion_sensor_t *s, const int16_t *offset,
		      int16_t temp)
{
	/* temperature is ignored */
	struct bma2x2_accel_data *data = s->drv_data;

	data->offset[X] = offset[X];
	data->offset[Y] = offset[Y];
	data->offset[Z] = offset[Z];

	return EC_SUCCESS;
}

static int get_offset(const struct motion_sensor_t *s, int16_t *offset,
		      int16_t *temp)
{
	struct bma2x2_accel_data *data = s->drv_data;

	offset[X] = data->offset[X];
	offset[Y] = data->offset[Y];
	offset[Z] = data->offset[Z];
	*temp = EC_MOTION_SENSE_INVALID_CALIB_TEMP;

	return EC_SUCCESS;
}

static int read(const struct motion_sensor_t *s, vector_3_t v)
{
	uint8_t acc[6];
	int ret, i, range;
	struct bma2x2_accel_data *data = s->drv_data;

	/* Read 6 bytes starting at X_AXIS_LSB. */
	mutex_lock(s->mutex);
	ret = raw_read_multi(s->port, s->addr, BMA2x2_X_AXIS_LSB_ADDR, acc, 6);
	mutex_unlock(s->mutex);

	if (ret != EC_SUCCESS)
		return ret;

	/*
	 * Convert acceleration to a signed 16-bit number. Note, based on
	 * the order of the registers:
	 *
	 * acc[0] = X_AXIS_LSB -> bit 7~4 for value, bit 0 for new data bit
	 * acc[1] = X_AXIS_MSB
	 * acc[2] = Y_AXIS_LSB -> bit 7~4 for value, bit 0 for new data bit
	 * acc[3] = Y_AXIS_MSB
	 * acc[4] = Z_AXIS_LSB -> bit 7~4 for value, bit 0 for new data bit
	 * acc[5] = Z_AXIS_MSB
	 *
	 * Add calibration offset before returning the data.
	 */
	for (i = X; i <= Z; i++)
		v[i] = (((int8_t)acc[i * 2 + 1]) << 8) | (acc[i * 2] & 0xf0);
	rotate(v, *s->rot_standard_ref, v);

	/* apply offset in the device coordinates */
	range = get_range(s);
	for (i = X; i <= Z; i++)
		v[i] += (data->offset[i] << 5) / range;

	return EC_SUCCESS;
}

static int init(const struct motion_sensor_t *s)
{
	int ret = 0, tries = 0, val, reg, reset_field;

	ret = raw_read8(s->port, s->addr, BMA2x2_CHIP_ID_ADDR, &val);
	if (ret)
		return EC_ERROR_UNKNOWN;

	if (val != BMA255_CHIP_ID_MAJOR)
		return EC_ERROR_ACCESS_DENIED;

	/* Reset the chip to be in a good state */
	reg = BMA2x2_RST_ADDR;
	reset_field = BMA2x2_CMD_SOFT_RESET;

	mutex_lock(s->mutex);

	ret = raw_read8(s->port, s->addr, reg, &val);
	if (ret != EC_SUCCESS) {
		mutex_unlock(s->mutex);
		return ret;
	}
	val |= reset_field;
	ret = raw_write8(s->port, s->addr, reg, val);
	if (ret != EC_SUCCESS) {
		mutex_unlock(s->mutex);
		return ret;
	}

	/* The SRST will be cleared when reset is complete. */
	do {
		ret = raw_read8(s->port, s->addr, reg, &val);

		/* Reset complete. */
		if ((ret == EC_SUCCESS) && !(val & reset_field))
			break;

		/* Check for tires. */
		if (tries++ > SENSOR_ENABLE_ATTEMPTS) {
			ret = EC_ERROR_TIMEOUT;
			mutex_unlock(s->mutex);
			return ret;
		}
		msleep(1);
	} while (1);
	mutex_unlock(s->mutex);

	return sensor_init_done(s);
}

const struct accelgyro_drv bma2x2_accel_drv = {
	.init = init,
	.read = read,
	.set_range = set_range,
	.get_range = get_range,
	.get_resolution = get_resolution,
	.set_data_rate = set_data_rate,
	.get_data_rate = get_data_rate,
	.set_offset = set_offset,
	.get_offset = get_offset,
	.perform_calib = NULL,
};
