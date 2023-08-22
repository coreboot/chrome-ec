/* Copyright 2020 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_SHIMMED_TASKS_H
#define __CROS_EC_SHIMMED_TASKS_H

#ifdef CONFIG_HAS_TASK_CHARGER
#define HAS_TASK_CHARGER 1
#endif /* CONFIG_HAS_TASK_CHARGER */

#ifdef CONFIG_HAS_TASK_CHG_RAMP
#define HAS_TASK_CHG_RAMP 1
#endif /* CONFIG_HAS_TASK_CHG_RAMP */

#ifdef CONFIG_HAS_TASK_CHIPSET
#define HAS_TASK_CHIPSET 1
#endif /* CONFIG_HAS_TASK_CHIPSET */

#ifdef CONFIG_HAS_TASK_DPS
#define HAS_TASK_DPS 1
#endif /* CONFIG_HAS_TASK_DPS */

#ifdef CONFIG_HAS_TASK_HOSTCMD
#define HAS_TASK_HOSTCMD 1
#define CONFIG_HOSTCMD_EVENTS
#endif /* CONFIG_HAS_TASK_HOSTCMD */

#ifdef CONFIG_HAS_TASK_KEYSCAN
#define HAS_TASK_KEYSCAN 1
#endif /* CONFIG_HAS_TASK_KEYSCAN */

#ifdef CONFIG_HAS_TASK_KEYPROTO
#define HAS_TASK_KEYPROTO 1
#endif /* CONFIG_HAS_TASK_KEYPROTO */

#ifdef CONFIG_HAS_TASK_MOTIONSENSE
#define HAS_TASK_MOTIONSENSE 1
#endif /* CONFIG_HAS_TASK_MOTIONSENSE */

#ifdef CONFIG_HAS_TASK_POWERBTN
#define HAS_TASK_POWERBTN 1
#endif /* CONFIG_HAS_TASK_POWERBTN */

#ifdef CONFIG_PLATFORM_EC_USB_MUX_TASK
#define HAS_TASK_USB_MUX 1
#endif /* CONFIG_PLATFORM_EC_USB_MUX_TASK */

#ifndef CONFIG_TASK_HOSTCMD_THREAD_MAIN
#define HAS_TASK_MAIN 1
#endif /* !CONFIG_TASK_HOSTCMD_THREAD_MAIN */

#if (defined(CONFIG_TASK_HOSTCMD_THREAD_DEDICATED) && \
     !defined(CONFIG_EC_HOST_CMD))
#define HAS_TASK_HOSTCMD_DEDICATED 1
#endif

#if (defined(CONFIG_SHELL_BACKEND_SERIAL) || \
     defined(CONFIG_SHELL_BACKEND_DUMMY))
#define HAS_TASK_SHELL 1
#endif

/* These non-shimmed (extra) tasks are always present */
#define HAS_TASK_IDLE 1
#define HAS_TASK_SYSWORKQ 1

#ifdef CONFIG_HAS_TASK_TOUCHPAD
#define HAS_TASK_TOUCHPAD 1
#endif /* CONFIG_HAS_TASK_TOUCHPAD */

#ifdef CONFIG_HAS_TASK_RWSIG
#define HAS_TASK_RWSIG 1
#endif /* CONFIG_HAS_TASK_RWSIG */

#ifdef CONFIG_HAS_TASK_CEC
#define HAS_TASK_CEC 1
#endif /* CONFIG_HAS_TASK_CEC */

#endif /* __CROS_EC_SHIMMED_TASKS_H */
