/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "adc.h"
#include "adc_chip.h"
#include "als.h"
#include "backlight.h"
#include "button.h"
#include "charge_manager.h"
#include "charge_state.h"
#include "charger.h"
#include "chipset.h"
#include "common.h"
#include "console.h"
#include "ec_commands.h"
#include "driver/accel_bma2x2.h"
#include "driver/accel_kionix.h"
#include "driver/accel_kx022.h"
#include "driver/accelgyro_bmi160.h"
#include "driver/als_opt3001.h"
#include "driver/baro_bmp280.h"
#include "driver/charger/bd9995x.h"
#include "driver/tcpm/fusb302.h"
#include "extpower.h"
#include "gpio.h"
#include "hooks.h"
#include "host_command.h"
#include "i2c.h"
#include "keyboard_scan.h"
#include "lid_switch.h"
#include "power.h"
#include "power_button.h"
#include "pwm.h"
#include "pwm_chip.h"
#include "registers.h"
#include "shi_chip.h"
#include "spi.h"
#include "switch.h"
#include "system.h"
#include "task.h"
#include "tcpm.h"
#include "timer.h"
#include "thermal.h"
#include "usb_charge.h"
#include "usb_mux.h"
#include "usb_pd_tcpm.h"
#include "util.h"

#define CPRINTS(format, args...) cprints(CC_USBCHARGE, format, ## args)
#define CPRINTF(format, args...) cprintf(CC_USBCHARGE, format, ## args)

static void tcpc_alert_event(enum gpio_signal signal)
{
#ifdef HAS_TASK_PDCMD
	/* Exchange status with TCPCs */
	host_command_pd_send_status(PD_CHARGE_NO_CHANGE);
#endif
}

static void overtemp_interrupt(enum gpio_signal signal)
{
	CPRINTS("AP wants shutdown");
	chipset_force_shutdown();
}

static void warm_reset_request_interrupt(enum gpio_signal signal)
{
	CPRINTS("AP wants warm reset");
	chipset_reset();
}

#include "gpio_list.h"

/******************************************************************************/
/* ADC channels. Must be in the exactly same order as in enum adc_channel. */
const struct adc_t adc_channels[] = {
	[ADC_BOARD_ID] = {
		"BOARD_ID", NPCX_ADC_CH0, ADC_MAX_VOLT, ADC_READ_MAX+1, 0 },
	[ADC_PP900_AP] = {
		"PP900_AP", NPCX_ADC_CH1, ADC_MAX_VOLT, ADC_READ_MAX+1, 0 },
	[ADC_PP1200_LPDDR] = {
		"PP1200_LPDDR", NPCX_ADC_CH2, ADC_MAX_VOLT, ADC_READ_MAX+1, 0 },
	[ADC_PPVAR_CLOGIC] = {
		"PPVAR_CLOGIC",
		NPCX_ADC_CH3, ADC_MAX_VOLT, ADC_READ_MAX+1, 0 },
	[ADC_PPVAR_LOGIC] = {
		"PPVAR_LOGIC", NPCX_ADC_CH4, ADC_MAX_VOLT, ADC_READ_MAX+1, 0 },
};
BUILD_ASSERT(ARRAY_SIZE(adc_channels) == ADC_CH_COUNT);

/******************************************************************************/
/* PWM channels. Must be in the exactly same order as in enum pwm_channel. */
const struct pwm_t pwm_channels[] = {
#ifdef BOARD_KEVIN
	[PWM_CH_LED_GREEN] = { 0, PWM_CONFIG_DSLEEP, 100 },
#endif
#ifdef BOARD_KEVIN
	[PWM_CH_DISPLIGHT] = { 2, 0, 210 },
#else
	/* ArcticSand part on Gru requires >= 2.6KHz */
	[PWM_CH_DISPLIGHT] = { 2, 0, 2600 },
#endif
	[PWM_CH_LED_RED] =   { 3, PWM_CONFIG_DSLEEP, 100 },
#ifdef BOARD_KEVIN
	[PWM_CH_LED_BLUE] =  { 4, PWM_CONFIG_DSLEEP, 100 },
#endif
};
BUILD_ASSERT(ARRAY_SIZE(pwm_channels) == PWM_CH_COUNT);

/******************************************************************************/
/* I2C ports */
const struct i2c_port_t i2c_ports[] = {
	{"tcpc0",   NPCX_I2C_PORT0_0, 1000, GPIO_I2C0_SCL0, GPIO_I2C0_SDA0},
	{"tcpc1",   NPCX_I2C_PORT0_1, 1000, GPIO_I2C0_SCL1, GPIO_I2C0_SDA1},
	{"sensors", NPCX_I2C_PORT1,    400, GPIO_I2C1_SCL,  GPIO_I2C1_SDA},
	{"charger", NPCX_I2C_PORT2,    400, GPIO_I2C2_SCL,  GPIO_I2C2_SDA},
	{"battery", NPCX_I2C_PORT3,    100, GPIO_I2C3_SCL,  GPIO_I2C3_SDA},
};
const unsigned int i2c_ports_used = ARRAY_SIZE(i2c_ports);

/* power signal list.  Must match order of enum power_signal. */
const struct power_signal_info power_signal_list[] = {
	{GPIO_PP5000_PG,         POWER_SIGNAL_ACTIVE_HIGH, "PP5000_PWR_GOOD"},
	{GPIO_TPS65261_PG,       POWER_SIGNAL_ACTIVE_HIGH, "SYS_PWR_GOOD"},
	{GPIO_AP_CORE_PG,        POWER_SIGNAL_ACTIVE_HIGH, "AP_PWR_GOOD"},
	{GPIO_AP_EC_S3_S0_L,     POWER_SIGNAL_ACTIVE_LOW, "SUSPEND_DEASSERTED"},
};
BUILD_ASSERT(ARRAY_SIZE(power_signal_list) == POWER_SIGNAL_COUNT);

/******************************************************************************/
/* SPI devices */
const struct spi_device_t spi_devices[] = {
	{ CONFIG_SPI_ACCEL_PORT, 1, GPIO_SPI_SENSOR_CS_L }
};
const unsigned int spi_devices_used = ARRAY_SIZE(spi_devices);

/******************************************************************************/
/* Wake-up pins for hibernate */
const enum gpio_signal hibernate_wake_pins[] = {
	GPIO_POWER_BUTTON_L, GPIO_CHARGER_INT_L, GPIO_LID_OPEN
};
const int hibernate_wake_pins_used = ARRAY_SIZE(hibernate_wake_pins);

/******************************************************************************/
/* Keyboard scan setting */
struct keyboard_scan_config keyscan_config = {
#ifdef BOARD_KEVIN
	.output_settle_us = 40,
#else
	/* Extra delay when KSO2 is tied to cr50 */
	.output_settle_us = 60,
#endif
	.debounce_down_us = 6 * MSEC,
	.debounce_up_us = 30 * MSEC,
	.scan_period_us = 1500,
	.min_post_scan_delay_us = 1000,
	.poll_timeout_us = SECOND,
	.actual_key_mask = {
		0x14, 0xff, 0xff, 0xff, 0xff, 0xf5, 0xff,
		0xa4, 0xff, 0xfe, 0x55, 0xfa, 0xc8  /* full set with lock key */
	},
};

const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
	{I2C_PORT_TCPC0, FUSB302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv},
	{I2C_PORT_TCPC1, FUSB302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv},
};

struct usb_mux usb_muxes[CONFIG_USB_PD_PORT_COUNT] = {
	{
		.port_addr = 0,
		.driver = &virtual_usb_mux_driver,
		.hpd_update = &virtual_hpd_update,
	},
	{
		.port_addr = 1,
		.driver = &virtual_usb_mux_driver,
		.hpd_update = &virtual_hpd_update,
	},
};

void board_reset_pd_mcu(void)
{
}

uint16_t tcpc_get_alert_status(void)
{
	uint16_t status = 0;

	if (!gpio_get_level(GPIO_USB_C0_PD_INT_L))
		status |= PD_STATUS_TCPC_ALERT_0;
	if (!gpio_get_level(GPIO_USB_C1_PD_INT_L))
		status |= PD_STATUS_TCPC_ALERT_1;

	return status;
}

int board_set_active_charge_port(int charge_port)
{
	enum bd9995x_charge_port bd9995x_port;
	int bd9995x_port_select = 1;

	switch (charge_port) {
	case 0: case 1:
		/* Don't charge from a source port */
		if (board_vbus_source_enabled(charge_port))
			return -1;

		bd9995x_port = charge_port;
		break;
	case CHARGE_PORT_NONE:
		bd9995x_port_select = 0;
		bd9995x_port = BD9995X_CHARGE_PORT_BOTH;
		break;
	default:
		panic("Invalid charge port\n");
		break;
	}

	CPRINTS("New chg p%d", charge_port);

	return bd9995x_select_input_port(bd9995x_port, bd9995x_port_select);
}

void board_set_charge_limit(int port, int supplier, int charge_ma,
			    int max_ma, int charge_mv)
{
	charge_set_input_current_limit(MAX(charge_ma,
			       CONFIG_CHARGER_INPUT_CURRENT), charge_mv);
}

int extpower_is_present(void)
{
	int port;
	int p0_src = board_vbus_source_enabled(0);
	int p1_src = board_vbus_source_enabled(1);

	/*
	 * The charger will indicate VBUS presence if we're sourcing 5V,
	 * so exclude such ports.
	 */
	if (p0_src && p1_src)
		return 0;
	else if (!p0_src && !p1_src)
		port = BD9995X_CHARGE_PORT_BOTH;
	else
		port = p0_src;

	return bd9995x_is_vbus_provided(port);
}

int pd_snk_is_vbus_provided(int port)
{
	if (port != 0 && port != 1)
		panic("Invalid charge port\n");

	return bd9995x_is_vbus_provided(port);
}

static void board_spi_enable(void)
{
	spi_enable(CONFIG_SPI_ACCEL_PORT, 1);
}
DECLARE_HOOK(HOOK_CHIPSET_STARTUP,
	     board_spi_enable,
	     MOTION_SENSE_HOOK_PRIO - 1);

static void board_spi_disable(void)
{
	spi_enable(CONFIG_SPI_ACCEL_PORT, 0);
}
DECLARE_HOOK(HOOK_CHIPSET_SHUTDOWN,
	     board_spi_disable,
	     MOTION_SENSE_HOOK_PRIO + 1);

/*
 * Reset our charger IC on power-on. This will briefly cut extpower to the
 * system, so skip the reset if our battery can't provide sufficient charge
 * to briefly power the system.
 * TODO(shawnn): Move to common code.
 */
static void board_reset_charger(void)
{
	int bat_pct = 0;

	if (!system_jumped_to_this_image() &&
	    battery_is_present() == BP_YES &&
	    battery_get_disconnect_state() != BATTERY_DISCONNECTED) {
		if (battery_state_of_charge_abs(&bat_pct) ||
		    bat_pct < CONFIG_CHARGER_MIN_BAT_PCT_FOR_POWER_ON)
			return;
		charger_set_mode(CHARGE_FLAG_POR_RESET);
	}
}
DECLARE_HOOK(HOOK_INIT, board_reset_charger, HOOK_PRIO_INIT_EXTPOWER - 1);

static void board_init(void)
{
	/* Enable TCPC alert interrupts */
	gpio_enable_interrupt(GPIO_USB_C0_PD_INT_L);
	gpio_enable_interrupt(GPIO_USB_C1_PD_INT_L);

	/* Enable charger interrupt for BC1.2 detection on attach / detach */
	gpio_enable_interrupt(GPIO_CHARGER_INT_L);

	/* Enable reboot / shutdown control inputs from AP */
	gpio_enable_interrupt(GPIO_WARM_RESET_REQ);
	gpio_enable_interrupt(GPIO_AP_OVERTEMP);

	/* Enable interrupts from BMI160 sensor. */
	gpio_enable_interrupt(GPIO_BASE_SIXAXIS_INT_L);

	/* Sensor Init */
	if (system_jumped_to_this_image() && chipset_in_state(CHIPSET_STATE_ON))
		board_spi_enable();
}
DECLARE_HOOK(HOOK_INIT, board_init, HOOK_PRIO_DEFAULT);

void board_hibernate(void)
{
	int i;
	int rv;

	/*
	 * Disable the power enables for the TCPCs since we're going into
	 * hibernate.  The charger VBUS interrupt will wake us up and reset the
	 * EC.  Upon init, we'll reinitialize the TCPCs to be at full power.
	 */
	CPRINTS("Set TCPCs to low power");
	for (i = 0; i < CONFIG_USB_PD_PORT_COUNT; i++) {
		rv = tcpc_write(i, TCPC_REG_POWER, TCPC_REG_POWER_PWR_LOW);
		if (rv)
			CPRINTS("Error setting TCPC %d", i);
	}

	cflush();
}

enum kevin_board_version {
	BOARD_VERSION_UNKNOWN = -1,
	BOARD_VERSION_REV0 = 0,
	BOARD_VERSION_REV1 = 1,
	BOARD_VERSION_REV2 = 2,
	BOARD_VERSION_REV3 = 3,
	BOARD_VERSION_REV4 = 4,
	BOARD_VERSION_REV5 = 5,
	BOARD_VERSION_REV6 = 6,
	BOARD_VERSION_REV7 = 7,
	BOARD_VERSION_REV8 = 8,
	BOARD_VERSION_REV9 = 9,
	BOARD_VERSION_REV10 = 10,
	BOARD_VERSION_REV11 = 11,
	BOARD_VERSION_REV12 = 12,
	BOARD_VERSION_REV13 = 13,
	BOARD_VERSION_REV14 = 14,
	BOARD_VERSION_REV15 = 15,
	BOARD_VERSION_COUNT,
};

struct {
	enum kevin_board_version version;
	int expect_mv;
} const kevin_boards[] = {
	{ BOARD_VERSION_REV0, 109 },   /* 51.1K , 2.2K(gru 3.3K) ohm */
	{ BOARD_VERSION_REV1, 211 },   /* 51.1k , 6.8K ohm */
	{ BOARD_VERSION_REV2, 319 },   /* 51.1K , 11K ohm */
	{ BOARD_VERSION_REV3, 427 },   /* 56K   , 17.4K ohm */
	{ BOARD_VERSION_REV4, 542 },   /* 51.1K , 22K ohm */
	{ BOARD_VERSION_REV5, 666 },   /* 51.1K , 30K ohm */
	{ BOARD_VERSION_REV6, 781 },   /* 51.1K , 39.2K ohm */
	{ BOARD_VERSION_REV7, 900 },   /* 56K   , 56K ohm */
	{ BOARD_VERSION_REV8, 1023 },  /* 47K   , 61.9K ohm */
	{ BOARD_VERSION_REV9, 1137 },  /* 47K   , 80.6K ohm */
	{ BOARD_VERSION_REV10, 1240 }, /* 56K   , 124K ohm */
	{ BOARD_VERSION_REV11, 1343 }, /* 51.1K , 150K ohm */
	{ BOARD_VERSION_REV12, 1457 }, /* 47K   , 200K ohm */
	{ BOARD_VERSION_REV13, 1576 }, /* 47K   , 330K ohm */
	{ BOARD_VERSION_REV14, 1684 }, /* 47K   , 680K ohm */
	{ BOARD_VERSION_REV15, 1800 }, /* 56K   , NC */
};
BUILD_ASSERT(ARRAY_SIZE(kevin_boards) == BOARD_VERSION_COUNT);

#define THRESHOLD_MV 56 /* Simply assume 1800/16/2 */

int board_get_version(void)
{
	static int version = BOARD_VERSION_UNKNOWN;
	int mv;
	int i;

	if (version != BOARD_VERSION_UNKNOWN)
		return version;

	gpio_set_level(GPIO_EC_BOARD_ID_EN_L, 0);
	/* Wait to allow cap charge */
	msleep(10);
	mv = adc_read_channel(ADC_BOARD_ID);

	/* TODO(crosbug.com/p/54971): Fix failure on first ADC conversion. */
	if (mv == ADC_READ_ERROR)
		mv = adc_read_channel(ADC_BOARD_ID);

	gpio_set_level(GPIO_EC_BOARD_ID_EN_L, 1);

	for (i = 0; i < BOARD_VERSION_COUNT; ++i) {
		if (mv < kevin_boards[i].expect_mv + THRESHOLD_MV) {
			version = kevin_boards[i].version;
			break;
		}
	}

	return version;
}

/* Mutexes */
static struct mutex g_base_mutex;
static struct mutex g_lid_mutex;

static struct bmi160_drv_data_t g_bmi160_data;

#ifdef BOARD_KEVIN
/* BMA255 private data */
static struct bma2x2_accel_data g_bma255_data;

/* Matrix to rotate accelrator into standard reference frame */
const matrix_3x3_t base_standard_ref = {
	{ 0, FLOAT_TO_FP(1),  0},
	{ FLOAT_TO_FP(1),  0, 0},
	{ 0,  0, FLOAT_TO_FP(-1)}
};

const matrix_3x3_t lid_standard_ref = {
	{ 0,  FLOAT_TO_FP(1), 0},
	{ FLOAT_TO_FP(-1),  0,  0},
	{ 0,  0, FLOAT_TO_FP(1)}
};
#else
/* Matrix to rotate accelerometer into standard reference frame */
const matrix_3x3_t base_standard_ref = {
	{ FLOAT_TO_FP(-1), 0,  0},
	{ 0,  FLOAT_TO_FP(1),  0},
	{ 0,  0, FLOAT_TO_FP(-1)}
};

const matrix_3x3_t lid_standard_ref = {
	{ 0, FLOAT_TO_FP(1),  0},
	{ FLOAT_TO_FP(-1), 0, 0},
	{ 0,  0, FLOAT_TO_FP(1)}
};

static struct kionix_accel_data g_kx022_data;
static struct bmp280_drv_data_t bmp280_drv_data;

/* ALS instances. Must be in same order as enum als_id. */
struct als_t als[] = {
	/* FIXME(dhendrix): verify attenuation_factor */
	{"TI", opt3001_init, opt3001_read_lux, 5},
};
BUILD_ASSERT(ARRAY_SIZE(als) == ALS_COUNT);
#endif /* BOARD_KEVIN */

struct motion_sensor_t motion_sensors[] = {
	/*
	 * Note: bmi160: supports accelerometer and gyro sensor
	 * Requirement: accelerometer sensor must init before gyro sensor
	 * DO NOT change the order of the following table.
	 */
	[BASE_ACCEL] = {
	 .name = "Base Accel",
	 .active_mask = SENSOR_ACTIVE_S0_S3,
	 .chip = MOTIONSENSE_CHIP_BMI160,
	 .type = MOTIONSENSE_TYPE_ACCEL,
	 .location = MOTIONSENSE_LOC_BASE,
	 .drv = &bmi160_drv,
	 .mutex = &g_base_mutex,
	 .drv_data = &g_bmi160_data,
	 .port = CONFIG_SPI_ACCEL_PORT,
	 .addr = BMI160_SET_SPI_ADDRESS(CONFIG_SPI_ACCEL_PORT),
	 .rot_standard_ref = &base_standard_ref,
	 .default_range = 2,  /* g, enough for laptop. */
	 .min_frequency = BMI160_ACCEL_MIN_FREQ,
	 .max_frequency = BMI160_ACCEL_MAX_FREQ,
	 .config = {
		 /* AP: by default use EC settings */
		 [SENSOR_CONFIG_AP] = {
			 .odr = 0,
			 .ec_rate = 0,
		 },
		 /* EC use accel for angle detection */
		 [SENSOR_CONFIG_EC_S0] = {
			 .odr = 10000 | ROUND_UP_FLAG,
			 .ec_rate = 100 * MSEC,
		 },
		 /* EC use accel for angle detection */
		 [SENSOR_CONFIG_EC_S3] = {
			.odr = 10000 | ROUND_UP_FLAG,
			.ec_rate = 0,
		 },
		 /* Sensor off in S3/S5 */
		 [SENSOR_CONFIG_EC_S5] = {
			 .odr = 0,
			 .ec_rate = 0
		 },
	 },
	},
	[BASE_GYRO] = {
	 .name = "Base Gyro",
	 .active_mask = SENSOR_ACTIVE_S0_S3,
	 .chip = MOTIONSENSE_CHIP_BMI160,
	 .type = MOTIONSENSE_TYPE_GYRO,
	 .location = MOTIONSENSE_LOC_BASE,
	 .drv = &bmi160_drv,
	 .mutex = &g_base_mutex,
	 .drv_data = &g_bmi160_data,
	 .port = CONFIG_SPI_ACCEL_PORT,
	 .addr = BMI160_SET_SPI_ADDRESS(CONFIG_SPI_ACCEL_PORT),
	 .default_range = 1000, /* dps */
#ifdef BOARD_KEVIN
	 .rot_standard_ref = &base_standard_ref,
#else
	 .rot_standard_ref = NULL, /* Identity matrix. */
#endif
	 .min_frequency = BMI160_GYRO_MIN_FREQ,
	 .max_frequency = BMI160_GYRO_MAX_FREQ,
	 .config = {
		 /* AP: by default shutdown all sensors */
		 [SENSOR_CONFIG_AP] = {
			 .odr = 0,
			 .ec_rate = 0,
		 },
		 /* EC does not need in S0 */
		 [SENSOR_CONFIG_EC_S0] = {
			 .odr = 0,
			 .ec_rate = 0,
		 },
		 /* Sensor off in S3/S5 */
		 [SENSOR_CONFIG_EC_S3] = {
			 .odr = 0,
			 .ec_rate = 0,
		 },
		 /* Sensor off in S3/S5 */
		 [SENSOR_CONFIG_EC_S5] = {
			 .odr = 0,
			 .ec_rate = 0,
		 },
	 },
	},
#ifdef BOARD_KEVIN
	[LID_ACCEL] = {
	 .name = "Lid Accel",
	 .active_mask = SENSOR_ACTIVE_S0_S3,
	 .chip = MOTIONSENSE_CHIP_BMA255,
	 .type = MOTIONSENSE_TYPE_ACCEL,
	 .location = MOTIONSENSE_LOC_LID,
	 .drv = &bma2x2_accel_drv,
	 .mutex = &g_lid_mutex,
	 .drv_data = &g_bma255_data,
	 .port = I2C_PORT_ACCEL,
	 .addr = BMA2x2_I2C_ADDR1,
	 .rot_standard_ref = &lid_standard_ref,
	 .default_range = 2, /* g, enough for laptop. */
	 .min_frequency = BMA255_ACCEL_MIN_FREQ,
	 .max_frequency = BMA255_ACCEL_MAX_FREQ,
	 .config = {
		/* AP: by default use EC settings */
		[SENSOR_CONFIG_AP] = {
			.odr = 0,
			.ec_rate = 0,
		},
		/* EC use accel for angle detection */
		[SENSOR_CONFIG_EC_S0] = {
			.odr = 10000 | ROUND_UP_FLAG,
			.ec_rate = 0,
		},
		 /* EC use accel for angle detection */
		[SENSOR_CONFIG_EC_S3] = {
			.odr = 10000 | ROUND_UP_FLAG,
			.ec_rate = 0,
		},
		[SENSOR_CONFIG_EC_S5] = {
			.odr = 0,
			.ec_rate = 0,
		},
	 },
	},
#else
	[LID_ACCEL] = {
	 .name = "Lid Accel",
	 .active_mask = SENSOR_ACTIVE_S0_S3,
	 .chip = MOTIONSENSE_CHIP_KX022,
	 .type = MOTIONSENSE_TYPE_ACCEL,
	 .location = MOTIONSENSE_LOC_LID,
	 .drv = &kionix_accel_drv,
	 .mutex = &g_lid_mutex,
	 .drv_data = &g_kx022_data,
	 .port = I2C_PORT_ACCEL,
	 .addr = KX022_ADDR0,
	 .rot_standard_ref = &lid_standard_ref,
	 .default_range = 2, /* g, enough for laptop. */
	 .min_frequency = KX022_ACCEL_MIN_FREQ,
	 .max_frequency = KX022_ACCEL_MAX_FREQ,
	 .config = {
		/* AP: by default use EC settings */
		[SENSOR_CONFIG_AP] = {
			.odr = 0,
			.ec_rate = 0,
		},
		/* EC use accel for angle detection */
		[SENSOR_CONFIG_EC_S0] = {
			.odr = 10000 | ROUND_UP_FLAG,
			.ec_rate = 0,
		},
		 /* EC use accel for angle detection */
		[SENSOR_CONFIG_EC_S3] = {
			.odr = 10000 | ROUND_UP_FLAG,
			.ec_rate = 0,
		},
		[SENSOR_CONFIG_EC_S5] = {
			.odr = 0,
			.ec_rate = 0,
		},
	 },
	},
	[BASE_BARO] = {
	 .name = "Base Baro",
	 .active_mask = SENSOR_ACTIVE_S0_S3,
	 .chip = MOTIONSENSE_CHIP_BMP280,
	 .type = MOTIONSENSE_TYPE_BARO,
	 .location = MOTIONSENSE_LOC_BASE,
	 .drv = &bmp280_drv,
	 .drv_data = &bmp280_drv_data,
	 .port = I2C_PORT_BARO,
	 .addr = BMP280_I2C_ADDRESS1,
	 .default_range = 1 << 18, /*  1bit = 4 Pa, 16bit ~= 2600 hPa */
	 .min_frequency = BMP280_BARO_MIN_FREQ,
	 .max_frequency = BMP280_BARO_MAX_FREQ,
	 .config = {
		 /* AP: by default shutdown all sensors */
		 [SENSOR_CONFIG_AP] = {
			.odr = 0,
			.ec_rate = 0,
		 },
		 /* EC does not need in S0 */
		 [SENSOR_CONFIG_EC_S0] = {
			.odr = 0,
			.ec_rate = 0,
		 },
		 /* Sensor off in S3/S5 */
		 [SENSOR_CONFIG_EC_S3] = {
			.odr = 0,
			.ec_rate = 0,
		 },
		 /* Sensor off in S3/S5 */
		 [SENSOR_CONFIG_EC_S5] = {
			.odr = 0,
			.ec_rate = 0,
		 },
	 },
	},
#endif /* BOARD_KEVIN */
};
const unsigned int motion_sensor_count = ARRAY_SIZE(motion_sensors);

#ifndef TEST_BUILD
void lid_angle_peripheral_enable(int enable)
{
	keyboard_scan_enable(enable, KB_SCAN_DISABLE_LID_ANGLE);
}
#endif

#ifdef BOARD_GRU
static void usb_charge_resume(void)
{
	/* Turn on USB-A ports on as we go into S0 from S3. */
	gpio_set_level(GPIO_USB_A_EN, 1);
	gpio_set_level(GPIO_USB_A_CHARGE_EN, 1);
}
DECLARE_HOOK(HOOK_CHIPSET_RESUME, usb_charge_resume, HOOK_PRIO_DEFAULT);

static void usb_charge_shutdown(void)
{
	/* Turn off USB-A ports as we go back to S5. */
	gpio_set_level(GPIO_USB_A_CHARGE_EN, 0);
	gpio_set_level(GPIO_USB_A_EN, 0);
}
DECLARE_HOOK(HOOK_CHIPSET_SHUTDOWN, usb_charge_shutdown, HOOK_PRIO_DEFAULT);
#endif

#define PWM_DISPLIGHT_SYSJUMP_TAG 0x5044 /* "PD" */
#define PWM_HOOK_VERSION 1

static void pwm_displight_restore_state(void)
{
	const int *prev;
	int version, size;

	prev = (const int *)system_get_jump_tag(PWM_DISPLIGHT_SYSJUMP_TAG,
						&version, &size);
	if (prev && version == PWM_HOOK_VERSION && size == sizeof(*prev))
		pwm_set_raw_duty(PWM_CH_DISPLIGHT, *prev);
}
DECLARE_HOOK(HOOK_INIT, pwm_displight_restore_state, HOOK_PRIO_INIT_PWM + 1);

static void pwm_displight_preserve_state(void)
{
	int pwm_displight_duty = pwm_get_raw_duty(PWM_CH_DISPLIGHT);

	system_add_jump_tag(PWM_DISPLIGHT_SYSJUMP_TAG, PWM_HOOK_VERSION,
			    sizeof(pwm_displight_duty), &pwm_displight_duty);
}
DECLARE_HOOK(HOOK_SYSJUMP, pwm_displight_preserve_state, HOOK_PRIO_DEFAULT);

int board_allow_i2c_passthru(int port)
{
	return (port == I2C_PORT_VIRTUAL_BATTERY);
}
