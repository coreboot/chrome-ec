/* Copyright 2021 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "ccd_config.h"
#include "common.h"
#include "gpio.h"
#include "link_defs.h"
#include "registers.h"
#include "shared_mem.h"
#include "spi.h"
#include "spi_controller.h"
#include "spi_flash.h"
#include "timer.h"
#include "usb_descriptor.h"
#include "usb_spi.h"
#include "util.h"

/*
 * This module implements support for SPI programming driven by the flashrom
 * utility over USB.
 *
 * Flashrom connects to the specific SPI USB endpoint and sends commands to
 * this driver to control the external SPI flash chip attached to the H1's SPI
 * host port.
 *
 * Some configuration actions might have to be performed before SPI
 * programming is possible (say asserting the AP reset, enabling a
 * multiplexer, etc.), these actions are not controlled/performed by this
 * module, it expects the 'H2<=>SPI flash' connection provisioned and the
 * flash chip ready to be programmed.
 *
 * The below protocol specification was copied from raiden_debug_spi.c in the
 * flashrom tree with addition of some clarifications.
 *
 * Note that this implementation imposes additional limits on SPI transactions
 * which include both read and write portions: the write size of such
 * transactions case can not exceed 60, the size of the payload of the first
 * USB packet carrying the transaction PDU.
 *
 * USB SPI Version 2:
 *
 *     USB SPI version 2 adds support for larger SPI transfers which reduces
 *     the number of USB packets transferred. This improves performance when
 *     writing or reading large chunks of memory from a device. Larger
 *     quantities are transferred in PDUs split into USB packets.
 *
 *     A packet ID field is used to distinguish the different USB packet
 *     types. Additional packets have been included to query the device for
 *     its configuration allowing the interface to be used on platforms with
 *     different SPI limitations. It includes validation and a packet to
 *     recover from the situations where USB packets are lost.
 *
 *     The device advertises the supported protocol version in the
 *     bInterfaceProtocol field of the USB descriptor, this allows the USB SPI
 *     hosts to be backwards compatible with version 1.
 *
 *
 * Example: USB SPI request with 128 byte write and 0 byte read.
 *
 *      Packet #1 Host to Device:
 *           packet id   = USB_SPI_PKT_ID_CMD_TRANSFER_START
 *           write count = 128
 *           read count  = 0
 *           payload     = First 58 bytes from the write buffer,
 *                         starting at byte 0 in the buffer
 *           packet size = 64 bytes
 *
 *      Packet #2 Host to Device:
 *           packet id   = USB_SPI_PKT_ID_CMD_TRANSFER_CONTINUE
 *           data index  = 58
 *           payload     = Next 60 bytes from the write buffer,
 *                         starting at byte 58 in the buffer
 *           packet size = 64 bytes
 *
 *      Packet #3 Host to Device:
 *           packet id   = USB_SPI_PKT_ID_CMD_TRANSFER_CONTINUE
 *           data index  = 118
 *           payload     = Next 10 bytes from the write buffer,
 *                         starting at byte 118 in the buffer
 *           packet size = 14 bytes
 *
 *      Packet #4 Device to Host:
 *           packet id   = USB_SPI_PKT_ID_RSP_TRANSFER_START
 *           status code = status code from device
 *           payload     = 0 bytes
 *           packet size = 4 bytes
 *
 * Example: USB SPI request with 2 byte write and 100 byte read.
 *
 *      Packet #1 Host to Device:
 *           packet id   = USB_SPI_PKT_ID_CMD_TRANSFER_START
 *           write count = 2
 *           read count  = 100
 *           payload     = The 2 byte write buffer
 *           packet size = 8 bytes
 *
 *      Packet #2 Device to Host:
 *           packet id   = USB_SPI_PKT_ID_RSP_TRANSFER_START
 *           status code = status code from device
 *           payload     = First 60 bytes from the read buffer,
 *                         starting at byte 0 in the buffer
 *           packet size = 64 bytes
 *
 *      Packet #3 Device to Host:
 *           packet id   = USB_SPI_PKT_ID_RSP_TRANSFER_CONTINUE
 *           data index  = 60
 *           payload     = Next 40 bytes from the read buffer,
 *                         starting at byte 60 in the buffer
 *           packet size = 44 bytes
 *
 *
 * Message Packets:
 *
 * Note that all 16 bit fields are transferred in little endian mode.
 *
 * Command Start Packet (Host to Device):
 *
 *      Start of the USB SPI command, contains the number of bytes to write
 *      and read on SPI and up to the first 58 bytes of write payload.
 *      Longer writes will use the continue packets with packet id
 *      USB_SPI_PKT_ID_CMD_TRANSFER_CONTINUE to transmit the remaining data.
 *
 *      The write part of the transaction is executed first; in case the
 *      packet includes both read and write requests, the write portion is
 *      required to fit into the first USB packet.
 *
 *     +----------------+------------------+-----------------+---------------+
 *     | packet id : 2B | write count : 2B | read count : 2B | w.p. : <= 58B |
 *     +----------------+------------------+-----------------+---------------+
 *
 *     packet id:     2 byte enum defined by packet_id_type
 *                    Valid values packet id = USB_SPI_PKT_ID_CMD_TRANSFER_START
 *
 *     write count:   2 byte, zero based count of bytes to write
 *
 *     read count:    2 byte, zero based count of bytes to read
 *                    UINT16_MAX indicates full duplex mode with a read count
 *                    equal to the write count.
 *
 *     write payload: Up to 58 bytes of data to write to SPI, the total
 *                    length of all TX packets must match write count.
 *                    Due to data alignment constraints, this must be an
 *                    even number of bytes unless this is the final packet.
 *
 *
 * Response Start Packet (Device to Host):
 *
 *      Start of the USB SPI response, contains the status code and up to
 *      the first 60 bytes of read payload. Longer reads will use the
 *      continue packets with packet id USB_SPI_PKT_ID_RSP_TRANSFER_CONTINUE
 *      to transmit the remaining data.
 *
 *     +----------------+------------------+-----------------------+
 *     | packet id : 2B | status code : 2B | read payload : <= 60B |
 *     +----------------+------------------+-----------------------+
 *
 *     packet id:     2 byte enum defined by packet_id_type
 *                    Valid values packet id = USB_SPI_PKT_ID_RSP_TRANSFER_START
 *
 *     status code: 2 byte status code
 *         0x0000: Success
 *         0x0001: SPI timeout
 *         0x0002: Busy, try again
 *             This can happen if someone else has acquired the shared memory
 *             buffer that the SPI driver uses as /dev/null
 *         0x0003: Write count invalid. The byte limit is platform specific
 *             and is set during the configure USB SPI response.
 *         0x0004: Read count invalid. The byte limit is platform specific
 *             and is set during the configure USB SPI response.
 *         0x0005: The SPI bridge is disabled.
 *         0x0006: The RX continue packet's data index is invalid. This
 *             can indicate a USB transfer failure to the device.
 *         0x0007: The RX endpoint has received more data than write count.
 *             This can indicate a USB transfer failure to the device.
 *         0x0008: An unexpected packet arrived that the device could not
 *             process.
 *         0x0009: The device does not support full duplex mode.
 *         0x8000: Unknown error mask
 *             The bottom 15 bits will contain the bottom 15 bits from the EC
 *             error code.
 *
 *     read payload: Up to 60 bytes of data read from SPI, the total
 *                   length of all RX packets must match read count
 *                   unless an error status was returned. Due to data
 *                   alignment constraints, this must be a even number
 *                   of bytes unless this is the final packet.
 *
 *
 * Continue Packet (Bidirectional):
 *
 *      Continuation packet for the writes and read buffers. Both packets
 *      follow the same format, a data index counts the number of bytes
 *      previously transferred in the USB SPI transfer and a payload of bytes.
 *
 *     +----------------+-----------------+-------------------------------+
 *     | packet id : 2B | data index : 2B | write / read payload : <= 60B |
 *     +----------------+-----------------+-------------------------------+
 *
 *     packet id:     2 byte enum defined by packet_id_type
 *                    The packet id has 2 values depending on direction:
 *                    packet id = USB_SPI_PKT_ID_CMD_TRANSFER_CONTINUE
 *                    indicates the packet is being transmitted from the host
 *                    to the device and contains SPI write payload.
 *                    packet id = USB_SPI_PKT_ID_RSP_TRANSFER_CONTINUE
 *                    indicates the packet is being transmitted from the device
 *                    to the host and contains SPI read payload.
 *
 *     data index:    The data index indicates the number of bytes in the
 *                    read or write buffers that have already been transmitted.
 *                    It is used to validate that no packets have been dropped
 *                    and that the prior packets have been correctly decoded.
 *                    This value corresponds to the offset bytes in the buffer
 *                    to start copying the payload into.
 *
 *     read and write payload:
 *                    Contains up to 60 bytes of payload data to transfer to
 *                    the SPI write buffer or from the SPI read buffer.
 *
 *
 * Command Get Configuration Packet (Host to Device):
 *
 *      Query the device to request its USB SPI configuration indicating
 *      the number of bytes it can write and read.
 *
 *     +----------------+
 *     | packet id : 2B |
 *     +----------------+
 *
 *     packet id:     2 byte enum USB_SPI_PKT_ID_CMD_GET_USB_SPI_CONFIG
 *
 * Response Configuration Packet (Device to Host):
 *
 *      Response packet form the device to report the maximum write and
 *      read size supported by the device.
 *
 *     +----------------+----------------+---------------+----------------+
 *     | packet id : 2B | max write : 2B | max read : 2B | feature bitmap |
 *     +----------------+----------------+---------------+----------------+
 *
 *     packet id:         2 byte enum USB_SPI_PKT_ID_RSP_USB_SPI_CONFIG
 *
 *     max write count:   2 byte count of the maximum number of bytes
 *                        the device can write to SPI in one transaction.
 *
 *     max read count:    2 byte count of the maximum number of bytes
 *                        the device can read from SPI in one transaction.
 *
 *     feature bitmap:    Bitmap of supported features.
 *                        BIT(0): Full duplex SPI mode is supported
 *                        BIT(1:15): Reserved for future use
 *
 * Command Restart Response Packet (Host to Device):
 *
 *      Command to restart the response transfer from the device. This enables
 *      the host to recover from a lost packet when reading the response
 *      without restarting the SPI transfer.
 *
 *     +----------------+
 *     | packet id : 2B |
 *     +----------------+
 *
 *     packet id:         2 byte enum USB_SPI_PKT_ID_CMD_RESTART_RESPONSE
 *
 * USB Error Codes:
 *
 * send_command return codes have the following format:
 *
 *     0x00000:         Status code success.
 *     0x00001-0x0FFFF: Error code returned by the USB SPI device.
 *     0x10001-0x1FFFF: Error code returned by the USB SPI host.
 *     0x20001-0x20063  Lower bits store the positive value representation
 *                      of the libusb_error enum. See the libusb documentation:
 *                      http://libusb.sourceforge.net/api-1.0/group__misc.html
 *
 */


#define CPUTS(outstr) cputs(CC_USB, outstr)
#define CPRINTS(format, args...) cprints(CC_USB, format, ## args)

static int16_t usb_spi_map_error(int error)
{
	switch (error) {
	case EC_SUCCESS:
		return USB_SPI_SUCCESS;
	case EC_ERROR_TIMEOUT:
		return USB_SPI_TIMEOUT;
	case EC_ERROR_BUSY:
		return USB_SPI_BUSY;
	default:
		return USB_SPI_UNKNOWN_ERROR | (error & 0x7fff);
	}
}

static uint16_t usb_spi_read_packet(struct usb_spi_config const *config)
{
	return QUEUE_REMOVE_UNITS(config->consumer.queue, config->buffer,
		queue_count(config->consumer.queue));
}

/*
 * Put a packet on USB TX queue.
 *
 * Return EC_SUCCESS if there was enough room on the queue or EC_ERROR_TIMEOUT
 * if the queue is full for longer than 500 ms
 */
static enum ec_error_list usb_spi_write_packet(
	struct usb_spi_config const *config,
	uint8_t count)
{
#define QUEUE_BACKOFF_TIMEOUT_MS 1
#define MAX_QUEUE_WAIT_MS 500
	int i = (MAX_QUEUE_WAIT_MS/QUEUE_BACKOFF_TIMEOUT_MS);

	/*
	 * Experiments show that while reading a 16M flash time spent waiting
	 * is less than 30 ms, which is negligible in this case, as the AP is
	 * held in reset.
	 */
	while (queue_space(config->tx_queue) < count) {
		if (i-- == 0)
			return EC_ERROR_TIMEOUT;
		msleep(1);
	}

	QUEUE_ADD_UNITS(config->tx_queue, config->buffer, count);

	return EC_SUCCESS;
}

enum packet_id_type {
	/* Request USB SPI configuration data from device. */
	USB_SPI_PKT_ID_CMD_GET_USB_SPI_CONFIG = 0,
	/* USB SPI configuration data from device. */
	USB_SPI_PKT_ID_RSP_USB_SPI_CONFIG = 1,
	/*
	 * Start a USB SPI transfer specifying number of bytes to write,
	 * read and deliver first packet of data to write.
	 */
	USB_SPI_PKT_ID_CMD_TRANSFER_START = 2,
	/* Additional packets containing write payload. */
	USB_SPI_PKT_ID_CMD_TRANSFER_CONTINUE = 3,
	/*
	 * Request the device restart the response enabling us to recover
	 * from packet loss without another SPI transfer.
	 */
	USB_SPI_PKT_ID_CMD_RESTART_RESPONSE = 4,
	/*
	 * First packet of USB SPI response with the status code
	 * and read payload if it was successful.
	 */
	USB_SPI_PKT_ID_RSP_TRANSFER_START = 5,
	/* Additional packets containing read payload. */
	USB_SPI_PKT_ID_RSP_TRANSFER_CONTINUE = 6,
	/* Inform the device that transfer is over. */
	USB_SPI_PKT_ID_CMD_HOST_DONE = 7,
	/* Inform the host that processing is finished. */
	USB_SPI_PKT_ID_RSP_DEVICE_DONE = 8,
};

enum feature_bitmap {
	/* Indicates the platform supports full duplex mode. */
	USB_SPI_FEATURE_FULL_DUPLEX_SUPPORTED = BIT(0)
};

/* Helper structures defining various PDU headers. */
struct raiden_response_hdr {
	uint16_t packet_id;
	uint16_t value; /* Either offset into the PDU or error code. */
};

struct raiden_cmd_start_hdr {
	uint16_t packet_id;
	uint16_t write_count;
	uint16_t read_count;
};

struct raiden_cmd_continue_hdr {
	uint16_t packet_id;
	uint16_t offset;
};

union raiden_cmd_hdr {
	struct raiden_cmd_start_hdr s;
	struct raiden_cmd_continue_hdr c;
};

#define START_PAYLOAD_SIZE \
	(USB_MAX_PACKET_SIZE - sizeof(struct raiden_cmd_start_hdr))
#define CONT_PAYLOAD_SIZE \
	(USB_MAX_PACKET_SIZE - sizeof(struct raiden_cmd_continue_hdr))
#define RESP_PAYLOAD_SIZE \
	(USB_MAX_PACKET_SIZE - sizeof(struct raiden_response_hdr))

BUILD_ASSERT(START_PAYLOAD_SIZE < SPI_BUF_SIZE);
/*
 * Hardcoded values used to communicate to the host maximum sizes of read and
 * write transactions.
 */
/*
 * Six bytes header, one byte command, up to four byte addr, 256 bytes data
 * page and then some in case there is a chip which uses a multibyte command.
 */
#define USB_SPI_MAX_WRITE_COUNT 270
/*
 * Flashrom deducts 5 from this value, remaining 2040 bytes size results in 34
 * USB packets of 4 byte header and 60 bytes data, getting the PDU size close
 * to 2K. Decreasing this size would decrease flashrom read efficiency,
 * increasing it would put more pressure on the heap size.
 */
#define USB_SPI_MAX_READ_COUNT (5 + 34 * RESP_PAYLOAD_SIZE)

BUILD_ASSERT(USB_SPI_MAX_READ_COUNT < (CONFIG_SHAREDMEM_MINIMUM_SIZE/2));

/* Hadrdcoded response to the config request packet. */
static const struct {
	uint16_t rsp_id;
	uint16_t max_write;
	uint16_t max_read;
	uint16_t features;
} config_rsp = { USB_SPI_PKT_ID_RSP_USB_SPI_CONFIG, USB_SPI_MAX_WRITE_COUNT,
		 USB_SPI_MAX_READ_COUNT, 0 };

/*
 * Prepare an error message to be sent back to the host and switch to
 * RAIDEN_IDLE state.
 *
 * The return value is the number of bytes to send to the controller.
 */
static uint8_t report_error(void *packet, uint16_t error,
			    struct usb_spi_state *state)
{
	/* Packet is guaranteed to be 2 bytes aligned. */
	struct raiden_response_hdr *rsp = (struct raiden_response_hdr *)packet;

	rsp->packet_id = USB_SPI_PKT_ID_RSP_TRANSFER_START;
	rsp->value = error;

	state->raiden_state = RAIDEN_IDLE;
	return sizeof(*rsp);
}

/*
 * Send buffer containing data read from AP flash to the host.
 *
 * The buffer is sent as a V2 PDU, split into as many USB packets as
 * necessary.
 */
static void ship_pdu(const struct usb_spi_config *config, const uint8_t *buf,
		     size_t size)
{
	struct raiden_response_hdr *rsp =
		(struct raiden_response_hdr *)config->buffer;
	size_t sent_so_far = 0;

	do {
		size_t this_count = size - sent_so_far;

		if (this_count > RESP_PAYLOAD_SIZE)
			this_count = RESP_PAYLOAD_SIZE;

		rsp->packet_id = sent_so_far ?
					 USB_SPI_PKT_ID_RSP_TRANSFER_CONTINUE :
					 USB_SPI_PKT_ID_RSP_TRANSFER_START;
		rsp->value = sent_so_far;

		memcpy(rsp + 1, buf + sent_so_far, this_count);
		if (usb_spi_write_packet(config, sizeof(*rsp) + this_count)
		    != EC_SUCCESS) {
			CPRINTS("%s: USB stuck, breaking out", __func__);
			return;
		}
		sent_so_far += this_count;

	} while (sent_so_far != size);
}

/*
 * Most recent SPI transaction request received from the host, used when it is
 * necessary to restart the transaction.
 */
static struct {
	uint16_t count; /* Actual size of the request packet. */
	union raiden_cmd_hdr hdr;
	uint8_t payload[6]; /* Enough for any SPI read command. */
} last_cmd;

/* Return length of the last response packet to send back to the host. */
static uint8_t process_raiden_packet(const struct usb_spi_config *config,
				     size_t count)
{
	/* Buffer is guaranteed to be 2 bytes aligned. */
	union raiden_cmd_hdr *packet = (union raiden_cmd_hdr *)config->buffer;
	struct raiden_response_hdr *rsp =
		(struct raiden_response_hdr *)config->buffer;
	uint16_t total_read_count;
	uint16_t read_so_far;
	uint16_t this_read_count = 0;
	uint16_t this_write_count = 0;
	bool last_sub_transaction;
	uint16_t res;
	uint16_t packet_id;
	struct usb_spi_state *state = config->state;

	if (count < sizeof(packet->s.packet_id))
		return report_error(config->buffer, USB_SPI_UNKNOWN_ERROR,
				    state);

	packet_id = packet->s.packet_id;
	switch (state->raiden_state) {
	case RAIDEN_IDLE: {
		char *read_buf;
		size_t read_limit;

		if ((packet_id == USB_SPI_PKT_ID_CMD_GET_USB_SPI_CONFIG) &&
		    (count == sizeof(uint16_t))) {
			last_cmd.count = 0;
			memcpy(config->buffer, &config_rsp, sizeof(config_rsp));
			return sizeof(config_rsp);
		}

		/*
		 * If this is a restart request and the most recent command is
		 * available - use the saved command to start over.
		 */
		if ((packet_id == USB_SPI_PKT_ID_CMD_RESTART_RESPONSE) &&
		    last_cmd.count) {
			packet = &last_cmd.hdr;
			packet_id = last_cmd.hdr.s.packet_id;
			count = last_cmd.count;
			CPRINTS("%s: will restart the response id %d count %d",
				__func__, packet_id, count);
		}

		if ((packet_id != USB_SPI_PKT_ID_CMD_TRANSFER_START) ||
		    (count < sizeof(struct raiden_cmd_start_hdr)))
			return report_error(config->buffer,
					    USB_SPI_RUNT_PACKET, state);

		/* Preserve the request in case host asks to restart. */
		if (count <= (sizeof(last_cmd) - sizeof(last_cmd.count))) {
			memcpy(&last_cmd.hdr, packet, count);
			last_cmd.count = count;
		} else {
			last_cmd.count = 0;
		}

		/* This is request to start a new transaction. */
		state->total_write_count = packet->s.write_count;
		total_read_count = packet->s.read_count;

		/* Some basic validity checks. */
		if (total_read_count > config_rsp.max_read)
			return report_error(config->buffer,
					    USB_SPI_READ_COUNT_INVALID, state);

		if (state->total_write_count > config_rsp.max_write)
			return report_error(config->buffer,
					    USB_SPI_WRITE_COUNT_INVALID, state);

		/*
		 * If write count fits into one USB packet, the size of packet
		 * must match.
		 */
		if ((state->total_write_count <=
		     (USB_MAX_PACKET_SIZE -
		      sizeof(struct raiden_cmd_start_hdr))) &&
		    (state->total_write_count !=
		     (count - sizeof(struct raiden_cmd_start_hdr))))
			return report_error(config->buffer,
					    USB_SPI_WRITE_COUNT_INVALID, state);

		/*
		 * Transactions with write count exceeding one USB packet AND
		 * requiring a read are not supported.
		 */
		if (total_read_count &&
		    (state->total_write_count > START_PAYLOAD_SIZE))
			return report_error(config->buffer,
					    USB_SPI_WRITE_COUNT_INVALID, state);

		read_so_far = 0;
		state->wrote_so_far = 0;

		if (state->total_write_count > START_PAYLOAD_SIZE)
			this_write_count = START_PAYLOAD_SIZE;
		else
			this_write_count = state->total_write_count;

		/* Allocate buffer to store the entire requested read size. */
		if (total_read_count) {
			if (shared_mem_acquire(total_read_count, &read_buf) !=
			    EC_SUCCESS) {
				ccprintf("%s: Failed to allocate %zd bytes!\n",
					 __func__, total_read_count);
				return report_error(config->buffer,
						    USB_SPI_UNKNOWN_ERROR,
						    state);
			}
		}

		last_sub_transaction = false;

		/*
		 * This will send the write portion and read the entire read
		 * portion in multiple SPI subtransactions.
		 *
		 * In case the write PDU is split into multiple USB packets
		 * this will send the first packet and change state to
		 * RAIDEN_WRITING, where it will process the rest of USB
		 * packets of this PDU.
		 */
		while (this_write_count || (read_so_far != total_read_count)) {

			this_read_count = total_read_count - read_so_far;


			/*
			 * If there is a write portion, read limit needs to be
			 * reduced.
			 */
			read_limit = SPI_BUF_SIZE - this_write_count;

			/* Limit read count by SPI driver capacity. */
			if (this_read_count > read_limit)
				this_read_count = read_limit;

			/*
			 * This transaction is over when we have read as much
			 * as requested (which could be zero), AND the entire
			 * write portion fit into the USB payload.
			 */
			if (((this_read_count + read_so_far) ==
			     total_read_count) &&
			    (state->total_write_count <= START_PAYLOAD_SIZE))
				last_sub_transaction = true;

			res = usb_spi_map_error(spi_sub_transaction(
				SPI_FLASH_DEVICE,
				config->buffer +
					sizeof(struct raiden_cmd_start_hdr),
				this_write_count, read_buf + read_so_far,
				this_read_count, last_sub_transaction));
			if (res) {
				if (total_read_count)
					shared_mem_release(read_buf);
				rsp->value = res;
				rsp->packet_id =
					USB_SPI_PKT_ID_RSP_TRANSFER_START;
				return sizeof(*rsp);
			}

			rsp->packet_id =
				read_so_far ?
					USB_SPI_PKT_ID_RSP_TRANSFER_CONTINUE :
					USB_SPI_PKT_ID_RSP_TRANSFER_START;
			rsp->value = read_so_far;

			state->wrote_so_far += this_write_count;
			read_so_far += this_read_count;

			if (state->total_write_count > state->wrote_so_far) {
				state->raiden_state = RAIDEN_WRITING;
				return 0;
			}

			if (last_sub_transaction) {
				if (total_read_count) {
					ship_pdu(config, read_buf,
						 total_read_count);
					shared_mem_release(read_buf);
				}
				break;
			}

			/*
			 * Make sure this does not keep the loop going, we
			 * wrote all there was to write.
			 */
			this_write_count = 0;
		}

		/*
		 * If this was a pure write transaction and it fit into one
		 * USB packet - send a response to the host.
		 */
		if (!total_read_count && state->total_write_count &&
		    (state->raiden_state == RAIDEN_IDLE))
			return sizeof(*rsp);
		return 0;
	}
	case RAIDEN_WRITING:
		if ((packet_id != USB_SPI_PKT_ID_CMD_TRANSFER_CONTINUE) ||
		    (count <= sizeof(struct raiden_cmd_continue_hdr)))
			return report_error(config->buffer,
					    USB_SPI_RX_UNEXPECTED_PACKET,
					    state);

		if (packet->c.offset != state->wrote_so_far)
			return report_error(config->buffer,
					    USB_SPI_RX_UNEXPECTED_PACKET,
					    state);

		this_write_count =
			count - sizeof(struct raiden_cmd_continue_hdr);

		/*
		 * This is the last sub transaction if the remainder of data
		 * to write fits into USB packet, CS will have to be
		 * deasserted.
		 */
		last_sub_transaction =
			(this_write_count + state->wrote_so_far) ==
			state->total_write_count;

		res = usb_spi_map_error(spi_sub_transaction(
			SPI_FLASH_DEVICE,
			config->buffer + sizeof(struct raiden_response_hdr),
			this_write_count, config->buffer, 0,
			last_sub_transaction));
		if (res) {
			rsp->value = res;
			rsp->packet_id = USB_SPI_PKT_ID_RSP_TRANSFER_START;
			state->raiden_state = RAIDEN_IDLE;
			return sizeof(*rsp);
		}

		if (last_sub_transaction) {
			state->raiden_state = RAIDEN_IDLE;
			rsp->value = 0;
			rsp->packet_id = USB_SPI_PKT_ID_RSP_TRANSFER_START;
			return sizeof(*rsp);
		}
		state->wrote_so_far += this_write_count;
		return 0;
	}

	return 0;
}

void usb_spi_deferred(struct usb_spi_config const *config)
{
	uint16_t count;
	int rv = EC_SUCCESS;

	/*
	 * If our overall enabled state has changed we call the board specific
	 * enable or disable routines and save our new state.
	 */
	int enabled = !!(config->state->enabled_host &
			 config->state->enabled_device);

	if (enabled ^ config->state->enabled) {
		if (enabled)
			rv = usb_spi_board_enable(config->state->enabled_host);

		else
			usb_spi_board_disable();

		/* Only update our state if we were successful. */
		if (rv == EC_SUCCESS)
			config->state->enabled = enabled;
	}

	/*
	 * And if there is a USB packet waiting we process it and generate a
	 * response.
	 */
	count = usb_spi_read_packet(config);
	if (count == 0)
		return;

	if (!config->state->enabled || usb_spi_shortcut_active()) {
		struct raiden_response_hdr rsp_hdr = {
			USB_SPI_PKT_ID_RSP_TRANSFER_START, USB_SPI_DISABLED
		};
		memcpy(config->buffer, &rsp_hdr, sizeof(rsp_hdr));
		usb_spi_write_packet(config, sizeof(rsp_hdr));
		return;
	}

	/*
	 * Call the protocol handler and send back the last message generated
	 * by the handler.
	 */
	usb_spi_write_packet(config, process_raiden_packet(config, count));
}

static void usb_spi_written(struct consumer const *consumer, size_t count)
{
	struct usb_spi_config const *config =
		DOWNCAST(consumer, struct usb_spi_config, consumer);

	hook_call_deferred(config->deferred, 0);
}

struct consumer_ops const usb_spi_consumer_ops = {
	.written = usb_spi_written,
};

void usb_spi_enable(struct usb_spi_config const *config, int enabled)
{
	config->state->enabled_device = 0;
	if (enabled) {
#ifdef CONFIG_CASE_CLOSED_DEBUG_V1
		if (ccd_is_cap_enabled(CCD_CAP_AP_FLASH))
			config->state->enabled_device |= USB_SPI_AP;
		if (ccd_is_cap_enabled(CCD_CAP_EC_FLASH))
			config->state->enabled_device |= USB_SPI_EC;
#else
		config->state->enabled_device = USB_SPI_ALL;
#endif
	}

	hook_call_deferred(config->deferred, 0);
}
