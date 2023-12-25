# Google Security Chip (GSC) Case Closed Debugging (CCD)

Cr50 or Ti50 is the firmware that runs on the Google Security Chip (GSC), which
has support for [Case Closed Debugging](CCD).

This document explains how to setup CCD, so you can access all of the necessary
features to develop firmware on your Chrome OS device, access
[debug consoles][consoles], and [disable hardware write protect][hw-wp].

[TOC]

## Overview

GSC CCD was designed to restrict CCD access to device owners and is implemented
through **CCD privilege levels** ([`Open`], [`Unlocked`], [`Locked`]) that can
be used to enable access to different **[CCD capabilities][cap]**. Capability
settings can be modified to require certain privilege levels to access each
capability. Device owners can use these settings to customize CCD so that it is
as open or restricted as they want.

GSC CCD exposes [3 debug consoles][consoles]: AP, EC, and GSC as well as control
over [Hardware Write Protect][hw-wp]. GSC CCD also allows
[flashing the AP firmware] or [flashing the EC firmware].

### Capability and Privilege Levels {#cap-priv}

Privilege Levels |
---------------- |
`Open`           |
`Unlocked`       |
`Locked`         |

Capability Setting | Definition
------------------ | ----------
`IfOpened`         | Specified capability is allowed if GSC Privilege Level is `Open`.
`UnlessLocked`     | Specified capability is allowed unless GSC Privilege Level is `Locked`.
`Always`           | Specified capability is always allowed, regardless of GSC Privilege Level.

Capability Setting | Privilege Level Required
------------------ | ----------------------------------------
`IfOpened`         | `Open`
`UnlessLocked`     | `Open` or `Unlocked`
`Always`           | `Open`, `Unlocked`, `Locked` (any state)

## CCD Capabilities {#cap}

The default GSC privilege level is [`Locked`] with the following capability
settings:

Capability        | Default    | Function
----------------- | ---------- | --------
`UartGscRxAPTx`   | `Always`   | AP console read access
`UartGscTxAPRx`   | `Always`   | AP console write access
`UartGscRxECTx`   | `Always`   | EC console read access
`UartGscTxECRx`   | `IfOpened` | EC console write access
[`FlashAP`]       | `IfOpened` | Allows flashing the AP
[`FlashEC`]       | `IfOpened` | Allows flashing the EC
[`OverrideWP`]    | `IfOpened` | Override hardware write protect
`RebootECAP`      | `IfOpened` | Allow rebooting the EC/AP from the GSC console
`GscFullConsole`  | `IfOpened` | Allow access to restricted GSC console commands
`UnlockNoReboot`  | `Always`   | Allow unlocking GSC without rebooting the AP
`UnlockNoShortPP` | `Always`   | Allow unlocking GSC without physical presence
`OpenNoTPMWipe`   | `IfOpened` | Allow opening GSC without wiping the TPM
`OpenNoLongPP`    | `IfOpened` | Allow opening GSC without physical presence
`BatteryBypassPP` | `Always`   | Allow opening GSC without physical presence and developer mode if the battery is removed
`Unused`          | `Always`   | Doesn't do anything
`I2C`             | `IfOpened` | Allow access to the I2C controller (used for measuring power)
`FlashRead`       | `Always`   | Allow dumping a hash of the AP or EC flash
`OpenNoDevMode`   | `IfOpened` | Allow opening GSC without developer mode
`OpenFromUSB`     | `IfOpened` | Allow opening GSC from USB

## Consoles {#consoles}

GSC presents 3 consoles through CCD: AP, EC, and GSC, each of which show up on
your host machine as a `/dev/ttyUSBX` device when a debug cable ([Suzy-Q] or
[Type-C Servo v4]) is plugged in to the DUT.

Console | Default access                              | Capability Name
------- | ------------------------------------------- | ---------------
GSC     | always read/write, but commands are limited | `GscFullConsole` enables the full set of GSC console commands
AP      | read/write                                  | `UartGscRxAPTx` / `UartGscTxAPRx`
EC      | read-only                                   | `UartGscRxECTx` / `UartGscTxECRx`

### Connecting to a Console

When a debug cable ([Suzy-Q] or [Type-C Servo v4]) is plugged in to the DUT, the
3 consoles will show up as `/dev/ttyUSBX` devices. You can connect to them with
your favorite terminal program (e.g., `minicom`, `screen`, etc). You can also
use the [`usb_console`] command to connect to Cr50 (`18d1:5014`) or Ti50
(`18d1:504a`) and specify the interface to choose between the consoles.

```bash
# Install `usb_console`
(chroot) sudo emerge ec-devutils
```

```bash
# Connect to Cr50 (GSC) console
(chroot) $ sudo usb_console -d 18d1:5014
# Connect to Ti50 (GSC) console
(chroot) $ sudo usb_console -d 18d1:504a
```

```bash
# Connect to AP console (on Cr50-based device)
(chroot) $ sudo usb_console -d 18d1:5014 -i 1
# Connect to AP console (on Ti50-based device)
(chroot) $ sudo usb_console -d 18d1:504a -i 1
```

```bash
# Connect to EC console (on Cr50-based device)
(chroot) $ sudo usb_console -d 18d1:5014 -i 2
# Connect to EC console (on Ti50-based device)
(chroot) $ sudo usb_console -d 18d1:504a -i 2
```

#### Using "servod" to access the console

[`servod`] can be used to create alternative console devices when combined with
a [Servo].

First, make sure your [servo firmware is updated][update servo v4].

Next, start [`servod`]:

```bash
(chroot) $ sudo servod -b $BOARD
```

Then use `dut-control` to display the console devices:

```bash
(chroot) $ dut-control gsc_uart_pty ec_uart_pty cpu_uart_pty
```

Connect to the console devices with your favorite terminal program (e.g.,
`minicom`, `screen`, etc.).

## CCD Open {#ccd-open}

Some basic CCD functionality is accessible by default: read-only access to the
EC console, read-write access to the AP console, and a few basic GSC console
commands. Note that while GSC has read-write access to the AP console by
default, the AP console itself is disabled for production devices.

In order to access all CCD functionality or to modify capability settings, GSC
CCD needs to be [`Open`].

1.  Connect to the GSC console by connecting a [Suzy-Q] or [Type-C Servo v4] to
    the DUT and running one of the following commands:

    ```bash
    # Connect to Cr50 (GSC) console
    (chroot) $ sudo usb_console -d 18d1:5014
    # Connect to Ti50 (GSC) console
    (chroot) $ sudo usb_console -d 18d1:504a
    ```

    *** note
    **NOTE**: If another program is already connected to the GSC console,
    you'll see `tx [Errno 16] Resource Busy`. For example, this will happen if
    [`servod`] is running.
    ***

1.  At the GSC console, use the `version` command to make sure you have a recent
    enough version to use CCD. The relevant version is either `RW_A` or `RW_B`,
    whichever has the asterisk next to it:

    ```
    cr50 > version

    Chip:    g cr50 B2-C
    Board:   0
    RO_A:  * 0.0.10/29d77172
    RO_B:    0.0.10/c2a3f8f9
    RW_A:  * 0.3.23/cr50_v1.9308_87_mp.320-aa1dd98  <---- This is the version
    RW_B:    0.3.18/cr50_v1.9308_87_mp.236-8052858
    BID A:   00000000:00000000:00000000 Yes
    BID B:   00000000:00000000:00000000 Yes
    Build:   0.3.23/cr50_v1.9308_87_mp.320-aa1dd98
             tpm2:v1.9308_26_0.36-d1631ea
             cryptoc:v1.9308_26_0.2-a4a45f5
             2019-10-14 19:18:05 @chromeos-ci-legacy-us-central2
    ```

1.  Production (`MP`) versions of Cr50 firmware use a [minor version][semver] of
    `3`: `0.3.x`. Production firmware versions `0.3.9` or newer support CCD.

    Production (`MP`) versions of Ti50 firmware use a [minor version][semver] of
    `23`: `0.23.x`.

    Development (`PrePVT`) versions of Cr50 firmware use a minor version of `4`:
    `0.4.x`. Development firmware versions `0.4.9` or newer support CCD.

    Development (`PrePVT`) versions of Ti50 firmware use a minor version of
    `24`: `0.24.x`.

    Your device likely supports CCD if it was manufactured in the last few
    years. If you have an older version, follow the [Updating Cr50] instructions
    before continuing.

1.  Put the device into [Recovery Mode] and enable [Developer Mode].

    *** note
    **NOTE**: Developer Mode has to be enabled as described. Using GBB flags to
    force Developer Mode will not work.
    ***

    If you can't put your device into [Developer Mode] because it doesn't boot,
    follow the [CCD Open Without Booting the Device] instructions.

1.  Verify GSC knows the device is in [Developer Mode] by finding `TPM:
    dev_mode` in the GSC console `ccd` command output:

    ```
    (gsc) > ccd
          ...
          TPM: dev_mode                     <==== This is the important part
          ...
    ```

1.  Start the CCD open process from the AP.

    ```bash
    (dut) $ gsctool -a -o
    ```

1.  Over the next 5 minutes you will be prompted to press the power button
    multiple times. After the last power button press the device will reboot.

    *** note
    **WARNING**: Opening CCD causes GSC to forget that it is in
    [Developer Mode], so when the device reboots, it will either say that
    the OS image is invalid or it will enter a bootloop. Use the key
    combinations to enter [Recovery Mode] and re-enable [Developer Mode].
    See [this bug] for details.
    ***

1.  Use the `ccd` command on the GSC console to verify the state is [`Open`]:

    ```
    (gsc) > ccd

    State: Opened
    ...
    ```

1.  **The [`Open`] state is lost if GSC reboots, the device loses power (e.g.,
    battery runs out and AC is not plugged in), or the battery is removed. Note
    that GSC does not reboot when the system reboots; it only reboots if it is
    updated, the devices loses power, the battery runs out, or it crashes**. If
    you plan on [flashing the AP firmware] or [flashing the EC firmware], it is
    recommended you modify the capability settings or set a CCD password, so you
    can reopen the device in the case that you accidentally brick it with bad
    firmware. The simplest way to do this is to reset to factory settings and
    enable testlab mode:

    ```
    (gsc) > ccd reset factory
    ```

    ```
    (gsc) > ccd testlab enable
    ```

    For full details, see the section on [CCD Open Without Booting the Device].

## Configuring CCD Capability Settings

CCD capabilities allow you to configure CCD to restrict or open the device as
much as you want. You can use the `ccd` command on the GSC console to check and
modify the capabilities, but CCD has to be [`Open`] to change the capabilities.

Setting capabilities you want to use to [`Always`] will make them accessible
even if CCD loses the [`Open`] state, which happens when GSC reboots or the
device loses power.

Basic CCD functionality is covered by `UartGscTxECRx`, `UartGscRxECTx`,
`UartGscTxAPRx`, `UartGscRxAPTx`, [`FlashAP`], [`FlashEC`], [`OverrideWP`], and
`GscFullConsole`.

```
(gsc) > ccd set $CAPABILITY $REQUIREMENT
```

### Examples

#### EC Console

If the EC console needs to be read-write even when CCD is [`Locked`] set the
capability to [`Always`]:

```
(gsc) > ccd set UartGscTxECRx Always
```

#### Restrict Consoles

If you want to restrict capabilities more than [`Always`], you can set them to
[`IfOpened`], which will make it so that it is only accessible when CCD is
[`Open`]ed, not [`Lock`]ed:

##### Restrict EC

```
(gsc) > ccd set UartGscTxECRx IfOpened
(gsc) > ccd set UartGscRxECTx IfOpened
```

##### Restrict AP

```
(gsc) > ccd set UartGscTxAPRx IfOpened
(gsc) > ccd set UartGscRxAPTx IfOpened
```

#### Most Accessible

If you want things as accessible as possible and want all capabilities to be
[`Always`], you can run

```
(gsc) > ccd reset factory
```

This will also permanently disable write protect. To reset write protect run

```
(gsc) > wp follow_batt_pres atboot
```

To reset capabilities to Default run

```
(gsc) > ccd reset
```

## Flashing EC {#flashec}

Flashing the EC is restricted by the `FlashEC` capability.

The steps to flash the EC differ based on the board being used, but the
[`flash_ec`] script will handle this for you.

```bash
(chroot) $ sudo servod -b $BOARD
(chroot) $ ~/trunk/src/platform/ec/util/flash_ec --image $IMAGE --board $BOARD
```

## Flashing the AP {#flashap}

*** note
**WARNING**: Before attempting to flash the AP firmware, start with the
[CCD Open] steps; if you flash broken firmware before opening CCD, you may make
it impossible to restore your device to a working state.
***

Flashing the AP is restricted by the `FlashAP` capability.

```bash
(chroot) $ sudo flashrom -p raiden_debug_spi:target=AP -w $IMAGE
```

This default flashing command takes a very long time to complete, there are ways
to [speed up the flashing process] by cutting some corners.

If you have many CCD devices connected, you may want to use the GSC serial
number:

```bash
# For Cr50-based device
(chroot) $ lsusb -vd 18d1:5014 | grep iSerial
# For Tir50-based device
(chroot) $ lsusb -vd 18d1:504a | grep iSerial
```

You can then add the serial number to the [`flashrom`] command:

```bash
(chroot) $ sudo flashrom -p raiden_debug_spi:target=AP,serial=$SERIAL -w $IMAGE
```

**If you don't see GSC print any messages when you're running the [`flashrom`]
command and you have more than one GSC device connected to your workstation, you
probably need to use the serial number.**

### Special Cases {#flashap-special-cases}

GSC puts the device in reset to flash the AP. Due to hardware limitations GSC
may not be able to disable hardware write protect while the device is in reset.
If you want to reflash the AP RO firmware using CCD and your board has issues
disabling hardware write protect, you may need to also disable software write
protect.

If you suspect the board you are using has this issue, you can try this:

1.  Disable write protect using the GSC console command:

    ```
    (gsc) > wp disable
    ```

2.  Disable software write protect via CCD:

    ```bash
    (chroot) $ sudo futility flash --wp-disable --servo
    ```

## Control Hardware Write Protect {#hw-wp}

Control of hardware write protect is restricted by the `OverrideWP` capability.
When the capability is allowed, the hardware write protect setting can be
controlled with the `wp` command in the GSC console. Otherwise, the hardware
write protect is determined based on the presence of the battery.

Hardware Write Protect Setting | Battery State                  | Hardware Write Protect State
------------------------------ | ------------------------------ | ----------------------------
`follow_batt_pres`             | Connected                      | Enabled
`follow_batt_pres`             | Disconnected                   | Disabled
`follow_batt_pres`             | N/A (Chromebox has no battery) | Write Protect Screw means Enabled
`enabled`                      | Any                            | Enabled
`disabled`                     | Any                            | Disabled

### Write Protect Commands

```
(gsc) > wp [enable|disable|follow_batt_pres]
```

There are two write protect settings: the current setting and the `atboot`
setting.

The `wp` command adjusts the current write protect setting that will last until
GSC reboots or loses power. Note that GSC does not reboot when the rest of the
system reboots. It will only reboot in the cases where the firmware is being
updated, it crashes, the battery completely drains, the battery is removed, or
power is otherwise lost.

The `atboot` setting is the state of the write protect when GSC boots; it
defaults to `follow_batt_pres`.

To change the `atboot` setting, add the `atboot` arg to the end of the `wp`
command:

```
(gsc) > wp [enable|disable|follow_batt_pres] atboot
```

You can query the write protect state with `gsctool`:

```bash
(dut) $ gsctool -a -w

...
Flash WP: forced disabled  <-- Current hardware write protect state
 at boot: forced disabled  <-- "atboot" hardware write protect state

```

`gsctool -a -w` Status | Hardware Write Protect State
---------------------- | ------------------------------------
`forced disabled`      | Disabled
`forced enabled`       | Enabled
`enabled`              | Enabled, following battery presence
`disabled`             | Disabled, following battery presence

### Special Case Devices

Bob devices have a write protect screw in addition to battery presence. The
write protect screw will force enable write protect until it's removed. If GSC
is set to `follow_batt_pres`, you need to remove the write protect screw and
disconnect the battery to disable write protect. If you run `wp disable`, you
will also need to remove the screw.

If you are attempting to flash the AP, see the [Flashing the AP Special Cases]
section for additional steps you may have to take to disable write protection.

## UART Rescue mode

### Overview

UART Rescue Mode is a feature of the GSC RO firmware that supports programming
the RW firmware using only the UART interface. This is used to recover a bad RW
firmware update (which should be rare).

This is also useful when bringing up new designs, as this allows to update GSC
image even before USB CCD or TPM interfaces are operational.

UART rescue works on all existing devices, all it requires is that GSC console
is mapped to a `/dev/xxx` device on the workstation (the same device used to
attach a terminal to the console).

Rescue works as follows: when the RO starts, after printing the regular banner
on the console it prints a magic string to the console and momentarily waits for
the host to send a sync symbol, to indicate that an alternative RW will have to
be loaded over UART. The RO also enters this mode if there is no valid RW to
run.

When rescue mode is triggered, the RO is expecting the host to transfer a single
RW image in hex format.

Follow the [brescue](./gsc_without_servod.md#gsc-rescue) procedure to perform a
GSC rescue.

## CCD Open Without Booting the Device {#ccd-open-no-boot}

If you can’t boot your device, you won’t be able to enable [Developer Mode] to
send the open command from the AP. If you have enabled CCD on the device before,
GSC may be configured in a way that you can still open GSC.

### Option 1: Remove the battery

If you can remove the battery, you can bypass the [Developer Mode] requirements.
`ccd open` is allowed from the GSC console if the Chrome OS Firmware Management
Parameters (`FWMP`) do not disable CCD and the battery is disconnected. This is
the most universal method and will work even if you haven’t enabled CCD before.

1.  Disconnect the battery

1.  Send `ccd open` from the GSC console.

### Option 2: "OpenNoDevMode" and "OpenFromUSB" are set to Always

If "OpenNoDevMode" and "OpenFromUSB" are set to Always, you will be able to open
GSC from the GSC console without enabling [Developer Mode]:

```
(gsc) > ccd open
```

You will still need physical presence (i.e., press the power button) unless
`testlab` mode is also enabled:

```
(gsc) > ccd testlab
       CCD test lab mode enabled
```

#### Enabling

If CCD is [`Open`], you can enable these settings with:

```
(gsc) > ccd set OpenFromUSB Always
(gsc) > ccd set OpenNoDevMode Always
```

### Option 3: CCD Password is Set

If the CCD password is set, you can open from the GSC console without
[Developer Mode].

```
(gsc) > ccd open $PASSWORD
(gsc) > ccd unlock $PASSWORD
```

Alternatively, you can use `gsctool`, entering the password when prompted:

```
(dut) $ gsctool -a -o
(dut) $ gsctool -a -u
```

#### Enabling

When CCD is [`Open`], run the `gsctool` command and enter the password when
prompted.

```bash
(chroot) $ gsctool -a -P
```

You can use the CCD command on the GSC console to check if the password is set.

```
(gsc) > ccd
       ...
       Password: [none|set]
       ...
```

#### Disabling

When CCD is [`Open`], you can use `gsctool` to clear the password:

```bash
(dut) $ gsctool -a -P clear:$PASSWORD
```

Alternatively, you can use the GSC console to clear the password and reset CCD
capabilities to their default values:

```
(gsc) > ccd reset
```

## Troubleshooting

### rddkeepalive

GSC only enables CCD when it detects a debug accessory is connected (e.g.,
[Suzy-Q] or [Type-C Servo v4]). It detects the cable based on the voltages on
the CC lines. If you are flashing the EC and AP or working with unstable
hardware, these CC voltages may become unreliable for detecting a debug
accessory.

To work around this, you can force GSC to always assume that a debug cable is
detected:

```
(gsc) > rddkeepalive enable
```

*** note
**NOTE**: Enabling `rddkeepalive` does increase power consumption.
***

To disable:

```
(gsc) > rddkeepalive disable
```

### Updating GSC {#updating-cr50}

Production (`MP`) versions of Cr50 firmware use a [minor version][semver] of
`3`: `0.3.x`. Production firmware versions `0.3.9` or newer support CCD.

Production (`MP`) versions of Ti50 firmware use a [minor version][semver] of
`23`: `0.23.x`.

Development (`PrePVT`) versions of Cr50 firmware use a minor version of `4`:
`0.4.x`. Development firmware versions `0.4.9` or newer support CCD.

Development (`PrePVT`) versions of Ti50 firmware use a minor version of `24`:
`0.24.x`.

There aren't many differences between the MP and PrePVT versions of images, but
it is a little easier to CCD [`Open`] PrePVT images. You can't run PrePVT images
on MP devices, so if you're trying to update to PrePVT and it fails try using
the MP image.

1.  Flash a test image newer than M66.

1.  Enable [Developer Mode] and connect a debug cable ([`Suzy-Q`] or [`Type-C
    Servo v4`]).

1.  Check the running GSC version with `gsctool`:

```bash
(dut) $ sudo gsctool -a -f

...
RW 0.4.26  <-- The "RW" version is the one to check
```

1.  Update GSC using the firmware in the OS image:

*Production (MP) image*:

```bash
(dut) $ sudo gsctool -a /opt/google/cr50/firmware/cr50.bin.prod
(dut) $ sudo gsctool -a /opt/google/ti50/firmware/ti50.bin.prod
```

*Development (PrePVT) image*:

```bash
(dut) $ sudo gsctool -a /opt/google/cr50/firmware/cr50.bin.prepvt
(dut) $ sudo gsctool -a /opt/google/ti50/firmware/ti50.bin.prepvt
```

1.  Check the GSC version again to make sure it's either `0.3.X` or `0.4.X`, or
    check the Ti50 version again to make sure it's either `0.23.X` or `0.24.X`.

### Speed up Flashing the AP {#speed-up-ap-flash}

In the [default AP flashing steps][flashap] [`flashrom`] reads the entire flash
contents and only erases and programs the pages that have to be modified.
However, when GSC controls the SPI bus, it can only run at 1.5 MHz, versus the
50 MHz that the AP normally runs it at.

We can take advantage of the fact that Chrome OS device AP firmware is split
into sections, only a few of which are essential for maintaining the device
identity and for booting the device in recovery mode to program faster by only
reading and writing sections we care about:

```bash
# This will save device flash map and VPD sections in
# /tmp/bios.essentials.bin. VPD sections contain information like device
# firmware ID, WiFi calibration, enrollment status, etc. Use the below command
# only if you need to preserve the DUT's identity, no need to run it in case
# the DUT flash is not programmed at all, or you do not care about preserving
# the device identity.
sudo flashrom -p raiden_debug_spi:target=AP -i FMAP -i RO_VPD -i RW_VPD -r /tmp/bios.essentials.bin

# This command will erase the entire flash chip in one shot, the fastest
# possible way to erase.
sudo flashrom -p raiden_debug_spi:target=AP -E

# This command will program essential flash sections necessary for the
# Chrome OS device to boot in recovery mode. Note that the SI_ALL section is
# not always present in the flash image, do not include it if it is not in
# dump_fmap output.
sudo flashrom -p raiden_debug_spi:target=AP -w image-atlas.bin -i FMAP -i WP_RO [-i SI_ALL] --noverify

# This command will restore the previously preserved VPD sections of the
# flash, provided it was saved in the first step above.
sudo flashrom -p raiden_debug_spi:target=AP -w /tmp/bios.essential.bin -i RO_VPD -i RW_VPD --noverify
```

Once flash is programmed, the device can be booted in recovery mode and start
Chrome OS from external storage, following the usual recovery procedure. Once
Chrome OS is installed, AP flash can be updated to include the rest of the image
by running [`flashrom`] or `futility` from the device bash prompt.

[Case Closed Debugging]: ./case_closed_debugging.md
[chromeos-cr50 ebuild]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/main/chromeos-base/chromeos-cr50/chromeos-cr50-0.0.1.ebuild
[chromeos-ti50 ebuild]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/main/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[Developer Mode]: https://chromium.googlesource.com/chromiumos/docs/+/main/developer_mode.md#dev-mode
[Recovery Mode]: https://chromium.googlesource.com/chromiumos/docs/+/main/debug_buttons.md
[Servo]: https://chromium.googlesource.com/chromiumos/third_party/hdctools/+/main/docs/servo.md
[`servod`]: https://chromium.googlesource.com/chromiumos/third_party/hdctools/+/main/docs/servo.md
[Type-C Servo v4]: https://chromium.googlesource.com/chromiumos/third_party/hdctools/+/main/docs/servo_v4.md
[update servo v4]: https://chromium.googlesource.com/chromiumos/third_party/hdctools/+/main/docs/servo_v4.md#updating-firmware
[Suzy-Q]: https://chromium.googlesource.com/chromiumos/third_party/hdctools/+/main/docs/ccd.md#SuzyQ-SuzyQable
[`hdctools`]: https://chromium.googlesource.com/chromiumos/third_party/hdctools/+/refs/heads/main/README.md
[`FlashAP`]: #flashap
[flashing the AP firmware]: #flashap
[flashap]: #flashap
[Flashing the AP Special Cases]: #flashap-special-cases
[`FlashEC`]: #flashec
[flashing the EC firmware]: #flashec
[`OverrideWP`]: #hw-wp
[`Always`]: #cap-priv
[`IfOpened`]: #cap-priv
[`Open`]: #cap-priv
[`Locked`]: #cap-priv
[`Unlocked`]: #cap-priv
[Updating Cr50]: #updating-cr50
[CCD Open Without Booting the Device]: #ccd-open-no-boot
[cap]: #cap
[consoles]: #consoles
[hw-wp]: #hw-wp
[`flash_ec`]: https://chromium.googlesource.com/chromiumos/platform/ec/+/main/util/flash_ec
[CCD Open]: #ccd-open
[`flashrom`]: https://chromium.googlesource.com/chromiumos/third_party/flashrom/+/main/README.chromiumos
[speed up the flashing process]: #speed-up-ap-flash
[this bug]: https://issuetracker.google.com/149420712
[semver]: https://semver.org/
[`usb_console`]: https://chromium.googlesource.com/chromiumos/platform/ec/+/main/extra/usb_serial/console.py
