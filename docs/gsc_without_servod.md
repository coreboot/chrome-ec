# Controlling GSC Without Servod
[TOC]
This write up describes an alternative method of controlling GSC and Chrome
OS devices using Servo Micro or C2D2 called `adapters` below.

The version you are looking at might be not the latest and greatest, [this
link](https://chromium.googlesource.com/chromiumos/platform/ec/+/refs/heads/cr50_stab/docs/gsc_without_servod.md)
points to the most updated copy.

In a typical setup these `adapters` are controlled by the `servod` utility.
This a very powerful tool, it allows to control the huge amount of various
Chrome OS devices, accounting for numerous differences between devices and
`adapters`, providing endless configuration options, etc.

This comes at a hefty price: the user must set up Chrome OS chroot, keep the
chroot in sync, periodically update the servod application, run `servod`
passing it the appropriate command line options depending on the
`adapter`type, etc., etc. Troubleshooting situations when `servod` fails to
start or reports errors during operation requires high level of expertise and
often is pretty time consuming.

Luckily all this complexity could be easily avoided when working with the
`adapters` directly. In each case connecting the `adapter` to the DUT and to
the workstation provides necessary communication channels to access GSC, AP,
and EC console and perform the `rescue` operation when GSC RW firmware needs to
be updated.

## Common One Time Setup

When the setup is connected (the `adapter` is attached to the DUT and connected
to a USB port on the workstation), four `/dev/ttyUSBx` devices are created on
the workstation, which allow access to all consoles of the `adapter` and of the
DUT.

Two helpful scripts available in `../util` in the Chrome OS Cr50 tree are
`maptty.sh` which shows how the TTY devices map to the `adapter(s)` and
`brescue.sh` which takes care of invoking `rescue` with proper command line
parameters (see below information about recovering from botched GSC updates).

If you don't have Chrome OS chroot, to access the scripts you can clone the EC
tree as follows:

```
$ git clone https://chromium.googlesource.com/chromiumos/platform/ec
$ cd ec
$ git checkout -b cr50 origin/cr50_stab
$ ls util/{maptty,brescue}.sh
util/brescue.sh*  util/maptty.sh*
```

## Controlling DUT With Servo Micro

To find the TTY devices created by Servo Micro run:
```
$ ../util/maptty.sh | grep Servo_Micro
/dev/ttyUSB0  /dev/serial/by-id/usb-Google_Inc._Servo_Micro_CMO653-00166-040489J04128-if00-port0
/dev/ttyUSB1  /dev/serial/by-id/usb-Google_Inc._Servo_Micro_CMO653-00166-040489J04128-if03-port0
/dev/ttyUSB2  /dev/serial/by-id/usb-Google_Inc._Servo_Micro_CMO653-00166-040489J04128-if05-port0
/dev/ttyUSB3  /dev/serial/by-id/usb-Google_Inc._Servo_Micro_CMO653-00166-040489J04128-if06-port0
```
The actual device names could be different depending on what is connected to
your workstation, but the order is fixed: the lowest index TTY device
(`/dev/ttyUSB0` in the above example) is the GSC console, the next device is
the Sevo Micro console, the next one is the AP console and the last one is the
EC console.

Run `version` command on the Servo Micro console to confirm that you are
connected to it, you should see something similar to:
```
> version
Chip:   stm stm32f07x
Board:  0
RO:     servo_micro_v2.0.10865+f8e42df1
                servo_micro_14307.0.21_10_31
RW:     servo_micro_v2.0.10865+f8e42df1
                servo_micro_14307.0.21_10_31
...
```
### Reset GSC Using Servo Micro
Servo Micro allows to power up the GSC even if the DUT is not powered, it has
a dedicated GPIO for that. The GSC reset signal is connected to one of the
outputs of TCA6416ARTWR GPIO expander, controlled through I2C. The expander is
attached to I2C port 0 and is configured to respond to bus address 0x20.

Run the following commands on the Servo Micro console to set it up to control the GSC:
```
> gpioset SPI1_VREF_33 1
> i2cxfer w 0 0x20 3 0x1f
```
Now you can generate a GSC reset pulse by running
```
> i2cxfer w 0 0x20 7 0x1f
> i2cxfer w 0 0x20 7 0x5f
```
This is all there is to it.

## Controlling DUT With C2D2

Using C2D2 is even simpler. The same `maptty.sh` script will show the TTY
devices connected when C2D2 is attached:
```
$ ../util/maptty.sh | grep C2D2
/dev/ttyUSB4  /dev/serial/by-id/usb-Google_Inc._C2D2_C2103110780-if00-port0
/dev/ttyUSB5  /dev/serial/by-id/usb-Google_Inc._C2D2_C2103110780-if03-port0
/dev/ttyUSB6  /dev/serial/by-id/usb-Google_Inc._C2D2_C2103110780-if05-port0
/dev/ttyUSB7  /dev/serial/by-id/usb-Google_Inc._C2D2_C2103110780-if06-port0
```

TTY device assignment order is the same as in case of Servo Micro, this is how
C2D2 `version` command looks like:
```
> version
Chip:    stm stm32f07x
Board:   0
RO:      c2d2_v2.4.35-f1113c92b
RW:      c2d2_v2.4.35-f1113c92b
...
```
### Reset GSC Using C2D2
C2D2 does not allow to power up the GSC (DUT power is required), but has a
dedicated console command for resetiing the GSC:
```
> h1_reset 1
> h1_reset 0
```
And this is all there is to using C2D2.

## GSC Rescue

GSC Rescue operation allows to recover from a corrupted GSC RW, when it is not
possible to update it using `gsctool`. Very few people outside of GSC firmware
team would need to use this regularly, but it might come handy in situations
like updating early DT chips.

Rescue procedure requires a utility, which can communicate with the GSC over
UART, the utility is called `rescue`.

If you have Chrome OS chroot the utility can be generated by running `sudo
emerge cr50-utils` and it becomes available as `/usr/bin/cr50-rescue`.

### Building Your Own Rescue Utility Outside Chroot

If you are working outside chroot you can build your own `rescue` executable
as follows (these instructions are for a recent Debian Mint environment,
installing necessary packages could require different commands on different
Linux distributions):
```
$ git clone https://chrome-internal.googlesource.com/chromeos/platform/cr50-utils
$ cd cr50-utils/software/tools/SPI
$ sudo apt-get install libc6 libelf-dev libgcc-s1 libssl-dev libstdc++6 libudev1 libusb-1.0-0-dev zlib1g
$ make rescue
```
This will create the `rescue` utility in the local directory, place it
somewhere to make it available through `PATH`.

### Performing GSC Rescue Procedure

To rescue H1 or DT chip which has a corrupted RW but is still 'alive', i.e.
reacts with the RO boot console output to reset pulse generated by the
`adapter`, first obtain a firmware image to rescue it to. If you have a chroot
you can run `sudo emerge chromeos-[ti|cr]50` and find the latest released
image in
```
/opt/google/[cr|ti]50/firmware/[cr|ti]50.bin.prepvt
```
or you can see the Care and Feeding document for your board of ask your
colleagues where to get a GSC image to use.

Then to rescue the GSC chip, do the following:

 - disconnect terminal from the GSC console TTY device
 - invoke `brescue.sh <path to the firmware image> <GSC console TTY device>`
 - generate GSC reset pulse using instructions based on the adapter used to
 connect to the GSC ([Servo Micro](#reset-gsc-using-servo-micro) or [C2D2](#reset-gsc-using-c2d2))

Here is an example of a Ti50 rescue session:
```
$ ../util/brescue.sh <path to>/ti50.bin.prepvt /dev/ttyUSB4
carved out binary /tmp/brescue.sh.y1uXW/rw.bin mapped to 0x88000
converted to /tmp/brescue.sh.y1uXW/rw.hex, waiting for target reset
flash_start_:  00008000 flash_end_:  00018000
low 00088000, high 000fffff
base 00088000, size 00078000
..maxAdr 0x00100000
..dropped to 0x000f08e8
..skipping from 0x00084000 to 0x00088000
435 frames
(waiting for "Bldr |")
Ravn4|00100000 7f4bdb+
Himg =DE83C230..A01206DE : 1
Hfss =3F08D3AE..015B326D : 1
Hinf =00CFE4A6..B4FEEEDC : 1
jump @00100400

Bldr |(waiting for "oops?|")694539 76b844
oops?|0.1.2.3.4.5.6.7.8.9.10.11.12.13.14.15.16.17.18.19.20.21.22.23.24.25.26.27.28.29.30.31.32.33.34.35.36.37.38.39.40.41.42.43.44.45.46.47.48.49.50.51.52.53.54.55.56.57.58.59.60.61.62.63.64.65.66.67.68.69.70.71.72.73.74.75.76.77.78.79.80.81.82.83.84.85.86.87.88.89.90.91.92.93.94.95.96.97.98.99.100.101.102.103.104.105.106.107.108.109.110.111.112.113.114.115.116.117.118.119.120.121.122.123.124.125.126.127.128.129.130.131.132.133.134.135.136.137.138.139.140.141.142.143.144.145.146.147.148.149.150.151.152.153.154.155.156.157.158.159.160.161.162.163.164.165.166.167.168.169.170.171.172.173.174.175.176.177.178.179.180.181.182.183.184.185.186.187.188.189.190.191.192.193.194.195.196.197.198.199.200.201.202.203.204.205.206.207.208.209.210.211.212.213.214.215.216.217.218.219.220.221.222.223.224.225.226.227.228.229.230.231.232.233.234.235.236.237.238.239.240.241.242.243.244.245.246.247.248.249.250.251.252.253.254.255.256.257.258.259.260.261.262.263.264.265.266.267.268.269.270.271.272.273.274.275.276.277.278.279.280.281.282.283.284.285.286.287.288.289.290.291.292.293.294.295.296.297.298.299.300.301.302.303.304.305.306.307.308.309.310.311.312.313.314.315.316.317.318.319.320.321.322.323.324.325.326.327.328.329.330.331.332.333.334.335.336.337.338.339.340.341.342.343.344.345.346.347.348.349.350.351.352.353.354.355.356.357.358.359.360.361.362.363.364.365.366.367.368.369.370.371.372.373.374.375.376.377.378.379.380.381.382.383.384.385.386.387.388.389.390.391.392.393.394.395.396.397.398.399.400.401.402.403.404.405.406.407.408.409.410.411.412.413.414.415.416.417.418.419.420.421.422.423.424.425.426.427.428.429.430.431.432.433.434.done!
```
### Rescue Troubleshooting

Sometimes after a successful rescue where the GSC starts the RW successfully and
shows up on CCD, the ChromeOS device still fails to boot and falls into
recovery. This could be due to stale TPM data.

If your device continuously falls into recovery on reboots after rescue, boot to
an OS test image via USB through the recovery screen. Once booted in the OS test
image, issue the `crossystem clear_tpm_owner_request=1` command on the kernel
console. Reboot and allow the OS on disk to boot.
