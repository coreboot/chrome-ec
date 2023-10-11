/*
 * Copyright 2015 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <asm/byteorder.h>
#include <ctype.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libusb.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "config.h"

#include "ap_ro_integrity_check.h"
#include "ccd_config.h"
#include "compile_time_macros.h"
#include "dauntless_event.h"
#include "flash_log.h"
#include "generated_version.h"
#include "gsctool.h"
#include "misc_util.h"
#include "signed_header.h"
#include "tpm_registers.h"
#include "tpm_vendor_cmds.h"
#include "upgrade_fw.h"
#include "u2f.h"
#include "usb_descriptor.h"
#include "verify_ro.h"

/*
 * This enum must match CcdCap enum in applications/sys_mgr/src/ccd.rs in the
 * Ti50 common git tree.
 */
enum Ti50CcdCapabilities {
	TI50_CCD_CAP_UART_GSC_RX_AP_TX = 0,
	TI50_CCD_CAP_UART_GSC_TX_AP_RX,
	TI50_CCD_CAP_UART_GSC_RX_EC_TX,
	TI50_CCD_CAP_UART_GSC_TX_EC_RX,
	TI50_CCD_CAP_UART_GSC_RX_FPMCU_TX,
	TI50_CCD_CAP_UART_GSC_TX_FPMCU_RX,
	TI50_CCD_CAP_FLASH_AP,
	TI50_CCD_CAP_FLASH_EC,
	TI50_CCD_CAP_OVERRIDE_WP,
	TI50_CCD_CAP_REBOOT_ECAP,
	TI50_CCD_CAP_GSC_FULL_CONSOLE,
	TI50_CCD_CAP_UNLOCK_NO_REBOOT,
	TI50_CCD_CAP_UNLOCK_NO_SHORT_PP,
	TI50_CCD_CAP_OPEN_NO_TPM_WIPE,
	TI50_CCD_CAP_OPEN_NO_LONG_PP,
	TI50_CCD_CAP_REMOVE_BATTERY_BYPASS_PP,
	TI50_CCD_CAP_I2_C,
	TI50_CCD_CAP_FLASH_READ,
	TI50_CCD_CAP_OPEN_NO_DEV_MODE,
	TI50_CCD_CAP_OPEN_FROM_USB,
	TI50_CCD_CAP_OVERRIDE_BATT,
	TI50_CCD_CAP_BOOT_UNVERIFIED_RO,
	TI50_CCD_CAP_COUNT,
};

static const struct ccd_capability_info ti50_cap_info[] = {
	{ "UartGscRxAPTx", CCD_CAP_STATE_ALWAYS },
	{ "UartGscTxAPRx", CCD_CAP_STATE_ALWAYS },
	{ "UartGscRxECTx", CCD_CAP_STATE_ALWAYS },
	{ "UartGscTxECRx", CCD_CAP_STATE_IF_OPENED },
	{ "UartGscRxFpmcuTx", CCD_CAP_STATE_ALWAYS },
	{ "UartGscTxFpmcuRx", CCD_CAP_STATE_IF_OPENED },
	{ "FlashAP", CCD_CAP_STATE_IF_OPENED },
	{ "FlashEC", CCD_CAP_STATE_IF_OPENED },
	{ "OverrideWP", CCD_CAP_STATE_IF_OPENED },
	{ "RebootECAP", CCD_CAP_STATE_IF_OPENED },
	{ "GscFullConsole", CCD_CAP_STATE_IF_OPENED },
	{ "UnlockNoReboot", CCD_CAP_STATE_ALWAYS },
	{ "UnlockNoShortPP", CCD_CAP_STATE_ALWAYS },
	{ "OpenNoTPMWipe", CCD_CAP_STATE_IF_OPENED },
	{ "OpenNoLongPP", CCD_CAP_STATE_IF_OPENED },
	{ "RemoveBatteryBypassPP", CCD_CAP_STATE_ALWAYS },
	{ "I2C", CCD_CAP_STATE_IF_OPENED },
	{ "FlashRead", CCD_CAP_STATE_ALWAYS },
	/*
	 * The below two settings do not match ccd.rs value, which is
	 * controlled at compile time.
	 */
	{ "OpenNoDevMode", CCD_CAP_STATE_IF_OPENED },
	{ "OpenFromUSB", CCD_CAP_STATE_IF_OPENED },
	{ "OverrideBatt", CCD_CAP_STATE_IF_OPENED },
	/* The below capability is presently set to 'never' in ccd.rs. */
	{ "AllowUnverifiedRo", CCD_CAP_STATE_IF_OPENED },
};

#define CR50_CCD_CAP_COUNT CCD_CAP_COUNT

/*
 * One of the basic assumptions of the code handling multiple ccd_info layouts
 * is that the number of words in the capabilities array is the same for all
 * layouts. Let's verify this at compile time.
 */
BUILD_ASSERT((CR50_CCD_CAP_COUNT / 32) == (TI50_CCD_CAP_COUNT / 32));

/*
 * Version 0 CCD info packet does not include the header, the actual packet
 * size is used to conclude that the received payload is of version 0.
 *
 * Let's hardcode this size to make sure that it is fixed even if the
 * underlying structure (struct ccd_info_response) size changes in the future.
 */
#define CCD_INFO_V0_SIZE 23

/*
 * This file contains the source code of a Linux application used to update
 * CR50 device firmware.
 *
 * The CR50 firmware image consists of multiple sections, of interest to this
 * app are the RO and RW code sections, two of each. When firmware update
 * session is established, the CR50 device reports locations of backup RW and RO
 * sections (those not used by the device at the time of transfer).
 *
 * Based on this information this app carves out the appropriate sections form
 * the full CR50 firmware binary image and sends them to the device for
 * programming into flash. Once the new sections are programmed and the device
 * is restarted, the new RO and RW are used if they pass verification and are
 * logically newer than the existing sections.
 *
 * There are two ways to communicate with the CR50 device: USB and /dev/tpm0
 * (when this app is running on a chromebook with the CR50 device). Originally
 * different protocols were used to communicate over different channels,
 * starting with version 3 the same protocol is used.
 *
 * This app provides backwards compatibility to ensure that earlier CR50
 * devices still can be updated.
 *
 *
 * The host (either a local AP or a workstation) is controlling the firmware
 * update protocol, it sends data to the cr50 device, which proceeses it and
 * responds.
 *
 * The encapsultation format is different between the /dev/tpm0 and USB cases:
 *
 *   4 bytes      4 bytes         4 bytes               variable size
 * +-----------+--------------+---------------+----------~~--------------+
 * + total size| block digest |  dest address |           data           |
 * +-----------+--------------+---------------+----------~~--------------+
 *  \           \                                                       /
 *   \           \                                                     /
 *    \           +----- FW update PDU sent over /dev/tpm0 -----------+
 *     \                                                             /
 *      +--------- USB frame, requires total size field ------------+
 *
 * The update protocol data unints (PDUs) are passed over /dev/tpm0, the
 * encapsulation includes integritiy verification and destination address of
 * the data (more of this later). /dev/tpm0 transactions pretty much do not
 * have size limits, whereas the USB data is sent in chunks of the size
 * determined when the USB connestion is set up. This is why USB requires an
 * additional encapsulation into frames to communicate the PDU size to the
 * client side so that the PDU can be reassembled before passing to the
 * programming function.
 *
 * In general, the protocol consists of two phases: connection establishment
 * and actual image transfer.
 *
 * The very first PDU of the transfer session is used to establish the
 * connection. The first PDU does not have any data, and the dest. address
 * field is set to zero. Receiving such a PDU signals the programming function
 * that the host intends to transfer a new image.
 *
 * The response to the first PDU varies depending on the protocol version.
 *
 * Note that protocol versions before 5 are described here for completeness,
 * but are not supported any more by this utility.
 *
 * Version 1 is used over /dev/tpm0. The response is either 4 or 1 bytes in
 * size. The 4 byte response is the *base address* of the backup RW section,
 * no support for RO updates. The one byte response is an error indication,
 * possibly reporting flash erase failure, command format error, etc.
 *
 * Version 2 is used over USB. The response is 8 bytes in size. The first four
 * bytes are either the *base address* of the backup RW section (still no RO
 * updates), or an error code, the same as in Version 1. The second 4 bytes
 * are the protocol version number (set to 2).
 *
 * All versions above 2 behave the same over /dev/tpm0 and USB.
 *
 * Version 3 response is 16 bytes in size. The first 4 bytes are the error code
 * the second 4 bytes are the protocol version (set to 3) and then 4 byte
 * *offset* of the RO section followed by the 4 byte *offset* of the RW section.
 *
 * Version 4 response in addition to version 3 provides header revision fields
 * for active RO and RW images running on the target.
 *
 * Once the connection is established, the image to be programmed into flash
 * is transferred to the CR50 in 1K PDUs. In versions 1 and 2 the address in
 * the header is the absolute address to place the block to, in version 3 and
 * later it is the offset into the flash.
 *
 * Protocol version 5 includes RO and RW key ID information into the first PDU
 * response. The key ID could be used to tell between prod and dev signing
 * modes, among other things.
 *
 * Protocol version 6 does not change the format of the first PDU response,
 * but it indicates the target's ablitiy to channel TPM vendor commands
 * through USB connection.
 *
 * When channeling TPM vendor commands the USB frame looks as follows:
 *
 *   4 bytes      4 bytes         4 bytes       2 bytes      variable size
 * +-----------+--------------+---------------+-----------+------~~~-------+
 * + total size| block digest |    EXT_CMD    | Vend. sub.|      data      |
 * +-----------+--------------+---------------+-----------+------~~~-------+
 *
 * Where 'Vend. sub' is the vendor subcommand, and data field is subcommand
 * dependent. The target tells between update PDUs and encapsulated vendor
 * subcommands by looking at the EXT_CMD value - it is set to 0xbaccd00a and
 * as such is guaranteed not to be a valid update PDU destination address.
 *
 * The vendor command response size is not fixed, it is subcommand dependent.
 *
 * The CR50 device responds to each update PDU with a confirmation which is 4
 * bytes in size in protocol version 2, and 1 byte in size in all other
 * versions. Zero value means success, non zero value is the error code
 * reported by CR50.
 *
 * Again, vendor command responses are subcommand specific.
 */

/* Look for Cr50 FW update interface */
#define H1_PID	 0x5014
#define D2_PID	 0x504A
#define SUBCLASS USB_SUBCLASS_GOOGLE_CR50
#define PROTOCOL USB_PROTOCOL_GOOGLE_CR50_NON_HC_FW_UPDATE

/*
 * CCD Info from GSC is communicated using different structure layouts.
 * Version 0 does not have a header and includes just the payload information.
 * Version 2 is prepended by a header which has a distinct value in the first
 * word, and the version number and the total size in the two halfwords after
 * that.
 *
 * Once the payload is received, the absence of the distinct value in the
 * first word and the match of the payload size to the expected size of the
 * version 0 payload indicates that this is indeed a version 0 packet. The
 * distinct first header value and the match of the size field indicates that
 * this is a later version packet.
 */
#define CCD_INFO_MAGIC 0x49444343 /* This is 'CCDI' in little endian. */
#define CCD_VERSION    1 /* Ti50 CCD INFO layout. */

struct ccd_info_response_header {
	uint32_t ccd_magic;
	uint16_t ccd_version;
	uint16_t ccd_size;
} __packed;

struct ccd_info_response_packet {
	struct ccd_info_response_header cir_header;
	struct ccd_info_response cir_body;
};

/*
 * Need to create an entire TPM PDU when upgrading over /dev/tpm0 and need to
 * have space to prepare the entire PDU.
 */
struct upgrade_pkt {
	__be16 tag;
	__be32 length;
	__be32 ordinal;
	__be16 subcmd;
	union {
		/*
		 * Upgrade PDUs as opposed to all other vendor and extension
		 * commands include two additional fields in the header.
		 */
		struct {
			__be32 digest;
			__be32 address;
			char data[0];
		} upgrade;
		struct {
			char data[0];
		} command;
	};
} __packed;

/*
 * Structure used to simplify mapping command line options into Boolean
 * variables. If an option is present, the corresponding integer value is set
 * to 1.
 */
struct options_map {
	char opt;
	int *flag;
};

/*
 * Type of the GSC device we're supposed to be connected to. Is determined
 * based on various inputs, like command line parameters and/or image supplied
 * for downloading.
 */
enum gsc_device {
	GSC_DEVICE_ANY = 0,
	GSC_DEVICE_H1,
	GSC_DEVICE_DT,
};

/*
 * Structure used to combine option description used by getopt_long() and help
 * text for the option.
 */
struct option_container {
	struct option opt;
	const char *help_text;
	enum gsc_device opt_device; /* Initted to ANY by default. */
};

static void sha_init(EVP_MD_CTX *ctx);
static void sha_update(EVP_MD_CTX *ctx, const void *data, size_t len);
static void sha_final_into_block_digest(EVP_MD_CTX *ctx, void *block_digest,
					size_t size);

/* Type of the GSC device we are talking to, determined at run time. */
static enum gsc_device gsc_dev = GSC_DEVICE_ANY;
/*
 * Type of the GSC device the currently processed command line option
 * requires, set during option scanning along with optarg.
 */
static enum gsc_device opt_gsc_dev = GSC_DEVICE_ANY;

/*
 * Current AP RO verification config setting version
 */
#define ARV_CONFIG_SETTING_CURRENT_VERSION 0x01

/*
 * AP RO verification config setting command choices
 */
enum arv_config_setting_command_e {
	arv_config_setting_command_spi_addressing_mode = 0,
	arv_config_setting_command_write_protect_descriptors = 1,
};

/*
 * AP RO verification config setting state
 */
enum arv_config_setting_state_e {
	arv_config_setting_state_present = 0,
	arv_config_setting_state_not_present = 1,
	arv_config_setting_state_corrupted = 2,
	arv_config_setting_state_invalid = 3,
};

/*
 * AP RO verification SPI read/write addressing mode configuration choices
 */
enum arv_config_spi_addr_mode_e {
	arv_config_spi_addr_mode_none = 0,
	arv_config_spi_addr_mode_get = 1,
	arv_config_spi_addr_mode_set_3byte = 2,
	arv_config_spi_addr_mode_set_4byte = 3,
};

/*
 * AP RO verification write protect descriptor configuration choices
 */
enum arv_config_wpsr_choice_e {
	arv_config_wpsr_choice_none = 0,
	arv_config_wpsr_choice_get = 1,
	arv_config_wpsr_choice_set = 2,
};

/*
 * AP RO verification write protect descriptor information
 */
struct __attribute__((__packed__)) arv_config_wpd {
	/* See `arv_config_setting_state_e` */
	uint8_t state;
	uint8_t expected_value;
	uint8_t mask;
};

/*
 * AP RO verification write protect descriptors. This is a helper type to
 * represent the three write protect descriptors.
 */
struct __attribute__((__packed__)) arv_config_wpds {
	struct arv_config_wpd data[3];
};

/*
 * This matches the largest vendor command response size we ever expect.
 */
#define MAX_RX_BUF_SIZE 2048

/*
 * Maximum update payload block size plus packet header size.
 */
#define MAX_TX_BUF_SIZE (SIGNED_TRANSFER_SIZE + sizeof(struct upgrade_pkt))

/*
 * Max. length of the board ID string representation.
 *
 * Board ID is either a 4-character ASCII alphanumeric string or an 8-digit
 * hex.
 */
#define MAX_BOARD_ID_LENGTH 9

/*
 * Length, in bytes, of the SN Bits serial number bits.
 */
#define SN_BITS_SIZE (96 >> 3)

/*
 * Max. length of FW version in the format of <epoch>.<major>.<minor>
 * (3 uint32_t string representation + 2 separators + NULL terminator).
 */
#define MAX_FW_VER_LENGTH 33

static int verbose_mode;
static uint32_t protocol_version;
static char *progname;

/*
 * List of command line options, ***sorted by the short form***.
 *
 * The help_text field does not include the short and long option strings,
 * they are retrieved from the opt structure. In case the help text needs to
 * have something printed immediately after the option strings (for example,
 * an optional parameter), it should be included in the beginning of help_text
 * string separated by the % character.
 *
 * usage() function which prints out the help message will concatenate the
 * short and long options and the optional parameter, if present, and then
 * print the rest of the text message at a fixed indentation.
 */
static const struct option_container cmd_line_options[] = {
	/* {{name   has_arg    *flag  val} long_desc dev_type} */
	{ { "get_apro_hash", no_argument, NULL, 'A' },
	  "get the stored ap ro hash" },
	{ { "any", no_argument, NULL, 'a' },
	  "Try any interfaces to find Cr50"
	  " (-d, -s, -t are all ignored)" },
	{ { "apro_boot", optional_argument, NULL, 'B' },
	  "[start] get the stored ap ro boot state or start ap ro verify",
	  GSC_DEVICE_DT },
	{ { "binvers", no_argument, NULL, 'b' },
	  "Report versions of Cr50 image's "
	  "RW and RO headers, do not update" },
	{ { "apro_config_spi_mode", optional_argument, NULL, 'C' },
	  "Get/set the ap ro verify spi mode either to `3byte` or `4byte`",
	  GSC_DEVICE_DT },
	{ { "corrupt", no_argument, NULL, 'c' }, "Corrupt the inactive rw" },
	{ { "dauntless", no_argument, NULL, 'D' },
	  "Communicate with Dauntless chip. This may also be implied.",
	  GSC_DEVICE_DT },
	{ { "device", required_argument, NULL, 'd' },
	  "VID:PID%USB device (default 18d1:5014 or 18d1:504a based on"
	  " image)" },
	{ { "apro_config_write_protect", optional_argument, NULL, 'E' },
	  "Get/set the ap ro verify write protect descriptors with hex "
	  "bytes (ex: 0x01, 0x1, 01 or 1) in the following format: "
	  "[sr1 mask1 [sr2 mask2] [sr3 mask3]]",
	  GSC_DEVICE_DT },
	{ { "endorsement_seed", optional_argument, NULL, 'e' },
	  "[state]%get/set the endorsement key seed" },
	{ { "factory", required_argument, NULL, 'F' },
	  "[enable|disable]%Control factory mode" },
	{ { "fwver", no_argument, NULL, 'f' },
	  "Report running Cr50 firmware versions" },
	{ { "get_time", no_argument, NULL, 'G' },
	  "Get time since last cold reset" },
	{ { "getbootmode", no_argument, NULL, 'g' },
	  "Get the system boot mode" },
	{ { "erase_ap_ro_hash", no_argument, NULL, 'H' },
	  "Erase AP RO hash (possible only if Board ID is not set)",
	  GSC_DEVICE_H1 },
	{ { "help", no_argument, NULL, 'h' }, "Show this message" },
	{ { "ccd_info", optional_argument, NULL, 'I' },
	  "[capability:value]%Get information about CCD state or set capability"
	  " value if allowed" },
	{ { "board_id", optional_argument, NULL, 'i' },
	  "[ID[:FLAGS]]%Get or set Info1 board ID fields. ID could be 32 bit "
	  "hex or 4 character string." },
	{ { "boot_trace", optional_argument, NULL, 'J' },
	  "[erase]%Retrieve boot trace from the chip, optionally erasing "
	  "the trace buffer",
	  GSC_DEVICE_DT },
	{ { "get_value", required_argument, NULL, 'K' },
	  "Get value of one of [chassis_open|dev_ids].",
	  GSC_DEVICE_DT },
	{ { "ccd_lock", no_argument, NULL, 'k' }, "Lock CCD" },
	{ { "flog", optional_argument, NULL, 'L' },
	  "[prev entry]%Retrieve contents of the flash log"
	  " (newer than <prev entry> if specified)" },
	{ { "console", no_argument, NULL, 'l' },
	  "Get console logs. This may need to be run multiple times to collect "
	  "all available logs.",
	  GSC_DEVICE_DT },
	{ { "machine", no_argument, NULL, 'M' },
	  "Output in a machine-friendly way. "
	  "Effective with -b, -f, -i, -J, -r, and -O." },
	{ { "tpm_mode", optional_argument, NULL, 'm' },
	  "[enable|disable]%Change or query tpm_mode" },
	{ { "serial", required_argument, NULL, 'n' },
	  "Cr50 CCD serial number" },
	{ { "openbox_rma", required_argument, NULL, 'O' },
	  "<desc_file>%Verify other device's RO integrity using information "
	  "provided in <desc file>" },
	{ { "ccd_open", no_argument, NULL, 'o' }, "Start CCD open sequence" },
	{ { "password", no_argument, NULL, 'P' },
	  "Set or clear CCD password. Use 'clear:<cur password>' to clear it" },
	{ { "post_reset", no_argument, NULL, 'p' },
	  "Request post reset after transfer" },
	{ { "force_ro", no_argument, NULL, 'q' }, "Force inactive RO update" },
	{ { "sn_rma_inc", required_argument, NULL, 'R' },
	  "RMA_INC%Increment SN RMA count by RMA_INC. RMA_INC should be 0-7." },
	{ { "rma_auth", optional_argument, NULL, 'r' },
	  "[auth_code]%Request RMA challenge, process "
	  "RMA authentication code" },
	{ { "sn_bits", required_argument, NULL, 'S' },
	  "SN_BITS%Set Info1 SN bits fields. SN_BITS should be 96 bit hex." },
	{ { "systemdev", no_argument, NULL, 's' },
	  "Use /dev/tpm0 (-d is ignored)" },
	{ { "tstamp", optional_argument, NULL, 'T' },
	  "[<tstamp>]%Get or set flash log timestamp base" },
	{ { "trunks_send", no_argument, NULL, 't' },
	  "Use `trunks_send --raw' (-d is ignored)" },
	{ { "ccd_unlock", no_argument, NULL, 'U' },
	  "Start CCD unlock sequence" },
	{ { "upstart", no_argument, NULL, 'u' },
	  "Upstart mode (strict header checks)" },
	{ { "verbose", no_argument, NULL, 'V' }, "Enable debug messages" },
	{ { "version", no_argument, NULL, 'v' },
	  "Report this utility version" },
	{ { "metrics", no_argument, NULL, 'W' },
	  "Get Ti50 metrics",
	  GSC_DEVICE_DT },
	{ { "wp", optional_argument, NULL, 'w' },
	  "[enable] Get the current WP setting or enable WP" },
	{ { "clog", no_argument, NULL, 'x' },
	  "Retrieve contents of the most recent crash log.",
	  GSC_DEVICE_DT },
	{ { "factory_config", optional_argument, NULL, 'y' },
	  "[value]%Sets the factory config bits in INFO. value should be 64 "
	  "bit hex." },
	{ { "reboot", optional_argument, NULL, 'z' },
	  "Tell the GSC to reboot with an optional reset timeout parameter "
	  "in milliseconds" },
};

/* Helper to print debug messages when verbose flag is specified. */
static void debug(const char *fmt, ...)
{
	va_list args;

	if (verbose_mode) {
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
	}
}

/* Helpers to convert between binary and hex ascii and back. */
static char to_hexascii(uint8_t c)
{
	if (c <= 9)
		return '0' + c;
	return 'a' + c - 10;
}

static int from_hexascii(char c)
{
	/* convert to lower case. */
	c = tolower(c);

	if (c < '0' || c > 'f' || ((c > '9') && (c < 'a')))
		return -1; /* Not an ascii character. */

	if (c <= '9')
		return c - '0';

	return c - 'a' + 10;
}

/* Functions to communicate with the TPM over the trunks_send --raw channel. */

/* File handle to share between write and read sides. */
static FILE *tpm_output;
static int ts_write(const void *out, size_t len)
{
	const char *cmd_head = "PATH=\"${PATH}:/usr/sbin\" trunks_send --raw ";
	size_t head_size = strlen(cmd_head);
	char full_command[head_size + 2 * len + 1];
	size_t i;

	strcpy(full_command, cmd_head);
	/*
	 * Need to convert binary input into hex ascii output to pass to the
	 * trunks_send command.
	 */
	for (i = 0; i < len; i++) {
		uint8_t c = ((const uint8_t *)out)[i];

		full_command[head_size + 2 * i] = to_hexascii(c >> 4);
		full_command[head_size + 2 * i + 1] = to_hexascii(c & 0xf);
	}

	/* Make it a proper zero terminated string. */
	full_command[sizeof(full_command) - 1] = 0;
	debug("cmd: %s\n", full_command);
	tpm_output = popen(full_command, "r");
	if (tpm_output)
		return len;

	fprintf(stderr, "Error: failed to launch trunks_send --raw\n");
	return -1;
}

static int ts_read(void *buf, size_t max_rx_size)
{
	int i;
	int pclose_rv;
	int rv;
	/* +1 to account for '\n' added by trunks_send. */
	char response[max_rx_size * 2 + 1];

	if (!tpm_output) {
		fprintf(stderr, "Error: attempt to read empty output\n");
		return -1;
	}

	rv = fread(response, 1, sizeof(response), tpm_output);
	if (rv > 0)
		rv -= 1; /* Discard the \n character added by trunks_send. */

	debug("response of size %d, max rx size %zd: %s\n", rv, max_rx_size,
	      response);

	pclose_rv = pclose(tpm_output);
	if (pclose_rv < 0) {
		fprintf(stderr, "Error: pclose failed: error %d (%s)\n", errno,
			strerror(errno));
		return -1;
	}

	tpm_output = NULL;

	if (rv & 1) {
		fprintf(stderr,
			"Error: trunks_send returned odd number of bytes: %s\n",
			response);
		return -1;
	}

	for (i = 0; i < rv / 2; i++) {
		uint8_t byte;
		char c;
		int nibble;

		c = response[2 * i];
		nibble = from_hexascii(c);
		if (nibble < 0) {
			fprintf(stderr,
				"Error: "
				"trunks_send returned non hex character %c\n",
				c);
			return -1;
		}
		byte = nibble << 4;

		c = response[2 * i + 1];
		nibble = from_hexascii(c);
		if (nibble < 0) {
			fprintf(stderr,
				"Error: "
				"trunks_send returned non hex character %c\n",
				c);
			return -1;
		}
		byte |= nibble;

		((uint8_t *)buf)[i] = byte;
	}

	return rv / 2;
}

/*
 * Prepare and transfer a block to either to /dev/tpm0 or through trunks_send
 * --raw, get a reply.
 */
static int tpm_send_pkt(struct transfer_descriptor *td, unsigned int digest,
			unsigned int addr, const void *data, int size,
			void *response, size_t *response_size, uint16_t subcmd)
{
	/* Used by transfer to /dev/tpm0 */
	static uint8_t outbuf[MAX_TX_BUF_SIZE];
	static uint8_t
		raw_response[MAX_RX_BUF_SIZE + sizeof(struct upgrade_pkt)];
	struct upgrade_pkt *out = (struct upgrade_pkt *)outbuf;
	int len, done;
	int response_offset = offsetof(struct upgrade_pkt, command.data);
	void *payload;
	size_t header_size;
	uint32_t rv;
	const size_t rx_size = sizeof(raw_response);

	debug("%s: sending to %#x %d bytes\n", __func__, addr, size);

	out->tag = htobe16(0x8001);
	out->subcmd = htobe16(subcmd);

	if (subcmd <= LAST_EXTENSION_COMMAND)
		out->ordinal = htobe32(CONFIG_EXTENSION_COMMAND);
	else
		out->ordinal = htobe32(TPM_CC_VENDOR_BIT_MASK);

	if (subcmd == EXTENSION_FW_UPGRADE) {
		/* FW Upgrade PDU header includes a couple of extra fields. */
		out->upgrade.digest = digest;
		out->upgrade.address = htobe32(addr);
		header_size = offsetof(struct upgrade_pkt, upgrade.data);
	} else {
		header_size = offsetof(struct upgrade_pkt, command.data);
	}

	payload = outbuf + header_size;
	len = size + header_size;

	out->length = htobe32(len);
	memcpy(payload, data, size);

	if (verbose_mode) {
		int i;

		debug("Writing %d bytes to TPM at %x\n", len, addr);
		for (i = 0; i < MIN(len, 20); i++)
			debug("%2.2x ", outbuf[i]);
		debug("\n");
	}

	switch (td->ep_type) {
	case dev_xfer:
		done = write(td->tpm_fd, out, len);
		break;
	case ts_xfer:
		done = ts_write(out, len);
		break;
	default:
		fprintf(stderr, "Error: %s:%d: unknown transfer type %d\n",
			__func__, __LINE__, td->ep_type);
		return -1;
	}

	if (done < 0) {
		perror("Could not write to TPM");
		return -1;
	} else if (done != len) {
		fprintf(stderr, "Error: Wrote %d bytes, expected to write %d\n",
			done, len);
		return -1;
	}

	switch (td->ep_type) {
	case dev_xfer: {
		int read_count;

		len = 0;
		do {
			uint8_t *rx_buf = raw_response + len;
			size_t rx_to_go = rx_size - len;

			read_count = read(td->tpm_fd, rx_buf, rx_to_go);

			len += read_count;
		} while (read_count);
		break;
	}
	case ts_xfer:
		len = ts_read(raw_response, rx_size);
		break;
	default:
		/*
		 * This sure will never happen, type is verifed in the
		 * previous switch statement.
		 */
		len = -1;
		break;
	}

	debug("Read %d bytes from TPM\n", len);
	if (len > 0) {
		int i;

		for (i = 0; i < len; i++)
			debug("%2.2x ", raw_response[i]);
		debug("\n");
	}
	len = len - response_offset;
	if (len < 0) {
		fprintf(stderr, "Problems reading from TPM, got %d bytes\n",
			len + response_offset);
		return -1;
	}

	if (response && response_size) {
		len = MIN(len, *response_size);
		memcpy(response, raw_response + response_offset, len);
		*response_size = len;
	}

	/* Return the actual return code from the TPM response header. */
	memcpy(&rv, &((struct upgrade_pkt *)raw_response)->ordinal, sizeof(rv));
	rv = be32toh(rv);

	/* Clear out vendor command return value offset.*/
	if ((rv & VENDOR_RC_ERR) == VENDOR_RC_ERR)
		rv &= ~VENDOR_RC_ERR;

	return rv;
}

/* Release USB device and return error to the OS. */
static void shut_down(struct usb_endpoint *uep)
{
	usb_shut_down(uep);
	exit(update_error);
}

static void usage(int errs)
{
	size_t i;
	const int indent = 27; /* This is the size used by gsctool all along. */

	printf("\nUsage: %s [options] [<binary image>]\n"
	       "\n"
	       "This utility allows to update Cr50 RW firmware, configure\n"
	       "various aspects of Cr50 operation, analyze Cr50 binary\n"
	       "images, etc.\n\n"
	       "<binary image> is the file name of a full RO+RW binary image.\n"
	       "\n"
	       "Options:\n\n",
	       progname);

	for (i = 0; i < ARRAY_SIZE(cmd_line_options); i++) {
		const char *help_text = cmd_line_options[i].help_text;
		int printed_length;
		const char *separator;

		/*
		 * First print the short and long forms of the command line
		 * option.
		 */
		printed_length = printf(" -%c,--%s",
					cmd_line_options[i].opt.val,
					cmd_line_options[i].opt.name);

		/*
		 * If there is something to print immediately after the
		 * options, print it.
		 */
		separator = strchr(help_text, '%');
		if (separator) {
			char buffer[80];
			size_t extra_size;

			extra_size = separator - help_text;
			if (extra_size >= sizeof(buffer)) {
				fprintf(stderr, "misformatted help text: %s\n",
					help_text);
				exit(-1);
			}
			memcpy(buffer, help_text, extra_size);
			buffer[extra_size] = '\0';
			printed_length += printf(" %s", buffer);
			help_text = separator + 1;
		}

		/*
		 * If printed length exceeds or is too close to indent, print
		 * help text on the next line.
		 */
		if (printed_length >= (indent - 1)) {
			printf("\n");
			printed_length = 0;
		}

		while (printed_length++ < indent)
			printf(" ");
		printf("%s\n", help_text);
	}
	printf("\n");
	exit(errs ? update_error : noop);
}

/* Read file into buffer */
static uint8_t *get_file_or_die(const char *filename, size_t *len_ptr)
{
	FILE *fp;
	struct stat st;
	uint8_t *data;
	size_t len;

	fp = fopen(filename, "rb");
	if (!fp) {
		perror(filename);
		exit(update_error);
	}
	if (fstat(fileno(fp), &st)) {
		perror("stat");
		exit(update_error);
	}

	len = st.st_size;

	data = malloc(len);
	if (!data) {
		perror("malloc");
		exit(update_error);
	}

	if (1 != fread(data, st.st_size, 1, fp)) {
		perror("fread");
		exit(update_error);
	}

	fclose(fp);

	*len_ptr = len;
	return data;
}

/* Returns true if parsed. */
static int parse_vidpid(const char *input, uint16_t *vid_ptr, uint16_t *pid_ptr)
{
	char *copy, *s, *e = 0;

	copy = strdup(input);

	s = strchr(copy, ':');
	if (!s)
		return 0;
	*s++ = '\0';

	*vid_ptr = (uint16_t)strtoul(copy, &e, 16);
	if (!*optarg || (e && *e))
		return 0;

	*pid_ptr = (uint16_t)strtoul(s, &e, 16);
	if (!*optarg || (e && *e))
		return 0;

	return 1;
}

struct update_pdu {
	uint32_t block_size; /* Total block size, include this field's size. */
	struct upgrade_command cmd;
	/* The actual payload goes here. */
};

static void do_xfer(struct usb_endpoint *uep, void *outbuf, int outlen,
		    void *inbuf, int inlen, int allow_less, size_t *rxed_count)
{
	if (usb_trx(uep, outbuf, outlen, inbuf, inlen, allow_less, rxed_count))
		shut_down(uep);
}

static int transfer_block(struct usb_endpoint *uep, struct update_pdu *updu,
			  uint8_t *transfer_data_ptr, size_t payload_size)
{
	size_t transfer_size;
	uint32_t reply;
	int actual;
	int r;

	/* First send the header. */
	do_xfer(uep, updu, sizeof(*updu), NULL, 0, 0, NULL);

	/* Now send the block, chunk by chunk. */
	for (transfer_size = 0; transfer_size < payload_size;) {
		int chunk_size;

		chunk_size = MIN(uep->chunk_len, payload_size - transfer_size);
		do_xfer(uep, transfer_data_ptr, chunk_size, NULL, 0, 0, NULL);
		transfer_data_ptr += chunk_size;
		transfer_size += chunk_size;
	}

	/* Now get the reply. */
	r = libusb_bulk_transfer(uep->devh, uep->ep_num | 0x80, (void *)&reply,
				 sizeof(reply), &actual, 1000);
	if (r) {
		if (r == -7) {
			fprintf(stderr, "Timeout!\n");
			return r;
		}
		USB_ERROR("libusb_bulk_transfer", r);
		shut_down(uep);
	}

	reply = *((uint8_t *)&reply);
	if (reply) {
		fprintf(stderr, "Error: status %#x\n", reply);
		exit(update_error);
	}

	return 0;
}

/**
 * Transfer an image section (typically RW or RO).
 *
 * td           - transfer descriptor to use to communicate with the target
 * data_ptr     - pointer at the section base in the image
 * section_addr - address of the section in the target memory space
 * data_len     - section size
 */
static void transfer_section(struct transfer_descriptor *td, uint8_t *data_ptr,
			     uint32_t section_addr, size_t data_len)
{
	/*
	 * Actually, we can skip trailing chunks of 0xff, as the entire
	 * section space must be erased before the update is attempted.
	 */
	while (data_len && (data_ptr[data_len - 1] == 0xff))
		data_len--;

	/*
	 * Make sure total size is 4 bytes aligned, this is required for
	 * successful flashing.
	 */
	data_len = (data_len + 3) & ~3;

	printf("sending 0x%zx bytes to %#x\n", data_len, section_addr);
	while (data_len) {
		size_t payload_size;
		EVP_MD_CTX *ctx;
		int max_retries;
		struct update_pdu updu;

		/* prepare the header to prepend to the block. */
		payload_size = MIN(data_len, SIGNED_TRANSFER_SIZE);
		updu.block_size =
			htobe32(payload_size + sizeof(struct update_pdu));

		updu.cmd.block_base = htobe32(section_addr);

		/* Calculate the digest. */
		ctx = EVP_MD_CTX_new();
		sha_init(ctx);
		sha_update(ctx, &updu.cmd.block_base,
			   sizeof(updu.cmd.block_base));
		sha_update(ctx, data_ptr, payload_size);
		sha_final_into_block_digest(ctx, &updu.cmd.block_digest,
					    sizeof(updu.cmd.block_digest));
		EVP_MD_CTX_free(ctx);

		if (td->ep_type == usb_xfer) {
			for (max_retries = 10; max_retries; max_retries--)
				if (!transfer_block(&td->uep, &updu, data_ptr,
						    payload_size))
					break;

			if (!max_retries) {
				fprintf(stderr,
					"Failed to transfer block, %zd to go\n",
					data_len);
				exit(update_error);
			}
		} else {
			uint8_t error_code[4];
			size_t rxed_size = sizeof(error_code);
			uint32_t block_addr;

			block_addr = section_addr;

			/*
			 * A single byte response is expected, but let's give
			 * the driver a few extra bytes to catch cases when a
			 * different amount of data is transferred (which
			 * would indicate a synchronization problem).
			 */
			if (tpm_send_pkt(td, updu.cmd.block_digest, block_addr,
					 data_ptr, payload_size, error_code,
					 &rxed_size,
					 EXTENSION_FW_UPGRADE) < 0) {
				fprintf(stderr,
					"Failed to trasfer block, %zd to go\n",
					data_len);
				exit(update_error);
			}
			if (rxed_size != 1) {
				fprintf(stderr, "Unexpected return size %zd\n",
					rxed_size);
				exit(update_error);
			}

			if (error_code[0]) {
				fprintf(stderr, "Error %d\n", error_code[0]);
				exit(update_error);
			}
		}
		data_len -= payload_size;
		data_ptr += payload_size;
		section_addr += payload_size;
	}
}

/* Information about the target */
static struct first_response_pdu targ;

/*
 * Each RO or RW section of the new image can be in one of the following
 * states.
 */
enum upgrade_status {
	not_needed = 0, /* Version below or equal that on the target. */
	not_possible, /*
		       * RO is newer, but can't be transferred due to
		       * target RW shortcomings.
		       */
	needed /*
		* This section needs to be transferred to the
		* target.
		*/
};

/* Index to refer to a section within sections array */
enum section {
	RO_A,
	RW_A,
	RO_B,
	RW_B,
};

/*
 * This array describes all four sections of the new image. Defaults are for
 * H1 images. D2 images are scanned for SignedHeaders in the image
 */
static struct {
	const char *name;
	uint32_t offset;
	uint32_t size;
	enum upgrade_status ustatus;
	struct signed_header_version shv;
	uint32_t keyid;
} sections[] = { [RO_A] = { "RO_A", CONFIG_RO_MEM_OFF, CONFIG_RO_SIZE },
		 [RW_A] = { "RW_A", CONFIG_RW_MEM_OFF, CONFIG_RW_SIZE },
		 [RO_B] = { "RO_B", CHIP_RO_B_MEM_OFF, CONFIG_RO_SIZE },
		 [RW_B] = { "RW_B", CONFIG_RW_B_MEM_OFF, CONFIG_RW_SIZE } };

/*
 * Remove these definitions so a developer doesn't accidentally use them in
 * the future. All lookups should go through the sections array.
 */
#undef CONFIG_RO_MEM_OFF
#undef CONFIG_RW_MEM_OFF
#undef CHIP_RO_B_MEM_OFF
#undef CONFIG_RW_B_MEM_OFF
#undef CONFIG_RO_SIZE
#undef CONFIG_RW_SIZE
#undef CONFIG_FLASH_SIZE

/* Returns true if the specified header is valid */
static bool valid_header(const struct SignedHeader *const h, const size_t size)
{
	if (size < sizeof(struct SignedHeader))
		return false;

	if (h->image_size > size)
		return false;

	if (h->image_size < CONFIG_FLASH_BANK_SIZE)
		return false;

	/* Only H1 and D2 are currently supported. */
	if (h->magic != MAGIC_HAVEN && h->magic != MAGIC_DAUNTLESS)
		return false;

	/*
	 * Both Rx base and Ro base are the memory mapped address, but they
	 * should have the same offset. The rx section starts after the header.
	 */
	if (h->rx_base != h->ro_base + sizeof(struct SignedHeader))
		return false;

	/* Ensure each section falls within full size */
	if (h->ro_max - h->ro_base > size)
		return false;

	if (h->rx_max - h->rx_base > size)
		return false;

	return true;
}

/* Rounds and address up to the next 2KB boundary if not one already */
static inline uint32_t round_up_2kb(const uint32_t addr)
{
	const uint32_t mask = (2 * 1024) - 1;

	return (addr + mask) & ~mask;
}

static const struct SignedHeader *as_header(const void *image, uint32_t offset)
{
	return (void *)((uintptr_t)image + offset);
}

/* Returns the RW header or -1 if one cannot be found */
static int32_t find_rw_header(const void *image, uint32_t offset,
			      const uint32_t end)
{
	offset = round_up_2kb(offset);

	while (offset < end) {
		if (valid_header(as_header(image, offset), end - offset))
			return offset;
		offset = round_up_2kb(offset + 1);
	}

	return -1;
}

/* Return true if we located headers and set sections correctly */
static bool locate_headers(const void *image, const uint32_t size)
{
	const uint32_t slot_a_end = size / 2;
	const struct SignedHeader *h;
	int32_t rw_offset;

	/*
	 * We assume that all 512KB images are "valid" H1 images. The DBG images
	 * from the signer do not set the magic to -1 and no not set valid
	 * section offsets. We do not want to break this case as it is used in
	 * testing. The H1 offsets are also static, so we don't need to scan
	 * for RW headers.
	 */
	if (size == (512 * 1024)) {
		if (gsc_dev == GSC_DEVICE_ANY) {
			gsc_dev = GSC_DEVICE_H1;
			return true;
		}
		if (gsc_dev != GSC_DEVICE_H1) {
			fprintf(stderr, "Error: Cannot use Cr50 image.\n");
			return false;
		}
		return true;
	}

	/*
	 * We know that all other image types supported (i.e. Dauntless) are
	 * 1MB in size.
	 */
	if (size != (1024 * 1024)) {
		fprintf(stderr, "\nERROR: Image size (%d KB) is invalid\n",
			size / 1024);
		return false;
	}

	/* Validate the RO_A header */
	h = as_header(image, 0);
	if (!valid_header(h, slot_a_end)) {
		fprintf(stderr, "\nERROR: RO_A header is invalid\n");
		return false;
	}

	if (h->magic != MAGIC_DAUNTLESS) {
		fprintf(stderr,
			"Error: Cannot use non-Ti50 image with dauntless.\n");
		return false;
	}

	if ((gsc_dev != GSC_DEVICE_ANY) && (gsc_dev != GSC_DEVICE_DT))
		return false;

	gsc_dev = GSC_DEVICE_DT;

	sections[RO_A].offset = 0;
	sections[RO_A].size = h->image_size;

	/* Find RW_A */
	rw_offset = find_rw_header(
		image, sections[RO_A].offset + sections[RO_A].size, slot_a_end);
	if (rw_offset == -1) {
		fprintf(stderr, "\nERROR: RW_A header cannot be found\n");
		return false;
	}
	sections[RW_A].offset = rw_offset;
	sections[RW_A].size = as_header(image, rw_offset)->image_size;

	/* Validate the RO_B header */
	h = as_header(image, slot_a_end);
	if (!valid_header(h, size - slot_a_end)) {
		fprintf(stderr, "\nERROR: RO_B header is invalid\n");
		return false;
	}
	sections[RO_B].offset = slot_a_end;
	sections[RO_B].size = h->image_size;

	/* Find RW_B */
	rw_offset = find_rw_header(
		image, sections[RO_B].offset + sections[RO_B].size, size);
	if (rw_offset == -1) {
		fprintf(stderr, "\nERROR: RW_B header cannot be found\n");
		return false;
	}
	sections[RW_B].offset = rw_offset;
	sections[RW_B].size = as_header(image, rw_offset)->image_size;

	/* We found all of the headers and updated offset/size in sections */
	return true;
}

/*
 * Scan the new image and retrieve versions of all four sections, two RO and
 * two RW, verifying that image size is not too short along the way.
 */
static bool fetch_header_versions(const void *image)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(sections); i++) {
		const struct SignedHeader *h;

		h = (const struct SignedHeader *)((uintptr_t)image +
						  sections[i].offset);

		if (h->image_size < CONFIG_FLASH_BANK_SIZE) {
			/*
			 * Return an error for incorrectly signed images. If
			 * it's a RO image with 0 as its size, ignore the error.
			 *
			 * TODO(b/273510573): revisit after dbg versioning is
			 * figured out.
			 */
			if (h->image_size || sections[i].offset) {
				fprintf(stderr,
					"Image at offset %#5x too short "
					"(%d bytes)\n",
					sections[i].offset, h->image_size);
				return false;
			}

			printf("warning: invalid RO_A (size 0)\n");
		}
		sections[i].shv.epoch = h->epoch_;
		sections[i].shv.major = h->major_;
		sections[i].shv.minor = h->minor_;
		sections[i].keyid = h->keyid;
	}
	return true;
}

/* Compare to signer headers and determine which one is newer. */
static int a_newer_than_b(const struct signed_header_version *a,
			  const struct signed_header_version *b)
{
	uint32_t fields[][3] = {
		{ a->epoch, a->major, a->minor },
		{ b->epoch, b->major, b->minor },
	};
	size_t i;

	for (i = 0; i < ARRAY_SIZE(fields[0]); i++) {
		uint32_t a_value;
		uint32_t b_value;

		a_value = fields[0][i];
		b_value = fields[1][i];

		/*
		 * Let's filter out images where the section is not
		 * initialized and the version field value is set to all ones.
		 */
		if (a_value == 0xffffffff)
			a_value = 0;

		if (b_value == 0xffffffff)
			b_value = 0;

		if (a_value != b_value)
			return a_value > b_value;
	}

	return 0; /* All else being equal A is no newer than B. */
}

/*
 * Determine if the current RW version can be upgrade to the potential RW
 * version. If not, will exit the program.
 */
static void check_rw_upgrade(const struct signed_header_version *current_rw,
			     const struct signed_header_version *to_rw)
{
	/*
	 * Disallow upgrade to 0.0.16+ without going through 0.0.15
	 * first. This check won't be needed after 2023-01-01
	 */
	const struct signed_header_version ver15 = { .epoch = 0,
						     .major = 0,
						     .minor = 15 };
	const int current_less_than_15 = a_newer_than_b(&ver15, current_rw);
	const int to_greater_than_15 = a_newer_than_b(to_rw, &ver15);

	if ((gsc_dev == GSC_DEVICE_DT) && current_less_than_15 &&
	    to_greater_than_15) {
		printf("Must upgrade to RW 0.0.15 first!\n");
		/*  Do not continue with any upgrades RW or RO */
		exit(update_error);
	}
}

/*
 * Pick sections to transfer based on information retrieved from the target,
 * the new image, and the protocol version the target is running.
 */
static void pick_sections(struct transfer_descriptor *td)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(sections); i++) {
		uint32_t offset = sections[i].offset;

		if ((i == RW_A) || (i == RW_B)) {
			/* Skip currently active RW section. */
			bool active_rw_slot_b = td->rw_offset <
						sections[RO_B].offset;
			if ((i == RW_B) == active_rw_slot_b)
				continue;
			/*
			 * Ok, this would be the RW section to transfer to the
			 * device. Is it newer in the new image than the
			 * running RW section on the device?
			 *
			 * If not in 'upstart' mode - transfer even if
			 * versions are the same, timestamps could be
			 * different.
			 */

			if (a_newer_than_b(&sections[i].shv, &targ.shv[1]) ||
			    !td->upstart_mode) {
				/* Check will exit if disallowed */
				check_rw_upgrade(&targ.shv[1],
						 &sections[i].shv);
				sections[i].ustatus = needed;
			}
			/* Rest of loop is RO */
			continue;
		}

		/* Skip currently active RO section. */
		if (offset != td->ro_offset)
			continue;
		/*
		 * Ok, this would be the RO section to transfer to the device.
		 * Is it newer in the new image than the running RO section on
		 * the device?
		 */
		if (a_newer_than_b(&sections[i].shv, &targ.shv[0]) ||
		    td->force_ro)
			sections[i].ustatus = needed;
	}
}

static void setup_connection(struct transfer_descriptor *td)
{
	size_t rxed_size;
	size_t i;
	uint32_t error_code;

	/*
	 * Need to be backwards compatible, communicate with targets running
	 * different protocol versions.
	 */
	union {
		struct first_response_pdu rpdu;
		uint32_t legacy_resp;
	} start_resp;

	/* Send start request. */
	printf("start\n");

	if (td->ep_type == usb_xfer) {
		struct update_pdu updu;

		memset(&updu, 0, sizeof(updu));
		updu.block_size = htobe32(sizeof(updu));
		do_xfer(&td->uep, &updu, sizeof(updu), &start_resp,
			sizeof(start_resp), 1, &rxed_size);
	} else {
		rxed_size = sizeof(start_resp);
		if (tpm_send_pkt(td, 0, 0, NULL, 0, &start_resp, &rxed_size,
				 EXTENSION_FW_UPGRADE) < 0) {
			fprintf(stderr, "Failed to start transfer\n");
			exit(update_error);
		}
	}

	/* We got something. Check for errors in response */
	if (rxed_size < 8) {
		fprintf(stderr, "Unexpected response size %zd: ", rxed_size);
		for (i = 0; i < rxed_size; i++)
			fprintf(stderr, " %02x", ((uint8_t *)&start_resp)[i]);
		fprintf(stderr, "\n");
		exit(update_error);
	}

	protocol_version = be32toh(start_resp.rpdu.protocol_version);
	if (protocol_version < 5) {
		fprintf(stderr, "Unsupported protocol version %d\n",
			protocol_version);
		exit(update_error);
	}

	printf("target running protocol version %d\n", protocol_version);

	error_code = be32toh(start_resp.rpdu.return_value);

	if (error_code) {
		fprintf(stderr, "Target reporting error %d\n", error_code);
		if (td->ep_type == usb_xfer)
			shut_down(&td->uep);
		exit(update_error);
	}

	td->rw_offset = be32toh(start_resp.rpdu.backup_rw_offset);
	td->ro_offset = be32toh(start_resp.rpdu.backup_ro_offset);

	/* Running header versions. */
	for (i = 0; i < ARRAY_SIZE(targ.shv); i++) {
		targ.shv[i].minor = be32toh(start_resp.rpdu.shv[i].minor);
		targ.shv[i].major = be32toh(start_resp.rpdu.shv[i].major);
		targ.shv[i].epoch = be32toh(start_resp.rpdu.shv[i].epoch);
	}

	for (i = 0; i < ARRAY_SIZE(targ.keyid); i++)
		targ.keyid[i] = be32toh(start_resp.rpdu.keyid[i]);

	printf("keyids: RO 0x%08x, RW 0x%08x\n", targ.keyid[0], targ.keyid[1]);
	printf("offsets: backup RO at %#x, backup RW at %#x\n", td->ro_offset,
	       td->rw_offset);

	pick_sections(td);
}

/*
 * Channel TPM extension/vendor command over USB. The payload of the USB frame
 * in this case consists of the 2 byte subcommand code concatenated with the
 * command body. The caller needs to indicate if a response is expected, and
 * if it is - of what maximum size.
 */
static int ext_cmd_over_usb(struct usb_endpoint *uep, uint16_t subcommand,
			    const void *cmd_body, size_t body_size, void *resp,
			    size_t *resp_size)
{
	struct update_frame_header *ufh;
	uint16_t *frame_ptr;
	size_t usb_msg_size;
	EVP_MD_CTX *ctx;

	usb_msg_size = sizeof(struct update_frame_header) + sizeof(subcommand) +
		       body_size;

	ufh = malloc(usb_msg_size);
	if (!ufh) {
		fprintf(stderr, "%s: failed to allocate %zd bytes\n", __func__,
			usb_msg_size);
		return -1;
	}

	ufh->block_size = htobe32(usb_msg_size);
	ufh->cmd.block_base = htobe32(CONFIG_EXTENSION_COMMAND);
	frame_ptr = (uint16_t *)(ufh + 1);
	*frame_ptr = htobe16(subcommand);

	if (body_size)
		memcpy(frame_ptr + 1, cmd_body, body_size);

	/* Calculate the digest. */
	ctx = EVP_MD_CTX_new();
	sha_init(ctx);
	sha_update(ctx, &ufh->cmd.block_base,
		   usb_msg_size - offsetof(struct update_frame_header,
					   cmd.block_base));
	sha_final_into_block_digest(ctx, &ufh->cmd.block_digest,
				    sizeof(ufh->cmd.block_digest));
	EVP_MD_CTX_free(ctx);

	do_xfer(uep, ufh, usb_msg_size, resp, resp_size ? *resp_size : 0, 1,
		resp_size);

	free(ufh);
	return 0;
}

/*
 * Indicate to the target that update image transfer has been completed. Upon
 * receiveing of this message the target state machine transitions into the
 * 'rx_idle' state. The host may send an extension command to reset the target
 * after this.
 */
static void send_done(struct usb_endpoint *uep)
{
	uint32_t out;

	/* Send stop request, ignoring reply. */
	out = htobe32(UPGRADE_DONE);
	do_xfer(uep, &out, sizeof(out), &out, 1, 0, NULL);
}

/*
 * Old cr50 images fail the update if sections are sent out of order. They
 * require each block to have an offset greater than the block that was sent
 * before. RO has a lower offset than RW, so old cr50 images reject RO if it's
 * sent right after RW.
 * This offset restriction expires after 60 seconds. Delay the RO update long
 * enough for cr50 to not care that it has a lower offset than RW.
 *
 * Make the delay 65 seconds instead of 60 to cover differences in the speed of
 * H1's clock and the host clock.
 */
#define NEXT_SECTION_DELAY 65

/*
 * H1 support for flashing RO immediately after RW added in 0.3.20/0.4.20.
 * D2 support exists in all versions.
 */
static int supports_reordered_section_updates(struct signed_header_version *rw)
{
	switch (gsc_dev) {
	case GSC_DEVICE_H1:
		return (rw->epoch || rw->major > 4 ||
			(rw->major >= 3 && rw->minor >= 20));
	case GSC_DEVICE_DT:
		return true;
	default:
		return false;
	}
}

/* Returns number of successfully transmitted image sections. */
static int transfer_image(struct transfer_descriptor *td, uint8_t *data,
			  size_t data_len)
{
	size_t i;
	int num_txed_sections = 0;
	int needs_delay = !supports_reordered_section_updates(&targ.shv[1]);

	/*
	 * In case both RO and RW updates are required, make sure the RW
	 * section is updated before the RO. The array below keeps sections
	 * offsets in the required order.
	 */
	const enum section update_order[] = { RW_A, RW_B, RO_A, RO_B };

	for (i = 0; i < ARRAY_SIZE(update_order); i++) {
		const enum section sect = update_order[i];

		if (sections[sect].ustatus != needed)
			continue;
		if (num_txed_sections && needs_delay) {
			/*
			 * Delays more than 5 seconds cause the update
			 * to timeout. End the update before the delay
			 * and set it up after to recover from the
			 * timeout.
			 */
			if (td->ep_type == usb_xfer)
				send_done(&td->uep);
			printf("Waiting %ds for %s update.\n",
			       NEXT_SECTION_DELAY, sections[sect].name);
			sleep(NEXT_SECTION_DELAY);
			setup_connection(td);
		}

		transfer_section(td, data + sections[sect].offset,
				 sections[sect].offset, sections[sect].size);
		num_txed_sections++;
	}

	if (!num_txed_sections)
		printf("nothing to do\n");
	else
		printf("-------\nupdate complete\n");
	return num_txed_sections;
}

uint32_t send_vendor_command(struct transfer_descriptor *td,
			     uint16_t subcommand, const void *command_body,
			     size_t command_body_size, void *response,
			     size_t *response_size)
{
	int32_t rv;

	if (td->ep_type == usb_xfer) {
		/*
		 * When communicating over USB the response is always supposed
		 * to have the result code in the first byte of the response,
		 * to be stripped from the actual response body by this
		 * function.
		 */
		uint8_t temp_response[MAX_RX_BUF_SIZE];
		size_t max_response_size;

		if (!response_size) {
			max_response_size = 1;
		} else if (*response_size < (sizeof(temp_response))) {
			max_response_size = *response_size + 1;
		} else {
			fprintf(stderr,
				"Error: Expected response too large (%zd)\n",
				*response_size);
			/* Should happen only when debugging. */
			exit(update_error);
		}

		ext_cmd_over_usb(&td->uep, subcommand, command_body,
				 command_body_size, temp_response,
				 &max_response_size);
		if (!max_response_size) {
			/*
			 * we must be talking to an older Cr50 firmware, which
			 * does not return the result code in the first byte
			 * on success, nothing to do.
			 */
			if (response_size)
				*response_size = 0;
			rv = 0;
		} else {
			rv = temp_response[0];
			if (response_size) {
				*response_size = max_response_size - 1;
				memcpy(response, temp_response + 1,
				       *response_size);
			}
		}
	} else {
		rv = tpm_send_pkt(td, 0, 0, command_body, command_body_size,
				  response, response_size, subcommand);

		if (rv == -1) {
			fprintf(stderr,
				"Error: Failed to send vendor command %d\n",
				subcommand);
			exit(update_error);
		}
	}

	return rv; /* This will be converted into uint32_t */
}

/*
 * Corrupt the header of the inactive rw image to make sure the system can't
 * rollback
 */
static void invalidate_inactive_rw(struct transfer_descriptor *td)
{
	/* Corrupt the rw image that is not running. */
	uint32_t rv;

	rv = send_vendor_command(td, VENDOR_CC_INVALIDATE_INACTIVE_RW, NULL, 0,
				 NULL, NULL);
	if (!rv) {
		printf("Inactive header invalidated\n");
		return;
	}

	fprintf(stderr, "*%s: Error %#x\n", __func__, rv);
	exit(update_error);
}

/*
 * Try setting CCD capability.
 *
 * The 'parameter' string includes capability and desired new state separated
 * by a ':', both parts could be abbreviated and checked for the match as case
 * insensitive.
 *
 * The result of the attempt depends on the policies installed on
 * Ti50. The result could be on of the following:
 *
 * - success (capability is successfully changed, or is already at the
 *   requested level),
 * - various errors if setting the capability is not allowed or something
 *   goes wrong on Ti50
 * - request for physical presence confirmation
 */
static enum exit_values process_set_capabililty(struct transfer_descriptor *td,
						const char *parameter)
{
	const char *colon;
	size_t len;
	size_t cap_index;
	size_t i;
	uint8_t rc;
	const char *error_text;
	const char *cr50_err =
		"Note: setting capabilities not available on Cr50\n";
	/*
	 * The payload is three bytes, command version, capability, and
	 * desired state, all expressed as u8.
	 */
	struct __attribute__((__packed__)) {
		uint8_t version;
		uint8_t cap;
		uint8_t value;
	} command;
	/*
	 * Translation table of possible desired capabilities, Cr50 values
	 * and duplicated in common/syscalls/src/ccd.rs::CcdCapState.
	 */
	struct {
		const char *state_name;
		enum ccd_capability_state desired_state;
	} states[] = {
		{ "default", CCD_CAP_STATE_DEFAULT },
		{ "always", CCD_CAP_STATE_ALWAYS },
		{ "if_opened", CCD_CAP_STATE_IF_OPENED },
	};

	/*
	 * Possible responses from Ti50 when trying to modify AllowUnverifiedRo
	 * capability. The values come from
	 * common/libs/tpm2/extension/src/lib.rs::TpmvReturnCode.
	 */
	enum set_allow_unverified_ro_responses {
		AUR_SUCCESS = 0,
		AUR_BOGUS_ARGS = 1,
		AUR_INTERNAL_ERROR = 6,
		AUR_NOT_ALLOWED = 7,
		AUR_IN_PROGRESS = 9,
	};

	/*
	 * Validate the parameter, for starters make sure that the colon
	 * symbol is present and is neither the first nor the last character
	 * in the string.
	 */
	colon = strchr(parameter, ':');
	if (!colon || (colon == parameter) || (colon[1] == '\0')) {
		fprintf(stderr, "Misformatted capability parameter: %s\n",
			parameter);
		exit(update_error);
	}

	/*
	 * Find the capability index in the table, reject ambiguous
	 * abbreviations.
	 */
	len = colon - parameter;
	for (i = 0, cap_index = ARRAY_SIZE(ti50_cap_info);
	     i < ARRAY_SIZE(ti50_cap_info); i++) {
		if (!strncasecmp(ti50_cap_info[i].name, parameter, len)) {
			if (cap_index != ARRAY_SIZE(ti50_cap_info)) {
				fprintf(stderr, "Ambiguous capability name\n");
				exit(update_error);
			}
			cap_index = i;
		}
	}
	if (cap_index == ARRAY_SIZE(ti50_cap_info)) {
		fprintf(stderr, "Unknown capability name\n%s", cr50_err);
		exit(update_error);
	}

	/* Calculate length of the desired value. */
	len = strlen(parameter) - len - 1;

	/* Find the value index in the table. */
	for (i = 0; i < ARRAY_SIZE(states); i++) {
		if (!strncasecmp(states[i].state_name, colon + 1, len))
			break;
	}
	if (i == ARRAY_SIZE(states)) {
		fprintf(stderr, "Unsupported capability value\n");
		return update_error;
	}

	/* Prepare and send vendor command to request setting capability. */
	command.version = CCD_VERSION;
	command.cap = (uint8_t)cap_index;
	command.value = (uint8_t)states[i].desired_state;

	i = 0;
	len = 1;
	send_vendor_command(td, VENDOR_CC_SET_CAPABILITY, &command,
			    sizeof(command), &rc, &len);

	if (len != 1) {
		fprintf(stderr, "Unexpected return message size %zd\n", len);
		if (len == 0)
			fprintf(stderr, "%s", cr50_err);
		return update_error;
	}

	switch (rc) {
	case AUR_IN_PROGRESS:
		/*
		 * Physical presence poll is required, note fall through to
		 * the next case.
		 */
		poll_for_pp(td, VENDOR_CC_CCD, CCDV_PP_POLL_SET_CAPABILITY);
	case AUR_SUCCESS:
		return noop; /* All is well, no need to do anything. */
	case AUR_BOGUS_ARGS:
		error_text = "BogusArgs";
		break;
	case AUR_INTERNAL_ERROR:
		error_text = "InternalError";
		break;
	case AUR_NOT_ALLOWED:
		error_text = "NotAllowed";
		break;
	default:
		error_text = "Unknown";
		break;
	}
	fprintf(stderr, "Got error %d(%s)\n", rc, error_text);
	return update_error;
}

static void process_erase_ap_ro_hash(struct transfer_descriptor *td)
{
	/* Try erasing AP RO hash, could fail if board ID is programmed. */
	uint32_t rv;
	uint8_t response;
	size_t response_size;
	char error_details[64];

	response_size = sizeof(response);
	rv = send_vendor_command(td, VENDOR_CC_SEED_AP_RO_CHECK, NULL, 0,
				 &response, &response_size);
	if (!rv) {
		printf("AP RO hash erased\n");
		exit(0);
	}

	if (response_size == sizeof(response)) {
		switch (response) {
		case ARCVE_FLASH_ERASE_FAILED:
			snprintf(error_details, sizeof(error_details),
				 "erase failure");
			break;
		case ARCVE_BID_PROGRAMMED:
			snprintf(error_details, sizeof(error_details),
				 "BID already programmed");
			break;
		default:
			snprintf(error_details, sizeof(error_details),
				 "Unexpected error rc %d, response %d", rv,
				 response);
			break;
		}
	} else {
		snprintf(error_details, sizeof(error_details),
			 "misconfigured response, rc=%d, size %zd", rv,
			 response_size);
	}

	fprintf(stderr, "Error: %s\n", error_details);

	exit(update_error);
}

static void generate_reset_request(struct transfer_descriptor *td)
{
	size_t response_size;
	uint8_t response;
	uint16_t subcommand;
	uint8_t command_body[2]; /* Max command body size. */
	size_t command_body_size;
	const char *reset_type;
	int rv;

	if (protocol_version < 6) {
		if (td->ep_type == usb_xfer) {
			/*
			 * Send a second stop request, which should reboot
			 * without replying.
			 */
			send_done(&td->uep);
		}
		/* Nothing we can do over /dev/tpm0 running versions below 6. */
		return;
	}

	/*
	 * If this is an upstart request, don't post a request now. The target
	 * should handle it on the next reboot.
	 */
	if (td->upstart_mode)
		return;

	/*
	 * If the user explicitly wants it, request post reset instead of
	 * immediate reset. In this case next time the target reboots, the GSC
	 * will reboot as well, and will consider running the uploaded code.
	 *
	 * Otherwise, to reset the target the host is supposed to send the
	 * command to enable the uploaded image disabled by default.
	 */
	response_size = 1;
	if (td->post_reset) {
		subcommand = EXTENSION_POST_RESET;
		command_body_size = 0;
		reset_type = "posted";
	} else {
		subcommand = VENDOR_CC_TURN_UPDATE_ON;
		command_body_size = sizeof(command_body);
		command_body[0] = 0;
		command_body[1] = 100; /* Reset in 100 ms. */
		reset_type = "requested";
	}

	rv = send_vendor_command(td, subcommand, command_body,
				 command_body_size, &response, &response_size);

	if (rv) {
		fprintf(stderr, "*%s: Error %#x\n", __func__, rv);
		exit(update_error);
	}
	printf("reboot %s\n", reset_type);
}

/* Forward to correct SHA implementation based on image type */
static void sha_init(EVP_MD_CTX *ctx)
{
	if (gsc_dev == GSC_DEVICE_H1)
		EVP_DigestInit_ex(ctx, EVP_sha1(), NULL);
	else if (gsc_dev == GSC_DEVICE_DT)
		EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
	else {
		fprintf(stderr, "Error: unknown GSC device type\n");
		exit(update_error);
	}
}

/* Forward to correct SHA implementation based on image type */
static void sha_update(EVP_MD_CTX *ctx, const void *data, size_t len)
{
	EVP_DigestUpdate(ctx, data, len);
}

/* Forward to correct SHA implementation based on image type */
static void sha_final_into_block_digest(EVP_MD_CTX *ctx, void *block_digest,
					size_t size)
{
	/* Big enough for either hash algo */
	uint8_t full_digest[SHA256_DIGEST_LENGTH];
	unsigned int length;

	EVP_DigestFinal(ctx, full_digest, &length);

	/* Copy out the smaller of the 2 byte counts. */
	memcpy(block_digest, full_digest, MIN(size, length));
}

/*
 * Machine output is formatted as "key=value", one key-value pair per line, and
 * parsed by other programs (e.g., debugd). The value part should be specified
 * in the printf-like way. For example:
 *
 *           print_machine_output("date", "%d/%d/%d", 2018, 1, 1),
 *
 * which outputs this line in console:
 *
 *           date=2018/1/1
 *
 * The key part should not contain '=' or newline. The value part may contain
 * special characters like spaces, quotes, brackets, but not newlines. The
 * newline character means end of value.
 *
 * Any output format change in this function may require similar changes on the
 * programs that are using this gsctool.
 */
__attribute__((__format__(__printf__, 2, 3))) static void print_machine_output(
	const char *key, const char *format, ...)
{
	va_list args;

	if (strchr(key, '=') != NULL || strchr(key, '\n') != NULL) {
		fprintf(stderr,
			"Error: key %s contains '=' or a newline character.\n",
			key);
		return;
	}

	if (strchr(format, '\n') != NULL) {
		fprintf(stderr,
			"Error: value format %s contains a newline character. "
			"\n",
			format);
		return;
	}

	va_start(args, format);

	printf("%s=", key);
	vprintf(format, args);
	printf("\n");

	va_end(args);
}

/*
 * Prints out the header, including FW versions and board IDs, of the given
 * image. Output in a machine-friendly format if show_machine_output is true.
 */
static int show_headers_versions(const void *image, bool show_machine_output)
{
	/*
	 * There are 2 FW slots in an image, and each slot has 2 sections, RO
	 * and RW. The 2 slots should have identical FW versions and board
	 * IDs.
	 */
	const size_t kNumSlots = 2;
	const size_t kNumSectionsPerSlot = 2;

	/*
	 * String representation of FW version (<epoch>:<major>:<minor>), one
	 * string for each FW section.
	 */
	char ro_fw_ver[kNumSlots][MAX_FW_VER_LENGTH];
	char rw_fw_ver[kNumSlots][MAX_FW_VER_LENGTH];

	uint32_t dev_id0_[kNumSlots];
	uint32_t dev_id1_[kNumSlots];
	uint32_t print_devid = 0;

	struct board_id {
		uint32_t id;
		uint32_t mask;
		uint32_t flags;
	} bid[kNumSlots];

	char bid_string[kNumSlots][MAX_BOARD_ID_LENGTH];

	size_t i;

	for (i = 0; i < ARRAY_SIZE(sections); i++) {
		const struct SignedHeader *h =
			(const struct SignedHeader *)((uintptr_t)image +
						      sections[i].offset);
		const size_t slot_idx = i / kNumSectionsPerSlot;

		uint32_t cur_bid;
		size_t j;

		if (sections[i].name[1] == 'O') {
			/* RO. */
			snprintf(ro_fw_ver[slot_idx], MAX_FW_VER_LENGTH,
				 "%d.%d.%d", h->epoch_, h->major_, h->minor_);
			/* No need to read board ID in an RO section. */
			continue;
		} else {
			/* RW. */
			snprintf(rw_fw_ver[slot_idx], MAX_FW_VER_LENGTH,
				 "%d.%d.%d", h->epoch_, h->major_, h->minor_);
		}

		/*
		 * For RW sections, retrieves the board ID fields' contents,
		 * which are stored XORed with a padding value.
		 */
		bid[slot_idx].id = h->board_id_type ^ SIGNED_HEADER_PADDING;
		bid[slot_idx].mask = h->board_id_type_mask ^
				     SIGNED_HEADER_PADDING;
		bid[slot_idx].flags = h->board_id_flags ^ SIGNED_HEADER_PADDING;

		dev_id0_[slot_idx] = h->dev_id0_;
		dev_id1_[slot_idx] = h->dev_id1_;
		/* Print the devid if any slot has a non-zero devid. */
		print_devid |= h->dev_id0_ | h->dev_id1_;

		/*
		 * If board ID is a 4-uppercase-letter string (as it ought to
		 * be), print it as 4 letters, otherwise print it as an 8-digit
		 * hex.
		 */
		cur_bid = bid[slot_idx].id;
		for (j = 0; j < sizeof(cur_bid); ++j)
			if (!isupper(((const char *)&cur_bid)[j]))
				break;

		if (j == sizeof(cur_bid)) {
			cur_bid = be32toh(cur_bid);
			snprintf(bid_string[slot_idx], MAX_BOARD_ID_LENGTH,
				 "%.4s", (const char *)&cur_bid);
		} else {
			snprintf(bid_string[slot_idx], MAX_BOARD_ID_LENGTH,
				 "%08x", cur_bid);
		}
	}

	if (show_machine_output) {
		print_machine_output("IMAGE_RO_FW_VER", "%s", ro_fw_ver[0]);
		print_machine_output("IMAGE_RW_FW_VER", "%s", rw_fw_ver[0]);
		print_machine_output("IMAGE_BID_STRING", "%s", bid_string[0]);
		print_machine_output("IMAGE_BID_MASK", "%08x", bid[0].mask);
		print_machine_output("IMAGE_BID_FLAGS", "%08x", bid[0].flags);
	} else {
		printf("RO_A:%s RW_A:%s[%s:%08x:%08x] ", ro_fw_ver[0],
		       rw_fw_ver[0], bid_string[0], bid[0].mask, bid[0].flags);
		printf("RO_B:%s RW_B:%s[%s:%08x:%08x]\n", ro_fw_ver[1],
		       rw_fw_ver[1], bid_string[1], bid[1].mask, bid[1].flags);

		if (print_devid) {
			printf("DEVID: 0x%08x 0x%08x", dev_id0_[0],
			       dev_id1_[0]);
			/*
			 * Only print the second devid if it's different.
			 * Separate the devids with tabs, so it's easier to
			 * read.
			 */
			if (dev_id0_[0] != dev_id0_[1] ||
			    dev_id1_[0] != dev_id1_[1])
				printf("\t\t\t\tDEVID_B: 0x%08x 0x%08x",
				       dev_id0_[1], dev_id1_[1]);
			printf("\n");
		}
	}

	return 0;
}

/*
 * The default flag value will allow to run images built for any hardware
 * generation of a particular board ID.
 */
#define DEFAULT_BOARD_ID_FLAG 0xff00
static int parse_bid(const char *opt, struct board_id *bid,
		     enum board_id_action *bid_action)
{
	char *e;
	const char *param2;
	size_t param1_length;

	if (!opt) {
		*bid_action = bid_get;
		return 1;
	}

	/* Set it here to make bailing out easier later. */
	bid->flags = DEFAULT_BOARD_ID_FLAG;

	*bid_action = bid_set; /* Ignored by caller on errors. */

	/*
	 * Pointer to the optional second component of the command line
	 * parameter, when present - separated by a colon.
	 */
	param2 = strchr(opt, ':');
	if (param2) {
		param1_length = param2 - opt;
		param2++;
		if (!*param2)
			return 0; /* Empty second parameter. */
	} else {
		param1_length = strlen(opt);
	}

	if (!param1_length)
		return 0; /* Colon is the first character of the string? */

	if (param1_length <= 4) {
		unsigned i;

		/* Input must be a symbolic board name. */
		bid->type = 0;
		for (i = 0; i < param1_length; i++)
			bid->type = (bid->type << 8) | opt[i];
	} else {
		bid->type = (uint32_t)strtoul(opt, &e, 0);
		if ((param2 && (*e != ':')) || (!param2 && *e))
			return 0;
	}

	if (param2) {
		bid->flags = (uint32_t)strtoul(param2, &e, 0);
		if (*e)
			return 0;
	}

	return 1;
}

/*
 * Reads a two-character hexadecimal byte from a string. If the string is
 * ill-formed, returns 0. Otherwise, |byte| contains the byte value and the
 * return value is non-zero.
 */
static int read_hex_byte(const char *s, uint8_t *byte)
{
	uint8_t b = 0;
	for (const char *end = s + 2; s < end; ++s) {
		if (*s >= '0' && *s <= '9')
			b = b * 16 + *s - '0';
		else if (*s >= 'A' && *s <= 'F')
			b = b * 16 + 10 + *s - 'A';
		else if (*s >= 'a' && *s <= 'f')
			b = b * 16 + 10 + *s - 'a';
		else
			return 0;
	}
	*byte = b;
	return 1;
}

static int parse_sn_bits(const char *opt, uint8_t *sn_bits)
{
	size_t len = strlen(opt);

	if (!strncmp(opt, "0x", 2)) {
		opt += 2;
		len -= 2;
	}
	if (len != SN_BITS_SIZE * 2)
		return 0;

	for (int i = 0; i < SN_BITS_SIZE; ++i, opt += 2)
		if (!read_hex_byte(opt, sn_bits++))
			return 0;

	return 1;
}

static int parse_sn_inc_rma(const char *opt, uint8_t *arg)
{
	uint32_t inc;
	char *e;

	inc = (uint32_t)strtoul(opt, &e, 0);

	if (opt == e || *e != '\0' || inc > 7)
		return 0;

	*arg = inc;
	return 1;
}

static uint32_t common_process_password(struct transfer_descriptor *td,
					enum ccd_vendor_subcommands subcmd)
{
	size_t response_size;
	uint8_t response;
	uint32_t rv;
	char *password = NULL;
	char *password_copy = NULL;
	ssize_t copy_len;
	ssize_t len;
	size_t zero = 0;
	struct termios oldattr, newattr;

	/* Suppress command line echo while password is being entered. */
	tcgetattr(STDIN_FILENO, &oldattr);
	newattr = oldattr;
	newattr.c_lflag &= ~ECHO;
	newattr.c_lflag |= (ICANON | ECHONL);
	tcsetattr(STDIN_FILENO, TCSANOW, &newattr);

	/* With command line echo suppressed request password entry twice. */
	printf("Enter password:");
	len = getline(&password, &zero, stdin);
	printf("Re-enter password:");
	zero = 0;
	copy_len = getline(&password_copy, &zero, stdin);

	/* Restore command line echo. */
	tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);

	/* Empty password will still have the newline. */
	if ((len <= 1) || !password_copy || (copy_len != len)) {
		fprintf(stderr, "Error reading password\n");
		if (password)
			free(password);
		if (password_copy)
			free(password_copy);
		if ((copy_len >= 0) && (len >= 0) && (copy_len != len))
			fprintf(stderr, "Password length mismatch\n");
		exit(update_error);
	}

	/* Compare the two inputs. */
	if (strcmp(password, password_copy)) {
		fprintf(stderr, "Entered passwords don't match\n");
		free(password);
		free(password_copy);
		exit(update_error);
	}

	/*
	 * Ok, we have a password, let's move it in the buffer to overwrite
	 * the newline and free a byte to prepend the subcommand code.
	 */
	memmove(password + 1, password, len - 1);
	password[0] = subcmd;
	response_size = sizeof(response);
	rv = send_vendor_command(td, VENDOR_CC_CCD, password, len, &response,
				 &response_size);
	free(password);
	free(password_copy);

	if ((rv != VENDOR_RC_SUCCESS) && (rv != VENDOR_RC_IN_PROGRESS))
		fprintf(stderr, "Error sending password: rv %d, response %d\n",
			rv, response_size ? response : 0);

	return rv;
}

static void process_password(struct transfer_descriptor *td)
{
	if (common_process_password(td, CCDV_PASSWORD) == VENDOR_RC_SUCCESS)
		return;

	exit(update_error);
}

void poll_for_pp(struct transfer_descriptor *td, uint16_t command,
		 uint8_t poll_type)
{
	uint8_t response;
	uint8_t prev_response;
	size_t response_size;
	int rv;

	prev_response = ~0; /* Guaranteed invalid value. */

	while (1) {
		response_size = sizeof(response);
		rv = send_vendor_command(td, command, &poll_type,
					 sizeof(poll_type), &response,
					 &response_size);

		if (((rv != VENDOR_RC_SUCCESS) &&
		     (rv != VENDOR_RC_IN_PROGRESS)) ||
		    (response_size != 1)) {
			fprintf(stderr, "Error: rv %d, response %d\n", rv,
				response_size ? response : 0);
			exit(update_error);
		}

		if (response == CCD_PP_DONE) {
			printf("PP Done!\n");
			return;
		}

		if (response == CCD_PP_CLOSED) {
			fprintf(stderr,
				"Error: Physical presence check timeout!\n");
			exit(update_error);
		}

		if (response == CCD_PP_AWAITING_PRESS) {
			printf("Press PP button now!\n");
		} else if (response == CCD_PP_BETWEEN_PRESSES) {
			if (prev_response != response)
				printf("Another press will be required!\n");
		} else {
			fprintf(stderr, "Error: unknown poll result %d\n",
				response);
			exit(update_error);
		}
		prev_response = response;

		usleep(500 * 1000); /* Poll every half a second. */
	}
}

/* Determine the longest capability name found in the passed in table. */
static size_t longest_cap_name_len(const struct ccd_capability_info *table,
				   size_t count)
{
	size_t i;
	size_t result = 0;

	for (i = 0; i < count; i++) {
		size_t this_size;

		this_size = strlen(table[i].name);
		if (this_size > result)
			result = this_size;
	}
	return result;
}

/*
 * Print the passed in capability name padded to the longest length determined
 * earlier.
 */
static void print_aligned(const char *name, size_t field_size)
{
	size_t name_len;

	name_len = strlen(name);

	printf("%s", name);
	while (name_len < field_size) {
		printf(" ");
		name_len++;
	}
}

/*
 * Translates "AnExampleTPMString" into "AN_EXAMPLE_TPM_STRING". Note that
 * output must be large enough to contain a 150%-sized string of input.
 */
static void to_upper_underscore(const char *input, char *output)
{
	bool first_char = true;
	bool needs_underscore = false;

	while (*input != '\0') {
		*output = toupper(*input);
		/*
		 * If we encounter an upper case char in the input, we may need
		 * to add an underscore in the output before (re)writing the
		 * output char
		 */
		if (*output == *input) {
			/*
			 * See if the next input letter is lower case, that
			 * means this uppercase letter needs a '_' before it.
			 */
			if (islower(*(input + 1)) && !first_char)
				needs_underscore = true;
			if (needs_underscore) {
				needs_underscore = false;
				*output++ = '_';
				*output = *input;
			}
		} else {
			/*
			 * We encountered a lower case, so the next upper case
			 * should have a '_' before it
			 */
			needs_underscore = true;
		}
		first_char = false;
		input++;
		output++;
	}
	*output = '\0';
}

/*
 * Ensure that the CCD info response is well formed otherwise exits. Upon return
 * the ccd_info variable will contain the structured ccd info response and the
 * return value is the version of ccd info to parse with (e.g. 0 for cr50 and
 * 1 for ti50).
 */
static uint32_t validate_into_ccd_info_response(
	void *response, size_t response_size,
	struct ccd_info_response *ccd_info)
{
	struct ccd_info_response_header ccd_info_header;
	size_t i;
	uint32_t ccd_info_version;

	if (response_size < sizeof(ccd_info_header)) {
		fprintf(stderr, "CCD info response too short %zd\n",
			response_size);
		exit(update_error);
	}

	/* Let's check if this is a newer version response. */
	memcpy(&ccd_info_header, response, sizeof(ccd_info_header));
	if ((ccd_info_header.ccd_magic == CCD_INFO_MAGIC) &&
	    (ccd_info_header.ccd_size == response_size) &&
	    /* Verify that payload size matches ccd_info size. */
	    ((response_size - sizeof(ccd_info_header)) == sizeof(*ccd_info))) {
		ccd_info_version = ccd_info_header.ccd_version;
		memcpy(ccd_info,
		       (uint8_t *)response +
			       sizeof(struct ccd_info_response_header),
		       sizeof(*ccd_info));
		/*
		 * V1 CCD info structure uses little endian for transmission.
		 * No need to update to host endianness since it is already LE.
		 */
	} else if (response_size == CCD_INFO_V0_SIZE) {
		ccd_info_version = 0; /* Default, Cr50 case. */
		memcpy(ccd_info, response, sizeof(*ccd_info));
		/*
		 * V0 CCD info structure uses big endian for transmission.
		 * Update fields to host endianness.
		 */
		ccd_info->ccd_flags = be32toh(ccd_info->ccd_flags);
		for (i = 0; i < ARRAY_SIZE(ccd_info->ccd_caps_current); i++) {
			ccd_info->ccd_caps_current[i] =
				be32toh(ccd_info->ccd_caps_current[i]);
			ccd_info->ccd_caps_defaults[i] =
				be32toh(ccd_info->ccd_caps_defaults[i]);
		}
	} else {
		fprintf(stderr, "Unexpected CCD info response size %zd\n",
			response_size);
		exit(update_error);
	}

	return ccd_info_version;
}

static void print_ccd_info(void *response, size_t response_size,
			   bool show_machine_output)
{
	struct ccd_info_response ccd_info;
	size_t i;
	const struct ccd_capability_info cr50_cap_info[] = CAP_INFO_DATA;
	const char *state_names[] = CCD_STATE_NAMES;
	const char *cap_state_names[] = CCD_CAP_STATE_NAMES;
	uint32_t caps_bitmap = 0;
	uint32_t ccd_info_version;

	/*
	 * CCD info structure is different for different GSCs. Two layouts are
	 * defined at this time, version 0 (Cr50) and version 1 (Ti50). The
	 * array below indexed by version number provides access to version
	 * specific information about the layout.
	 */
	const struct {
		size_t cap_count;
		const struct ccd_capability_info *info_table;
	} version_to_ccd[] = {
		{ CR50_CCD_CAP_COUNT, cr50_cap_info },
		{ TI50_CCD_CAP_COUNT, ti50_cap_info },
	};

	/* Run time determined properties of the CCD info table. */
	size_t gsc_cap_count;
	const struct ccd_capability_info *gsc_capability_info;
	size_t name_column_width;

	ccd_info_version = validate_into_ccd_info_response(
		response, response_size, &ccd_info);

	if (ccd_info_version >= ARRAY_SIZE(version_to_ccd)) {
		fprintf(stderr, "Unsupported CCD info version number %d\n",
			ccd_info_version);
		exit(update_error);
	}

	/* Now report CCD state on the console. */
	const char *const state = ccd_info.ccd_state > ARRAY_SIZE(state_names) ?
					  "Error" :
					  state_names[ccd_info.ccd_state];
	const char *const password = (ccd_info.ccd_indicator_bitmap &
				      CCD_INDICATOR_BIT_HAS_PASSWORD) ?
					     "Set" :
					     "None";
	if (show_machine_output) {
		print_machine_output("STATE", "%s", state);
		print_machine_output("PASSWORD", "%s", password);
		print_machine_output("CCD_FLAGS", "%#06x", ccd_info.ccd_flags);
		print_machine_output(
			"CCD_FLAG_TESTLAB_MODE", "%c",
			(ccd_info.ccd_flags & CCD_FLAG_TEST_LAB) ? 'Y' : 'N');
		print_machine_output("CCD_FLAG_FACTORY_MODE", "%c",
				     (ccd_info.ccd_flags &
				      CCD_FLAG_FACTORY_MODE_ENABLED) ?
					     'Y' :
					     'N');
	} else {
		printf("State: %s\n", state);
		printf("Password: %s\n", password);
		printf("Flags: %#06x\n", ccd_info.ccd_flags);
		printf("Capabilities, current and default:\n");
	}

	gsc_cap_count = version_to_ccd[ccd_info_version].cap_count;
	gsc_capability_info = version_to_ccd[ccd_info_version].info_table;
	name_column_width =
		longest_cap_name_len(gsc_capability_info, gsc_cap_count) + 1;
	for (i = 0; i < gsc_cap_count; i++) {
		int is_enabled;
		int index;
		int shift;
		int cap_current;
		int cap_default;

		index = i / (32 / CCD_CAP_BITS);
		shift = (i % (32 / CCD_CAP_BITS)) * CCD_CAP_BITS;

		cap_current = (ccd_info.ccd_caps_current[index] >> shift) &
			      CCD_CAP_BITMASK;
		cap_default = (ccd_info.ccd_caps_defaults[index] >> shift) &
			      CCD_CAP_BITMASK;

		if (ccd_info.ccd_force_disabled) {
			is_enabled = 0;
		} else {
			switch (cap_current) {
			case CCD_CAP_STATE_ALWAYS:
				is_enabled = 1;
				break;
			case CCD_CAP_STATE_UNLESS_LOCKED:
				is_enabled = (ccd_info.ccd_state !=
					      CCD_STATE_LOCKED);
				break;
			default:
				is_enabled = (ccd_info.ccd_state ==
					      CCD_STATE_OPENED);
				break;
			}
		}

		if (show_machine_output) {
			char upper[80];

			to_upper_underscore(gsc_capability_info[i].name, upper);
			print_machine_output(upper, "%c",
					     is_enabled ? 'Y' : 'N');
		} else {
			printf("  ");
			print_aligned(gsc_capability_info[i].name,
				      name_column_width);
			printf("%c %s", is_enabled ? 'Y' : '-',
			       cap_state_names[cap_current]);

			if (cap_current != cap_default)
				printf("  (%s)", cap_state_names[cap_default]);

			printf("\n");
		}

		if (is_enabled)
			caps_bitmap |= (1 << i);
	}
	if (show_machine_output) {
		print_machine_output("CCD_CAPS_BITMAP", "%#x", caps_bitmap);
		print_machine_output("CAPABILITY_MODIFIED", "%c",
				     (ccd_info.ccd_indicator_bitmap &
				      CCD_INDICATOR_BIT_ALL_CAPS_DEFAULT) ?
					     'N' :
					     'Y');
		print_machine_output("INITIAL_FACTORY_MODE", "%c",
				     (ccd_info.ccd_indicator_bitmap &
				      CCD_INDICATOR_BIT_INITIAL_FACTORY_MODE) ?
					     'Y' :
					     'N');

	} else {
		printf("CCD caps bitmap: %#x\n", caps_bitmap);
		printf("Capabilities are %s.\n",
		       (ccd_info.ccd_indicator_bitmap &
			CCD_INDICATOR_BIT_ALL_CAPS_DEFAULT) ?
			       "default" :
			       "modified");
		if (ccd_info.ccd_indicator_bitmap &
		    CCD_INDICATOR_BIT_INITIAL_FACTORY_MODE) {
			printf("Chip factory mode.");
		}
	}
}

static void process_ccd_state(struct transfer_descriptor *td, int ccd_unlock,
			      int ccd_open, int ccd_lock, int ccd_info,
			      bool show_machine_output)
{
	uint8_t payload;
	/* Max possible response size is when ccd_info is requested. */
	uint8_t response[sizeof(struct ccd_info_response_packet)];
	size_t response_size;
	int rv;

	if (ccd_unlock)
		payload = CCDV_UNLOCK;
	else if (ccd_open)
		payload = CCDV_OPEN;
	else if (ccd_lock)
		payload = CCDV_LOCK;
	else
		payload = CCDV_GET_INFO;

	response_size = sizeof(response);
	rv = send_vendor_command(td, VENDOR_CC_CCD, &payload, sizeof(payload),
				 &response, &response_size);

	/*
	 * If password is required - try sending the same subcommand
	 * accompanied by user password.
	 */
	if (rv == VENDOR_RC_PASSWORD_REQUIRED)
		rv = common_process_password(td, payload);

	if (rv == VENDOR_RC_SUCCESS) {
		if (ccd_info)
			print_ccd_info(response, response_size,
				       show_machine_output);
		return;
	}

	if (rv != VENDOR_RC_IN_PROGRESS) {
		fprintf(stderr, "Error: rv %d, response %d\n", rv,
			response_size ? response[0] : 0);
		exit(update_error);
	}

	/*
	 * Physical presence process started, poll for the state the user
	 * asked for. Only two subcommands would return 'IN_PROGRESS'.
	 */
	if (ccd_unlock)
		poll_for_pp(td, VENDOR_CC_CCD, CCDV_PP_POLL_UNLOCK);
	else
		poll_for_pp(td, VENDOR_CC_CCD, CCDV_PP_POLL_OPEN);
}

/*
 * Ensure that the AllowUnverifiedRO capability is set to always. If called for
 * Cr50 (which does not have this capability), the program will exit instead.
 */
static bool is_unverified_ro_allowed(struct transfer_descriptor *td)
{
	uint8_t cmd = CCDV_GET_INFO;
	/* Max possible response size is when ccd_info is requested. */
	uint8_t response[sizeof(struct ccd_info_response_packet)];
	size_t response_size = sizeof(response);
	struct ccd_info_response ccd_info;
	uint32_t ccd_info_version;
	int allow_unverified_ro_cap;
	int rv;

	rv = send_vendor_command(td, VENDOR_CC_CCD, &cmd, sizeof(cmd),
				 &response, &response_size);
	if (rv != VENDOR_RC_SUCCESS) {
		fprintf(stderr, "Error: rv %d, response %d\n", rv,
			response_size ? response[0] : 0);
		exit(update_error);
	}
	ccd_info_version = validate_into_ccd_info_response(
		response, response_size, &ccd_info);
	if (ccd_info_version != 1) {
		/*
		 * We also don't know what order future ccd info versions will
		 * place the AllowUnverifiedRO capability. We need to ensure
		 * that the array lookup below is still correct for future
		 * version if/when they become available
		 */
		fprintf(stderr,
			"Error: CCD info version incorrect (%d).\n"
			"Cr50 does not support this operation.\n",
			ccd_info_version);
		exit(update_error);
	}
	/*
	 * Pull out the AllowUnverifiedRo cap from the list.
	 * See ti50_cap_info for full order of V1 capabilities
	 */
	allow_unverified_ro_cap = (ccd_info.ccd_caps_current[1] >> 10) &
				  CCD_CAP_BITMASK;

	return allow_unverified_ro_cap == CCD_CAP_STATE_ALWAYS;
}

static enum exit_values process_wp(struct transfer_descriptor *td,
				   enum wp_options wp)
{
	size_t response_size;
	uint8_t response;
	int rv = 0;
	uint8_t command = wp;

	response_size = sizeof(response);

	/*
	 * Ti50 supports enable, disable, and follow, but cr50 doesn't and will
	 * return an error from the chip. gsctool supports the superset.
	 */
	switch (wp) {
	case WP_DISABLE:
	case WP_FOLLOW:
		/* Ensure that AllowUnverifiedRo is true then fallthrough */
		if (!is_unverified_ro_allowed(td)) {
			fprintf(stderr,
				"Error: Must set AllowUnverifiedRo cap to "
				"always first.\n"
				"Otherwise changes to AP RO may cause system "
				"to no longer boot.\n"
				"Use `gsctool -I AllowUnverifiedRo:always`\n");
			return update_error;
		}
	case WP_ENABLE:
		printf("Setting WP\n");
		/* Enabling write protect doesn't require any special checks */
		rv = send_vendor_command(td, VENDOR_CC_WP, &command,
					 sizeof(command), &response,
					 &response_size);
		break;
	default:
		/* Just check the wp status without a parameter */
		printf("Getting WP\n");
		rv = send_vendor_command(td, VENDOR_CC_WP, NULL, 0, &response,
					 &response_size);
	}

	/*
	 * If we tried to disable and we got in progress, then prompt the
	 * user for power button pushes
	 */
	if (wp == WP_DISABLE && rv == VENDOR_RC_IN_PROGRESS) {
		/* Progress physical button request then get the wp again */
		poll_for_pp(td, VENDOR_CC_CCD, CCDV_PP_POLL_WP_DISABLE);
		/* Reset expected response size and get WP status again */
		response_size = sizeof(response);
		rv = send_vendor_command(td, VENDOR_CC_WP, NULL, 0, &response,
					 &response_size);
	}
	/*
	 * Give user a more detailed error for not allowed. That means CCD
	 * must be open before we can process the command
	 */
	if (rv == VENDOR_RC_NOT_ALLOWED) {
		fprintf(stderr, "Error: OverrideWP must be enabled first.\n"
				"Use `gsctool -I OverrideWP:always`\n");
		return update_error;
	}

	if (rv != VENDOR_RC_SUCCESS) {
		fprintf(stderr, "Error %d %sting write protect\n", rv,
			(wp == WP_ENABLE) ? "set" : "get");
		if (wp == WP_ENABLE) {
			fprintf(stderr,
				"Early Cr50 versions do not support setting WP"
				"\n");
		}
		return update_error;
	}
	if (response_size != sizeof(response)) {
		fprintf(stderr,
			"Unexpected response size %zd while getting "
			"write protect\n",
			response_size);
		return update_error;
	}

	printf("WP: %08x\n", response);
	printf("Flash WP: %s%s%s\n",
	       response & WPV_FWMP_FORCE_WP_EN ? "fwmp " : "",
	       response & WPV_FORCE ? "forced " : "",
	       response & WPV_ENABLE ? "enabled" : "disabled");
	printf(" at boot: %s\n",
	       response & WPV_FWMP_FORCE_WP_EN ? "fwmp enabled" :
	       !(response & WPV_ATBOOT_SET)    ? "follow_batt_pres" :
	       response & WPV_ATBOOT_ENABLE    ? "forced enabled" :
						 "forced disabled");
	return noop;
}

static enum exit_values process_get_chassis_open(struct transfer_descriptor *td)
{
	struct chassis_open_repsonse {
		uint8_t version;
		uint8_t chassis_open;
	} response;
	size_t response_size;
	int rv;

	response_size = sizeof(response);

	rv = send_vendor_command(td, VENDOR_CC_GET_CHASSIS_OPEN, NULL, 0,
				 &response, &response_size);

	if (rv != VENDOR_RC_SUCCESS) {
		fprintf(stderr, "Error %d getting chassis open\n", rv);
		return update_error;
	}
	if (response_size != sizeof(response)) {
		fprintf(stderr,
			"Unexpected response size %zd while getting "
			"chassis open\n",
			response_size);
		return update_error;
	}
	if (response.version != 1) {
		fprintf(stderr,
			"Unexpected response version %d while getting "
			"chassis open\n",
			response.version);
		return update_error;
	}

	printf("Chassis Open: %s\n",
	       response.chassis_open & 1 ? "true" : "false");
	return noop;
}

static enum exit_values process_get_dev_ids(struct transfer_descriptor *td,
					    bool show_machine_output)
{
	struct sys_info_repsonse {
		uint32_t ro_keyid;
		uint32_t rw_keyid;
		uint32_t dev_id0;
		uint32_t dev_id1;
	} response;
	size_t response_size;
	int rv;

	response_size = sizeof(response);

	rv = send_vendor_command(td, VENDOR_CC_SYSINFO, NULL, 0,
				 &response, &response_size);

	if (rv != VENDOR_RC_SUCCESS) {
		fprintf(stderr, "Error %d getting device ids\n", rv);
		return update_error;
	}
	if (response_size != sizeof(response)) {
		fprintf(stderr,
			"Unexpected response size %zd while getting "
			"device ids\n",
			response_size);
		return update_error;
	}

	/* Convert from BE transimision format */
	response.dev_id0 = be32toh(response.dev_id0);
	response.dev_id1 = be32toh(response.dev_id1);

	if (show_machine_output) {
		print_machine_output("DEV_ID0", "%08x", response.dev_id0);
		print_machine_output("DEV_ID1", "%08x", response.dev_id1);
	} else {
		printf("DEVID: 0x%08x 0x%08x\n", response.dev_id0,
		       response.dev_id1);
	}
	return noop;
}

static int process_get_apro_hash(struct transfer_descriptor *td)
{
	size_t response_size;
	uint8_t response[SHA256_DIGEST_SIZE];
	const char *const desc = "getting apro hash";
	int rv = 0;
	int i;

	response_size = sizeof(response);

	rv = send_vendor_command(td, VENDOR_CC_GET_AP_RO_HASH, NULL, 0,
				 &response, &response_size);

	if (response_size == 1) {
		printf("get hash rc: %d ", response[0]);
		switch (response[0]) {
		case ARCVE_NOT_PROGRAMMED:
			printf("AP RO hash unprogrammed\n");
			return 0;
		case ARCVE_FLASH_READ_FAILED:
			printf("flash read failed\n");
			return 0;
		case ARCVE_BOARD_ID_BLOCKED:
			printf("board id blocked\n");
			return 0;
		default:
			fprintf(stderr, "unexpected error\n");
			return update_error;
		}
	} else if (rv != VENDOR_RC_SUCCESS) {
		fprintf(stderr, "Error %d %s\n", rv, desc);
		return update_error;
	} else if (response_size != SHA256_DIGEST_SIZE) {
		fprintf(stderr, "Error in the size of response, %zu.\n",
			response_size);
		return update_error;
	}
	printf("digest: ");
	for (i = 0; i < SHA256_DIGEST_SIZE; i++)
		printf("%x", response[i]);
	printf("\n");
	return 0;
}

static int process_start_apro_verify(struct transfer_descriptor *td)
{
	int rv = 0;

	rv = send_vendor_command(td, VENDOR_CC_AP_RO_VALIDATE, NULL, 0, NULL,
				 NULL);
	if (rv != VENDOR_RC_SUCCESS) {
		fprintf(stderr, "Error %d starting RO verify\n", rv);
		return update_error;
	}
	return 0;
}

static int process_get_apro_boot_status(struct transfer_descriptor *td)
{
	size_t response_size;
	uint8_t response;
	const char *const desc = "getting apro status";
	int rv = 0;

	response_size = sizeof(response);

	rv = send_vendor_command(td, VENDOR_CC_GET_AP_RO_STATUS, NULL, 0,
				 &response, &response_size);
	if (rv != VENDOR_RC_SUCCESS) {
		fprintf(stderr, "Error %d %s\n", rv, desc);
		return update_error;
	}
	if (response_size != 1) {
		fprintf(stderr, "Unexpected response size %zd while %s\n",
			response_size, desc);
		return update_error;
	}

	/* Print the response and meaning, as in 'enum ap_ro_status'. */
	printf("apro result (%d) : ", response);
	switch (response) {
	case AP_RO_NOT_RUN:
		printf("not run\n");
		break;
	case AP_RO_PASS:
	case AP_RO_V2_SUCCESS:
		printf("pass\n");
		break;
	case AP_RO_PASS_UNVERIFIED_GBB:
		printf("pass - unverified gbb!\n");
		break;
	case AP_RO_V2_NON_ZERO_GBB_FLAGS:
		printf("pass - except non-zero gbb flags!\n");
		break;
	case AP_RO_FAIL:
	case AP_RO_V2_FAILED_VERIFICATION:
		printf("FAIL\n");
		break;
	case AP_RO_UNSUPPORTED_TRIGGERED:
		printf("not supported\ntriggered: yes\n");
		break;
	case AP_RO_UNSUPPORTED_UNKNOWN:
		printf("not supported\ntriggered: unknown\n");
		break;
	case AP_RO_UNSUPPORTED_NOT_TRIGGERED:
		printf("not supported\ntriggered: no\n");
		break;
	case AP_RO_IN_PROGRESS:
		printf("in progress.");
		break;
	case AP_RO_V2_INCONSISTENT_GSCVD:
		printf("inconsistent gscvd\n");
		break;
	case AP_RO_V2_INCONSISTENT_KEYBLOCK:
		printf("inconsistent keyblock\n");
		break;
	case AP_RO_V2_INCONSISTENT_KEY:
		printf("inconsistent key\n");
		break;
	case AP_RO_V2_SPI_READ:
		printf("spi read failure\n");
		break;
	case AP_RO_V2_UNSUPPORTED_CRYPTO_ALGORITHM:
		printf("unsupported crypto algo\n");
		break;
	case AP_RO_V2_VERSION_MISMATCH:
		printf("header version mismatch\n");
		break;
	case AP_RO_V2_OUT_OF_MEMORY:
		printf("out of memory\n");
		break;
	case AP_RO_V2_INTERNAL:
		printf("internal\n");
		break;
	case AP_RO_V2_TOO_BIG:
		printf("too many areas\n");
		break;
	case AP_RO_V2_MISSING_GSCVD:
		printf("missing gscvd\n");
		break;
	case AP_RO_V2_BOARD_ID_MISMATCH:
		printf("board id mismatch\n");
		break;
	case AP_RO_V2_SETTING_NOT_PROVISIONED:
		printf("setting not provisioned\n");
		break;
	case AP_RO_V2_WRONG_ROOT_KEY:
		printf("key is recognized but disallowed (e.g. preMP key)\n");
		break;
	default:
		printf("unknown\n");
		fprintf(stderr, "unknown status\n");
		return update_error;
	}

	return 0;
}

static int process_arv_config_spi_addr_mode(struct transfer_descriptor *td,
					    int arv_config_spi_addr_mode)
{
	enum ap_ro_config_spi_mode_e {
		ap_ro_spi_config_3byte = 0,
		ap_ro_spi_config_4byte = 1,
	};

	struct __attribute__((__packed__)) ap_ro_config_spi_mode_msg {
		uint8_t version;
		uint8_t command;
		uint8_t state;
		uint8_t mode;
	};

	struct ap_ro_config_spi_mode_msg msg = {
		.version = ARV_CONFIG_SETTING_CURRENT_VERSION,
		.command = arv_config_setting_command_spi_addressing_mode,
		.state = arv_config_setting_state_present,
		.mode = ap_ro_spi_config_4byte
	};
	size_t response_size = sizeof(msg);
	int rv = 0;

	switch (arv_config_spi_addr_mode) {
	case arv_config_spi_addr_mode_get:
		rv = send_vendor_command(td, VENDOR_CC_GET_AP_RO_VERIFY_SETTING,
					 &msg, sizeof(msg), &msg,
					 &response_size);
		if (rv != VENDOR_RC_SUCCESS) {
			fprintf(stderr,
				"Error %d getting ap ro spi addr mode\n", rv);
			return update_error;
		}

		if (response_size != sizeof(msg)) {
			fprintf(stderr,
				"Error getting ap ro spi addr mode response\n");
			return update_error;
		}

		if (msg.state != arv_config_setting_state_present) {
			switch (msg.state) {
			case arv_config_setting_state_not_present:
				fprintf(stderr, "not provisioned\n");
				break;
			case arv_config_setting_state_corrupted:
				fprintf(stderr, "corrupted\n");
				break;
			case arv_config_setting_state_invalid:
				fprintf(stderr, "invalid\n");
				break;
			default:
				fprintf(stderr,
					"unexpected message response state\n");
				return update_error;
			}
			return 0;
		}

		switch (msg.mode) {
		case ap_ro_spi_config_3byte:
			printf("3byte\n");
			break;
		case ap_ro_spi_config_4byte:
			printf("4byte\n");
			break;
		default:
			fprintf(stderr, "unknown spi mode\n");
			return update_error;
		}

		break;
	case arv_config_spi_addr_mode_set_3byte:
		msg.mode = ap_ro_spi_config_3byte;
		/* Fallthrough */
	case arv_config_spi_addr_mode_set_4byte:
		/* The default is 4byte addressing */
		rv = send_vendor_command(td, VENDOR_CC_SET_AP_RO_VERIFY_SETTING,
					 &msg, sizeof(msg), &msg,
					 &response_size);
		if (rv != VENDOR_RC_SUCCESS) {
			fprintf(stderr,
				"Error %d setting ap ro spi addr mode\n", rv);
			return update_error;
		}
		break;
	default:
		return update_error;
	}

	return 0;
}

/*
 * Reads an ascii hex byte in the following forms:
 *  - 0x01
 *  - 0x1
 *  - 01
 *  - 0
 *
 * 1 is returned on success, 0 on failure.
 */
static int read_hex_byte_string(char *s, uint8_t *byte)
{
	uint8_t b = 0;

	if (!strncmp(s, "0x", 2))
		s += 2;

	if (strlen(s) == 0)
		return 0;

	for (const char *end = s + 2; s < end; ++s) {
		if (*s >= '0' && *s <= '9')
			b = b * 16 + *s - '0';
		else if (*s >= 'A' && *s <= 'F')
			b = b * 16 + 10 + *s - 'A';
		else if (*s >= 'a' && *s <= 'f')
			b = b * 16 + 10 + *s - 'a';
		else if (*s == '\0')
			/* Single nibble, do nothing instead of `break`
			 * to keep checkpatch happy
			 */
			;
		else
			/* Invalid nibble */
			return 0;
	}
	*byte = b;
	return 1;
}

static int parse_wpsrs(const char *opt, struct arv_config_wpds *wpds)
{
	size_t len = strlen(opt);
	char *ptr, *p, *delim = " ";
	uint8_t b;
	int rv = 0;
	struct arv_config_wpd *wpd;

	ptr = malloc(len + 1);
	strcpy(ptr, opt);
	p = strtok(ptr, delim);

	while (p != NULL) {
		if (read_hex_byte_string(p, &b)) {
			wpd = &wpds->data[rv / 2];
			if (rv % 2 == 0) {
				wpd->expected_value = b;
			} else {
				wpd->mask = b;
				wpd->state = arv_config_setting_state_present;
			}
			rv++;
		} else {
			break;
		}
		p = strtok(NULL, delim);
	}
	free(ptr);

	return rv;
}

static void print_wpd(size_t i, struct arv_config_wpd *wpd)
{
	if (wpd->state == arv_config_setting_state_not_present) {
		printf("not provisioned");
		return;
	} else if (wpd->state == arv_config_setting_state_corrupted) {
		printf("corrupted");
		return;
	} else if (wpd->state == arv_config_setting_state_invalid) {
		printf("invalid");
		return;
	}

	printf("%zu: %02x & %02x", i, wpd->expected_value, wpd->mask);
}

static int process_arv_config_wpds(struct transfer_descriptor *td,
				   enum arv_config_wpsr_choice_e choice,
				   struct arv_config_wpds *wpds)
{
	struct __attribute__((__packed__)) arv_config_wpds_message {
		uint8_t version;
		/* See `arv_config_setting_command_e` */
		uint8_t command;
		struct arv_config_wpds wpds;
	};
	int rv = 0;

	struct arv_config_wpds_message msg = {
		.version = ARV_CONFIG_SETTING_CURRENT_VERSION,
		.command = arv_config_setting_command_write_protect_descriptors,
	};
	size_t response_size = sizeof(msg);

	if (choice == arv_config_wpsr_choice_get) {
		rv = send_vendor_command(td, VENDOR_CC_GET_AP_RO_VERIFY_SETTING,
					 &msg, sizeof(msg), &msg,
					 &response_size);
		if (rv != VENDOR_RC_SUCCESS) {
			fprintf(stderr,
				"Error %d getting ap ro write protect descriptors\n",
				rv);
			return update_error;
		}

		if (response_size != sizeof(msg)) {
			fprintf(stderr,
				"Error getting ap ro write protect descriptors response\n");
			return update_error;
		}

		if (msg.wpds.data[0].state ==
		    arv_config_setting_state_present) {
			printf("expected values: ");
		}
		print_wpd(1, &msg.wpds.data[0]);
		if (msg.wpds.data[1].state !=
		    arv_config_setting_state_not_present) {
			printf(", ");
			print_wpd(2, &msg.wpds.data[1]);
		}
		if (msg.wpds.data[2].state !=
		    arv_config_setting_state_not_present) {
			printf(", ");
			print_wpd(3, &msg.wpds.data[2]);
		}

		printf("\n");
	} else if (choice == arv_config_wpsr_choice_set) {
		msg.wpds = *wpds;

		rv = send_vendor_command(td, VENDOR_CC_SET_AP_RO_VERIFY_SETTING,
					 &msg, sizeof(msg), &msg,
					 &response_size);
		if (rv != VENDOR_RC_SUCCESS) {
			fprintf(stderr,
				"Error %d setting ap ro write protect descriptors\n",
				rv);
			return update_error;
		}
	}

	return 0;
}

static int process_get_boot_mode(struct transfer_descriptor *td)
{
	size_t response_size;
	uint8_t response;
	const char *const desc = "Getting boot mode";
	int rv = 0;

	response_size = sizeof(response);

	rv = send_vendor_command(td, VENDOR_CC_GET_BOOT_MODE, NULL, 0,
				 &response, &response_size);
	if (rv != VENDOR_RC_SUCCESS) {
		fprintf(stderr, "Error %d in %s\n", rv, desc);
		return update_error;
	}
	if (response_size != 1) {
		fprintf(stderr, "Unexpected response size %zd while %s\n",
			response_size, desc);
		return update_error;
	}

	/* Print the response and meaning, as in 'enum boot_mode'. */
	printf("Boot mode = 0x%02x: ", response);
	switch (response) {
	case 0x00:
		printf("NORMAL\n");
		break;
	case 0x01:
		printf("NO_BOOT\n");
		break;
	default:
		fprintf(stderr, "unknown boot mode\n");
		return update_error;
	}

	return 0;
}

void process_bid(struct transfer_descriptor *td,
		 enum board_id_action bid_action, struct board_id *bid,
		 bool show_machine_output)
{
	size_t response_size;

	if (bid_action == bid_get) {
		response_size = sizeof(*bid);
		send_vendor_command(td, VENDOR_CC_GET_BOARD_ID, bid,
				    sizeof(*bid), bid, &response_size);

		if (response_size != sizeof(*bid)) {
			fprintf(stderr,
				"Error reading board ID: response size %zd, "
				"first byte %#02x\n",
				response_size,
				response_size ? *(uint8_t *)&bid : -1);
			exit(update_error);
		}

		if (show_machine_output) {
			print_machine_output("BID_TYPE", "%08x",
					     be32toh(bid->type));
			print_machine_output("BID_TYPE_INV", "%08x",
					     be32toh(bid->type_inv));
			print_machine_output("BID_FLAGS", "%08x",
					     be32toh(bid->flags));

			for (int i = 0; i < 4; i++) {
				if (!isupper(((const char *)bid)[i])) {
					print_machine_output("BID_RLZ", "%s",
							     "????");
					return;
				}
			}

			print_machine_output("BID_RLZ", "%c%c%c%c",
					     ((const char *)bid)[0],
					     ((const char *)bid)[1],
					     ((const char *)bid)[2],
					     ((const char *)bid)[3]);
		} else {
			printf("Board ID space: %08x:%08x:%08x\n",
			       be32toh(bid->type), be32toh(bid->type_inv),
			       be32toh(bid->flags));
		}

		return;
	}

	if (bid_action == bid_set) {
		/* Sending just two fields: type and flags. */
		uint32_t command_body[2];
		uint8_t response;

		command_body[0] = htobe32(bid->type);
		command_body[1] = htobe32(bid->flags);

		response_size = sizeof(command_body);
		send_vendor_command(td, VENDOR_CC_SET_BOARD_ID, command_body,
				    sizeof(command_body), command_body,
				    &response_size);

		/*
		 * Speculative assignment: the response is expected to be one
		 * byte in size and be placed in the first byte of the buffer.
		 */
		response = *((uint8_t *)command_body);

		if (response_size == 1) {
			if (!response)
				return; /* Success! */

			fprintf(stderr, "Error %d while setting board id\n",
				response);
		} else {
			fprintf(stderr,
				"Unexpected response size %zd"
				" while setting board id\n",
				response_size);
		}
		exit(update_error);
	}
}

static void process_sn_bits(struct transfer_descriptor *td, uint8_t *sn_bits)
{
	int rv;
	uint8_t response_code;
	size_t response_size = sizeof(response_code);

	rv = send_vendor_command(td, VENDOR_CC_SN_SET_HASH, sn_bits,
				 SN_BITS_SIZE, &response_code, &response_size);

	if (rv) {
		fprintf(stderr, "Error %d while sending vendor command\n", rv);
		exit(update_error);
	}

	if (response_size != 1) {
		fprintf(stderr,
			"Unexpected response size while setting sn bits\n");
		exit(update_error);
	}

	if (response_code != 0) {
		fprintf(stderr, "Error %d while setting sn bits\n",
			response_code);
		exit(update_error);
	}
}

static void process_sn_inc_rma(struct transfer_descriptor *td, uint8_t arg)
{
	int rv;
	uint8_t response_code;
	size_t response_size = sizeof(response_code);

	rv = send_vendor_command(td, VENDOR_CC_SN_INC_RMA, &arg, sizeof(arg),
				 &response_code, &response_size);
	if (rv) {
		fprintf(stderr, "Error %d while sending vendor command\n", rv);
		exit(update_error);
	}

	if (response_size != 1) {
		fprintf(stderr, "Unexpected response size while "
				"incrementing sn rma count\n");
		exit(update_error);
	}

	if (response_code != 0) {
		fprintf(stderr, "Error %d while incrementing rma count\n",
			response_code);
		exit(update_error);
	}
}

/* Get/Set the primary seed of the info1 manufacture state. */
static int process_endorsement_seed(struct transfer_descriptor *td,
				    const char *endorsement_seed_str)
{
	uint8_t endorsement_seed[32];
	uint8_t response_seed[32];
	size_t seed_size = sizeof(endorsement_seed);
	size_t response_size = sizeof(response_seed);
	size_t i;
	int rv;

	if (!endorsement_seed_str) {
		rv = send_vendor_command(td, VENDOR_CC_ENDORSEMENT_SEED, NULL,
					 0, response_seed, &response_size);
		if (rv) {
			fprintf(stderr, "Error sending vendor command %d\n",
				rv);
			return update_error;
		}
		printf("Endorsement key seed: ");
		for (i = 0; i < response_size; i++)
			printf("%02x", response_seed[i]);
		printf("\n");
		return 0;
	}
	if (seed_size * 2 != strlen(endorsement_seed_str)) {
		printf("Invalid seed %s\n", endorsement_seed_str);
		return update_error;
	}

	for (i = 0; i < seed_size; i++) {
		int nibble;
		char c;

		c = endorsement_seed_str[2 * i];
		nibble = from_hexascii(c);
		if (nibble < 0) {
			fprintf(stderr, "Error: Non hex character in seed %c\n",
				c);
			return update_error;
		}
		endorsement_seed[i] = nibble << 4;

		c = endorsement_seed_str[2 * i + 1];
		nibble = from_hexascii(c);
		if (nibble < 0) {
			fprintf(stderr, "Error: Non hex character in seed %c\n",
				c);
			return update_error;
		}
		endorsement_seed[i] |= nibble;
	}

	printf("Setting seed: %s\n", endorsement_seed_str);
	rv = send_vendor_command(td, VENDOR_CC_ENDORSEMENT_SEED,
				 endorsement_seed, seed_size, response_seed,
				 &response_size);
	if (rv == VENDOR_RC_NOT_ALLOWED) {
		fprintf(stderr, "Seed already set\n");
		return update_error;
	}
	if (rv) {
		fprintf(stderr, "Error sending vendor command %d\n", rv);
		return update_error;
	}
	printf("Updated endorsement key seed.\n");
	return 0;
}

/*
 * Retrieve the RMA authentication challenge from the Cr50, print out the
 * challenge on the console, then prompt the user for the authentication code,
 * and send the code back to Cr50. The Cr50 would report if the code matched
 * its expectations or not. Output in a machine-friendly format if
 * show_machine_output is true.
 */
static void process_rma(struct transfer_descriptor *td, const char *authcode,
			bool show_machine_output)
{
	char rma_response[81];
	size_t response_size = sizeof(rma_response);
	size_t i;
	size_t auth_size = 0;

	if (!authcode) {
		send_vendor_command(td, VENDOR_CC_RMA_CHALLENGE_RESPONSE, NULL,
				    0, rma_response, &response_size);

		if (response_size == 1) {
			fprintf(stderr, "error %d\n", rma_response[0]);
			if (td->ep_type == usb_xfer)
				shut_down(&td->uep);
			exit(update_error);
		}

		if (show_machine_output) {
			rma_response[response_size] = '\0';
			print_machine_output("CHALLENGE", "%s", rma_response);
		} else {
			printf("Challenge:");
			for (i = 0; i < response_size; i++) {
				if (!(i % 5)) {
					if (!(i % 40))
						printf("\n");
					printf(" ");
				}
				printf("%c", rma_response[i]);
			}
			printf("\n");
		}
		return;
	}

	if (!*authcode) {
		printf("Empty response.\n");
		exit(update_error);
		return;
	}

	if (!strcmp(authcode, "disable")) {
		printf("Invalid arg. Try using 'gsctool -F disable'\n");
		exit(update_error);
		return;
	}

	printf("Processing response...\n");
	auth_size = strlen(authcode);
	response_size = sizeof(rma_response);

	send_vendor_command(td, VENDOR_CC_RMA_CHALLENGE_RESPONSE, authcode,
			    auth_size, rma_response, &response_size);

	if (response_size == 1) {
		fprintf(stderr, "\nrma unlock failed, code %d ", *rma_response);
		switch (*rma_response) {
		case VENDOR_RC_BOGUS_ARGS:
			fprintf(stderr, "(wrong authcode size)\n");
			break;
		case VENDOR_RC_INTERNAL_ERROR:
			fprintf(stderr, "(authcode mismatch)\n");
			break;
		default:
			fprintf(stderr, "(unknown error)\n");
			break;
		}
		if (td->ep_type == usb_xfer)
			shut_down(&td->uep);
		exit(update_error);
	}
	printf("RMA unlock succeeded.\n");
}

/*
 * Enable or disable factory mode. Factory mode will only be enabled if HW
 * write protect is removed.
 */
static void process_factory_mode(struct transfer_descriptor *td,
				 const char *arg)
{
	uint8_t rma_response;
	size_t response_size = sizeof(rma_response);
	char *cmd_str;
	int rv;
	uint16_t subcommand;

	if (!strcasecmp(arg, "disable")) {
		subcommand = VENDOR_CC_DISABLE_FACTORY;
		cmd_str = "dis";
	} else if (!strcasecmp(arg, "enable")) {
		subcommand = VENDOR_CC_RESET_FACTORY;
		cmd_str = "en";

	} else {
		fprintf(stderr, "Invalid factory mode arg %s", arg);
		exit(update_error);
	}

	printf("%sabling factory mode\n", cmd_str);
	rv = send_vendor_command(td, subcommand, NULL, 0, &rma_response,
				 &response_size);
	if (rv) {
		fprintf(stderr,
			"Failed %sabling factory mode\nvc error "
			"%d\n",
			cmd_str, rv);
		if (response_size == 1)
			fprintf(stderr, "ec error %d\n", rma_response);
		exit(update_error);
	}
	printf("Factory %sable succeeded.\n", cmd_str);
}

static void report_version(void)
{
	/* Get version from the generated file, ignore the underscore prefix. */
	const char *v = strchr(VERSION, '_');

	printf("Version: %s, built on %s by %s\n", v ? v + 1 : "?", DATE,
	       BUILDER);
	exit(0);
}

/*
 * Either change or query TPM mode value.
 */
static int process_tpm_mode(struct transfer_descriptor *td, const char *arg)
{
	int rv;
	size_t command_size;
	size_t response_size;
	uint8_t response;
	uint8_t command_body;

	response_size = sizeof(response);
	if (!arg) {
		command_size = 0;
	} else if (!strcasecmp(arg, "disable")) {
		command_size = sizeof(command_body);
		command_body = (uint8_t)TPM_MODE_DISABLED;
	} else if (!strcasecmp(arg, "enable")) {
		command_size = sizeof(command_body);
		command_body = (uint8_t)TPM_MODE_ENABLED;
	} else {
		fprintf(stderr, "Invalid tpm mode arg: %s.\n", arg);
		return update_error;
	}

	rv = send_vendor_command(td, VENDOR_CC_TPM_MODE, &command_body,
				 command_size, &response, &response_size);
	if (rv) {
		fprintf(stderr, "Error %d in setting TPM mode.\n", rv);
		return update_error;
	}
	if (response_size != sizeof(response)) {
		fprintf(stderr,
			"Error in the size of response,"
			" %zu.\n",
			response_size);
		return update_error;
	}
	if (response >= TPM_MODE_MAX) {
		fprintf(stderr,
			"Error in the value of response,"
			" %d.\n",
			response);
		return update_error;
	}

	printf("TPM Mode: %s (%d)\n",
	       (response == TPM_MODE_DISABLED) ? "disabled" : "enabled",
	       response);

	return rv;
}

#define MAX_PAYLOAD_SIZE 256
struct parsed_flog_entry {
	bool end_of_list;
	char payload[MAX_PAYLOAD_SIZE];
	size_t payload_size;
	uint64_t raw_timestamp;
	time_t timestamp;
	uint32_t event_type;
	bool timestamp_reliable;
};

static int pop_flog_dt(struct transfer_descriptor *td,
		       struct parsed_flog_entry *parsed_entry)
{
	union dt_entry_u entry;
	size_t resp_size = sizeof(entry);
	int rv = send_vendor_command(td, VENDOR_CC_POP_LOG_ENTRY_MS,
				     &parsed_entry->raw_timestamp,
				     sizeof(parsed_entry->raw_timestamp),
				     &entry, &resp_size);
	if (rv)
		return rv;
	if (resp_size == 0) {
		parsed_entry->end_of_list = true;
		return 0;
	}
	parsed_entry->event_type = entry.evt.event_type;
	parsed_entry->payload_size =
		MIN(entry.evt.size - sizeof(entry.evt.event_type),
		    MAX_PAYLOAD_SIZE);
	memcpy(parsed_entry->payload, entry.evt.payload,
	       parsed_entry->payload_size);
	parsed_entry->raw_timestamp = entry.evt.time;
	parsed_entry->timestamp =
		(parsed_entry->raw_timestamp & ~(1ULL << 63)) / 1000;
	parsed_entry->timestamp_reliable =
		(parsed_entry->raw_timestamp >> 63) == 0;
	return rv;
}

static int pop_flog(struct transfer_descriptor *td,
		    struct parsed_flog_entry *parsed_entry)
{
	union entry_u entry;
	size_t resp_size = sizeof(entry);
	uint32_t ts = (uint32_t)parsed_entry->raw_timestamp;
	int rv = send_vendor_command(td, VENDOR_CC_POP_LOG_ENTRY, &ts,
				     sizeof(ts), &entry, &resp_size);
	if (rv)
		return rv;
	if (resp_size == 0) {
		parsed_entry->end_of_list = true;
		return 0;
	}
	parsed_entry->event_type = entry.r.type;
	parsed_entry->payload_size =
		MIN(FLASH_LOG_PAYLOAD_SIZE(entry.r.size), MAX_PAYLOAD_SIZE);
	memcpy(parsed_entry->payload, entry.r.payload,
	       parsed_entry->payload_size);
	parsed_entry->raw_timestamp = entry.r.timestamp;
	parsed_entry->timestamp = entry.r.timestamp;
	parsed_entry->timestamp_reliable = true;
	return rv;
}

/*
 * Retrieve from H1 flash log entries which are newer than the passed in
 * timestamp.
 *
 * On error retry a few times just in case flash log is locked by a concurrent
 * access.
 */
static int process_get_flog(struct transfer_descriptor *td, uint64_t prev_stamp,
			    bool show_machine_output)
{
	int rv;
	const int max_retries = 3;
	int retries = max_retries;
	bool time_zone_reported = false;
	bool is_dauntless;

	/*
	 * For backwards compatibility assume DT only if was explicitly
	 * requested.
	 */
	is_dauntless = (gsc_dev == GSC_DEVICE_DT);

	while (retries--) {
		struct parsed_flog_entry entry = { 0 };
		entry.raw_timestamp = prev_stamp;
		size_t i;
		struct tm loc_time;
		char date_str[25];
		if (is_dauntless) {
			rv = pop_flog_dt(td, &entry);
		} else {
			rv = pop_flog(td, &entry);
		}

		if (rv) {
			/*
			 * Flash log could be momentarily locked by a
			 * concurrent access, let it settle and try again, 10
			 * ms should be enough.
			 */
			usleep(10 * 1000);
			continue;
		}

		if (entry.end_of_list)
			return 0;

		prev_stamp = entry.raw_timestamp;
		if (show_machine_output) {
			printf("%10" PRIu64 ":%02x", prev_stamp,
			       entry.event_type);
		} else {
			localtime_r(&entry.timestamp, &loc_time);

			if (!time_zone_reported) {
				strftime(date_str, sizeof(date_str), "%Z",
					 &loc_time);
				printf("Log time zone is %s\n", date_str);
				time_zone_reported = true;
			}

			/* Date format is MMM DD YY HH:mm:ss */
			strftime(date_str, sizeof(date_str), "%b %d %y %T",
				 &loc_time);
			printf("%s : %02x", date_str, entry.event_type);
		}
		for (i = 0; i < entry.payload_size; i++)
			printf(" %02x", entry.payload[i]);
		if (entry.timestamp_reliable == false)
			printf(" -- TIMESTAMP UNRELIABLE!");
		printf("\n");
		retries = max_retries;
	}

	fprintf(stderr, "%s: error %d\n", __func__, rv);

	return rv;
}

static int process_tstamp(struct transfer_descriptor *td,
			  const char *tstamp_ascii)
{
	char *e;
	size_t expected_response_size;
	size_t message_size;
	size_t response_size;
	uint32_t rv;
	uint32_t tstamp = 0;
	uint8_t max_response[sizeof(uint32_t)];

	if (tstamp_ascii) {
		tstamp = strtoul(tstamp_ascii, &e, 10);
		if (*e) {
			fprintf(stderr, "invalid base timestamp value \"%s\"\n",
				tstamp_ascii);
			return -1;
		}
		tstamp = htobe32(tstamp);
		expected_response_size = 0;
		message_size = sizeof(tstamp);
	} else {
		expected_response_size = 4;
		message_size = 0;
	}

	response_size = sizeof(max_response);
	rv = send_vendor_command(td, VENDOR_CC_FLOG_TIMESTAMP, &tstamp,
				 message_size, max_response, &response_size);

	if (rv) {
		fprintf(stderr, "error: return value %d\n", rv);
		return rv;
	}
	if (response_size != expected_response_size) {
		fprintf(stderr, "error: got %zd bytes, expected %zd\n",
			response_size, expected_response_size);
		return -1; /* Should never happen. */
	}

	if (response_size) {
		memcpy(&tstamp, max_response, sizeof(tstamp));
		printf("Current H1 time is %d\n", be32toh(tstamp));
	}
	return 0;
}

static int process_reboot_gsc(struct transfer_descriptor *td, size_t timeout_ms)
{
	/* Reboot timeout in milliseconds.
	 * Maximum value is 1000ms on Ti50.
	 */
	uint16_t msg = htobe16((uint16_t)timeout_ms);
	int rv = 0;

	rv = send_vendor_command(td, VENDOR_CC_IMMEDIATE_RESET, &msg,
				 sizeof(msg), NULL, 0);
	if (rv != VENDOR_RC_SUCCESS) {
		fprintf(stderr, "Error %d sending immediate reset command\n",
			rv);
		return update_error;
	}

	return 0;
}

/*
 * Search the passed in zero terminated array of options_map structures for
 * option 'option'.
 *
 * If found - set the corresponding integer to 1 and return 1. If not found -
 * return 0.
 */
static int check_boolean(const struct options_map *omap, char option)
{
	do {
		if (omap->opt != option)
			continue;

		*omap->flag = 1;
		return 1;
	} while ((++omap)->opt);

	return 0;
}

/*
 * Set the long_opts table and short_opts string.
 *
 * This function allows to avoid maintaining two command line option
 * descriptions, for short and long forms.
 *
 * The long_opts table is built based on the cmd_line_options table contents,
 * and the short form is built based on the long_opts table contents.
 *
 * The 'required_argument' short options are followed by ':'.
 *
 * The passed in long_opts array and short_opts string are guaranteed to
 * accommodate all necessary objects/characters.
 */
static void set_opt_descriptors(struct option *long_opts, char *short_opts)
{
	size_t i;
	int j;

	for (i = j = 0; i < ARRAY_SIZE(cmd_line_options); i++) {
		long_opts[i] = cmd_line_options[i].opt;
		short_opts[j++] = long_opts[i].val;
		if (long_opts[i].has_arg == required_argument)
			short_opts[j++] = ':';
	}
}

/*
 * Find the long_opts table index where .val field is set to the passed in
 * short option value.
 */
static int get_longindex(int short_opt, const struct option *long_opts)
{
	int i;

	for (i = 0; long_opts[i].name; i++)
		if (long_opts[i].val == short_opt)
			return i;

	/*
	 * We could never come here as the short options list is compiled
	 * based on long options table.
	 */
	fprintf(stderr, "Command line error, parameter argument missing\n");
	exit(1);

	return -1; /* Not reached. */
}

/*
 * Combine searching for command line parameters and optional arguments.
 *
 * The canonical short options description string does not allow to specify
 * that a command line argument expects an optional parameter. but gsctool
 * users expect to be able to use the following styles for optional
 * parameters:
 *
 * a)   -x <param value>
 * b)  --x_long <param_value>
 * c)  --x_long=<param_value>
 *
 * Styles a) and b) are not supported standard getopt_long(), this function
 * adds ability to handle cases a) and b).
 */
static int getopt_all(int argc, char *argv[])
{
	int longindex = -1;
	static char short_opts[2 * ARRAY_SIZE(cmd_line_options)] = {};
	static struct option long_opts[ARRAY_SIZE(cmd_line_options) + 1] = {};
	int i;

	if (!short_opts[0])
		set_opt_descriptors(long_opts, short_opts);

	i = getopt_long(argc, argv, short_opts, long_opts, &longindex);
	if (i != -1) {
		if (longindex < 0) {
			/*
			 * longindex is not set, this must have been the short
			 * option case, Find the long_opts table index based
			 * on the short option value.
			 */
			longindex = get_longindex(i, long_opts);
		}

		opt_gsc_dev = cmd_line_options[longindex].opt_device;

		if (long_opts[longindex].has_arg == optional_argument) {
			/*
			 * This command line option may include an argument,
			 * let's check if it is there as the next token in the
			 * command line.
			 */
			if (!optarg && argv[optind] && argv[optind][0] != '-')
				/* Yes, it is. */
				optarg = argv[optind++];
		}
	}

	return i;
}

static int get_crashlog(struct transfer_descriptor *td)
{
	uint32_t rv;
	uint8_t response[2048] = { 0 };
	size_t response_size = sizeof(response);

	rv = send_vendor_command(td, VENDOR_CC_GET_CRASHLOG, NULL, 0, response,
				 &response_size);
	if (rv != VENDOR_RC_SUCCESS) {
		printf("Get crash log failed. (%X)\n", rv);
		return 1;
	}

	for (size_t i = 0; i < response_size; i++) {
		if (i % 64 == 0 && i > 0)
			printf("\n");
		printf("%02x", response[i]);
	}
	printf("\n");
	return 0;
}

static int get_console_logs(struct transfer_descriptor *td)
{
	uint32_t rv;
	uint8_t response[2048] = { 0 };
	size_t response_size = sizeof(response);

	rv = send_vendor_command(td, VENDOR_CC_GET_CONSOLE_LOGS, NULL, 0,
				 response, &response_size);
	if (rv != VENDOR_RC_SUCCESS) {
		printf("Get console logs failed. (%X)\n", rv);
		return 1;
	}

	printf("%s", response);
	printf("\n");
	return 0;
}

static int process_get_factory_config(struct transfer_descriptor *td)
{
	uint32_t rv;
	uint64_t response = 0;
	size_t response_size = sizeof(response);

	rv = send_vendor_command(td, VENDOR_CC_GET_FACTORY_CONFIG, NULL, 0,
				 (uint8_t *)&response, &response_size);
	if (rv != VENDOR_RC_SUCCESS) {
		printf("Get factory config failed. (%X)\n", rv);
		return 1;
	}

	if (response_size < sizeof(uint64_t)) {
		printf("Unexpected response size. (%zu)", response_size);
		return 2;
	}

	uint64_t out = be64toh(response);
	bool is_x_branded = (out >> 4) & 1;
	uint8_t compliance_version = out & 0xF;

	printf("raw value: %016" PRIX64 "\n", out);
	printf("chassis_x_branded: %s\n", is_x_branded ? "true" : "false");
	printf("hw_x_compliance_version: %02X\n", compliance_version);
	return 0;
}

static int process_set_factory_config(struct transfer_descriptor *td,
				      uint64_t val)
{
	uint64_t val_be = htobe64(val);
	uint32_t rv;

	rv = send_vendor_command(td, VENDOR_CC_SET_FACTORY_CONFIG, &val_be,
				 sizeof(val_be), NULL, NULL);
	if (rv != VENDOR_RC_SUCCESS) {
		printf("Factory config failed. (%X)\n", rv);
		return 1;
	}

	return 0;
}

static int process_get_time(struct transfer_descriptor *td)
{
	uint32_t rv;
	uint64_t response = 0;
	size_t response_size = sizeof(response);

	rv = send_vendor_command(td, VENDOR_CC_GET_TIME, NULL, 0,
				 (uint8_t *)&response, &response_size);
	if (rv != VENDOR_RC_SUCCESS) {
		printf("Get time failed. (%X)\n", rv);
		return 1;
	}

	if (response_size < sizeof(uint64_t)) {
		printf("Unexpected response size. (%zu)", response_size);
		return 2;
	}

	uint64_t out = be64toh(response);

	printf("%" PRIu64 "\n", out);
	return 0;
}

static int process_get_metrics(struct transfer_descriptor *td,
			       bool show_machine_output)
{
	uint32_t rv;
	/* Allocate extra space in case future versions add more data. */
	struct ti50_stats response[4] = { 0 };
	size_t response_size = sizeof(response);

	rv = send_vendor_command(td, VENDOR_CC_GET_TI50_STATS, NULL, 0,
				 (uint8_t *)&response, &response_size);
	if (rv != VENDOR_RC_SUCCESS) {
		printf("Get stats failed. (%X)\n", rv);
		return 1;
	}

	if (response_size < sizeof(struct ti50_stats)) {
		printf("Unexpected response size. (%zu)\n", response_size);
		return 2;
	}

	if (show_machine_output) {
		uint8_t *raw_response = (uint8_t *)response;

		for (size_t i = 0; i < response_size; i++)
			printf("%02X", raw_response[i]);
	} else {
		struct ti50_stats stats = *response;

		stats.fs_init_time = be32toh(stats.fs_init_time);
		stats.fs_usage = be32toh(stats.fs_usage);
		stats.aprov_time = be32toh(stats.aprov_time);
		stats.expanded_aprov_status =
			be32toh(stats.expanded_aprov_status);
		stats.misc_status = be32toh(stats.misc_status);
		uint32_t bits_used = stats.misc_status >>
				     METRICSV_BITS_USED_SHIFT;

		printf("fs_init_time:          %d\n", stats.fs_init_time);
		printf("fs_usage:              %d\n", stats.fs_usage);
		printf("aprov_time:            %d\n", stats.aprov_time);
		printf("expanded_aprov_status: %X\n",
		       stats.expanded_aprov_status);

		if (bits_used >= 4) {
			printf("rdd_keepalive:         %d\n",
			       stats.misc_status &
				       METRICSV_RDD_KEEP_ALIVE_MASK);
			printf("rdd_keepalive_at_boot: %d\n",
			       (stats.misc_status &
				METRICSV_RDD_KEEP_ALIVE_AT_BOOT_MASK) >>
				       METRICSV_RDD_KEEP_ALIVE_AT_BOOT_SHIFT);
			printf("ccd_mode:              %d\n",
			       (stats.misc_status & METRICSV_CCD_MODE_MASK) >>
				       METRICSV_CCD_MODE_SHIFT);
		}
	}
	return 0;
}

/*
 * The below variables and array must be held in sync with the appropriate
 * counterparts in defined in ti50:common/{hil,capsules}/src/boot_tracer.rs.
 */
#define MAX_BOOT_TRACE_SIZE 54
#define TIMESPAN_EVENT	    0
#define TIME_SHIFT	    11
#define MAX_TIME_MS	    (1 << TIME_SHIFT)
static const char *const boot_tracer_stages[] = {
	"Timespan", /* This one will not be displayed separately. */
	"ProjectStart",	   "EcRstAsserted", "EcRstDeasserted", "TpmRstAsserted",
	"TmRstDeasserted", "FirstApComms",  "PcrExtension",    "TpmAppReady"
};

static int process_get_boot_trace(struct transfer_descriptor *td, bool erase,
				  bool show_machine_output)
{
	/* zero means no erase, 1 means erase. */
	uint32_t payload = htobe32(erase);
	uint16_t boot_trace[MAX_BOOT_TRACE_SIZE / sizeof(uint16_t)];
	size_t response_size = sizeof(boot_trace);
	uint32_t rv;
	uint64_t timespan = 0;
	size_t i;

	rv = send_vendor_command(td, VENDOR_CC_GET_BOOT_TRACE, &payload,
				 sizeof(payload), &boot_trace, &response_size);

	if (rv != VENDOR_RC_SUCCESS) {
		printf("Get boot trace failed. (%X)\n", rv);
		return 1;
	}

	if (response_size == 0)
		return 0; /* Trace is empty. */

	if (!show_machine_output)
		printf("    got %zd bytes back:\n", response_size);
	if (response_size > 0) {
		for (i = 0; i < response_size / sizeof(uint16_t); i++) {
			uint16_t entry = boot_trace[i];
			uint16_t event_id = entry >> TIME_SHIFT;
			uint16_t delta_time = entry & ((1 << TIME_SHIFT) - 1);

			if (show_machine_output) {
				printf(" %04x", entry);
				continue;
			}

			if (event_id >= ARRAY_SIZE(boot_tracer_stages)) {
				printf("Unknown event %d\n", event_id);
				continue;
			}

			if (event_id == TIMESPAN_EVENT) {
				timespan += (uint64_t)delta_time * MAX_TIME_MS;
				continue;
			}
			printf(" %20s: %4" PRId64 " ms\n",
			       boot_tracer_stages[event_id],
			       timespan + delta_time);
			timespan = 0;
		}
		printf("\n");
	}
	return 0;
}

/*
 * Try setting the GSC device type.
 *
 * dev - type to set to
 * option - short command line option requiring this device type
 * error_counter - pointer to the counter to increment in case of error.
 *
 * Returns false and increments error counter if gsc_dev is already set to a
 * different device type.
 */
static bool set_device_type(enum gsc_device dev, char option,
			    int *error_counter)
{
	if (gsc_dev == dev)
		return true;

	if (gsc_dev == GSC_DEVICE_ANY) {
		gsc_dev = dev;
		return true;
	}

	fprintf(stderr, "Inconsistent -%c option\n", option);
	(*error_counter)++;
	return false;
}

int main(int argc, char *argv[])
{
	struct transfer_descriptor td;
	int rv = 0;
	int errorcnt;
	uint8_t *fw_image_data = 0;
	size_t data_len = 0;
	uint16_t vid = 0;
	uint16_t pid = 0;
	int i;
	size_t j;
	int transferred_sections = 0;
	int binary_vers = 0;
	int show_fw_ver = 0;
	int rma = 0;
	const char *rma_auth_code = "";
	int get_endorsement_seed = 0;
	const char *endorsement_seed_str = "";
	int corrupt_inactive_rw = 0;
	struct board_id bid;
	enum board_id_action bid_action;
	int password = 0;
	int ccd_open = 0;
	int ccd_unlock = 0;
	int ccd_lock = 0;
	int ccd_info = 0;
	int get_flog = 0;
	uint32_t prev_log_entry = 0;
	enum wp_options wp = WP_NONE;
	int get_boot_mode = 0;
	int try_all_transfer = 0;
	int tpm_mode = 0;
	int get_apro_hash = 0;
	int get_apro_boot_status = 0;
	int start_apro_verify = 0;
	bool show_machine_output = false;
	int tstamp = 0;
	const char *tstamp_arg = NULL;
	enum arv_config_spi_addr_mode_e arv_config_spi_addr_mode =
		arv_config_spi_addr_mode_none;
	enum arv_config_wpsr_choice_e arv_config_wpsr_choice =
		arv_config_wpsr_choice_none;
	struct arv_config_wpds arv_config_wpds = { 0 };

	const char *exclusive_opt_error =
		"Options -a, -s and -t are mutually exclusive\n";
	const char *openbox_desc_file = NULL;
	int factory_mode = 0;
	char *factory_mode_arg = "";
	char *tpm_mode_arg = NULL;
	char *serial = NULL;
	int sn_bits = 0;
	uint8_t sn_bits_arg[SN_BITS_SIZE];
	int sn_inc_rma = 0;
	uint8_t sn_inc_rma_arg = 0;
	int erase_ap_ro_hash = 0;
	int set_capability = 0;
	const char *capability_parameter = "";
	bool reboot_gsc = false;
	size_t reboot_gsc_timeout = 0;
	int get_clog = 0;
	int get_console = 0;
	int factory_config = 0;
	int set_factory_config = 0;
	uint64_t factory_config_arg = 0;
	int get_time = 0;
	bool get_boot_trace = false;
	bool erase_boot_trace = false;
	bool get_metrics = false;
	bool get_chassis_open = false;
	bool get_dev_ids = false;

	/*
	 * All options which result in setting a Boolean flag to True, along
	 * with addresses of the flags. Terminated by a zeroed entry.
	 */
	const struct options_map omap[] = {
		{ 'b', &binary_vers },
		{ 'c', &corrupt_inactive_rw },
		{ 'f', &show_fw_ver },
		{ 'g', &get_boot_mode },
		{ 'H', &erase_ap_ro_hash },
		{ 'k', &ccd_lock },
		{ 'o', &ccd_open },
		{ 'P', &password },
		{ 'p', &td.post_reset },
		{ 'U', &ccd_unlock },
		{ 'u', &td.upstart_mode },
		{ 'V', &verbose_mode },
		{},
	};

	/*
	 * Explicitly sets buffering type to line buffered so that output
	 * lines can be written to pipe instantly. This is needed when the
	 * cr50-verify-ro.sh execution in verify_ro is moved from crosh to
	 * debugd.
	 */
	setlinebuf(stdout);

	progname = strrchr(argv[0], '/');
	if (progname)
		progname++;
	else
		progname = argv[0];

	/* Usb transfer - default mode. */
	memset(&td, 0, sizeof(td));
	td.ep_type = usb_xfer;

	bid_action = bid_none;
	errorcnt = 0;
	opterr = 0; /* quiet, you */

	while ((i = getopt_all(argc, argv)) != -1) {
		if (opt_gsc_dev != GSC_DEVICE_ANY) {
			if (!set_device_type(opt_gsc_dev, i, &errorcnt))
				continue;
		}
		if (check_boolean(omap, i))
			continue;
		switch (i) {
		case 'A':
			get_apro_hash = 1;
			break;
		case 'a':
			if (td.ep_type) {
				errorcnt++;
				fprintf(stderr, "%s", exclusive_opt_error);
				break;
			}
			try_all_transfer = 1;
			/* Try dev_xfer first. */
			td.ep_type = dev_xfer;
			break;
		case 'B':
			if (optarg && !strcmp(optarg, "start"))
				start_apro_verify = 1;
			else
				get_apro_boot_status = 1;
			break;
		case 'C':
			if (optarg && !strncmp(optarg, "3byte", strlen(optarg)))
				arv_config_spi_addr_mode =
					arv_config_spi_addr_mode_set_3byte;
			else if (optarg &&
				 !strncmp(optarg, "4byte", strlen(optarg)))
				arv_config_spi_addr_mode =
					arv_config_spi_addr_mode_set_4byte;
			else
				arv_config_spi_addr_mode =
					arv_config_spi_addr_mode_get;
			break;
		case 'D':
			/*
			 * as a result of processing this command line option
			 * gsc_dev has been set to GSC_DEVICE_DT by
			 * set_device_type(), no further action is required.
			 */
			break;
		case 'd':
			if (!parse_vidpid(optarg, &vid, &pid)) {
				fprintf(stderr,
					"Invalid device argument: \"%s\"\n",
					optarg);
				errorcnt++;
			}
			break;
		case 'e':
			get_endorsement_seed = 1;
			endorsement_seed_str = optarg;
			break;
		case 'E':
			if (!optarg) {
				arv_config_wpsr_choice =
					arv_config_wpsr_choice_get;
			} else if (optarg && strlen(optarg) > 0) {
				arv_config_wpds.data[0].state =
					arv_config_setting_state_not_present;
				arv_config_wpds.data[1].state =
					arv_config_setting_state_not_present;
				arv_config_wpds.data[2].state =
					arv_config_setting_state_not_present;

				rv = parse_wpsrs(optarg, &arv_config_wpds);
				if (rv == 2 || rv == 4 || rv == 6) {
					arv_config_wpsr_choice =
						arv_config_wpsr_choice_set;
				} else {
					fprintf(stderr,
						"Invalid write protect descriptors "
						"hex string: \"%s\"\n",
						optarg);
					exit(update_error);
				}
			} else {
				fprintf(stderr,
					"Invalid the write protect descriptors "
					"hex string length\n");
				exit(update_error);
			}

			break;
		case 'F':
			factory_mode = 1;
			factory_mode_arg = optarg;
			break;
		case 'G':
			get_time = 1;
			break;
		case 'h':
			usage(errorcnt);
			break;
		case 'I':
			if (optarg) {
				set_capability = 1;
				capability_parameter = optarg;
				/* Supported on Dauntless only. */
				set_device_type(GSC_DEVICE_DT, i, &errorcnt);
			} else {
				ccd_info = 1;
			}
			break;
		case 'i':
			if (!parse_bid(optarg, &bid, &bid_action)) {
				fprintf(stderr,
					"Invalid board id argument: \"%s\"\n",
					optarg);
				errorcnt++;
			}
			break;
		case 'J':
			get_boot_trace = true;
			if (!optarg)
				break;
			if (strncasecmp(optarg, "erase", strlen(optarg))) {
				fprintf(stderr,
					"Invalid boot trace argument: "
					"\"%s\"\n",
					optarg);
				errorcnt++;
			}
			erase_boot_trace = true;
			break;
		case 'K':
			/* We only support a single get_value option as of now*/
			if (!strncasecmp(optarg, "chassis_open",
					 strlen(optarg))) {
				get_chassis_open = true;
			} else if (!strncasecmp(optarg, "dev_ids",
						strlen(optarg))) {
				get_dev_ids = true;
			} else {
				fprintf(stderr,
					"Invalid get_value argument: "
					"\"%s\"\n",
					optarg);
				errorcnt++;
			}
			break;
		case 'L':
			get_flog = 1;
			if (optarg)
				prev_log_entry = strtoul(optarg, NULL, 0);
			break;
		case 'l':
			get_console = 1;
			break;
		case 'M':
			show_machine_output = true;
			break;
		case 'm':
			tpm_mode = 1;
			tpm_mode_arg = optarg;
			break;
		case 'n':
			serial = optarg;
			break;
		case 'O':
			openbox_desc_file = optarg;
			break;
		case 'q':
			td.force_ro = 1;
			break;
		case 'r':
			rma = 1;
			rma_auth_code = optarg;
			break;
		case 'R':
			sn_inc_rma = 1;
			if (!parse_sn_inc_rma(optarg, &sn_inc_rma_arg)) {
				fprintf(stderr,
					"Invalid sn_rma_inc argument: \"%s\"\n",
					optarg);
				errorcnt++;
			}

			break;
		case 's':
			if (td.ep_type || try_all_transfer) {
				errorcnt++;
				fprintf(stderr, "%s", exclusive_opt_error);
				break;
			}
			td.ep_type = dev_xfer;
			break;
		case 'S':
			sn_bits = 1;
			if (!parse_sn_bits(optarg, sn_bits_arg)) {
				fprintf(stderr,
					"Invalid sn_bits argument: \"%s\"\n",
					optarg);
				errorcnt++;
			}

			break;
		case 't':
			if (td.ep_type || try_all_transfer) {
				errorcnt++;
				fprintf(stderr, "%s", exclusive_opt_error);
				break;
			}
			td.ep_type = ts_xfer;
			break;
		case 'T':
			tstamp = 1;
			tstamp_arg = optarg;
			break;
		case 'v':
			report_version(); /* This will call exit(). */
			break;
		case 'W':
			get_metrics = true;
			break;
		case 'w':
			if (!optarg) {
				wp = WP_CHECK;
				break;
			}
			if (!strcasecmp(optarg, "enable")) {
				wp = WP_ENABLE;
				break;
			}
			if (!strcasecmp(optarg, "disable")) {
				wp = WP_DISABLE;
				/* Supported on Dauntless only. */
				set_device_type(GSC_DEVICE_DT, i, &errorcnt);
				break;
			}
			if (!strcasecmp(optarg, "follow")) {
				wp = WP_FOLLOW;
				/* Supported on Dauntless only. */
				set_device_type(GSC_DEVICE_DT, i, &errorcnt);
				break;
			}
			fprintf(stderr, "Illegal wp option \"%s\"\n", optarg);
			errorcnt++;
			break;
		case 'x':
			get_clog = 1;
			break;
		case 'y':
			factory_config = 1;
			if (optarg) {
				set_factory_config = 1;
				factory_config_arg = strtoull(optarg, NULL, 16);
			}
			break;
		case 'z':
			reboot_gsc = true;
			/* Set a 1ms default reboot time to avoid libusb errors
			 * when the GSC resets too quickly.
			 */
			reboot_gsc_timeout = 1;
			if (optarg)
				reboot_gsc_timeout = strtoul(optarg, NULL, 0);
			break;
		case 0: /* auto-handled option */
			break;
		case '?':
			if (optopt)
				fprintf(stderr, "Unrecognized option: -%c\n",
					optopt);
			else
				fprintf(stderr, "Unrecognized option: %s\n",
					argv[optind - 1]);
			errorcnt++;
			break;
		case ':':
			fprintf(stderr, "Missing argument to %s\n",
				argv[optind - 1]);
			errorcnt++;
			break;
		default:
			fprintf(stderr, "Internal error at %s:%d\n", __FILE__,
				__LINE__);
			exit(update_error);
		}
	}

	if (errorcnt)
		usage(errorcnt);

	if ((bid_action == bid_none) &&
	    (arv_config_spi_addr_mode == arv_config_spi_addr_mode_none) &&
	    (arv_config_wpsr_choice == arv_config_wpsr_choice_none) &&
	    !ccd_info && !ccd_lock && !ccd_open && !ccd_unlock &&
	    !corrupt_inactive_rw && !get_apro_hash && !get_apro_boot_status &&
	    !get_boot_mode && !get_boot_trace && !get_clog && !get_console &&
	    !get_flog && !get_endorsement_seed && !get_metrics && !get_time &&
	    !factory_config && !factory_mode && !erase_ap_ro_hash &&
	    !password && !reboot_gsc && !rma && !set_capability &&
	    !show_fw_ver && !sn_bits && !sn_inc_rma && !start_apro_verify &&
	    !openbox_desc_file && !tstamp && !tpm_mode && (wp == WP_NONE) &&
	    !get_chassis_open && !get_dev_ids) {
		if (optind >= argc) {
			fprintf(stderr,
				"\nERROR: Missing required <binary image>\n\n");
			usage(1);
		}

		fw_image_data = get_file_or_die(argv[optind], &data_len);
		printf("read %zd(%#zx) bytes from %s\n", data_len, data_len,
		       argv[optind]);

		/* Validate image size and locate headers within image */
		if (!locate_headers(fw_image_data, data_len))
			exit(update_error);

		if (!fetch_header_versions(fw_image_data))
			exit(update_error);

		if (binary_vers)
			exit(show_headers_versions(fw_image_data,
						   show_machine_output));
	} else {
		if (optind < argc)
			printf("Ignoring binary image %s\n", argv[optind]);
	}

	if (((bid_action != bid_none) + !!rma + !!password + !!ccd_open +
	     !!ccd_unlock + !!ccd_lock + !!ccd_info + !!get_flog +
	     !!get_boot_mode + !!openbox_desc_file + !!factory_mode +
	     (wp != WP_NONE) + !!get_endorsement_seed + !!erase_ap_ro_hash +
	     !!set_capability + !!get_clog + !!get_console) > 1) {
		fprintf(stderr,
			"Error: options "
			"-e, -F, -g, -H, -I, -i, -k, -L, -l, -O, -o, -P, -r,"
			"-U, -x and -w are mutually exclusive\n");
		exit(update_error);
	}

	if (td.ep_type == usb_xfer) {
		/* Extra variables only used to prevent 80+ character lines */
		const uint16_t subclass = USB_SUBCLASS_GOOGLE_CR50;
		const uint16_t protocol =
			USB_PROTOCOL_GOOGLE_CR50_NON_HC_FW_UPDATE;
		/*
		 * If no usb device information was given, default to the using
		 * haven or dauntless vendor and product id to find the usb
		 * device, but then try the other if the first isn't found
		 */
		if (!serial && !vid && !pid) {
			vid = USB_VID_GOOGLE;
			/*
			 * Set default product id based on expected device
			 * type set when processing command line options. If
			 * device type is not set - start with H1.
			 */
			pid = (gsc_dev == GSC_DEVICE_DT) ? D2_PID : H1_PID;
			if (usb_findit(serial, vid, pid, subclass, protocol,
				       &td.uep)) {
				/*
				 * If a certain device was requested and has
				 * not been found - exit.
				 */
				if (gsc_dev != GSC_DEVICE_ANY)
					exit(update_error);
				/*
				 * Try Dauntless, as the only way to get here
				 * is when a particular device was not
				 * requested and we tried H1 first.
				 */
				pid = D2_PID;
				if (usb_findit(serial, vid, pid, subclass,
					       protocol, &td.uep))
					exit(update_error);
				gsc_dev = GSC_DEVICE_DT;
			}
		} else {
			if (usb_findit(serial, vid, pid, subclass, protocol,
				       &td.uep))
				exit(update_error);
		}
		/* Make sure device type is set. */
		if (gsc_dev == GSC_DEVICE_ANY) {
			switch (pid) {
			case D2_PID:
				gsc_dev = GSC_DEVICE_DT;
				break;
			case H1_PID:
				gsc_dev = GSC_DEVICE_H1;
				break;
			default:
				fprintf(stderr,
					"EROOR: Unsupported USB PID %04x\n",
					pid);
				exit(update_error);
			}
		}
	} else if (td.ep_type == dev_xfer) {
		td.tpm_fd = open("/dev/tpm0", O_RDWR);
		if (td.tpm_fd < 0) {
			if (!try_all_transfer) {
				perror("Could not open TPM");
				exit(update_error);
			}
			td.ep_type = ts_xfer;
		}
	}

	/* If device type still not clear - fall back to H1. */
	if (gsc_dev == GSC_DEVICE_ANY)
		gsc_dev = GSC_DEVICE_H1;

	if (openbox_desc_file)
		return verify_ro(&td, openbox_desc_file, show_machine_output);

	if (ccd_unlock || ccd_open || ccd_lock || ccd_info)
		process_ccd_state(&td, ccd_unlock, ccd_open, ccd_lock, ccd_info,
				  show_machine_output);

	if (set_capability)
		exit(process_set_capabililty(&td, capability_parameter));

	if (password)
		process_password(&td);

	if (bid_action != bid_none)
		process_bid(&td, bid_action, &bid, show_machine_output);

	if (get_endorsement_seed)
		exit(process_endorsement_seed(&td, endorsement_seed_str));

	if (rma)
		process_rma(&td, rma_auth_code, show_machine_output);

	if (factory_mode)
		process_factory_mode(&td, factory_mode_arg);
	if (wp != WP_NONE)
		exit(process_wp(&td, wp));

	if (get_chassis_open)
		exit(process_get_chassis_open(&td));

	if (get_dev_ids)
		exit(process_get_dev_ids(&td, show_machine_output));

	if (corrupt_inactive_rw)
		invalidate_inactive_rw(&td);

	if (tpm_mode) {
		int rv = process_tpm_mode(&td, tpm_mode_arg);

		exit(rv);
	}

	if (tstamp)
		return process_tstamp(&td, tstamp_arg);

	if (sn_bits)
		process_sn_bits(&td, sn_bits_arg);

	if (sn_inc_rma)
		process_sn_inc_rma(&td, sn_inc_rma_arg);

	if (get_apro_hash)
		exit(process_get_apro_hash(&td));

	if (get_apro_boot_status)
		exit(process_get_apro_boot_status(&td));

	if (start_apro_verify)
		exit(process_start_apro_verify(&td));

	if (get_boot_mode)
		exit(process_get_boot_mode(&td));

	if (get_flog)
		process_get_flog(&td, prev_log_entry, show_machine_output);

	if (erase_ap_ro_hash)
		process_erase_ap_ro_hash(&td);

	if (arv_config_spi_addr_mode)
		exit(process_arv_config_spi_addr_mode(
			&td, arv_config_spi_addr_mode));

	if (arv_config_wpsr_choice)
		exit(process_arv_config_wpds(&td, arv_config_wpsr_choice,
					     &arv_config_wpds));

	if (reboot_gsc)
		exit(process_reboot_gsc(&td, reboot_gsc_timeout));

	if (get_clog)
		exit(get_crashlog(&td));

	if (get_console)
		exit(get_console_logs(&td));

	if (factory_config) {
		if (set_factory_config)
			exit(process_set_factory_config(&td,
							factory_config_arg));
		else
			exit(process_get_factory_config(&td));
	}

	if (get_time) {
		exit(process_get_time(&td));
	}

	if (get_boot_trace)
		exit(process_get_boot_trace(&td, erase_boot_trace,
					    show_machine_output));

	if (get_metrics)
		exit(process_get_metrics(&td, show_machine_output));

	if (fw_image_data || show_fw_ver) {
		setup_connection(&td);

		if (fw_image_data) {
			transferred_sections =
				transfer_image(&td, fw_image_data, data_len);
			free(fw_image_data);
		}

		/*
		 * Move USB updater sate machine to idle state so that
		 * vendor commands can be processed later, if any.
		 */
		if (td.ep_type == usb_xfer)
			send_done(&td.uep);

		if (transferred_sections)
			generate_reset_request(&td);

		if (show_fw_ver) {
			if (show_machine_output) {
				print_machine_output("RO_FW_VER", "%d.%d.%d",
						     targ.shv[0].epoch,
						     targ.shv[0].major,
						     targ.shv[0].minor);
				print_machine_output("RW_FW_VER", "%d.%d.%d",
						     targ.shv[1].epoch,
						     targ.shv[1].major,
						     targ.shv[1].minor);
			} else {
				printf("Current versions:\n");
				printf("RO %d.%d.%d\n", targ.shv[0].epoch,
				       targ.shv[0].major, targ.shv[0].minor);
				printf("RW %d.%d.%d\n", targ.shv[1].epoch,
				       targ.shv[1].major, targ.shv[1].minor);
			}
		}
	}

	if (td.ep_type == usb_xfer) {
		libusb_close(td.uep.devh);
		libusb_exit(NULL);
	}

	if (!transferred_sections)
		return noop;
	/*
	 * We should indicate if RO update was not done because of the
	 * insufficient RW version.
	 */
	for (j = 0; j < ARRAY_SIZE(sections); j++)
		if (sections[j].ustatus == not_possible) {
			/* This will allow scripting repeat attempts. */
			printf("Failed to update RO, run the command again\n");
			return rw_updated;
		}

	printf("image updated\n");
	return all_updated;
}
