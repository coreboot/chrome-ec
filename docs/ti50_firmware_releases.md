# Ti50 Firmware Releases

This document captures major feature differences between Ti50 firmware releases

[TOC]

# ChromeOS Release

ChromeOS Version    | PrePVT version | Prod Version
------------------- | -------------- | ------------
[ToT][ToT ebuild]   | 0.24.101       | 0.23.101
[M127][127 release] | 0.24.101       | 0.23.101
[M126][126 release] | 0.24.90        | 0.23.90
[M125][125 release] | 0.24.81        | 0.23.81
[M124][124 release] | 0.24.71        | 0.23.71
[M123][123 release] | 0.24.71        | 0.23.71
[M122][122 release] | 0.24.71        | 0.23.71
[M121][121 release] | 0.24.62        | 0.23.62
[M120][120 release] | 0.24.60        | 0.23.60
[M119][119 release] | 0.24.51        | 0.23.51
[M118][118 release] | 0.24.30        | 0.23.30
[M117][117 release] | 0.24.30        | 0.23.30
[M116][116 release] | 0.24.30        | 0.23.30
[M115][115 release] | 0.24.30        | 0.23.30
[M114][114 release] | 0.24.30        | 0.23.30
[M113][113 release] | 0.24.13        | 0.23.14
[M112][112 release] | 0.24.13        | 0.23.3
[M111][111 release] | 0.24.3         | 0.23.3
[M110][110 release] | 0.24.3         | 0.23.3
[M109][109 release] | 0.24.3         | 0.23.3
[M108][108 release] | 0.24.1         | 0.23.1
[M107][107 release] | 0.22.6         | 0.21.0
[M106][106 release] | 0.22.2         | 0.21.0
[M105][105 release] | 0.22.1         | 0.21.0

# Ti50 Features

This table should cover major features, so it's easy to check what features are
supported by a specific Ti50 image.

This table was started with 0.22.6 M107. Some features were complete before
this. It only has information for features completed after 0.22.6.

CCD, EC-EFS2, Factory Mode, Pinweaver, U2F, and Board ID are all supported in
0.22.6. The were added before 0.22.6, so they aren't included in the table.

Feature Description                  | Feature Added | Feature Complete | Release Landed
------------------------------------ | ------------- | ---------------- | --------------
ZTE Serial Number                    |               | 0.22.6           | M107
CCD Open preserved across deep sleep |               | 0.22.6           | M107
AP RO WP Sense                       | 0.22.6        |                  | M107
AP RO Verification (without reset)   | 0.24.0        |                  | M108
Fix updates after PoR and deep sleep | 0.24.14       | 0.23.14          | M113
AP RO Verification Enforcement       | 0.24.61       |                  | M121

# RO revisions

## 0.0.32

Released with 0.0.26 in R107-15100.0.0

*   cryptolib 1.3.0

## 0.0.36 in M108

Released with RW 0.23.0 in M108

*   Rescue timeout improvements
*   Hardware crypto library 1.3.3 with following improvements:
    *   Fixed read issue with 4k RSA keys
    *   BigNumber optimizations (code size and performance)
    *   Optimized blinded p/2 computation in RSA
    *   Added prime checks for RSA key gen from primes
    *   AES GCM now stores and restores context implicitly
    *   Added additional checks that padded value in RSA encrypt is less than N

## 0.0.38 released on 12/21/2022

Released with RW 0.23.3 and 0.24.3

*   Fixed potential RSA key import bug fix in crypto library 1.3.4

## 0.0.40 released on 03/13/2023

Released with RW 0.23.20 and 0.24.20

*   Fix issue signed images headers

## 0.0.46 released on 04/17/2023

*   Cryptolib 1.3.8 with following improvements:
    *   Enabled P384, TDES, CMAC support
    *   Hardened ECDSA error checking in cryptolib
    *   Code size optimizations
    *   AES, GCM, CMAC, RSA security hardening
*   RO code size optimizations, updated internal layout
*   Removed additional protection of the RW INFO rollback space, EFI images will
    be able to erase both Board ID and RW Rollback information stored in INFO
    pages.

## 0.0.52 released on 09/14/2023

Released with RW 0.24.51

## 0.0.56 released on 04/9/2024

Released with RW 0.24.81

*   Updated header enforcing post personalization fuse settings.
    [b/181261702](https://buganizer.corp.google.com/issues/181261702)

# RW revisions

Previously released RW images can be downloaded from
`gs://chromeos-localmirror/distfiles/`, e.g.
`gs://chromeos-localmirror/distfiles/ti50.ro.0.0.26.rw.0.22.1_FFFF_00000000_00000010.tar.xz`

The latest official images are also distributed through the `chromeos-ti50`
portage package in the chroot.

## Rollback Era

Ti50 RW images include a rollback protection field in the header, which is used
to lock out earlier versions from running on the GSC chip after a certain
version has run.

The below tables lists a row for every rollback era. Once both images slots on
GSC progress to a lower row, then FW versions in previous rows are
unavailable -- even with the rescue tool. All versions are **inclusive**.

Bits | Lowest MP | Highest MP | Lowest PrePVT | Highest PrePVT | Reason
---- | --------- | ---------- | ------------- | -------------- | ------
0    | N/A       | N/A        | 0.0.4         | 0.0.16         | Initial development
1    | 0.21.0    | 0.21.1     | 0.22.0        | 0.22.9         | Initial GUC Factory release
2    | 0.23.0    | 0.23.14    | 0.24.0        | 0.24.14        | First MP image shipping on devices
3    | 0.23.20   | 0.23.71    | 0.24.20       | 0.24.71        | Image header fixes
4    | 0.23.74   | current    | 0.24.81       | current        | Enable AP RO verification by default

## MP images

### 0.21.0: Released 05/13/22 in M104-14826.0.0

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/3647883)

Artifacts:
[loc](https://pantheon.corp.google.com/storage/browser/chromeos-releases/firmware-ti50-postsubmit/R103-14778.3.0-1-chromeos/led/hardtmad_google.com/40f35cc72dff5eeeadf4947527013cb9d6da802b81a5f3a27ce02c57ac5c91a2/ti50.tar.bz2)

Release tarball:
gs://chromeos-releases/firmware-ti50-postsubmit/R103-14778.3.0-1-chromeos/led/hardtmad_google.com/40f35cc72dff5eeeadf4947527013cb9d6da802b81a5f3a27ce02c57ac5c91a2/ti50.tar.bz2/

Feature Notes:

*   Released between 0.0.16 and 0.22.0

```
Build: ti50_common:v0.0.2187-caec6ab3
       libtock-rs:v0.0.906-9ddb6ac
       tock:v0.0.9593-4b88c2376
       ms-tpm-20-ref:v0.0.247-f007cc5
       chrome-bot@chromeos-ci-postsubmit-us-east1-d-x32-134-b2s1 2022-05-12 14:19:02
```

### 0.21.1: Not released in ChromeOS - First GUC Image

Artifacts:
[loc](https://pantheon.corp.google.com/storage/browser/chromeos-releases/firmware-ti50-postsubmit/R103-14778.3.0-1-chromeos/led/engeg_google.com/18aa516c77b9dc752a1fe3702b633409711aa228fb33a1d78d4b8bbc2b9f901f/ti50.tar.bz2)

Release tarball:
gs://chromeos-releases/firmware-ti50-postsubmit/R103-14778.3.0-1-chromeos/led/engeg_google.com/18aa516c77b9dc752a1fe3702b633409711aa228fb33a1d78d4b8bbc2b9f901f/ti50.tar.bz2/

Feature Notes:

*   Released between 0.0.16 and 0.22.0
*   First GUC image.

### 0.23.0: Released 11/14/22 in M108

First MP image released on shipping devices.

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4022274)

Artifacts:
[15224.3.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15224.3.0)

Release tarball
gs://chromeos-releases/canary-channel/betty/15224.3.0/ChromeOS-firmware-R107-15224.3.0-betty.tar.bz2

Known Issues:

*   Factory mode detected differently; this causes GSC to re-enter factory mode
    after upgrading from 0.21.1
*   For i2c-based TPM devices, Ti50 won't communicate with AP on first attempt
    if GSC is in deep sleep and system wakes up due to lid open event. Shows up
    as "0x63 Failed to get boot mode from Cr50" error (b/259510330, b/259663369)

Feature Notes:

*   Add AP RO Verification feature, but it does not hold system in reset upon
    failure yet (b/161483233)
*   Add Zero Touch Enrollment support (b/234857025)
*   Add Pinweaver support
*   Add U2F support
*   Add attestation support
*   Add network recovery support
*   Improve SPI and I2C TPM bus stability (b/237493220, b/247168128,
    b/245034621, b/251191468)
*   Fix connection issues with Google-A network (b/240506338)
*   Improve boot time performance (b/241986964)
*   General stability improvements for ti50

```
Build:   ti50_common:v0.0.2613-dbba229a
         libtock-rs:v0.0.913-61d23b3
         tock:v0.0.9622-397f4aaa0
         tpm2:v0.0.292-1a7d322
         @chromeos-ci-firmware-us-east1-d-x32-0-soad 2022-11-07 14:44:25
```

### 0.23.1 Released 12/02/22 in R108

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4077027)

Manifest snapshot: gs://chromeos-manifest-versions/buildspecs/107/15224.5.0.xml

**Added Features:**

*   Fix "0x63 Failed to get boot mode from Cr50" error when waking i2c-based tpm
    device with lid open wake event (b/259510330, b/259663369).

```
Build:   ti50_common:v0.0.2616-f4c7c42d
         libtock-rs:v0.0.913-61d23b3
         tock:v0.0.9622-8d5f2ecda
         tpm2:v0.0.292-a7f6f39
         @chromeos-ci-firmware-us-east1-d-x32-0-ci43 2022-12-02 10:33:22
```

### 0.23.3 Released on 12/21/2022

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4121474)

Builder:
[9](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-mp-15224.B-branch/9/overview)

Artifacts:
[15224.9.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15224.9.0)

Manifest snapshot: gs://chromeos-manifest-versions/buildspecs/107/15224.9.0.xml

**Bug Fixes**

*   Improve G2F signature security
    [b/261874682](https://b.corp.google.com/issues/261874682)
*   Fix U2F attestation problems
    [b/242678758](https://b.corp.google.com/issues/242678758)
*   Do not run AP RO verification on deep sleep wake
    [b/261635049](https://b.corp.google.com/issues/261635049)

**Added Features**

*   Allow setting serial number when BID flags are set, but BID type is blank
    [b/238137986](https://b.corp.google.com/issues/238137986)
*   Process TPM vendor commands from USB even when AP is off
    [b/258320966](https://b.corp.google.com/issues/258320966)

```
Build:   ti50_common:v0.0.2620-2cdd9003
         libtock-rs:v0.0.913-61d23b3
         tock:v0.0.9622-8d5f2ecda
         tpm2:v0.0.292-a7f6f39
         @chromeos-ci-firmware-us-east1-d-x32-0-mkcu 2022-12-16 15:03:49
```

### 0.23.14 Released on 03/10/2023 in M113

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4327051)

Builder:
[15](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-mp-15224.B-branch/15/overview)

Artifacts:
[15224.12.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15224.12.0)

Manifest snapshot: gs://chromeos-manifest-versions/buildspecs/107/15224.12.0.xml

**Bug Fixes**

*   Allow changing AP RO write protect settings until board ID is set
    [b/229016958](https://b.corp.google.com/issues/229016958)
*   Improve USB stablity
    [b/259590362](https://b.corp.google.com/issues/259590362)
*   Allow 0x prefix when entering Board ID flags
    [b/265461193](https://b.corp.google.com/issues/265461193)
*   Make sysinfo command output compatible with Cr50
    [b/263579376](https://b.corp.google.com/issues/263579376)
*   Require short physical presence to enable testlab
    [b/265822083](https://b.corp.google.com/issues/265822083)
*   Disable watchdog only around sleep
    [b/266015400](https://b.corp.google.com/issues/266015400)
*   Do not report false TPM2 p256 errors
    [b/234159838](https://b.corp.google.com/issues/234159838)
*   Fix TPM evict object serialization bug
    [b/263168766](https://b.corp.google.com/issues/263168766)
*   Fix GSC reboot issue when accessing orderly counters from previous ti50 FW
    versions. [b/263168766](https://b.corp.google.com/issues/263168766)
*   Allow update within 60s of PoR
    [b/270401267](https://b.corp.google.com/issues/270401267)
*   Fix turning on updates after deep sleep
    [b/270401267](https://b.corp.google.com/issues/270401267)
*   Fix handling RO update failures
    [b/271503973](https://b.corp.google.com/issues/271503973)
*   Update AP RO verification NonZeroGbbFlags and WrongRootKey error codes

**Added Features**

*   Allow to exclude GBB flags from AP RO hash calculations
    [b/261763740](https://b.corp.google.com/issues/261763740)
*   Show INFO space factory mode state in `sysinfo` output
*   Add I2C errors to FLOG
*   Support two root keys AP RO verification, prepvt and mp
    [b/261600803](https://b.corp.google.com/issues/261600803)
*   Add FLOG entry for crashes
*   Use initial factory mode indicator (INFO space value) to allow setting SN
    [b/264261220](https://b.corp.google.com/issues/264261220)
*   Process TPM vendor commands according their source (USB vs TPM)
    [b/266955081](https://b.corp.google.com/issues/266955081)

```
Build:   ti50_common:v0.0.2802-000016bf
         libtock-rs:v0.0.913-61d23b3
         tock:v0.0.9624-338968540
         ms-tpm-20-ref:v0.0.310-9f3037a
         @chromeos-ci-firmware-us-east1-d-x32-0-v1rm 2023-03-07 10:44:14
```

### 0.23.21 Released on 4/18/2023 in M114

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4442649)

Builder:
[16](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-mp-15224.B-branch/16/overview)

Artifacts:
[15224.13.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15224.13.0)

Manifest snapshot: gs://chromeos-manifest-versions/buildspecs/107/15224.13.0.xml

**New Features**

*   New additional metrics, including boot time and a detailed error code for AP
    RO verification. Now available through the `GetTi50Metrics` vendor command
    with value 0x41. [b/262608026](https://b.corp.google.com/issues/262608026),
    [b/263298180](https://b.corp.google.com/issues/263298180)
*   New vendor commands 0x42 and 0x43 to get crash and console logs.
    [b/268396021](https://b.corp.google.com/issues/268396021),
    [b/265310865](https://b.corp.google.com/issues/265310865)
*   Support pinweaver v2.
    [b/248209280](https://b.corp.google.com/issues/248209280)
*   Add Widevine UDS to virtual NV
    [b/248610274](https://b.corp.google.com/issues/248610274)
*   Add GetRSUDevID command to TPM for RMA.
    [b/265309995](https://b.corp.google.com/issues/265309995)

**Bug Fixes**

*   Fix CCD open failure with the battery disconnected.
    [b/270712314](https://b.corp.google.com/issues/270712314)
*   Fix rejection of RO and RW header updates with invalid fields.
    [b/272057805](https://b.corp.google.com/issues/272057805)
*   Fix `wp follow_batt_pres` command in GSC console.
    [b/269218898](https://b.corp.google.com/issues/269218898)
*   Fix FIDO errors, reporting an invalid P-256 public key.
    [b/271795015](https://b.corp.google.com/issues/271795015)
*   Restrict the `recbtnforce` command to the GscFullConsole CCD capability
    [b/268219945](https://b.corp.google.com/issues/268219945)
*   Fix pinweaver key import/export to be compatible with v2.
    [b/267729980](https://b.corp.google.com/issues/267729980)
*   Fix AP/EC comms broken by EC then AP flash resulting in detached I2C lines.
    [b/264817647](https://b.corp.google.com/issues/264817647)
*   Stabilize console output line order.
    [b/276491121](https://b.corp.google.com/issues/276491121)
*   Fix `\r\r\n` console output.
    [b/242980684](https://b.corp.google.com/issues/242980684)
*   Fix issue where EC not put in reset on recovery key combo when GSC in deep
    sleep. [b/248161678](https://b.corp.google.com/issues/248161678)

```
Build:   ti50_common:v0.0.2949-4ee72fd9
         libtock-rs:v0.0.915-d883b40
         tock:v0.0.9629-77d147129
         ms-tpm-20-ref:v0.0.312-affdc53
         @chromeos-ci-firmware-us-central1-b-x32-0-j9et 2023-04-11 06:54:23
```

### 0.23.30 Released on 4/21/2023 in M114 (GUC version)

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4460212)

Builder:
[17](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-mp-15224.B-branch/17/overview)

Artifacts:
[15224.14.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15224.14.0)

Manifest snapshot: gs://chromeos-manifest-versions/buildspecs/107/15224.14.0.xml

Starting June 2023, the GSC comes preloaded from the GUC factory with this
version.

**Features**

*   Add factory config support
    [b/275356839](https://b.corp.google.com/issues/275356839)

**Bug Fixes**

*   Add PCR-based policy to update fwmp and antirollback spaces
    [b/274977008](https://b.corp.google.com/issues/274977008)
*   Remove crash id parameter from GetCrashLog command
    [b/265310865](https://b.corp.google.com/issues/265310865)
*   AP RO verification returns detailed results
    [b/263298180](https://b.corp.google.com/issues/263298180)

```
Build:   ti50_common_mp-15224.B:v0.0.186-6bcd2134
         libtock-rs:v0.0.918-4fc5bc9
         tock:v0.0.9631-d746cb946
         ms-tpm-20-ref:v0.0.316-e4c9719
         @chromeos-ci-firmware-us-east1-d-x32-0-1zci 2023-04-18 13:30:17
```

### 0.23.40 Released on 6/14/2023 in M116

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4615051)

Builder:
[18](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-mp-15224.B-branch/18/overview)

Artifacts:
[15224.15.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15224.15.0)

Manifest snapshot: gs://chromeos-manifest-versions/buildspecs/107/15224.15.0.xml

**Features**

*   Add support for overwriting AP RO verification settings of `0 0` in field
    for OS scipts [b/260721505](https://b.corp.google.com/issues/260721505)
*   Add Shimless RMA keycombo support for verification failure case
    [b/260721505](https://b.corp.google.com/issues/260721505)

**Bug Fixes**

*   Include improvement/fix for 0x63 boot issues
    [b/273189926](https://b.corp.google.com/issues/273189926)

```
Build:   ti50_common_mp-15224.B:v0.0.302-2afc1adc
         libtock-rs:v0.0.918-4fc5bc9
         tock:v0.0.9644-adf05c6cf
         ms-tpm-20-ref:v0.0.318-945d2e4
         @chromeos-ci-firmware-us-central1-b-x32-0-n85q 2023-06-07 21:19:14
```

### 0.23.51 Released on 9/27/2023 in M119

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4895385)

Builder
[32](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-mp-15224.B-branch/32/overview)

Artifacts:
[15224.29.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15224.29.0)

**Features**

*   `ecrst pulse` command is now safe
*   Add initial factory mode to `gsctool`'s CCD print out
*   SPI flash performance for AP twice as fast at Cr50 now
*   Add `rddkeepalive` state in feedback reports
*   GSC console improved logging by adding timestamp prefix to each line
*   Added GSC bootloader stage to boot tracer time monitor
    [b/275390574](https://b.corp.google.com/issues/275390574)

**Bug Fixes**

*   Improve DT clock stretching behavior for I2C
    [b/285366491](https://b.corp.google.com/issues/285366491)
*   Recover after unexpected reads
    [b/225044349](https://b.corp.google.com/issues/225044349)
*   Fix race condition with deep sleep resume resetting EC
    [b/296518779](https://b.corp.google.com/issues/296518779)
*   Improve flashing EC/AP firmware through CCD when external CCD is unstable
    [b/295584404](https://b.corp.google.com/issues/295584404)

```
Build:   ti50_common_mp-15224.B:v0.0.621-b1796c1e
        libtock-rs:v0.0.925-7239450
        tock:v0.0.9658-4c5d1f940
        ms-tpm-20-ref:v0.0.326-65222ec
        @chromeos-ci-firmware-us-central2-d-x32-0-ca6m 2023-09-22 07:23:46
```

### 0.23.60 Released on 10/26/2023 in M120

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4980930)

Builder
[36](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-mp-15224.B-branch/36/overview)

Artifacts:
[15224.33.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15224.33.0)

**Features**

*   Add Widevine RoT virtual NV.
    [b/248610274](https://b.corp.google.com/issues/248610274)
*   Add the CIK cert virtual NV.
    [b/248610274](https://b.corp.google.com/issues/248610274)
*   Enforce WP forced enabled when FWMP dev mode disable is present.
    [b/299947142](https://b.corp.google.com/issues/299947142)
*   Add extended AP RO verification status.
    [b/259098185](https://b.corp.google.com/issues/259098185)
*   Ensure ccd mode active and power button resets GSC if failed AP RO
    verification. [b/259098185](https://b.corp.google.com/issues/259098185)
*   Add GetChassisOpen TPMV command.
    [b/257255419](https://b.corp.google.com/issues/257255419)

**Bug Fixes**

*   Fix get console logs vendor command.
    [b/302383688](https://b.corp.google.com/issues/302383688)
*   Prevent flog error from reading past the end of the page.
    [b/302383688](https://b.corp.google.com/issues/302383688)

```
Build:   ti50_common_mp-15224.B:v0.0.729-2ab3d1fb
         libtock-rs:v0.0.925-7239450
         tock:v0.0.9660-5bae23fce
         ms-tpm-20-ref:v0.0.329-585067c
         @chromeos-ci-firmware-us-central2-d-x32-0-mjce 2023-10-16 13:29:26
```

### 0.23.62 Released on 12/01/2023 in M121

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/5078265)

Builder
[44](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-mp-15224.B-branch/44/overview)

Artifacts:
[15224.41.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15224.41.0)

**Features**

*   Add more information to AP RO Verification result UMA report
    [b/259098185](https://b.corp.google.com/issues/259098185)

```
Build:   ti50_common_mp-15224.B:v0.0.732-7f94b899
         libtock-rs:v0.0.925-7239450
         tock:v0.0.9660-5bae23fce
         ms-tpm-20-ref:v0.0.329-585067c
         @chromeos-ci-firmware-us-central2-d-x32-0-e2uq 2023-11-30 07:33:10
```

### 0.23.70 Released on 1/17/2024 in M122

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/5207755)

Builder
[52](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-mp-15224.B-branch/52/overview)

Artifacts:
[15224.49.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15224.49.0)

**Features**

*   rsu: Increase key generation limit from 10 to 100.
    [b/301156378](https://b.corp.google.com/issues/301156378)
*   ap-ro: Add exception for Frostflow RLZ codes.
    [b/309473916](https://b.corp.google.com/issues/309473916)
*   tpm2: Allow platform read for virtual nvmem.

**Bug Fixes**

*   usb_spi: handle setup packet errors properly.
    [b/302691530](https://b.corp.google.com/issues/302691530)
*   usb_client: prevent lockups when users don't consume RX data.
    [b/302691530](https://b.corp.google.com/issues/302691530)
*   wp: do not set at_boot setting for WP TPMV Cmd disable.
    [b/257255419](https://b.corp.google.com/issues/257255419)
*   tpm2: Fix the wrong signature of widevine cert.
    [b/248610274](https://b.corp.google.com/issues/248610274)
*   cryptolib: adjust CIK & CEK key gen and certs to match actuals.
    [b/308473146](https://b.corp.google.com/issues/308473146)
*   flog: Recover from corrupted entries.
    [b/302383688](https://b.corp.google.com/issues/302383688)
*   fwmp: Reload WP setting when TPM is wiped.
    [b/312396594](https://b.corp.google.com/issues/312396594)
*   ap_ro_verification: Always re-check verification if cached failed.
    [b/315341905](https://b.corp.google.com/issues/315341905)
*   rbox: Do not reset GSC on power button push during ccd open.
    [b/314185172](https://b.corp.google.com/issues/314185172)
*   capsules/i2c_programmer.rs: Respect I2C CCD capability.
    [b/317087536](https://b.corp.google.com/issues/317087536)
*   sys_mgr.rs: Advertise SPI/I2C in board properties.
    [b/307539350](https://b.corp.google.com/issues/307539350)
*   tpm: Save PCR values to NV.
    [b/316884342](https://b.corp.google.com/issues/316884342)
*   tpm_vendor: some commands are disallowed over USB in non DBG mode.
    [b/318518004](https://b.corp.google.com/issues/318518004)

```
Build:   ti50_common_mp-15224.B:v0.0.876-5b460716
         libtock-rs:v0.0.929-0b84d08
         tock:v0.0.9663-71efb979a
         ms-tpm-20-ref:v0.0.331-6f7f352
         @chromeos-ci-firmware-us-east1-d-x32-0-z9ng 2024-01-12 12:55:08
```

### 0.23.71 Released on 1/19/2024 in M122

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/5217758)

Builder
[53](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-mp-15224.B-branch/53/overview)

Artifacts:
[15224.50.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15224.50.0)

**Features**

*   Change default write protect setting to force enabled (does not follow
    chassis open gpio by default)
    [b/257255419](https://b.corp.google.com/issues/257255419)
*   Enforce system reset upon AP RO verification failure.
    [b/259098185](https://b.corp.google.com/issues/259098185)

```
Build:   ti50_common_mp-15224.B:v0.0.879-637bdde3
         libtock-rs:v0.0.929-0b84d08
         tock:v0.0.9663-71efb979a
         ms-tpm-20-ref:v0.0.331-6f7f352
         @chromeos-ci-firmware-us-central1-b-x32-0-e7r7 2024-01-17 14:47:03
```

### 0.23.74 Released to GUC 06/2024 (GUC version)

Builder
[66](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-mp-15224.B-branch/66/overview)

Artifacts:
[15224.63.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15224.63.0)

This is the first version in the 4-bit [Rollback Era](#Rollback-Era).

Starting June 2024, the GSC comes preloaded from the GUC factory with this
version.

```
Build:   ti50_common_mp-15224.B:v0.0.884-70a01408
         libtock-rs:v0.0.929-0b84d08
         tock:v0.0.9663-71efb979a
         ms-tpm-20-ref:v0.0.331-6f7f352
         @chromeos-ci-firmware-us-east1-d-x32-0-sbb9 2024-03-26 11:42:52
```

### 0.23.81 Released on 4/12/2024 in M125

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/5450420)

Builder
[69](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-mp-15224.B-branch/69/overview)

Artifacts:
[15224.66.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15224.66.0)

**Features**

*   flog: Add entries for AP RO verification
*   rbox: Set key debounce to 20us
*   wp: Monitor WP_SENSE_L and WP state for GSC reboots
*   tpm2: Allow platform read for virtual nvmem
*   filesystem: Print NV partition on release builds.

```
Build:   ti50_common_mp-15224.B:v0.0.1091-c88c4ab9
         libtock-rs:v0.0.929-0b84d08
         tock:v0.0.9683-f0ca4d1a7
         ms-tpm-20-ref:v0.0.334-628c70e
         @chromeos-ci-firmware-us-central2-d-x32-0-hijo 2024-04-08 15:08:25
```

**Bug Fixes**

*   i2c_programmer: Ensure that ITE waveform response is always 4 bytes
    [b/326258077](https://b.corp.google.com/issues/326258077)
*   crashlog: Fix generation incrementation.
    [b/317804130](https://b.corp.google.com/issues/317804130)
*   flog: Attempt clear flog if initialization fails.
    [b/317221434](https://b.corp.google.com/issues/317221434)
*   ports/dauntless: Fix race in I2C driver
    [b/322037216](https://b.corp.google.com/issues/322037216)
*   filesystem: Handle compaction when all pages are full.
    [b/322037216](https://b.corp.google.com/issues/323043338)
*   event_log: Ensure time always moves forward on init.
    [b/329326190](https://b.corp.google.com/issues/329326190)

### 0.23.90 Released on 5/09/2024 in M126

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/5529740)

Builder
[74](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-mp-15224.B-branch/74/overview)

Artifacts:
[15224.71.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15224.71.0)

**Features**

*   capsules: Allow dynamically changing baud rate

```
Build:   ti50_common_mp-15224.B:v0.0.1148-c04edba0
         libtock-rs:v0.0.932-419cdc2
         tock:v0.0.9685-1ae1fff89
         ms-tpm-20-ref:v0.0.334-628c70e
         @chromeos-ci-firmware-us-central1-b-x32-0-8m95 2024-05-06 10:59:19
```

**Bug Fixes**

*   tpm2: Check if a key is already wrapped before wrapping
    [b/302699979](https://b.corp.google.com/issues/302699979)
*   cryptolib: Prevent buffer overrun
    [b/327499069](https://b.corp.google.com/issues/327499069)
*   dispatcher: Ensure buffer is inaccessible after enqueued
    [b/332326497](https://b.corp.google.com/issues/332326497)

### 0.23.101 Released on 6/25/2024 in M128 (cherry-picked to M127)

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/5651548)

Builder
[79](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-mp-15224.B-branch/79/overview)

Artifacts:
[15224.76.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15224.76.0)

**Features**

*   Print chip ID on boot
*   Print reset type earlier
*   pmu: Delay sleep when sleep mask changes
*   rbox: Update tablet RMA sequence to use taps

```
Build:   ti50_common_mp-15224.B:v0.0.1203-81f5f518
         libtock-rs:v0.0.932-419cdc2
         tock:v0.0.9687-a764056a2
         ms-tpm-20-ref:v0.0.336-d9aef2b
         @chromeos-ci-firmware-us-central2-d-x32-0-dbfd 2024-06-04 12:40:30
```

**Bug Fixes**

*   fix ti50 hang issue
    [b/339262751](https://b.corp.google.com/issues/339262751)


## PrePVT images

### 0.22.0 Released 06/21/22

From post submit release
[19748](https://luci-milo.appspot.com/ui/p/chromeos/builders/postsubmit/firmware-ti50-postsubmit/19748/overview).

*   Add ChromeOS Identity/Attestation support (b/173326151)
*   Adds U2F application (b/233971198)
*   Add network recovery support (b/217278402)
*   Improve FAFT stability (e.g. FWMP and RMA unlock)
*   Wait 10 seconds after RMA/CCD open until forced AP reboot (b/231222819)
*   Fix `dut-control active_dut_controller` case insensitivity issue
    (b/233283958)
*   Fixed intermittent watchdog resets (b/235344334)

```
for d in $(repo list | sed 's/ .*//'); do
  printf '%32s %s\n' $d $(git -C $d rev-parse HEAD);
done
                        chromite 0581b6624322177b9a15a6dd585ab02e14164a48
                          common b39a1736c0c26418a69fc44dd6f7b910a8eab4b2
                        manifest 2ad62b0d138785249ac98283f17a1d6ee9428be8
                ports/cr50-utils da48c5d2ef77a7de7755633386f53540f7db1b2c
                 ports/pinweaver fc39c8b509da8a45869d7c0e44b263dd631c6fb4
               ports/tpm2_server 6bbf32f9ae1c59df3ec8754d18cd3b065281c3f5
                       repohooks e322b4af8abd1bc63a98c42bb4e831320d02f79a
      third_party/cargo/registry 320b5afca7f3044af5ccd8d6e935355498a47bb5
   third_party/lowrisc/opentitan cf34c94db6cfb4687353babdf3b0557166241c64
      third_party/tock/libtock-c fd756aa2695cdfad8dc3391c6963eb5b65f595b8
     third_party/tock/libtock-rs 9cf55aca92cdac47a608cec226490d92d92cf93b
           third_party/tock/tock c12c1a08ffddaa7ce49dce7164dd630f4e525418
                third_party/tpm2 1159ee2ab3500199dea727c920e131951210b69d
```

### 0.22.1 Released 07/06/22

*   Fix TPM quote and sign bugs making attestation impossible
*   Add u2f support
*   Improve CCD USB reset behavior
*   Prevent EC reset on wake from deep sleep
*   Increase number of AES key contexts

```
for d in $(repo list | sed 's/ .*//'); do
  printf '%32s %s\n' $d $(git -C $d rev-parse HEAD)
done
                        chromite b1d07170c2910c7aa7e9149f6ff2950a91f9ac69
                          common a8929162addb521f39db5cdb99b6228f57091055
                        manifest a8bfc9a825dfaf432324d5edbb58e8a2606d4d07
                ports/cr50-utils da48c5d2ef77a7de7755633386f53540f7db1b2c
                 ports/pinweaver cb73fa7ecf332e8f04a9ae411c851ca9e0fff41a
               ports/tpm2_server 6bbf32f9ae1c59df3ec8754d18cd3b065281c3f5
                       repohooks b03ba18e5d45a6782555c1e41fca0bb218f3868a
      third_party/cargo/registry 4a334c947a3b6b5489379da61121960442f9b8d9
   third_party/lowrisc/opentitan cf34c94db6cfb4687353babdf3b0557166241c64
      third_party/tock/libtock-c fd756aa2695cdfad8dc3391c6963eb5b65f595b8
     third_party/tock/libtock-rs 958193e42ef6a003330e3b47b11cac906d1c7685
           third_party/tock/tock 2f42815fd678b0a908377da99a01d1df2309d984
                third_party/tpm2 1159ee2ab3500199dea727c920e131951210b69d
```

### 0.22.2 Released 07/25/22

From post submit release
[21333](https://luci-milo.appspot.com/ui/p/chromeos/builders/postsubmit/firmware-ti50-postsubmit/21333/overview).

*   Fix issue with I2C-based EC flashing (b/234422943)
*   Add serial number and RMA support for zero touch enrollment (b/230491627)
*   Improve platform level cold boot stress testing performance (b/228429691,
    b/239642389, b/235185547, and b/235553213)
*   Wipe GSC filesystem between developer and production image transition
*   Fix UART race condition that causes intermittent watchdog resets
    (b/235344334)
*   Add user presences timestamp detection for FPMCU automated testing
    (b/217974287)
*   Detect factory mode differently. This causes GSC to re-enter factory mode
    after upgrade to 0.22.2 or later.

```
for d in $(repo list | sed 's/ .*//'); do
  printf '%32s %s\n' $d $(git -C $d rev-parse HEAD)
done
                        chromite e2c258fc1143b37e96a8d17fee12428851aff5bd
                          common 38180a22bc689d2af0d12caa799aee385729f4a6
                        manifest a8bfc9a825dfaf432324d5edbb58e8a2606d4d07
                ports/cr50-utils da48c5d2ef77a7de7755633386f53540f7db1b2c
                 ports/pinweaver cb73fa7ecf332e8f04a9ae411c851ca9e0fff41a
               ports/tpm2_server 6bbf32f9ae1c59df3ec8754d18cd3b065281c3f5
                       repohooks dcb7597b7d8473aef208b87b165c7f14898eafda
      third_party/cargo/registry aa78805c82b9ef0238adae4e81218d09ad248919
   third_party/lowrisc/opentitan cf34c94db6cfb4687353babdf3b0557166241c64
      third_party/tock/libtock-c fd756aa2695cdfad8dc3391c6963eb5b65f595b8
     third_party/tock/libtock-rs f6dab4f4174d9b00fb44adad51f5a26ae4a7b3b2
           third_party/tock/tock 012f3e5e6a8e7a3ce58774278caa5f1ac1af1922
                third_party/tpm2 47c6c19153c6e32933c7112ff6095d60d1632754
```

### 0.22.3 Released 08/11/22

From post submit release
[22127](https://luci-milo.appspot.com/ui/p/chromeos/builders/postsubmit/firmware-ti50-postsubmit/22127/overview)

**Known Issues:**

*   Crypto faults causes issues with log in and GSC FW update (b/242744329)

**Added Features:**

*   Remove internal pull resistors, which prevents leakage current onto SoC
    rails (b/239791508)
*   Refresh key passed through to EC during power button press (b/239674288)
*   Power consumption of normal sleep reduce by 25% down to 9mW
*   Improve cancellation of long running crypto operation
    *   Improves reboot stress tests
*   Add Pinweaver application, which adds pin support for log in
*   Honor `FullGscConosle` CCD cap instead of requiring `ccd open`
*   Fix regression with power button input for `ccd open`
*   Reboot AP instead of GSC after `ccd open` (still delayed by 10 seconds)
*   Improve runtime performance at startup by serializing data, which led to
    less data written to NVMem

```
for d in $(repo list | sed 's/ .*//'); do
  printf '%32s %s\n' $d $(git -C $d rev-parse HEAD)
done
                        chromite b352bb0a31b29d81391ce18c1070fcd34926da1b
                          common 57b43bda52911bc739bb03dee2084ad49ea55bbb
                        manifest a8bfc9a825dfaf432324d5edbb58e8a2606d4d07
                ports/cr50-utils da48c5d2ef77a7de7755633386f53540f7db1b2c
                 ports/pinweaver 3abfd77090d24ca8d2d7260d6ba6aaec2e4c35ae
               ports/tpm2_server 6bbf32f9ae1c59df3ec8754d18cd3b065281c3f5
                       repohooks 32b1168199c41dc9e6e0b91dfe37b0568dee538d
      third_party/cargo/registry e88a2f79e298d9107f82b861d2418f26c9d84c4c
   third_party/lowrisc/opentitan cf34c94db6cfb4687353babdf3b0557166241c64
      third_party/tock/libtock-c fd756aa2695cdfad8dc3391c6963eb5b65f595b8
     third_party/tock/libtock-rs f6dab4f4174d9b00fb44adad51f5a26ae4a7b3b2
           third_party/tock/tock ae35db7727c1b9524fd38459e6834f009476c1cc
                third_party/tpm2 47c6c19153c6e32933c7112ff6095d60d1632754
```

### 0.22.4 Released 08/18/22

From post submit release
[22405](https://luci-milo.appspot.com/ui/p/chromeos/builders/postsubmit/firmware-ti50-postsubmit/22405/overview)

**Known Issues:**

*   EC console lost after ITE EC programming; not a regression (b/243076325)
*   Occasional TPM ready IRQ timeout for spi devices; not a regression
    (b/242137071)
*   ZTE enrollment does not work due to serial number endianness issue; not a
    regression (b/238137986)

**Added Features:**

*   Improve stability around crypto faults (b/242744329)
*   Implement TPM version string command; less error message in ti50 and AP logs

```
for d in $(repo list | sed 's/ .*//'); do
  printf '%32s %s\n' $d $(git -C $d rev-parse HEAD)
done
                        chromite 7de50cba80c38fdd637cffd43a0a35931cc4d7fa
                          common 7430395c23103cade652110c9f0433d15a51368c
                        manifest a8bfc9a825dfaf432324d5edbb58e8a2606d4d07
                ports/cr50-utils da48c5d2ef77a7de7755633386f53540f7db1b2c
                 ports/pinweaver 3abfd77090d24ca8d2d7260d6ba6aaec2e4c35ae
               ports/tpm2_server 6bbf32f9ae1c59df3ec8754d18cd3b065281c3f5
                       repohooks 32b1168199c41dc9e6e0b91dfe37b0568dee538d
      third_party/cargo/registry e88a2f79e298d9107f82b861d2418f26c9d84c4c
   third_party/lowrisc/opentitan cf34c94db6cfb4687353babdf3b0557166241c64
      third_party/tock/libtock-c fd756aa2695cdfad8dc3391c6963eb5b65f595b8
     third_party/tock/libtock-rs f6dab4f4174d9b00fb44adad51f5a26ae4a7b3b2
           third_party/tock/tock ae35db7727c1b9524fd38459e6834f009476c1cc
                third_party/tpm2 47c6c19153c6e32933c7112ff6095d60d1632754
```

### 0.22.6 Released 09/07/22 in R107

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/3879336)

From post submit release
[23031](https://luci-milo.appspot.com/ui/p/chromeos/builders/postsubmit/firmware-ti50-postsubmit/23031/overview)

Manifest
[snapshot](https://chrome-internal.googlesource.com/chromeos/manifest-internal/+/defca58bc62d924441776145690e3a588a7e26ae/snapshot.xml)
from build page.

**Known Issues:**

*   DCRYPTO_FAULT occurring in 0.22.3+ (b/242744329)

**Added Features:**

*   Handles GSC console input/output better around ‘\r\n’
*   Write protect sensing console prints are connected to GSC console (for AP RO
    verification)
*   SPI communication stability
*   Cold reboot stress test improvements
*   ZTE should be fully functional with final fixes

```
Use manifest snapshot instead of repo list. Here's the basic version output.

Build:   ti50_common:v0.0.2437-3f888584
     libtock-rs:v0.0.911-f6dab4f
     tock:v0.0.9607-ae35db772
     ms-tpm-20-ref:v0.0.273-54c1dac
     chrome-bot@chromeos-ci-postsubmit-us-central1-b-x32-52-62pp 2022-09-01 12:35:18

```

### 0.22.7 Released 09/23/22

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/3913922)

From postsubmit build
[24085](https://luci-milo.appspot.com/ui/p/chromeos/builders/postsubmit/firmware-ti50-postsubmit/24085/overview)

Manifest
[snapshot](https://chrome-internal.googlesource.com/chromeos/manifest-internal/+/e04db0a1e95d1fed785b798d25e3b1227a6841e4/snapshot.xml)

**Known Issues:**

*   TPM_RC_HASH error connecting to Google Wifi (b/240506338).
*   SPI communication issues on reboot that can lead to recovery screen
    occasionally (~1/4000 rate) (b/247168128).
*   Rare TPM_RC_POLICY_FAIL on login (b/248109533).

**Added Features:**

*   Crypto alert fixes (b/242744329).
*   ZTE fixes (b/234857025).
*   Boot time improvement: delay NV writes to flash (b/241986964).
*   TPM version reporting (b/245950072).
*   Owner seed reset (b/247811154).

```
Build:   ti50_common:v0.0.2474-5fd512d0
     libtock-rs:v0.0.911-f6dab4f
     tock:v0.0.9608-e951d16b8
     ms-tpm-20-ref:v0.0.275-e3ce8bb
     chrome-bot@chromeos-ci-postsubmit-us-east1-d-x32-118-21s0 2022-09-21 20:36:39
```

### 0.22.9 Released 10/10/22

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/3942977)

From postsubmit build
[24729](https://luci-milo.appspot.com/ui/p/chromeos/builders/postsubmit/firmware-ti50-postsubmit/24729/overview)

Artifacts:
gs://chromeos-releases/firmware-ti50-postsubmit/R108-15168.0.0-71987-8801006591156662625/

Manifest
[snapshot](https://chrome-internal.googlesource.com/chromeos/manifest-internal/+/11fb83313a9cb7059477344e7e0376765d8550f1/snapshot.xml)

**Known Issues:**

*   False warnings about I2CP bus stuck when AP is in low power mode (b/5025966)

**Added Features:**

*   New RO with improved cryptolib performance
*   Fixed several FAFT tests
*   SPI driver synchronization fixes
*   I2C driver wedge bus recovery
*   Ecfs USB to UART cap fix
*   RSA support for NULL padding
*   Core OS: fixed issue that could lead to delaying scheduled short alarms
*   New CCD command `ap_ro_verify` to provision SPI settings for AP RO
    verification. The values are not yet checked for system correctness.

```
Build:   ti50_common:v0.0.2510-ff8e5ad9
         libtock-rs:v0.0.911-f6dab4f
         tock:v0.0.9616-b881615a7
         ms-tpm-20-ref:v0.0.276-8c00699
         chrome-bot@chromeos-ci-postsubmit-us-east1-d-x32-3-787r 2022-10-06 17:18:40
```

### 0.24.0 Released 11/11/22 in R108

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4021007)

From pre-PVT builder
[17](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/17/overview)

Artifacts:
[15086.13.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15086.13.0)

Manifest snapshot: gs://chromeos-manifest-versions/buildspecs/107/15086.13.0.xml

**Known Issues:**

*   Previously enrolled power button gnubby (G2F) may need to be re-enrolled
    (b/252818957)
*   For i2c-based TPM devices, Ti50 won't communicate with AP on first attempt
    if GSC is in deep sleep and system wakes up due to lid open event. Shows up
    as "0x63 Failed to get boot mode from Cr50" error (b/259510330, b/259663369)

**Added Features:**

*   Added AP RO verification feature without holding EC in reset (b/161483233)
*   Improved SPI and I2C TPM bus stability (b/237493220, b/247168128,
    b/245034621, b/251191468)
*   Fixed connection issues with Google-A network (b/240506338)
*   Fixed transient leakage power on UART pins at GSC startup
*   Changed how G2F (Power button as gnubby) serial numbers are generated
    (b/252818957)
*   Improved EFS2 hash invalidation for firmware_UpdateFirmwareDataKeyVersion
    and firmware_UpdateFirmwareVersion FAFT tests (b/253337357)
*   Improved filesystem performance (b/253662388, b/235873536)
*   Decreased flash size needed through more performant syscalls (b/236994893)

```
Build:   ti50_common:v0.0.2613-dbba229a
         libtock-rs:v0.0.913-61d23b3
         tock:v0.0.9622-397f4aaa0
         tpm2:v0.0.292-1a7d322
         @chromeos-ci-firmware-us-east1-d-x32-0-soad 2022-11-07 14:44:25
```

### 0.24.1 Released 12/02/22 in R108

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4077027)

From pre-PVT builder
[18](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/18/overview)

Artifacts:
[15086.14.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15086.14.0)

Manifest snapshot: gs://chromeos-manifest-versions/buildspecs/107/15086.14.0.xml

**Added Features:**

*   Fix "0x63 Failed to get boot mode from Cr50" error when waking i2c-based tpm
    device with lid open wake event (b/259510330, b/259663369).

```
Build:   ti50_common:v0.0.2616-fe48da33
         libtock-rs:v0.0.913-61d23b3
         tock:v0.0.9622-397f4aaa0
         tpm2:v0.0.292-1a7d322
         @chromeos-ci-firmware-us-central1-b-x32-0-2aia 2022-12-02 10:38:30
```

### 0.24.3 Released on 12/21/2022

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4121474)

Builder
[22](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/22/overview)

Artifacts:
[15086.18.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/dev-channel/betty/15086.18.0)

Manifest snapshot: gs://chromeos-manifest-versions/buildspecs/107/15086.18.0.xml

**Bug Fixes**

*   Improve G2F signature security
    [b/261874682](https://b.corp.google.com/issues/261874682)
*   Fix U2F attestation problems
    [b/242678758](https://b.corp.google.com/issues/242678758)
*   Do not run AP RO verification on deep sleep wake
    [b/261635049](https://b.corp.google.com/issues/261635049)

**Added Features**

*   Allow setting serial number when BID flags are set, but BID type is blank
    [b/238137986](https://b.corp.google.com/issues/238137986)
*   Process TPM vendor commands from USB even when AP is off
    [b/258320966](https://b.corp.google.com/issues/258320966)

```
Build:   ti50_common:v0.0.2628-56003e0f
         libtock-rs:v0.0.913-61d23b3
         tock:v0.0.9622-397f4aaa0
         tpm2:v0.0.295-36025e4
         @chromeos-ci-firmware-us-central2-d-x32-0-keps 2022-12-16 14:53:22
```

### 0.24.12 Released on 2/1/2023

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4214172)

Builder
[22](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/28/overview)

Artifacts:
[15086.24.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15086.24.0)

Manifest snapshot: gs://chromeos-manifest-versions/buildspecs/107/15086.24.0.xml

**Bug Fixes**

*   Allow changing AP RO write protect settings until board ID is set
    [b/229016958](https://b.corp.google.com/issues/229016958)
*   Improve USB stablity
    [b/259590362](https://b.corp.google.com/issues/259590362)
*   Allow 0x prefix when entering Board ID flags
    [b/265461193](https://b.corp.google.com/issues/265461193)
*   Make sysinfo command output compatible with Cr50
    [b/263579376](https://b.corp.google.com/issues/263579376)
*   Require short physical presence to enable testlab
    [b/265822083](https://b.corp.google.com/issues/265822083)
*   Disable watchdog only around sleep
    [b/266015400](https://b.corp.google.com/issues/266015400)
*   Do not report false TPM2 p256 errors
    [b/234159838](https://b.corp.google.com/issues/234159838)
*   Fix TPM evict object serialization bug
    [b/263168766](https://b.corp.google.com/issues/263168766)
*   Fix GSC reboot issue when accessing orderly counters from previous ti50 FW
    versions. [b/263168766](https://b.corp.google.com/issues/263168766)

**Added Features**

*   Allow to exclude GBB flags from AP RO hash calculations
    [b/261763740](https://b.corp.google.com/issues/261763740)
*   Show INFO space factory mode state in `sysinfo` output
*   Add I2C errors to FLOG
*   Support two root keys AP RO verification, prepvt and mp
    [b/261600803](https://b.corp.google.com/issues/261600803)
*   Add FLOG entry for crashes
*   Use initial factory mode indicator (INFO space value) to allow setting SN
    [b/264261220](https://b.corp.google.com/issues/264261220)
*   Process TPM vendor commands according their source (USB vs TPM)
    [b/266955081](https://b.corp.google.com/issues/266955081)

```
Build:   ti50_common:v0.0.2779-8d972cb6
         libtock-rs:v0.0.913-61d23b3
         tock:v0.0.9622-397f4aaa0
         ms-tpm-20-ref:v0.0.308-b3e5f5e
         @chromeos-ci-firmware-us-central2-d-x32-0-qlml 2023-01-30 14:12:36
```

### 0.24.13 Released on 2/10/2023

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4241502)

Builder
[29](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/29/overview)

Artifacts:
[15086.25.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15086.25.0)

Manifest snapshot: gs://chromeos-manifest-versions/buildspecs/107/15086.25.0.xml

**Bug Fixes**

*   Fix enter to recovery mode
    [b/248161678](https://b.corp.google.com/issues/248161678),
    [b/267703710](https://b.corp.google.com/issues/267703710)

```
Build:   ti50_common:v0.0.2783-8368c19f
         libtock-rs:v0.0.913-61d23b3
         tock:v0.0.9622-397f4aaa0
         ms-tpm-20-ref:v0.0.308-b3e5f5e
         @chromeos-ci-firmware-us-central1-b-x32-0-33ar 2023-02-08 10:36:28
```

### 0.24.14 Released on 3/10/2023 in M113

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4326634)

Builder
[30](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/30/overview)

Artifacts:
[15086.26.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15086.26.0)

Manifest snapshot: gs://chromeos-manifest-versions/buildspecs/107/15086.26.0.xml

**Bug Fixes**

*   Allow update within 60s of PoR
    [b/270401267](https://b.corp.google.com/issues/270401267)
*   Fix turning on updates after deep sleep
    [b/270401267](https://b.corp.google.com/issues/270401267)
*   Fix handling RO update failures
    [b/271503973](https://b.corp.google.com/issues/271503973)
*   Update AP RO verification NonZeroGbbFlags and WrongRootKey error codes

```
Build:   ti50_common:v0.0.2790-4c1a74e8
         libtock-rs:v0.0.913-61d23b3
         tock:v0.0.9622-397f4aaa0
         ms-tpm-20-ref:v0.0.308-b3e5f5e
         @chromeos-ci-firmware-us-east1-d-x32-0-v1rm 2023-03-06 11:13:07
```

### 0.24.21 Released on 4/5/2023

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4405126)

Builder
[40](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/40/overview)

Artifacts
[15086.35.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15086.35.0)

Manifest snapshot: gs://chromeos-manifest-versions/buildspecs/107/15086.35.0.xml

**New Features**

*   New additional metrics, including boot time and a detailed error code for AP
    RO verification. Now available through the `GetTi50Metrics` vendor command
    with value 0x41. [b/262608026](https://b.corp.google.com/issues/262608026),
    [b/263298180](https://b.corp.google.com/issues/263298180)
*   New vendor commands 0x42 and 0x43 to get crash and console logs.
    [b/268396021](https://b.corp.google.com/issues/268396021),
    [b/265310865](https://b.corp.google.com/issues/265310865)
*   Support pinweaver v2.
    [b/248209280](https://b.corp.google.com/issues/248209280)
*   Add Widevine UDS to virtual NV
    [b/248610274](https://b.corp.google.com/issues/248610274)
*   Add GetRSUDevID command to TPM for RMA.
    [b/265309995](https://b.corp.google.com/issues/265309995)

**Bug Fixes**

*   Fix CCD open failure with the battery disconnected.
    [b/270712314](https://b.corp.google.com/issues/270712314)
*   Fix rejection of RO and RW header updates with invalid fields.
    [b/272057805](https://b.corp.google.com/issues/272057805)
*   Fix `wp follow_batt_pres` command in GSC console.
    [b/269218898](https://b.corp.google.com/issues/269218898)
*   Fix FIDO errors, reporting an invalid P-256 public key.
    [b/271795015](https://b.corp.google.com/issues/271795015)
*   Restrict the `recbtnforce` command to the GscFullConsole CCD capability
    [b/268219945](https://b.corp.google.com/issues/268219945)
*   Fix pinweaver key import/export to be compatible with v2.
    [b/267729980](https://b.corp.google.com/issues/267729980)
*   Fix AP/EC comms broken by EC then AP flash resulting in detached I2C lines.
    [b/264817647](https://b.corp.google.com/issues/264817647)
*   Stabilize console output line order.
    [b/276491121](https://b.corp.google.com/issues/276491121)
*   Fix `\r\r\n` console output.
    [b/242980684](https://b.corp.google.com/issues/242980684)
*   Fix issue where EC not put in reset on recovery key combo when GSC in deep
    sleep. [b/248161678](https://b.corp.google.com/issues/248161678)

```
Build:   ti50_common:v0.0.2939-57543958
         libtock-rs:v0.0.915-7efdaf5
         tock:v0.0.9628-93b95c696
         ms-tpm-20-ref:v0.0.310-953df73
         @chromeos-ci-firmware-us-central1-b-x32-0-pnrp 2023-04-03 09:47:17
```

### 0.24.30 Released on 4/21/2023 in M114

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4460051)

Builder
[37](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/37/overview)

Artifacts:
[15086.37.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15086.37.0)

Manifest snapshot: gs://chromeos-manifest-versions/buildspecs/107/15086.37.0.xml

**Features**

*   Add factory config support
    [b/275356839](https://b.corp.google.com/issues/275356839)

**Bug Fixes**

*   Add PCR-based policy to update fwmp and antirollback spaces
    [b/274977008](https://b.corp.google.com/issues/274977008)
*   Remove crash id parameter from GetCrashLog command
    [b/265310865](https://b.corp.google.com/issues/265310865)
*   AP RO verification returns detailed results
    [b/263298180](https://b.corp.google.com/issues/263298180)

```
Build:   ti50_common_prepvt-15086.B:v0.0.239-60fad06f
         libtock-rs:v0.0.918-d13e197
         tock:v0.0.9630-0fa93d584
         ms-tpm-20-ref:v0.0.314-b366a8a
         @chromeos-ci-firmware-us-central2-d-x32-0-zjfs 2023-04-18 10:59:06
```

### 0.24.40 Released on 6/07/2023 in M116

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4598064)

Builder
[43](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/43/overview)

Artifacts:
[15086.38.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15086.38.0)

Manifest snapshot: gs://chromeos-manifest-versions/buildspecs/107/15086.38.0.xml

**Features**

*   Add support for overwriting AP RO verification settings of `0 0` in field
    for OS scipts [b/260721505](https://b.corp.google.com/issues/260721505)
*   Add Shimless RMA keycombo support for verification failure case
    [b/260721505](https://b.corp.google.com/issues/260721505)

**Bug Fixes**

*   Include improvement/fix for 0x63 boot issues
    [b/273189926](https://b.corp.google.com/issues/273189926)

```
Build:   ti50_common_prepvt-15086.B:v0.0.355-15c69d7f
         libtock-rs:v0.0.918-d13e197
         tock:v0.0.9643-c973271b1
         ms-tpm-20-ref:v0.0.316-a7bd523
         @chromeos-ci-firmware-us-central2-d-x32-0-5zc7 2023-06-05 07:51:35
```

### 0.24.51 Released on 9/14/2023 in M119

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4860496)

Builder
[53](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/53/overview)

Artifacts:
[15086.48.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15086.48.0)

**Features**

*   `ecrst pulse` command is now safe
*   Add initial factory mode to `gsctool`'s CCD print out
*   SPI flash performance for AP twice as fast at Cr50 now
*   Add `rddkeepalive` state in feedback reports
*   GSC console improved logging by adding timestamp prefix to each line
*   Added GSC bootloader stage to boot tracer time monitor
    [b/275390574](https://b.corp.google.com/issues/275390574)

**Bug Fixes**

*   Improve DT clock stretching behavior for I2C
    [b/285366491](https://b.corp.google.com/issues/285366491)
*   Recover after unexpected reads
    [b/225044349](https://b.corp.google.com/issues/225044349)
*   Fix race condition with deep sleep resume resetting EC
    [b/296518779](https://b.corp.google.com/issues/296518779)
*   Improve flashing EC/AP firmware through CCD when external CCD is unstable
    [b/295584404](https://b.corp.google.com/issues/295584404)

```
Build:   ti50_common_prepvt-15086.B:v0.0.674-2ad344ef
         libtock-rs:v0.0.925-c38b187
         tock:v0.0.9657-44d75a018
         ms-tpm-20-ref:v0.0.324-7e7a3da
         @chromeos-ci-firmware-us-central2-d-x32-0-l4sc 2023-09-05 13:28:14
```

### 0.24.60 Released on 10/18/2023 in M120

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/4953488)

Builder
[60](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/60/overview)

Artifacts:
[15086.55.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15086.55.0)

**Features**

*   Add Widevine RoT virtual NV.
    [b/248610274](https://b.corp.google.com/issues/248610274)
*   Add the CIK cert virtual NV.
    [b/248610274](https://b.corp.google.com/issues/248610274)
*   Enforce WP forced enabled when FWMP dev mode disable is present.
    [b/299947142](https://b.corp.google.com/issues/299947142)
*   Add extended AP RO verification status.
    [b/259098185](https://b.corp.google.com/issues/259098185)
*   Ensure ccd mode active and power button resets GSC if failed AP RO
    verification. [b/259098185](https://b.corp.google.com/issues/259098185)
*   Add GetChassisOpen TPMV command.
    [b/257255419](https://b.corp.google.com/issues/257255419)

**Bug Fixes**

*   Fix get console logs vendor command.
    [b/302383688](https://b.corp.google.com/issues/302383688)
*   Prevent flog error from reading past the end of the page.
    [b/302383688](https://b.corp.google.com/issues/302383688)

```
Build:   ti50_common_prepvt-15086.B:v0.0.782-aca516e7
         libtock-rs:v0.0.925-c38b187
         tock:v0.0.9659-b09193d54
         ms-tpm-20-ref:v0.0.327-8e3c8b3
         @chromeos-ci-firmware-us-central1-b-x32-0-qvwt 2023-10-09 15:15:08
```

### 0.24.61 Released on 11/10/2023 in M121

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/5021577)

Builder
[64](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/64/overview)

Artifacts:
[15086.59.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15086.59.0)

**Features**

*   Enforce system reset upon AP RO verification failure.
    [b/259098185](https://b.corp.google.com/issues/259098185)

```
Build:   ti50_common_prepvt-15086.B:v0.0.784-2e565ca2
         libtock-rs:v0.0.925-c38b187
         tock:v0.0.9659-b09193d54
         ms-tpm-20-ref:v0.0.327-8e3c8b3
         @chromeos-ci-firmware-us-central2-d-x32-0-lw3f 2023-11-01 13:57:25
```

### 0.24.62 Released on 12/01/2023 in M121

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/5078265)

Builder
[70](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/70/overview)

Artifacts:
[15086.65.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15086.65.0)

**Features**

*   Add more information to AP RO Verification result UMA report
    [b/259098185](https://b.corp.google.com/issues/259098185)

```
Build:   ti50_common_prepvt-15086.B:v0.0.787-ab6858a7
         libtock-rs:v0.0.925-c38b187
         tock:v0.0.9659-b09193d54
         ms-tpm-20-ref:v0.0.327-8e3c8b3
         @chromeos-ci-firmware-us-east1-d-x32-0-o01k 2023-11-30 07:32:57
```

### 0.24.70 Released on 1/11/2024 in M122

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/5187955)

Builder
[77](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/77/overview)

Artifacts:
[15086.72.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15086.72.0)

**Features**

*   rsu: Increase key generation limit from 10 to 100.
    [b/301156378](https://b.corp.google.com/issues/301156378)
*   ap-ro: Add exception for Frostflow RLZ codes.
    [b/309473916](https://b.corp.google.com/issues/309473916)
*   tpm2: Allow platform read for virtual nvmem.

**Bug Fixes**

*   usb_spi: handle setup packet errors properly.
    [b/302691530](https://b.corp.google.com/issues/302691530)
*   usb_client: prevent lockups when users don't consume RX data.
    [b/302691530](https://b.corp.google.com/issues/302691530)
*   wp: do not set at_boot setting for WP TPMV Cmd disable.
    [b/257255419](https://b.corp.google.com/issues/257255419)
*   tpm2: Fix the wrong signature of widevine cert.
    [b/248610274](https://b.corp.google.com/issues/248610274)
*   cryptolib: adjust CIK & CEK key gen and certs to match actuals.
    [b/308473146](https://b.corp.google.com/issues/308473146)
*   flog: Recover from corrupted entries.
    [b/302383688](https://b.corp.google.com/issues/302383688)
*   fwmp: Reload WP setting when TPM is wiped.
    [b/312396594](https://b.corp.google.com/issues/312396594)
*   ap_ro_verification: Always re-check verification if cached failed.
    [b/315341905](https://b.corp.google.com/issues/315341905)
*   rbox: Do not reset GSC on power button push during ccd open.
    [b/314185172](https://b.corp.google.com/issues/314185172)
*   capsules/i2c_programmer.rs: Respect I2C CCD capability.
    [b/317087536](https://b.corp.google.com/issues/317087536)
*   sys_mgr.rs: Advertise SPI/I2C in board properties.
    [b/307539350](https://b.corp.google.com/issues/307539350)
*   tpm: Save PCR values to NV.
    [b/316884342](https://b.corp.google.com/issues/316884342)
*   tpm_vendor: some commands are disallowed over USB in non DBG mode.
    [b/318518004](https://b.corp.google.com/issues/318518004)

```
Build:   ti50_common_prepvt-15086.B:v0.0.931-91dec51b
         libtock-rs:v0.0.929-ecde39c
         tock:v0.0.9662-478a746e5
         ms-tpm-20-ref:v0.0.329-138a187
         @chromeos-ci-firmware-us-central1-b-x32-0-j5k1 2024-01-05 19:41:43
```

### 0.24.71 Released on 1/19/2024 in M122

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/5217757)

Builder
[79](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/79/overview)

Artifacts:
[15086.74.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15086.74.0)

**Features**

*   Change default write protect setting to force enabled (does not follow
    chassis open gpio by default)
    [b/257255419](https://b.corp.google.com/issues/257255419)

```
Build:   ti50_common_prepvt-15086.B:v0.0.934-720e4c92
         libtock-rs:v0.0.929-ecde39c
         tock:v0.0.9662-478a746e5
         ms-tpm-20-ref:v0.0.329-138a187
         @chromeos-ci-firmware-us-central1-b-x32-0-e7r7 2024-01-17 13:26:11
```

### 0.24.81 Released on 4/9/2024 in M125

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/5441536)

Builder
[94](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/94/overview)

Artifacts:
[15086.89.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15086.89.0)

**Features**

*   flog: Add entries for AP RO verification
*   rbox: Set key debounce to 20us
*   wp: Monitor WP_SENSE_L and WP state for GSC reboots
*   tpm2: Allow platform read for virtual nvmem
*   filesystem: Print NV partition on release builds.

```
Build:   ti50_common_prepvt-15086.B:v0.0.1147-1170d5a9
         libtock-rs:v0.0.929-ecde39c
         tock:v0.0.9682-1b39efeb9
         ms-tpm-20-ref:v0.0.333-50b2409
         @chromeos-ci-firmware-us-central2-d-x32-0-2g96 2024-04-02 13:05:03
```

**Bug Fixes**

*   i2c_programmer: Ensure that ITE waveform response is always 4 bytes
    [b/326258077](https://b.corp.google.com/issues/326258077)
*   crashlog: Fix generation incrementation.
    [b/317804130](https://b.corp.google.com/issues/317804130)
*   flog: Attempt clear flog if initialization fails.
    [b/317221434](https://b.corp.google.com/issues/317221434)
*   ports/dauntless: Fix race in I2C driver
    [b/322037216](https://b.corp.google.com/issues/322037216)
*   filesystem: Handle compaction when all pages are full.
    [b/322037216](https://b.corp.google.com/issues/323043338)
*   event_log: Ensure time always moves forward on init.
    [b/329326190](https://b.corp.google.com/issues/329326190)

### 0.24.90 Released on 4/9/2024 in M126

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/5515766)

Builder
[98](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/98/overview)

Artifacts:
[15086.93.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15086.93.0)

**Features**

*   capsules: Allow dynamically changing baud rate

```
Build:   ti50_common_prepvt-15086.B:v0.0.1204-5ad11b3f
         libtock-rs:v0.0.932-0f90e08
         tock:v0.0.9684-aae949e75
         ms-tpm-20-ref:v0.0.333-50b2409
         @chromeos-ci-firmware-us-east1-d-x32-0-y2he 2024-04-25 15:11:17
```

**Bug Fixes**

*   tpm2: Check if a key is already wrapped before wrapping
    [b/302699979](https://b.corp.google.com/issues/302699979)
*   cryptolib: Prevent buffer overrun
    [b/327499069](https://b.corp.google.com/issues/327499069)
*   dispatcher: Ensure buffer is inaccessible after enqueued
    [b/332326497](https://b.corp.google.com/issues/332326497)

### 0.24.101 Released on 6/8/2024 in M127

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/5609008)

Builder
[106](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15086.B-branch/106/overview)

Artifacts:
[15086.101.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15086.101.0)

**Features**

*   Print chip ID on boot
*   Print reset type earlier
*   pmu: Delay sleep when sleep mask changes
*   rbox: Update tablet RMA sequence to use taps

```
Build:   ti50_common_prepvt-15086.B:v0.0.1259-527d854e
         libtock-rs:v0.0.932-0f90e08
         tock:v0.0.9686-3fd401f26
         ms-tpm-20-ref:v0.0.335-dfaf9c2
         @chromeos-ci-firmware-us-central2-d-x32-0-cik0 2024-06-04 10:39:47
```

**Bug Fixes**

*   fix ti50 hang issue
    [b/339262751](https://b.corp.google.com/issues/339262751)

### 0.24.112 Released on 2024-08-07 in M129

Release
[CL](https://chromium-review.googlesource.com/c/chromiumos/overlays/chromiumos-overlay/+/5768030)

Builder
[firmware-ti50-prepvt-15974.B-branch/1](https://ci.chromium.org/ui/p/chromeos/builders/firmware/firmware-ti50-prepvt-15974.B-branch/1/overview)

Artifacts:
[15974.2.0](https://pantheon.corp.google.com/storage/browser/chromeos-releases/canary-channel/betty/15974.2.0)

**Features**

*   Set EC UART baud rate and parity (via bitbang command or USB)
    [b/333811294](https://b.corp.google.com/issues/333811294)

```
Build:   ti50_common_tot:v0.0.1414-fd2d8291
         libtock-rs:v0.0.925-1213708
         tock:v0.0.9673-2649e0509
         ms-tpm-20-ref:v0.0.318-9942b1f
         @chromeos-ci-firmware-us-central1-b-x32-0-zc52 2024-08-01 08:25:40
```

**Bug Fixes**

*   Fix tock error 6 during CCD open
    [b/258716147](https://b.corp.google.com/issues/258716147)
*   Always allow update within 60s of POR
    [b/352518342](https://b.corp.google.com/issues/352518342)
*   Print PCR0 in ccdstate output
    [b/329439532](https://b.corp.google.com/issues/329439532)
*   Print AP RO verification latch state

<!-- Links -->

[105 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R105-14989.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[106 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R106-15054.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[107 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R107-15117.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[108 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R108-15183.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[109 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R109-15236.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[110 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R110-15278.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[111 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R111-15329.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[112 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R112-15359.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[113 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R113-15393.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[114 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R114-15437.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[115 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R115-15474.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[116 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R116-15509.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[117 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R117-15572.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[118 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R118-15604.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[119 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R119-15633.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[120 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R120-15662.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[121 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R121-15699.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[122 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R122-15753.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[123 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R123-15786.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[124 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R124-15823.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[125 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R125-15853.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[126 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R126-15886.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[127 release]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/release-R127-15917.B/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
[ToT ebuild]: https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/refs/heads/main/chromeos-base/chromeos-ti50/chromeos-ti50-0.0.1.ebuild
