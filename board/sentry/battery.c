/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "adc.h"
#include "battery.h"
#include "battery_smart.h"
#include "console.h"
#include "extpower.h"
#include "gpio.h"
#include "hooks.h"
#include "host_command.h"
#include "smbus.h"
#include "system.h"
#include "util.h"

/* FET ON/OFF cammand write to fet off register */
#define SB_FET_OFF      0x34
#define SB_FETOFF_DATA1 0x0000
#define SB_FETOFF_DATA2 0x1000
#define SB_FETON_DATA1  0x2000
#define SB_FETON_DATA2  0x4000
#define BATTERY_FETOFF  0x0100

/* First use day base */
#define BATT_FUD_BASE   0x38

/*
 * Green book support parameter
 * Enable this will make battery meet JEITA standard
 */
#define GREEN_BOOK_SUPPORT      (1 << 2)

/* Shutdown mode parameter to write to manufacturer access register */
#define SB_SHUTDOWN_DATA	0x0010

static const struct battery_info info = {
	.voltage_max = 13050,/* mV */
	.voltage_normal = 11400,
	.voltage_min = 9000,
	.precharge_current = 150,/* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 45,
	.charging_min_c = 0,
	.charging_max_c = 45,
	.discharging_min_c = -20,
	.discharging_max_c = 60,
};

const struct battery_info *battery_get_info(void)
{
	static struct battery_info __bss_slow batt_info;

	if (battery_is_present() == BP_NO) {
		batt_info = info;
		batt_info.voltage_min = batt_info.voltage_max;
		return &batt_info;
	} else
		return &info;
}

static void battery_wakeup(void)
{
	int d;
	int mode;

	/* Add Green Book support */
	if (sb_read(SB_BATTERY_MODE, &mode)) {
		mode |= GREEN_BOOK_SUPPORT;
		sb_write(SB_BATTERY_MODE, mode);
        }

	sb_read(SB_FET_OFF, &d);
	if (extpower_is_present() && (BATTERY_FETOFF == d)) {
		sb_write(SB_FET_OFF, SB_FETON_DATA1);
		sb_write(SB_FET_OFF, SB_FETON_DATA2);
	}
}
DECLARE_HOOK(HOOK_INIT, battery_wakeup, HOOK_PRIO_DEFAULT);

static int battery_cutoff(void)
{
	int rv;

	/* Ship mode command must be sent twice to take effect */
	rv = sb_write(SB_FET_OFF, SB_FETOFF_DATA1);

	if (rv != EC_SUCCESS)
		return rv;

	return sb_write(SB_FET_OFF, SB_FETOFF_DATA2);
}

int board_cut_off_battery(void)
{
	return battery_cutoff();
}

int battery_get_vendor_param(uint32_t param, uint32_t *value)
{
	return EC_ERROR_UNIMPLEMENTED;
}

/* parameter 0 for first use day */
int battery_set_vendor_param(uint32_t param, uint32_t value)
{
	if (param == 0) {
		int rv, ymd;

		rv = sb_read(BATT_FUD_BASE, &ymd);
		if (rv != EC_SUCCESS)
			return EC_ERROR_UNKNOWN;
		if (ymd == 0)
			return sb_write(BATT_FUD_BASE, value) ?
					EC_ERROR_UNKNOWN : EC_SUCCESS;

		rv = sb_read(BATT_FUD_BASE | 0x03, &ymd);
		if (rv != EC_SUCCESS)
			return EC_ERROR_UNKNOWN;
		if (ymd == 0)
			return sb_write(BATT_FUD_BASE | 0x03, value) ?
					EC_ERROR_UNKNOWN : EC_SUCCESS;

		rv = sb_read(BATT_FUD_BASE | 0x07, &ymd);
		if (rv != EC_SUCCESS)
			return EC_ERROR_UNKNOWN;
		if (ymd == 0)
			return sb_write(BATT_FUD_BASE | 0x07, value) ?
					EC_ERROR_UNKNOWN : EC_SUCCESS;

		return EC_ERROR_UNKNOWN;
	} else {
		return EC_ERROR_UNIMPLEMENTED;
	}
}

#ifdef CONFIG_BATTERY_PRESENT_CUSTOM
/*
 * Physical detection of battery via ADC.
 *
 * Upper limit of valid voltage level (mV), when battery is attached to ADC
 * port, is across the internal thermistor with external pullup resistor.
 */
#define BATT_PRESENT_MV  1500
enum battery_present battery_is_present(void)
{
	enum battery_present batt_pres;
	int batt_status;
	uint16_t batt_fw_mode = 0;

	/* To confirm whether battery in fw update mode which won't output power
	 * Read 0x35 for Battery firmware update status
	 * Bit8: firmware update mode or normal mode
	 */
	smbus_read_word(I2C_PORT_BATTERY, BATTERY_ADDR, 0x35, &batt_fw_mode);

	/*
	 * if voltage is below certain level (dependent on ratio of
	 * internal thermistor and external pullup resister),
	 * battery is attached.
	 */
	batt_pres = (adc_read_channel(ADC_BATT_PRESENT) > BATT_PRESENT_MV) ?
		BP_NO : BP_YES;

	/*
	 * Make sure battery status is implemented, I2C transactions are
	 * success & the battery status is Initialized to find out if it
	 * is a working battery and it is not in the cut-off mode.
	 *
	 * FETs are turned off after Power Shutdown time.
	 * The device will wake up when a voltage is applied to PACK.
	 * Battery status will be inactive until it is initialized.
	 */
	if (batt_pres == BP_YES && !battery_status(&batt_status))
		if (!(batt_status & STATUS_INITIALIZED))
			batt_pres = BP_NO;

	/*
	 * Bit 8 in offset 0x35 is high mean in firmware updat mode.
	 * Battery not supply power, report battery not present to prevent rebooting.
	 */
	if ((batt_fw_mode & 0x100) == 0x100)
		batt_pres = BP_NO;

	return batt_pres;
}
#endif
