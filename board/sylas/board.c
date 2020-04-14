/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Poppy board-specific configuration */

#include "adc.h"
#include "adc_chip.h"
#include "anx7447.h"
#include "board_config.h"
#include "button.h"
#include "charge_manager.h"
#include "charge_state.h"
#include "charge_ramp.h"
#include "charger.h"
#include "chipset.h"
#include "console.h"
#include "cros_board_info.h"
#include "driver/pmic_tps650x30.h"
#include "driver/accelgyro_bmi160.h"
#include "driver/accel_bma2x2.h"
#include "driver/accel_kionix.h"
#include "driver/baro_bmp280.h"
#include "driver/led/lm3509.h"
#include "driver/tcpm/ps8xxx.h"
#include "driver/tcpm/tcpci.h"
#include "driver/tcpm/tcpm.h"
#include "driver/temp_sensor/f75303.h"
#include "extpower.h"
#include "gpio.h"
#include "hooks.h"
#include "host_command.h"
#include "i2c.h"
#include "isl923x.h"
#include "keyboard_8042_sharedlib.h"
#include "keyboard_backlight.h"
#include "keyboard_config.h"
#include "keyboard_raw.h"
#include "keyboard_scan.h"
#include "lid_switch.h"
#include "math_util.h"
#include "motion_lid.h"
#include "motion_sense.h"
#include "pi3usb9281.h"
#include "power.h"
#include "power_button.h"
#include "pwm.h"
#include "pwm_chip.h"
#include "spi.h"
#include "switch.h"
#include "system.h"
#include "tablet_mode.h"
#include "task.h"
#include "temp_sensor.h"
#include "timer.h"
#include "uart.h"
#include "usb_charge.h"
#include "usb_mux.h"
#include "usb_pd.h"
#include "usb_pd_tcpm.h"
#include "util.h"
#include "espi.h"
#include "fan.h"
#include "fan_chip.h"

#define CPRINTS(format, args...) cprints(CC_USBCHARGE, format, ## args)
#define CPRINTF(format, args...) cprintf(CC_USBCHARGE, format, ## args)

#define USB_PD_PORT_PS8751	0
#define USB_PD_PORT_ANX7447	1

/******************************************************************************/
/* Physical fans. These are logically separate from pwm_channels. */

const struct fan_conf fan_conf_0 = {
	.flags = FAN_USE_RPM_MODE,
	.ch = MFT_CH_0,	/* Use MFT id to control fan */
	.pgood_gpio = -1,
	.enable_gpio = -1,
};

/* Default, Nami, Vayne */
const struct fan_rpm fan_rpm_0 = {
	.rpm_min = 3100,
	.rpm_start = 3100,
	.rpm_max = 6900,
};

/* Sona */
const struct fan_rpm fan_rpm_1 = {
	.rpm_min = 2700,
	.rpm_start = 2700,
	.rpm_max = 6000,
};

/* Pantheon */
const struct fan_rpm fan_rpm_2 = {
	.rpm_min = 2100,
	.rpm_start = 2300,
	.rpm_max = 5100,
};

/* Akali */
const struct fan_rpm fan_rpm_3 = {
	.rpm_min = 2700,
	.rpm_start = 2700,
	.rpm_max = 5500,
};

const struct fan_rpm fan_rpm_4 = {
	.rpm_min = 2400,
	.rpm_start = 2400,
	.rpm_max = 4500,
};

struct fan_t fans[FAN_CH_COUNT] = {
	[FAN_CH_0] = { .conf = &fan_conf_0, .rpm = &fan_rpm_0, },
};

/******************************************************************************/
/* MFT channels. These are logically separate from pwm_channels. */
const struct mft_t mft_channels[] = {
	[MFT_CH_0] = {NPCX_MFT_MODULE_2, TCKC_LFCLK, PWM_CH_FAN},
};
BUILD_ASSERT(ARRAY_SIZE(mft_channels) == MFT_CH_COUNT);

/* I2C port map */
const struct i2c_port_t i2c_ports[]  = {
	{"tcpc0",     NPCX_I2C_PORT0_0, 400, GPIO_I2C0_0_SCL, GPIO_I2C0_0_SDA},
	{"tcpc1",     NPCX_I2C_PORT0_1, 400, GPIO_I2C0_1_SCL, GPIO_I2C0_1_SDA},
	{"battery",   NPCX_I2C_PORT1,   100, GPIO_I2C1_SCL,   GPIO_I2C1_SDA},
	{"charger",   NPCX_I2C_PORT2,   100, GPIO_I2C2_SCL,   GPIO_I2C2_SDA},
	{"pmic",      NPCX_I2C_PORT2,   400, GPIO_I2C2_SCL,   GPIO_I2C2_SDA},
	{"accelgyro", NPCX_I2C_PORT3,   400, GPIO_I2C3_SCL,   GPIO_I2C3_SDA},
};
const unsigned int i2c_ports_used = ARRAY_SIZE(i2c_ports);

/*
 * F75303_Remote1 is near CPU, and F75303_Remote2 is near 5V power IC.
 */
const struct temp_sensor_t temp_sensors[TEMP_SENSOR_COUNT] = {
	{"F75303_Local", TEMP_SENSOR_TYPE_BOARD, f75303_get_val,
		F75303_IDX_LOCAL, 4},
	{"F75303_Remote1", TEMP_SENSOR_TYPE_CPU, f75303_get_val,
		F75303_IDX_REMOTE1, 4},
	{"F75303_Remote2", TEMP_SENSOR_TYPE_BOARD, f75303_get_val,
		F75303_IDX_REMOTE2, 4},
};

struct ec_thermal_config thermal_params[TEMP_SENSOR_COUNT];

/* Nami/Vayne Remote 1, 2 */
const static struct ec_thermal_config thermal_a = {
	.temp_host = {
		[EC_TEMP_THRESH_WARN] = 0,
		[EC_TEMP_THRESH_HIGH] = C_TO_K(75),
		[EC_TEMP_THRESH_HALT] = C_TO_K(80),
	},
	.temp_host_release = {
		[EC_TEMP_THRESH_WARN] = 0,
		[EC_TEMP_THRESH_HIGH] = C_TO_K(65),
		[EC_TEMP_THRESH_HALT] = 0,
	},
	.temp_fan_off = C_TO_K(39),
	.temp_fan_max = C_TO_K(50),
};

/* Sona Remote 1 */
const static struct ec_thermal_config thermal_b1 = {
	.temp_host = {
		[EC_TEMP_THRESH_WARN] = 0,
		[EC_TEMP_THRESH_HIGH] = C_TO_K(82),
		[EC_TEMP_THRESH_HALT] = C_TO_K(89),
	},
	.temp_host_release = {
		[EC_TEMP_THRESH_WARN] = 0,
		[EC_TEMP_THRESH_HIGH] = C_TO_K(72),
		[EC_TEMP_THRESH_HALT] = 0,
	},
	.temp_fan_off = C_TO_K(38),
	.temp_fan_max = C_TO_K(58),
};

/* Sona Remote 2 */
const static struct ec_thermal_config thermal_b2 = {
	.temp_host = {
		[EC_TEMP_THRESH_WARN] = 0,
		[EC_TEMP_THRESH_HIGH] = C_TO_K(84),
		[EC_TEMP_THRESH_HALT] = C_TO_K(91),
	},
	.temp_host_release = {
		[EC_TEMP_THRESH_WARN] = 0,
		[EC_TEMP_THRESH_HIGH] = C_TO_K(74),
		[EC_TEMP_THRESH_HALT] = 0,
	},
	.temp_fan_off = C_TO_K(40),
	.temp_fan_max = C_TO_K(60),
};

/* Pantheon Remote 1 */
const static struct ec_thermal_config thermal_c1 = {
	.temp_host = {
		[EC_TEMP_THRESH_WARN] = 0,
		[EC_TEMP_THRESH_HIGH] = C_TO_K(66),
		[EC_TEMP_THRESH_HALT] = C_TO_K(80),
	},
	.temp_host_release = {
		[EC_TEMP_THRESH_WARN] = 0,
		[EC_TEMP_THRESH_HIGH] = C_TO_K(56),
		[EC_TEMP_THRESH_HALT] = 0,
	},
	.temp_fan_off = C_TO_K(38),
	.temp_fan_max = C_TO_K(61),
};

/* Pantheon Remote 2 */
const static struct ec_thermal_config thermal_c2 = {
	.temp_host = {
		[EC_TEMP_THRESH_WARN] = 0,
		[EC_TEMP_THRESH_HIGH] = C_TO_K(74),
		[EC_TEMP_THRESH_HALT] = C_TO_K(82),
	},
	.temp_host_release = {
		[EC_TEMP_THRESH_WARN] = 0,
		[EC_TEMP_THRESH_HIGH] = C_TO_K(64),
		[EC_TEMP_THRESH_HALT] = 0,
	},
	.temp_fan_off = C_TO_K(38),
	.temp_fan_max = C_TO_K(61),
};

/* Akali Local */
const static struct ec_thermal_config thermal_d0 = {
	.temp_host = {
		[EC_TEMP_THRESH_WARN] = C_TO_K(79),
		[EC_TEMP_THRESH_HIGH] = 0,
		[EC_TEMP_THRESH_HALT] = C_TO_K(81),
	},
	.temp_host_release = {
		[EC_TEMP_THRESH_WARN] = C_TO_K(80),
		[EC_TEMP_THRESH_HIGH] = 0,
		[EC_TEMP_THRESH_HALT] = C_TO_K(82),
	},
	.temp_fan_off = C_TO_K(35),
	.temp_fan_max = C_TO_K(70),
};

/* Akali Remote 1 */
const static struct ec_thermal_config thermal_d1 = {
	.temp_host = {
		[EC_TEMP_THRESH_WARN] = C_TO_K(59),
		[EC_TEMP_THRESH_HIGH] = 0,
		[EC_TEMP_THRESH_HALT] = 0,
	},
	.temp_host_release = {
		[EC_TEMP_THRESH_WARN] = C_TO_K(60),
		[EC_TEMP_THRESH_HIGH] = 0,
		[EC_TEMP_THRESH_HALT] = 0,
	},
	.temp_fan_off = 0,
	.temp_fan_max = 0,
};

/* Akali Remote 2 */
const static struct ec_thermal_config thermal_d2 = {
	.temp_host = {
		[EC_TEMP_THRESH_WARN] = C_TO_K(59),
		[EC_TEMP_THRESH_HIGH] = 0,
		[EC_TEMP_THRESH_HALT] = 0,
	},
	.temp_host_release = {
		[EC_TEMP_THRESH_WARN] = C_TO_K(60),
		[EC_TEMP_THRESH_HIGH] = 0,
		[EC_TEMP_THRESH_HALT] = 0,
	},
	.temp_fan_off = 0,
	.temp_fan_max = 0,
};

static void setup_fans(void)
{
	switch (oem) {
	case PROJECT_SONA:
		if (model == MODEL_SYNDRA)
			fans[FAN_CH_0].rpm = &fan_rpm_4;
		else
			fans[FAN_CH_0].rpm = &fan_rpm_1;
		thermal_params[TEMP_SENSOR_REMOTE1] = thermal_b1;
		thermal_params[TEMP_SENSOR_REMOTE2] = thermal_b2;
		break;
	case PROJECT_PANTHEON:
		fans[FAN_CH_0].rpm = &fan_rpm_2;
		thermal_params[TEMP_SENSOR_REMOTE1] = thermal_c1;
		thermal_params[TEMP_SENSOR_REMOTE2] = thermal_c2;
		break;
	case PROJECT_AKALI:
		fans[FAN_CH_0].rpm = &fan_rpm_3;
		thermal_params[TEMP_SENSOR_LOCAL] = thermal_d0;
		thermal_params[TEMP_SENSOR_REMOTE1] = thermal_d1;
		thermal_params[TEMP_SENSOR_REMOTE2] = thermal_d2;
		break;
	case PROJECT_NAMI:
	case PROJECT_VAYNE:
	default:
		thermal_params[TEMP_SENSOR_REMOTE1] = thermal_a;
		thermal_params[TEMP_SENSOR_REMOTE2] = thermal_a;
	}
}

void board_reset_pd_mcu(void)
{
	if (oem == PROJECT_AKALI && board_version < 0x0200) {
		if (anx7447_flash_erase(USB_PD_PORT_ANX7447))
			CPRINTS("Failed to erase OCM flash");

	}

	/* Assert reset */
	gpio_set_level(GPIO_USB_C0_PD_RST_L, 0);
	gpio_set_level(GPIO_USB_C1_PD_RST, 1);
	msleep(1);
	gpio_set_level(GPIO_USB_C0_PD_RST_L, 1);
	gpio_set_level(GPIO_USB_C1_PD_RST, 0);
	/* After TEST_R release, anx7447/3447 needs 2ms to finish eFuse
	 * loading. */
	msleep(2);
}

static int ps8751_tune_mux(const struct usb_mux *mux)
{
	/* 0x98 sets lower EQ of DP port (3.6db) */
	tcpc_write(mux->port_addr, PS8XXX_REG_MUX_DP_EQ_CONFIGURATION, 0x98);
	return EC_SUCCESS;
}

void board_tcpc_init(void)
{
	int port;

	/* Only reset TCPC if not sysjump */
	if (!system_jumped_to_this_image())
		board_reset_pd_mcu();

	/* Enable TCPC interrupts */
	gpio_enable_interrupt(GPIO_USB_C0_PD_INT_ODL);
	gpio_enable_interrupt(GPIO_USB_C1_PD_INT_ODL);

	if (oem == PROJECT_SONA && model != MODEL_SYNDRA)
		usb_muxes[USB_PD_PORT_PS8751].board_init = ps8751_tune_mux;

	/*
	 * Initialize HPD to low; after sysjump SOC needs to see
	 * HPD pulse to enable video path
	 */
	for (port = 0; port < CONFIG_USB_PD_PORT_COUNT; port++) {
		const struct usb_mux *mux = &usb_muxes[port];
		mux->hpd_update(port, 0, 0);
	}
}
DECLARE_HOOK(HOOK_INIT, board_tcpc_init, HOOK_PRIO_INIT_I2C + 2);

static void board_init(void)
{
#ifndef TEST_BUILD
	if (oem == PROJECT_AKALI && model == MODEL_BARD) {
		/* Search key is moved to col=0,row=3 */
		keyscan_config.actual_key_mask[0] = 0x1c;
		keyscan_config.actual_key_mask[1] = 0xfe;
		/* No need to swap scancode_set2[0][3] and [1][0] because both
		 * are mapped to search key. */
	}
#endif
}
DECLARE_HOOK(HOOK_INIT, board_init, HOOK_PRIO_DEFAULT);

void board_set_charge_limit(int port, int supplier, int charge_ma,
			    int max_ma, int charge_mv)
{
	/*
	 * Limit the input current to 96% negotiated limit,
	 * to account for the charger chip margin.
	 */
	if (oem == PROJECT_AKALI &&
		(model == MODEL_EKKO || model == MODEL_BARD))
		charge_ma = charge_ma * 95 / 100;
	else
		charge_ma = charge_ma * 96 / 100;
	charge_set_input_current_limit(
			MAX(charge_ma, CONFIG_CHARGER_INPUT_CURRENT),
			charge_mv);
}

void board_kblight_init(void)
{
	if (!(sku & SKU_ID_MASK_KBLIGHT))
		return;

	switch (oem) {
	default:
	case PROJECT_NAMI:
	case PROJECT_AKALI:
	case PROJECT_VAYNE:
	case PROJECT_PANTHEON:
		kblight_register(&kblight_lm3509);
		break;
	case PROJECT_SONA:
		kblight_register(&kblight_pwm);
		break;
	}
}

int board_is_lid_angle_tablet_mode(void)
{
	/* Boards with no GMR sensor use lid angles to detect tablet mode. */
	return oem != PROJECT_AKALI;
}

enum critical_shutdown board_critical_shutdown_check(
		struct charge_state_data *curr)
{
	if (oem == PROJECT_VAYNE)
		return CRITICAL_SHUTDOWN_CUTOFF;
	else
		return CRITICAL_SHUTDOWN_HIBERNATE;

}

/* Lid Sensor mutex */
static struct mutex g_lid_mutex;
static struct mutex g_base_mutex;

/* Lid accel private data */
static struct bmi160_drv_data_t g_bmi160_data;
static struct kionix_accel_data g_kx022_data;

/* BMA255 private data */
static struct accelgyro_saved_data_t g_bma255_data;

/* Matrix to rotate accelrator into standard reference frame */
const matrix_3x3_t base_standard_ref = {
	{ 0, FLOAT_TO_FP(-1), 0},
	{ FLOAT_TO_FP(1), 0, 0},
	{ 0, 0, FLOAT_TO_FP(1)}
};

const matrix_3x3_t lid_standard_ref = {
	{ FLOAT_TO_FP(1), 0, 0},
	{ 0, FLOAT_TO_FP(-1), 0},
	{ 0, 0, FLOAT_TO_FP(-1)}
};

const matrix_3x3_t rotation_x180_z90 = {
	{ 0, FLOAT_TO_FP(-1), 0 },
	{ FLOAT_TO_FP(-1), 0, 0 },
	{ 0, 0, FLOAT_TO_FP(-1) }
};

struct motion_sensor_t motion_sensors[] = {
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
		.min_frequency = BMA255_ACCEL_MIN_FREQ,
		.max_frequency = BMA255_ACCEL_MAX_FREQ,
		.default_range = 2, /* g, to support tablet mode */
		.config = {
			/* EC use accel for angle detection */
			[SENSOR_CONFIG_EC_S0] = {
				.odr = 10000 | ROUND_UP_FLAG,
				.ec_rate = 0,
			},
			/* Sensor on in S3 */
			[SENSOR_CONFIG_EC_S3] = {
				.odr = 10000 | ROUND_UP_FLAG,
				.ec_rate = 0,
			},
		},
	},
	[BASE_ACCEL] = {
		.name = "Base Accel",
		.active_mask = SENSOR_ACTIVE_S0_S3,
		.chip = MOTIONSENSE_CHIP_BMI160,
		.type = MOTIONSENSE_TYPE_ACCEL,
		.location = MOTIONSENSE_LOC_BASE,
		.drv = &bmi160_drv,
		.mutex = &g_base_mutex,
		.drv_data = &g_bmi160_data,
		.port = I2C_PORT_ACCEL,
		.addr = BMI160_ADDR0,
		.rot_standard_ref = &base_standard_ref,
		.min_frequency = BMI160_ACCEL_MIN_FREQ,
		.max_frequency = BMI160_ACCEL_MAX_FREQ,
		.default_range = 2, /* g, to support tablet mode  */
		.config = {
			/* EC use accel for angle detection */
			[SENSOR_CONFIG_EC_S0] = {
				.odr = 10000 | ROUND_UP_FLAG,
				.ec_rate = 100 * MSEC,
			},
			/* Sensor on in S3 */
			[SENSOR_CONFIG_EC_S3] = {
				.odr = 10000 | ROUND_UP_FLAG,
				.ec_rate = 0,
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
		.port = I2C_PORT_ACCEL,
		.addr = BMI160_ADDR0,
		.default_range = 1000, /* dps */
		.rot_standard_ref = &base_standard_ref,
		.min_frequency = BMI160_GYRO_MIN_FREQ,
		.max_frequency = BMI160_GYRO_MAX_FREQ,
	},
};
unsigned int motion_sensor_count = ARRAY_SIZE(motion_sensors);

const struct motion_sensor_t lid_accel_1 = {
	.name = "Lid Accel",
	.active_mask = SENSOR_ACTIVE_S0_S3,
	.chip = MOTIONSENSE_CHIP_KX022,
	.type = MOTIONSENSE_TYPE_ACCEL,
	.location = MOTIONSENSE_LOC_LID,
	.drv = &kionix_accel_drv,
	.mutex = &g_lid_mutex,
	.drv_data = &g_kx022_data,
	.port = I2C_PORT_ACCEL,
	.addr = KX022_ADDR1,
	.rot_standard_ref = &rotation_x180_z90,
	.min_frequency = KX022_ACCEL_MIN_FREQ,
	.max_frequency = KX022_ACCEL_MAX_FREQ,
	.default_range = 2, /* g, to support tablet mode */
	.config = {
		/* EC use accel for angle detection */
		[SENSOR_CONFIG_EC_S0] = {
			.odr = 10000 | ROUND_UP_FLAG,
		},
		/* Sensor on in S3 */
		[SENSOR_CONFIG_EC_S3] = {
			.odr = 10000 | ROUND_UP_FLAG,
		},
	},
};

static void setup_motion_sensors(void)
{
	if (sku & SKU_ID_MASK_CONVERTIBLE) {
		if (oem == PROJECT_AKALI) {
			/* Rotate axis for Akali 360 */
			motion_sensors[LID_ACCEL] = lid_accel_1;
			motion_sensors[BASE_ACCEL].rot_standard_ref = NULL;
			motion_sensors[BASE_GYRO].rot_standard_ref = NULL;
		}
	} else {
		/* Clamshells have no accel/gyro */
		motion_sensor_count = 0;
	}
}

/*
 * Read CBI from i2c eeprom and initialize variables for board variants
 */
static void cbi_init(void)
{
	uint32_t val;

	if (cbi_get_board_version(&val) == EC_SUCCESS && val <= UINT16_MAX)
		board_version = val;
	CPRINTS("Board Version: 0x%04x", board_version);

	if (cbi_get_oem_id(&val) == EC_SUCCESS && val < PROJECT_COUNT)
		oem = val;
	CPRINTS("OEM: %d", oem);

	if (cbi_get_sku_id(&val) == EC_SUCCESS)
		sku = val;
	CPRINTS("SKU: 0x%08x", sku);

	if (cbi_get_model_id(&val) == EC_SUCCESS)
		model = val;
	CPRINTS("MODEL: 0x%08x", model);

	if (board_version < 0x300)
		/* Previous boards have GPIO42 connected to TP_INT_CONN */
		gpio_set_flags(GPIO_USB2_ID, GPIO_INPUT);

	setup_motion_sensors();

	setup_fans();
}
DECLARE_HOOK(HOOK_INIT, cbi_init, HOOK_PRIO_INIT_I2C + 1);

