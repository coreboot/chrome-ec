/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * TI bq24727 battery charger driver.
 */

#ifndef __CROS_EC_CHARGER_BQ24727_H
#define __CROS_EC_CHARGER_BQ24727_H

/* Chip specific commands */
#define BQ24727_CHARGE_OPTION           0x12
#define BQ24727_INPUT_CURRENT           0x3f
#define BQ24727_MANUFACTURE_ID          0xfe
#define BQ24727_DEVICE_ID               0xff

/* ChargeOption 0x12 */
#define OPTION_CHARGE_INHIBIT           (1 << 0) /* POR VALUE 0 */
#define OPTION_FAST_DPM_THRESHOLD       (1 << 1)
#define OPTION_COMPARATOR_THRESHOLD     (1 << 4)
#define OPTION_IOUT_SELECTION           (1 << 5)
#define OPTION_IOUT_OUTPUT_LEVEL        (1 << 6)
#define OPTION_IFAULT_LOW_THRESHOLD     (1 << 7)
#define OPTION_IFAULT_HI_THRESHOLD      (1 << 8)
#define OPTION_EMI_FREQ_ENABLE          (1 << 9) /* POR VALUE 0 */
#define OPTION_EMI_FREQ_ADJ             (1 << 10)
#define OPTION_WATCHDOG_TIMER           (3 << 13)
#define OPTION_AOC_DELITCH_TIME         (1 << 15)
/* OPTION_FAST_DPM_THRESHOLD */
#define FAST_DPM_THRESHOLD_107PERCENT   (0 << 1) /* POR */
#define FAST_DPM_THRESHOLD_114PERCENT   (1 << 1)
/* OPTION_COMPARATOR_THRESHOLD */
#define COMPARATOR_THRESHOLD_600MV      (0 << 4) /* POR */
#define COMPARATOR_THRESHOLD_2400MV     (1 << 4)
/* OPTION_IOUT_SELECTION */
#define OPTION_IOUT_SELECTION_ADAPTER   (0 << 5) /* POR */
#define OPTION_IOUT_SELECTION_CHARGE    (1 << 5)
/* OPTION_IOUT_OUTPUT_LEVEL */
#define OPTION_IOUT_OUTPUT_LEVEL_20X    (0 << 6) /* POR */
#define OPTION_IOUT_OUTPUT_LEVEL_40X    (1 << 6)
/* OPTION_IFAULT_LOW_THRESHOLD */
#define IFAULT_LOW_THRESHOLD_135MV      (0 << 7) /* POR */
#define IFAULT_LOW_THRESHOLD_230MV      (1 << 7)
/* OPTION_IFAULT_HI_THRESHOLD */
#define IFAULT_HI_THRESHOLD_DISABLED    (0 << 8)
#define IFAULT_HI_THRESHOLD_750MV       (1 << 8) /* POR */
/* OPTION_EMI_FREQ_ADJ */
#define OPTION_EMI_FREQ_ADJ_615KHZ      (0 << 10) /* POR */
#define OPTION_EMI_FREQ_ADJ_885KHZ      (1 << 10)
/* OPTION_WATCHDOG_TIMER */
#define CHARGE_WATCHDOG_DISABLE         (0 << 13)
#define CHARGE_WATCHDOG_44SEC           (1 << 13)
#define CHARGE_WATCHDOG_88SEC           (2 << 13)
#define CHARGE_WATCHDOG_175SEC_DEFAULT  (3 << 13) /* POR */
/* OPTION_AOC_DELITCH_TIME */
#define OPTION_AOC_DELITCH_TIME_2MSEC    (0 << 15)
#define OPTION_AOC_DELITCH_TIME_1300MSEC (1 << 15) /* POR */

#endif /* __CROS_EC_CHARGER_BQ24727_H */

