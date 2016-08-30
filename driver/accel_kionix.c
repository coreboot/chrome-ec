/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Kionix Accelerometer driver for Chrome EC
 *
 * Supported: KX022, KXCJ9
 */

#include "accelgyro.h"
#include "common.h"
#include "console.h"
#include "driver/accel_kionix.h"
#include "driver/accel_kx022.h"
#include "driver/accel_kxcj9.h"
#include "i2c.h"
#include "math_util.h"
#include "task.h"
#include "util.h"

#define CPUTS(outstr) cputs(CC_ACCEL, outstr)
#define CPRINTF(format, args...) cprintf(CC_ACCEL, format, ## args)

/* Number of times to attempt to enable sensor before giving up. */
#define SENSOR_ENABLE_ATTEMPTS 3

#if defined(CONFIG_ACCEL_KXCJ9) && !defined(CONFIG_ACCEL_KX022)
#define V(s_) 1
#elif defined(CONFIG_ACCEL_KX022) && !defined(CONFIG_ACCEL_KXCJ9)
#define V(s_) 0
#else
#define V(s_) ((s_)->chip == MOTIONSENSE_CHIP_KXCJ9)
#endif

/**
 * Read register from accelerometer.
 */
static int raw_read8(const int addr, const int reg, int *data_ptr)
{
	return i2c_read8(I2C_PORT_ACCEL, addr, reg, data_ptr);
}

/**
 * Write register from accelerometer.
 */
static int raw_write8(const int addr, const int reg, int data)
{
	return i2c_write8(I2C_PORT_ACCEL, addr, reg, data);
}

/**
 * Disable sensor by taking it out of operating mode. When disabled, the
 * acceleration data does not change.
 *
 * Note: This is intended to be called in a pair with enable_sensor().
 *
 * @param s Pointer to motion sensor data
 * @param reg_val Pointer to location to store control register after disabling
 *
 * @return EC_SUCCESS if successful, EC_ERROR_* otherwise
 */
static int disable_sensor(const struct motion_sensor_t *s, int *reg_val)
{
	int i, ret, reg, pc1_field;

	reg = KIONIX_CTRL1_REG(V(s));
	pc1_field = KIONIX_PC1_FIELD(V(s));

	/*
	 * Read the current state of the control register
	 * so that we can restore it later.
	 */
	for (i = 0; i < SENSOR_ENABLE_ATTEMPTS; i++) {
		ret = raw_read8(s->addr, reg, reg_val);
		if (ret != EC_SUCCESS)
			continue;

		*reg_val &= ~pc1_field;

		ret = raw_write8(s->addr, reg, *reg_val);
		if (ret == EC_SUCCESS)
			break;
	}
	return ret;
}

/**
 * Enable sensor by placing it in operating mode.
 *
 * Note: This is intended to be called in a pair with disable_sensor().
 *
 * @param s Pointer to motion sensor data
 * @param reg_val Value of the control register to write to sensor
 *
 * @return EC_SUCCESS if successful, EC_ERROR_* otherwise
 */
static int enable_sensor(const struct motion_sensor_t *s, int reg_val)
{
	int i, ret, reg, pc1_field;

	reg = KIONIX_CTRL1_REG(V(s));
	pc1_field = KIONIX_PC1_FIELD(V(s));

	for (i = 0; i < SENSOR_ENABLE_ATTEMPTS; i++) {
		ret = raw_read8(s->addr, reg, &reg_val);
		if (ret != EC_SUCCESS)
			continue;

		/* Enable accelerometer based on reg_val value. */
		ret = raw_write8(s->addr, reg,
				reg_val | pc1_field);

		/* On first success, we are done. */
		if (ret == EC_SUCCESS)
			break;
	}
	return ret;
}

/**
 * Set a register value.
 *
 * @param s Pointer to motion sensor data
 * @param reg     Register to write to
 * @param reg_val Value of the control register to write to sensor
 * @param field   Bitfield to modify.
 *
 * @return EC_SUCCESS if successful, EC_ERROR_* otherwise
 */
static int set_value(const struct motion_sensor_t *s, int reg, int val,
		     int field)
{
	int ret, reg_val_new, reg_val;

	/* Disable the sensor to allow for changing of critical parameters. */
	mutex_lock(s->mutex);
	ret = disable_sensor(s, &reg_val);
	if (ret != EC_SUCCESS) {
		mutex_unlock(s->mutex);
		return ret;
	}

	/* Determine new value of control reg and attempt to write it. */
	reg_val_new = (reg_val & ~field) | val;
	ret = raw_write8(s->addr, reg, reg_val_new);

	/* If successfully written, then save the range. */
	if (ret == EC_SUCCESS)
		/* Re-enable the sensor. */
		ret = enable_sensor(s, reg_val_new);

	mutex_unlock(s->mutex);
	return ret;
}

static int set_range(const struct motion_sensor_t *s, int range, int rnd)
{
	int reg, range_field, range_val;

	range_field = KIONIX_RANGE_FIELD(V(s));
	reg = KIONIX_CTRL1_REG(V(s));
	if (V(s))
		range_val = KXCJ9_GSEL_8G;
	else
		range_val = KX022_GSEL_8G;
	return set_value(s, reg, range_val, range_field);
}

static int get_range(const struct motion_sensor_t *s)
{
	return 8;
}

static int set_resolution(const struct motion_sensor_t *s, int res, int rnd)
{
	int reg, res_field, res_val;

	if (V(s))
		res_val = KXCJ9_RES_12BIT;
	else
		res_val = KX022_RES_16BIT;
	res_field = KIONIX_RES_FIELD(V(s));
	reg = KIONIX_CTRL1_REG(V(s));

	return set_value(s, reg, res_val, res_field);
}

static int get_resolution(const struct motion_sensor_t *s)
{
	if (V(s))
		return 12;
	else
		return 16;
}

static int set_data_rate(const struct motion_sensor_t *s, int rate, int rnd)
{
	int ret, reg, odr_field, odr_val, real_rate;
	struct kionix_accel_data *data = s->drv_data;

	/* Find ODR from rate between 12.5Hz and 1600Hz */
	if (rate <= 12500) {
		/*
		 * On minnie, EC is polling at 10Hz, so that the lowest we
		 * will got
		 */
		odr_val = KX_OSA_12_50HZ;
	} else {
		odr_val = 31 - __builtin_clz(rate / 12500);
	}
	real_rate = 12500 << odr_val;
	if (odr_val < KX_OSA_1600HZ && real_rate < rate && rnd) {
		odr_val++;
		real_rate *= 2;
	}

	reg = KIONIX_ODR_REG(V(s));
	odr_field = KIONIX_ODR_FIELD(V(s));
	ret = set_value(s, reg, odr_val, odr_field);
	if (ret == EC_SUCCESS)
		data->sensor_datarate = real_rate;
	return ret;
}

static int get_data_rate(const struct motion_sensor_t *s)
{
	struct kionix_accel_data *data = s->drv_data;
	return data->sensor_datarate;
}

static int set_offset(const struct motion_sensor_t *s, const int16_t *offset,
		      int16_t temp)
{
	/* temperature is ignored */
	struct kionix_accel_data *data = s->drv_data;
	data->offset[X] = offset[X];
	data->offset[Y] = offset[Y];
	data->offset[Z] = offset[Z];
	return EC_SUCCESS;
}

static int get_offset(const struct motion_sensor_t *s, int16_t *offset,
		      int16_t *temp)
{
	struct kionix_accel_data *data = s->drv_data;
	offset[X] = data->offset[X];
	offset[Y] = data->offset[Y];
	offset[Z] = data->offset[Z];
	*temp = EC_MOTION_SENSE_INVALID_CALIB_TEMP;
	return EC_SUCCESS;
}

static int read(const struct motion_sensor_t *s, vector_3_t v)
{
	uint8_t acc[6];
	uint8_t reg;
	int ret, i, range, resolution;
	struct kionix_accel_data *data = s->drv_data;

	/* Read 6 bytes starting at XOUT_L. */
	reg = KIONIX_XOUT_L(V(s));
	mutex_lock(s->mutex);
	i2c_lock(I2C_PORT_ACCEL, 1);
	ret = i2c_xfer(I2C_PORT_ACCEL, s->addr, &reg, 1, acc, 6,
		       I2C_XFER_SINGLE);
	i2c_lock(I2C_PORT_ACCEL, 0);
	mutex_unlock(s->mutex);

	if (ret != EC_SUCCESS)
		return ret;

	/*
	 * Convert acceleration to a signed 16-bit number. Note, based on
	 * the order of the registers:
	 *
	 * acc[0] = XOUT_L
	 * acc[1] = XOUT_H
	 * acc[2] = YOUT_L
	 * acc[3] = YOUT_H
	 * acc[4] = ZOUT_L
	 * acc[5] = ZOUT_H
	 *
	 * Add calibration offset before returning the data.
	 */
	resolution = get_resolution(s);
	for (i = X; i <= Z; i++) {
		if (V(s)) {
			v[i] = (((int8_t)acc[i * 2 + 1]) << 4) |
				(acc[i * 2] >> 4);
			v[i] <<= 16 - resolution;
		} else {
			if (resolution == 8)
				acc[i * 2] = 0;
			v[i] = (((int8_t)acc[i * 2 + 1]) << 8) | acc[i * 2];
		}
	}
	rotate(v, *s->rot_standard_ref, v);

	/* apply offset in the device coordinates */
	range = get_range(s);
	for (i = X; i <= Z; i++)
		v[i] += (data->offset[i] << 5) / range;

	return EC_SUCCESS;
}

static int init(const struct motion_sensor_t *s)
{
	int ret, val, reg, reset_field;
	uint8_t timeout;

	reg = KIONIX_CTRL2_REG(V(s));
	reset_field = KIONIX_RESET_FIELD(V(s));

	/* Issue a software reset. */
	mutex_lock(s->mutex);

	/* Place the sensor in standby mode to make changes. */
	ret = disable_sensor(s, &val);
	if (ret != EC_SUCCESS) {
		mutex_unlock(s->mutex);
		return ret;
	}
	ret = raw_read8(s->addr, reg, &val);
	if (ret != EC_SUCCESS) {
		mutex_unlock(s->mutex);
		return ret;
	}
	val |= reset_field;
	ret = raw_write8(s->addr, reg, val);
	if (ret != EC_SUCCESS) {
		mutex_unlock(s->mutex);
		return ret;
	}

	/* The SRST will be cleared when reset is complete. */
	timeout = 0;
	do {
		msleep(1);

		ret = raw_read8(s->addr, reg, &val);
		if (ret != EC_SUCCESS) {
			mutex_unlock(s->mutex);
			return ret;
		}

		/* Reset complete. */
		if ((ret == EC_SUCCESS) && !(val & reset_field))
			break;

		/* Check for timeout. */
		if (timeout++ > 5) {
			ret = EC_ERROR_TIMEOUT;
			mutex_unlock(s->mutex);
			return ret;
		}
	} while (1);
	mutex_unlock(s->mutex);

	/* Initialize with the desired parameters. */
	ret = set_range(s, s->default_range, 1);
	if (ret != EC_SUCCESS)
		return ret;

	if (V(s))
		ret = set_resolution(s, 12, 1);
	else
		ret = set_resolution(s, 16, 1);
	if (ret != EC_SUCCESS)
		return ret;

	CPRINTF("[%T %s: Done Init type:0x%X range:%d]\n",
		s->name, s->type, get_range(s));

	mutex_unlock(s->mutex);
	return ret;
}

const struct accelgyro_drv kionix_accel_drv = {
	.init = init,
	.read = read,
	.set_range = set_range,
	.get_range = get_range,
	.set_resolution = set_resolution,
	.get_resolution = get_resolution,
	.set_data_rate = set_data_rate,
	.get_data_rate = get_data_rate,
	.set_offset = set_offset,
	.get_offset = get_offset,
};
