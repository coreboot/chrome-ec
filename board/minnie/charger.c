/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Board-specific configuration of BQ24773 charger.
 */

#include "driver/charger/bq24773.h"

int board_init_charger(void)
{
	int rv;

	/* Charge option 0
	 *
	 * [15]    = 0	Low power mode enable
	 * [14:13] = 11	WATCHDOG timer adject
	 * [12]    = 0  IDPM auto disable
	 * [11]    = 0  SYSOVP status & clear
	 * [10]    = 0  Audio frequency limit
	 * [9:8]   = 01 Switching frequence
	 * [7]     = 1  ACOC setting
	 * [6]     = 0  LSFET OCP threshold
	 * [5]     = 0  LEARN Enable
	 * [4]     = 0  IADP amplifier radio
	 * [3]     = 1  IBAT amplifier radio for discharge current
	 * [2]     = 1  Reserved
	 * [1]     = 1  IDPM enable
	 * [0]     = 0  Charge inhibit
	 */
	rv = raw_write16(REG_CHARGE_OPTION0, 0x618e);
	if (rv)
		return rv;

	/* Charge option 1
	 *
	 * [15:13] = 000 Reserved
	 * [12]    = 0   RSNS_RATIO
	 * [11]    = 0   IBAT enable
	 * [10]    = 0   PMON enable
	 * [9]     = 1   PMON gain
	 * [8]     = 0   Reserved
	 * [7]     = 0   CMP_REF
	 * [6]     = 0   CMP_POL
	 * [5:4]   = 01  CMP_DEG
	 * [3]     = 0   FET latch-off enable
	 * [2]     = 0   FORCE BATFET off
	 * [1]     = 0   Discharge BAT enable
	 * [0]     = 0   Auto wakeup enable
	 */
	rv = raw_write16(REG_CHARGE_OPTION1, 0x0210);
	if (rv)
		return rv;

	/* Charge option 2
	 *
	 * [15:8] = 00000000 Reserved
	 * [7]    = 0        External ILIM Enable
	 * [6]    = 0        IBAT output select
	 * [5:0]  = 000000   Reserved
	 */
	rv = raw_write16(REG_CHARGE_OPTION2, 0);
	if (rv)
		return rv;

	/* Prochot option 0
	 *
	 * [15:11] = 01001 ICRIT comparator threshold
	 * [10:9]  = 01    ICRIT comparator deglitch
	 * [8]     = 1     Input OCP threshold
	 * [7:6]   = 10    VSYS comparator threshold
	 * [5]     = 1     PROCHOT pulse extension enable
	 * [4:3]   = 01    PROCHOT pulse width
	 * [2]     = 0     PROCHOT host clear
	 * [1]     = 1     INOM comparator deglitch time
	 * [0]     = 0     Reserved
	 */
	rv = raw_write16(REG_PROTECT_OPTION0, 0x4baa);
	if (rv)
		return rv;

	/* Prochot option 1
	 *
	 * [15:10] 001000  IDCHG comparator threshold: 4096mA
	 * [9:8]   00      IDCHG comparator deglitch time: 1.6ms
	 * [7]     0       Reserved
	 * [6:0]   0111100 PROCHOT envelop selector: Enable-peak,
	 *                                           Average input current,
	 *                                           Dsch current
	 *                                           VSYS
	 */
	rv = raw_write16(REG_PROTECT_OPTION1, 0x203c);
	if (rv)
		return rv;

	/* Maximum charge voltage: 4350 mV */
	rv = raw_write16(REG_MAX_CHARGE_VOLTAGE, 0x10fe);
	if (rv)
		return rv;

	/* Minimum system voltage: 3328 mV */
	return raw_write16(REG_MIN_SYSTEM_VOLTAGE, 0xd00);
}
