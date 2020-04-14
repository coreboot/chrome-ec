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

uint16_t board_version;
uint8_t oem;
uint32_t sku;
uint8_t model;

/*
 * We have total 30 pins for keyboard connecter {-1, -1} mean
 * the N/A pin that don't consider it and reserve index 0 area
 * that we don't have pin 0.
 */
const int keyboard_factory_scan_pins[][2] = {
	{-1, -1}, {0, 5}, {1, 1}, {1, 0}, {0, 6},
	{0, 7}, {-1, -1}, {-1, -1}, {1, 4}, {1, 3},
	{-1, -1}, {1, 6}, {1, 7}, {3, 1}, {2, 0},
	{1, 5}, {2, 6}, {2, 7}, {2, 1}, {2, 4},
	{2, 5}, {1, 2}, {2, 3}, {2, 2}, {3, 0},
	{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1},
	{-1, -1},
};

const int keyboard_factory_scan_pins_used =
		ARRAY_SIZE(keyboard_factory_scan_pins);

static void tcpc_alert_event(enum gpio_signal signal)
{
	int port = -1;

	switch (signal) {
	case GPIO_USB_C0_PD_INT_ODL:
		port = 0;
		break;
	case GPIO_USB_C1_PD_INT_ODL:
		port = 1;
		break;
	default:
		return;
	}

	schedule_deferred_pd_interrupt(port);
}

/* Set PD discharge whenever VBUS detection is high (i.e. below threshold). */
static void vbus_discharge_handler(void)
{
	pd_set_vbus_discharge(0, gpio_get_level(GPIO_USB_C0_VBUS_WAKE_L));
	pd_set_vbus_discharge(1, gpio_get_level(GPIO_USB_C1_VBUS_WAKE_L));
}
DECLARE_DEFERRED(vbus_discharge_handler);

void vbus0_evt(enum gpio_signal signal)
{
	/* VBUS present GPIO is inverted */
	usb_charger_vbus_change(0, !gpio_get_level(signal));
	task_wake(TASK_ID_PD_C0);
	hook_call_deferred(&vbus_discharge_handler_data, 0);
}

void vbus1_evt(enum gpio_signal signal)
{
	/* VBUS present GPIO is inverted */
	usb_charger_vbus_change(1, !gpio_get_level(signal));
	task_wake(TASK_ID_PD_C1);
	hook_call_deferred(&vbus_discharge_handler_data, 0);
}

void usb0_evt(enum gpio_signal signal)
{
	task_set_event(TASK_ID_USB_CHG_P0, USB_CHG_EVENT_BC12, 0);
}

void usb1_evt(enum gpio_signal signal)
{
	task_set_event(TASK_ID_USB_CHG_P1, USB_CHG_EVENT_BC12, 0);
}

#include "gpio_list.h"

/* power signal list.  Must match order of enum power_signal. */
const struct power_signal_info power_signal_list[] = {
#ifdef CONFIG_POWER_S0IX
	{GPIO_PCH_SLP_S0_L,
		POWER_SIGNAL_ACTIVE_HIGH | POWER_SIGNAL_DISABLE_AT_BOOT,
		"SLP_S0_DEASSERTED"},
#endif
	{VW_SLP_S3_L,		POWER_SIGNAL_ACTIVE_HIGH, "SLP_S3_DEASSERTED"},
	{VW_SLP_S4_L,		POWER_SIGNAL_ACTIVE_HIGH, "SLP_S4_DEASSERTED"},
	{GPIO_PCH_SLP_SUS_L,	POWER_SIGNAL_ACTIVE_HIGH, "SLP_SUS_DEASSERTED"},
	{GPIO_RSMRST_L_PGOOD,	POWER_SIGNAL_ACTIVE_HIGH, "RSMRST_L_PGOOD"},
	{GPIO_PMIC_DPWROK,	POWER_SIGNAL_ACTIVE_HIGH, "PMIC_DPWROK"},
};
BUILD_ASSERT(ARRAY_SIZE(power_signal_list) == POWER_SIGNAL_COUNT);

/* ADC channels */
const struct adc_t adc_channels[] = {
	/* Vbus sensing (10x voltage divider). PPVAR_BOOSTIN_SENSE */
	[ADC_VBUS] = {"VBUS", NPCX_ADC_CH2, ADC_MAX_VOLT*10, ADC_READ_MAX+1, 0},
	/*
	 * Adapter current output or battery charging/discharging current (uV)
	 * 18x amplification on charger side.
	 */
	[ADC_AMON_BMON] = {"AMON_BMON", NPCX_ADC_CH1, ADC_MAX_VOLT*1000/18,
			   ADC_READ_MAX+1, 0},
};
BUILD_ASSERT(ARRAY_SIZE(adc_channels) == ADC_CH_COUNT);

/* TCPC mux configuration */
const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
	[USB_PD_PORT_PS8751] = {
		.i2c_host_port = NPCX_I2C_PORT0_0,
		.i2c_slave_addr = PS8751_I2C_ADDR1,
		.drv = &ps8xxx_tcpm_drv,
		.pol = TCPC_ALERT_ACTIVE_LOW,
	},
	[USB_PD_PORT_ANX7447] = {
		.i2c_host_port = NPCX_I2C_PORT0_1,
		.i2c_slave_addr = AN7447_TCPC3_I2C_ADDR, /* Verified on v1.1 */
		.drv = &anx7447_tcpm_drv,
		.pol = TCPC_ALERT_ACTIVE_LOW,
	},
};

struct usb_mux usb_muxes[CONFIG_USB_PD_PORT_COUNT] = {
	[USB_PD_PORT_PS8751] = {
		.port_addr = USB_PD_PORT_PS8751,
		.driver = &tcpci_tcpm_usb_mux_driver,
		.hpd_update = &ps8xxx_tcpc_update_hpd_status,
	},
	[USB_PD_PORT_ANX7447] = {
		.port_addr = USB_PD_PORT_ANX7447,
		.driver = &anx7447_usb_mux_driver,
		.hpd_update = &anx7447_tcpc_update_hpd_status,
	}
};

struct pi3usb9281_config pi3usb9281_chips[] = {
	{
		.i2c_port = I2C_PORT_USB_CHARGER_0,
		.mux_lock = NULL,
	},
	{
		.i2c_port = I2C_PORT_USB_CHARGER_1,
		.mux_lock = NULL,
	},
};
BUILD_ASSERT(ARRAY_SIZE(pi3usb9281_chips) ==
	     CONFIG_BC12_DETECT_PI3USB9281_CHIP_COUNT);


uint16_t tcpc_get_alert_status(void)
{
	uint16_t status = 0;

	if (!gpio_get_level(GPIO_USB_C0_PD_INT_ODL)) {
		if (gpio_get_level(GPIO_USB_C0_PD_RST_L))
			status |= PD_STATUS_TCPC_ALERT_0;
	}

	if (!gpio_get_level(GPIO_USB_C1_PD_INT_ODL)) {
		if (!gpio_get_level(GPIO_USB_C1_PD_RST))
			status |= PD_STATUS_TCPC_ALERT_1;
	}

	return status;
}

#define I2C_PMIC_READ(reg, data) \
		i2c_read8(I2C_PORT_PMIC, TPS650X30_I2C_ADDR1, (reg), (data))
#define I2C_PMIC_WRITE(reg, data) \
		i2c_write8(I2C_PORT_PMIC, TPS650X30_I2C_ADDR1, (reg), (data))

static void board_pmic_init(void)
{
	int err;
	int error_count = 0;
	static uint8_t pmic_initialized = 0;

	if (pmic_initialized)
		return;

	/* Read vendor ID */
	while (1) {
		int data;
		err = I2C_PMIC_READ(TPS650X30_REG_VENDORID, &data);
		if (!err && data == TPS650X30_VENDOR_ID)
			break;
		else if (error_count > 5)
			goto pmic_error;
		error_count++;
	}

	/*
	 * VCCIOCNT register setting
	 * [6] : CSDECAYEN
	 * otherbits: default
	 */
	err = I2C_PMIC_WRITE(TPS650X30_REG_VCCIOCNT, 0x4A);
	if (err)
		goto pmic_error;

	/*
	 * VRMODECTRL:
	 * [4] : VCCIOLPM clear
	 * otherbits: default
	 */
	err = I2C_PMIC_WRITE(TPS650X30_REG_VRMODECTRL, 0x2F);
	if (err)
		goto pmic_error;

	/*
	 * PGMASK1 : Exclude VCCIO from Power Good Tree
	 * [7] : MVCCIOPG clear
	 * otherbits: default
	 */
	err = I2C_PMIC_WRITE(TPS650X30_REG_PGMASK1, 0x80);
	if (err)
		goto pmic_error;

	/*
	 * PWFAULT_MASK1 Register settings
	 * [7] : 1b V4 Power Fault Masked
	 * [4] : 1b V7 Power Fault Masked
	 * [2] : 1b V9 Power Fault Masked
	 * [0] : 1b V13 Power Fault Masked
	 */
	err = I2C_PMIC_WRITE(TPS650X30_REG_PWFAULT_MASK1, 0x95);
	if (err)
		goto pmic_error;

	/*
	 * Discharge control 4 register configuration
	 * [7:6] : 00b Reserved
	 * [5:4] : 01b V3.3S discharge resistance (V6S), 100 Ohm
	 * [3:2] : 01b V18S discharge resistance (V8S), 100 Ohm
	 * [1:0] : 01b V100S discharge resistance (V11S), 100 Ohm
	 */
	err = I2C_PMIC_WRITE(TPS650X30_REG_DISCHCNT4, 0x15);
	if (err)
		goto pmic_error;

	/*
	 * Discharge control 3 register configuration
	 * [7:6] : 01b V1.8U_2.5U discharge resistance (V9), 100 Ohm
	 * [5:4] : 01b V1.2U discharge resistance (V10), 100 Ohm
	 * [3:2] : 01b V100A discharge resistance (V11), 100 Ohm
	 * [1:0] : 01b V085A discharge resistance (V12), 100 Ohm
	 */
	err = I2C_PMIC_WRITE(TPS650X30_REG_DISCHCNT3, 0x55);
	if (err)
		goto pmic_error;

	/*
	 * Discharge control 2 register configuration
	 * [7:6] : 01b V5ADS3 discharge resistance (V5), 100 Ohm
	 * [5:4] : 01b V33A_DSW discharge resistance (V6), 100 Ohm
	 * [3:2] : 01b V33PCH discharge resistance (V7), 100 Ohm
	 * [1:0] : 01b V18A discharge resistance (V8), 100 Ohm
	 */
	err = I2C_PMIC_WRITE(TPS650X30_REG_DISCHCNT2, 0x55);
	if (err)
		goto pmic_error;

	/*
	 * Discharge control 1 register configuration
	 * [7:2] : 00b Reserved
	 * [1:0] : 01b VCCIO discharge resistance (V4), 100 Ohm
	 */
	err = I2C_PMIC_WRITE(TPS650X30_REG_DISCHCNT1, 0x01);
	if (err)
		goto pmic_error;

	/*
	 * Increase Voltage
	 *  [7:0] : 0x2a default
	 *  [5:4] : 10b default (5.0V)
	 *  [5:4] : 00b +3% (5.15V)
	 */
	err = I2C_PMIC_WRITE(TPS650X30_REG_V5ADS3CNT, 0x0a);
	if (err)
		goto pmic_error;

	/*
	 * PBCONFIG Register configuration
	 *   [7] :      1b Power button debounce, 0ms (no debounce)
	 *   [6] :      0b Power button reset timer logic, no action (default)
	 * [5:0] : 011111b Force an Emergency reset time, 31s (default)
	 */
	err = I2C_PMIC_WRITE(TPS650X30_REG_PBCONFIG, 0x9F);
	if (err)
		goto pmic_error;

	CPRINTS("PMIC init done");
	pmic_initialized = 1;
	return;

pmic_error:
	CPRINTS("PMIC init failed: %d", err);
}

void chipset_pre_init_callback(void)
{
	board_pmic_init();
}

/**
 * Buffer the AC present GPIO to the PCH.
 */
static void board_extpower(void)
{
	gpio_set_level(GPIO_PCH_ACPRESENT, extpower_is_present());
}
DECLARE_HOOK(HOOK_AC_CHANGE, board_extpower, HOOK_PRIO_DEFAULT);

/* Set active charge port -- only one port can be active at a time. */
int board_set_active_charge_port(int charge_port)
{
	/* charge port is a physical port */
	int is_real_port = (charge_port >= 0 &&
			    charge_port < CONFIG_USB_PD_PORT_COUNT);
	/* check if we are sourcing VBUS on the port */
	/* dnojiri: revisit */
	int is_source = gpio_get_level(charge_port == 0 ?
			GPIO_USB_C0_5V_EN : GPIO_USB_C1_5V_EN);

	if (is_real_port && is_source) {
		CPRINTF("No charging on source port p%d is ", charge_port);
		return EC_ERROR_INVAL;
	}

	CPRINTF("New chg p%d", charge_port);

	if (charge_port == CHARGE_PORT_NONE) {
		/* Disable both ports */
		gpio_set_level(GPIO_USB_C0_CHARGE_L, 1);
		gpio_set_level(GPIO_USB_C1_CHARGE_L, 1);
	} else {
		/* Make sure non-charging port is disabled */
		/* dnojiri: revisit. there is always this assumption that
		 * battery is present. If not, this may cause brownout. */
		gpio_set_level(charge_port ? GPIO_USB_C0_CHARGE_L :
					     GPIO_USB_C1_CHARGE_L, 1);
		/* Enable charging port */
		gpio_set_level(charge_port ? GPIO_USB_C1_CHARGE_L :
					     GPIO_USB_C0_CHARGE_L, 0);
	}

	return EC_SUCCESS;
}

void board_hibernate(void)
{
	CPRINTS("Triggering PMIC shutdown.");
	uart_flush_output();
	gpio_set_level(GPIO_EC_HIBERNATE, 1);
	while (1)
		;
}

const struct pwm_t pwm_channels[] = {
	[PWM_CH_LED1]   = { 3, PWM_CONFIG_DSLEEP, 1200 },
	[PWM_CH_LED2] = { 5, PWM_CONFIG_DSLEEP, 1200 },
	[PWM_CH_FAN] = {4, PWM_CONFIG_OPEN_DRAIN, 25000},
	/*
	 * 1.2kHz is a multiple of both 50 and 60. So a video recorder
	 * (generally designed to ignore either 50 or 60 Hz flicker) will not
	 * alias with refresh rate.
	 */
	[PWM_CH_KBLIGHT] = { 2, 0, 1200 },
};
BUILD_ASSERT(ARRAY_SIZE(pwm_channels) == PWM_CH_COUNT);

/* Enable or disable input devices, based on chipset state and tablet mode */
#ifndef TEST_BUILD
void lid_angle_peripheral_enable(int enable)
{
	/* If the lid is in 360 position, ignore the lid angle,
	 * which might be faulty. Disable keyboard.
	 */
	if (tablet_get_mode() || chipset_in_state(CHIPSET_STATE_ANY_OFF))
		enable = 0;
	keyboard_scan_enable(enable, KB_SCAN_DISABLE_LID_ANGLE);
}
#endif

/* Called on AP S3 -> S0 transition */
static void board_chipset_resume(void)
{
	gpio_set_level(GPIO_ENABLE_BACKLIGHT_L, 0);
	gpio_set_level(GPIO_USB3_POWER_DOWN_L, 1);
}
DECLARE_HOOK(HOOK_CHIPSET_RESUME, board_chipset_resume, HOOK_PRIO_DEFAULT);

/* Called on AP S0 -> S3 transition */
static void board_chipset_suspend(void)
{
	gpio_set_level(GPIO_ENABLE_BACKLIGHT_L, 1);
	gpio_set_level(GPIO_USB3_POWER_DOWN_L, 0);
}
DECLARE_HOOK(HOOK_CHIPSET_SUSPEND, board_chipset_suspend, HOOK_PRIO_DEFAULT);

/*
 * Read CBI from i2c eeprom and initialize variables for board variants
 */
static void baseboard_cbi_init(void)
{
	uint32_t val;

	if (cbi_get_board_version(&val) == EC_SUCCESS && val <= UINT16_MAX)
		board_version = val;
	CPRINTS("Board Version: 0x%04x", board_version);

	if (cbi_get_oem_id(&val) == EC_SUCCESS)
		oem = val;
	CPRINTS("OEM: %d", oem);

	if (cbi_get_sku_id(&val) == EC_SUCCESS)
		sku = val;
	CPRINTS("SKU: 0x%08x", sku);

	if (cbi_get_model_id(&val) == EC_SUCCESS)
		model = val;
	CPRINTS("MODEL: 0x%08x", model);

}
DECLARE_HOOK(HOOK_INIT, baseboard_cbi_init, HOOK_PRIO_INIT_I2C + 1);

/* Keyboard scan setting */
struct keyboard_scan_config keyscan_config = {
	/*
	 * F3 key scan cycle completed but scan input is not
	 * charging to logic high when EC start scan next
	 * column for "T" key, so we set .output_settle_us
	 * to 80us from 50us.
	 */
	.output_settle_us = 80,
	.debounce_down_us = 9 * MSEC,
	.debounce_up_us = 30 * MSEC,
	.scan_period_us = 3 * MSEC,
	.min_post_scan_delay_us = 1000,
	.poll_timeout_us = 100 * MSEC,
	.actual_key_mask = {
		0x14, 0xff, 0xff, 0xff, 0xff, 0xf5, 0xff,
		0xa4, 0xff, 0xfe, 0x55, 0xfe, 0xff, 0xff, 0xff,  /* full set */
	},
};

static void anx7447_set_aux_switch(void)
{
	const int port = USB_PD_PORT_ANX7447;

	/* Debounce */
	if (gpio_get_level(GPIO_CCD_MODE_ODL))
		return;

	CPRINTS("C%d: AUX_SW_SEL=0x%x", port, 0xc);
	if (tcpc_write(port, ANX7447_REG_TCPC_AUX_SWITCH, 0xc))
		CPRINTS("C%d: Setting AUX_SW_SEL failed", port);
}
DECLARE_DEFERRED(anx7447_set_aux_switch);

void ccd_mode_isr(enum gpio_signal signal)
{
	/* Wait 2 seconds until all mux setting is done by PD task */
	hook_call_deferred(&anx7447_set_aux_switch_data, 2 * SECOND);
}

static void baseboard_board_init(void)
{
	int reg;

	/*
	 * This enables pull-down on F_DIO1 (SPI MISO), and F_DIO0 (SPI MOSI),
	 * whenever the EC is not doing SPI flash transactions. This avoids
	 * floating SPI buffer input (MISO), which causes power leakage (see
	 * b/64797021).
	 */
	NPCX_PUPD_EN1 |= (1 << NPCX_DEVPU1_F_SPI_PUD_EN);

	/* Provide AC status to the PCH */
	gpio_set_level(GPIO_PCH_ACPRESENT, extpower_is_present());

	/* Reduce Buck-boost mode switching frequency to reduce heat */
	if (i2c_read16(I2C_PORT_CHARGER, I2C_ADDR_CHARGER,
		       ISL9238_REG_CONTROL3, &reg) == EC_SUCCESS) {
		reg |= ISL9238_C3_BB_SWITCHING_PERIOD;
		if (i2c_write16(I2C_PORT_CHARGER, I2C_ADDR_CHARGER,
			    ISL9238_REG_CONTROL3, reg))
			CPRINTF("Failed to set isl9238\n");
	}

	/* Enable VBUS interrupt */
	gpio_enable_interrupt(GPIO_USB_C0_VBUS_WAKE_L);
	gpio_enable_interrupt(GPIO_USB_C1_VBUS_WAKE_L);

	/* Enable pericom BC1.2 interrupts */
	gpio_enable_interrupt(GPIO_USB_C0_BC12_INT_L);
	gpio_enable_interrupt(GPIO_USB_C1_BC12_INT_L);

	/* Trigger once to set mux in case CCD cable is already connected. */
	ccd_mode_isr(GPIO_CCD_MODE_ODL);
	gpio_enable_interrupt(GPIO_CCD_MODE_ODL);

	/* Enable Accel/Gyro interrupt for convertibles. */
	if (sku & SKU_ID_MASK_CONVERTIBLE)
		gpio_enable_interrupt(GPIO_ACCELGYRO3_INT_L);

#ifndef TEST_BUILD
	/* Disable scanning KSO13 & 14 if keypad isn't present. */
	if (!(sku & SKU_ID_MASK_KEYPAD)) {
		keyboard_raw_set_cols(KEYBOARD_COLS_NO_KEYPAD);
		keyscan_config.actual_key_mask[11] = 0xfa;
		keyscan_config.actual_key_mask[12] = 0xca;
	}
	if (sku & SKU_ID_MASK_UK2)
		/*
		 * Observed on Shyvana with UK keyboard,
		 *   \|:     0x0061->0x61->0x56
		 *   r-ctrl: 0xe014->0x14->0x1d
		 */
		swap(scancode_set2[0][4], scancode_set2[7][2]);
#endif
}
DECLARE_HOOK(HOOK_INIT, baseboard_board_init, HOOK_PRIO_DEFAULT);

uint32_t board_override_feature_flags0(uint32_t flags0)
{
	if (!(sku & SKU_ID_MASK_KBLIGHT))
		return (flags0 & ~EC_FEATURE_MASK_0(EC_FEATURE_PWM_KEYB));

	return flags0;
}

uint32_t board_override_feature_flags1(uint32_t flags1)
{
	return flags1;
}
