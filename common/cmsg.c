/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Packetized console interface.
 */

#include "console.h"
#include "crc8.h"
#include "task.h"
#include "timer.h"
#include "uart.h"
#include "usb_console.h"
#include "util.h"

#define CONSOLE_PACKET_HEADER_LEN 13
#define CONSOLE_PACKET_MAGIC 0xc2
#define CONSOLE_PACKET_END 0xc1

/*
 * Packet header structure was borrowed from the Acropora project and adopted
 * for use in Cr50.
 */
struct console_packet {
	/* Magic number = CONSOLE_PACKET_MAGIC */
	uint8_t magic;

	/*
	 * Packet sequence number, incremented each time sender sends a packet.
	 * So receiver can detect small numbers of corrupt/dropped packets.
	 */
	uint8_t sequence : 4;

	/* Set if the sender had to discard packets due to buffer overflow. */
	uint8_t overflow : 1;
	uint8_t dummy : 3;

	/* Channel; values from enum console_channel */
	uint8_t channel;

	/* Bottom 48 bits of system time; enough for 8 years @ 1 us */
	uint8_t timestamp[6];

	/*
	 * Length of variable-length section in bytes, not including the
	 * packet end trailer.
	 */
	uint8_t data_len;

	uint16_t str_index; /* Index of the string in the database. */

	/* Header checksum */
	uint8_t crc;

	/* Fixed length header to here. */

	/*
	 * Followed by variable-length data, if data_len > 0.
	 *
	 * params: 1-8 of objects matching the format of the string indexed by
	 * 'str_index' above.
	 *
	 * CONSOLE_PACKET_END, as a sanity-check that we haven't dropped
	 * anything.  A checksum or CRC would be kinda expensive for debug
	 * data.  Note that it is not present if data_len == 0.
	 */
} __packed;

BUILD_ASSERT(sizeof(struct console_packet) == CONSOLE_PACKET_HEADER_LEN);

/*
 * Types of parameters recognized by util/util_precompile.py when processing
 * format string in .E files.
 *
 * When invoking the cmsgX functions all parameters are typecasted to
 * uintptr_t, and as such must fit into 4 bytes (on the 32 bit machine
 * cortex-m is).
 */
enum param_types {
	PARAM_INT = 1,
	/* Parameter is a pointer to an 8 byte integer. */
	PARAM_LONG = 2,
	/* Parameter is a pointer to an ASCIIZ string. */
	PARAM_STRING = 3,
	PARAM_PTR = 4,
	PARAM_HEX_BUF = 5,
	/*
	 * Parameter is an int, the number of the function name string in the
	 * dictionary prepared by util_precompile.py. Since it is passed as a
	 * %s parameter, to allow the terminal to tell the difference this
	 * value is sent as 5 bytes, first 0xff and then the function number.
	 */
	PARAM_FUNC_NAME = 6,
	/*
	 * Parameter is a pointer to a 8 byte value, unless the parameter is
	 * zero, in which case current time is used.
	 */
	PARAM_TIMESTAMP = 7,
};

/*
 * copy_params: copy actual parameter values into the console channel.
 *
 * This function is processing parameters 'on the fly' and sends them into the
 * console channel. There are two channels (uart and usb), so this fucnction
 * is called twice. It could be called once and save the parameters in a
 * buffer, but the buffer would have to be allocated on the stack, this
 * remains under consideration.
 *
 * mask is a set of 4 bit fields (enum param_types),
 * params[] includes as many values as there are non-zero fields in mask.
 * cmpuc is a pointer to the function which actually sends data.
 */
static void copy_params(uint32_t mask, uintptr_t params[],
			int (*cmputc)(const char *out, int len))
{
	enum param_types param_type;
	uintptr_t param;
	uint64_t t;

	while ((param_type = mask & 0xf) != 0) {
		const uint8_t ff = 0xff;

		param = *params++;
		mask >>= 4;

		switch (param_type) {
		case PARAM_FUNC_NAME:
			cmputc(&ff, sizeof(ff));
			/* fallthrough. */
		case PARAM_INT:
		case PARAM_PTR:
			cmputc((const void *)&param, sizeof(param));
			break;

		case PARAM_LONG:
			cmputc((const void *)param, sizeof(uint64_t));
			break;

		case PARAM_TIMESTAMP:
			if (param == 0)
				/*
				 * Terminal will use timestamp saved in the
				 * packet header.
				 */
				t = 0;
			else
				t = get_time().val;
			cmputc((const void *)&t, sizeof(t));
			break;

		case PARAM_HEX_BUF: {
			const struct hex_buffer_params *hbp =
				(const struct hex_buffer_params *)param;
			cmputc((const void *)&hbp->size, sizeof(hbp->size));
			cmputc((const void *)hbp->buffer, hbp->size);
			break;
		}

		case PARAM_STRING:
			/* Include trailing zero. */
			cmputc((const void *)param,
			       strlen((const char *)param) + 1);
		}
	}
}

/* A structure describing a communication channel and its state. */
struct packet_channel {
	/* Function to send data on the channel. */
	int (*cmputc)(const char *out, int len);
	/*
	 * Function to check how much room is left in the channel buffer, the
	 * packet is not sent if it won't fit.
	 */
	size_t (*room_check)(void);
	/*
	 * Channel run time context - sequence number to be used on the next
	 * packet and the indicator of channel overflow when packet had to be
	 * dropped because there was no room in the channel buffer.
	 */
	uint8_t seq_num;
	uint8_t overflow;
};

/*
 * ship_packet:  send a console packet on a channel.
 *
 * cpp - pointer to a partially filled packet header.
 * mask - a set of 4 bit parameter descriptions
 * params - an array of parameters matching the mask fields
 * pc - pointer to the packet channel structure used to send the packet.
 *
 * Returns overflow error if there is no room in the channel buffer to send
 *     the packet.
 */
static int ship_packet(struct console_packet *cpp, uint32_t mask,
		       uintptr_t params[], struct packet_channel *pc)
{
	pc->seq_num++;

	/* See if there is room to send this packet. */
	if (pc->room_check() < (sizeof(*cpp) + cpp->data_len + 1)) {
		pc->overflow = 1;
		return EC_ERROR_OVERFLOW;
	}

	cpp->sequence = pc->seq_num;
	cpp->overflow = pc->overflow & 1;
	pc->overflow = 0;

	/* Now crc can be calculated. */
	cpp->crc = crc8((void *)cpp, sizeof(*cpp) - sizeof(cpp->crc));
	pc->cmputc((void *)cpp, sizeof(*cpp));
	if (mask) {
		const char pe = CONSOLE_PACKET_END;

		copy_params(mask, params, pc->cmputc);
		pc->cmputc(&pe, 1);
	}
	return EC_SUCCESS;
}

/*
 * zz_msg - send console packets on both console physical channels.
 *
 * Calculate the size of the packet (needs to be done in advance to decide if
 * there is enough room in the channel buffer) and then call channel specific
 * transmit functions.
 *
 * chan - logical console channel, the one which can be turned and off by
 *        the cli 'channel' command.
 * str_index - index of the original format string in the list of strings
 *        prepared by util_precompile.py
 * fmt_mask is a set of 4 bit fields (enum param_types),
 * params[] includes as many values as there are non-zero fields in mask.
 *
 * Returns overflow error if either or both channels could not fit the packet.
 */
static int zz_msg(enum console_channel chan, int str_index, uint32_t fmt_mask,
		  uintptr_t params[])
{
	uint16_t total_param_length = 0;
	enum param_types param_type;
	int param_count = 0;
	uint64_t t;
	uint32_t mask_copy;
	int rv_uart;
	int rv_usb;
	static struct packet_channel pcs[] = {
		{ .cmputc = uart_put, .room_check = uart_buffer_room },
		{ .cmputc = usb_put, .room_check = usb_buffer_room }
	};

	struct console_packet cp = {
		.magic = CONSOLE_PACKET_MAGIC,
	};

	if (!(CC_MASK(chan) & channel_mask))
		return EC_SUCCESS;

	/* Let's scan the mask to see how many parameters there are. */
	mask_copy = fmt_mask;
	param_type = mask_copy & 0xf;
	while (param_type) {
		switch (param_type) {
		case PARAM_FUNC_NAME:
			total_param_length += 1;
			/* Falltrhough. */
		case PARAM_INT:
		case PARAM_PTR:
			total_param_length += sizeof(uintptr_t);
			break;
		case PARAM_LONG:
		case PARAM_TIMESTAMP:
			total_param_length += sizeof(uint64_t);
			break;
		case PARAM_HEX_BUF: {
			const struct hex_buffer_params *hbp =
				(void *)params[param_count];

			total_param_length += hbp->size + sizeof(hbp->size);
			break;
		}

		case PARAM_STRING:
			/* Include trailing zero. */
			total_param_length +=
				strlen((const char *)params[param_count]) + 1;
			break;

		default:
			/* TODO(vbendeb): add panic message generation. */
			break;
		}

		mask_copy >>= 4;
		param_type = mask_copy & 0xf;
		param_count++;
	}

	cp.channel = chan;

	t = get_time().val;
	memcpy(cp.timestamp, &t, sizeof(cp.timestamp));
	cp.str_index = str_index;
	cp.data_len = total_param_length;

	if (!in_interrupt_context())
		interrupt_disable();

	rv_uart = ship_packet(&cp, fmt_mask, params, &pcs[0]);
	rv_usb = ship_packet(&cp, fmt_mask, params, &pcs[1]);

	if (!in_interrupt_context())
		interrupt_enable();

	if (rv_uart != EC_SUCCESS)
		return rv_uart;

	return rv_usb;
}

/* Packet mode console interface functions. */
int cmsg0(enum console_channel chan, int str_index)
{
	return zz_msg(chan, str_index, 0, NULL);
}

int cmsg1(enum console_channel chan, int str_index, uint32_t mask, uintptr_t p1)
{
	uintptr_t params[] = { p1 };

	return zz_msg(chan, str_index, mask, params);
}

int cmsg2(enum console_channel chan, int str_index, uint32_t mask, uintptr_t p1,
	  uintptr_t p2)
{
	uintptr_t params[] = { p1, p2 };

	return zz_msg(chan, str_index, mask, params);
}

int cmsg3(enum console_channel chan, int str_index, uint32_t mask, uintptr_t p1,
	  uintptr_t p2, uintptr_t p3)
{
	uintptr_t params[] = { p1, p2, p3 };

	return zz_msg(chan, str_index, mask, params);
}

int cmsg4(enum console_channel chan, int str_index, uint32_t mask, uintptr_t p1,
	  uintptr_t p2, uintptr_t p3, uintptr_t p4)
{
	uintptr_t params[] = { p1, p2, p3, p4 };

	return zz_msg(chan, str_index, mask, params);
}

int cmsg5(enum console_channel chan, int str_index, uint32_t mask, uintptr_t p1,
	  uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5)
{
	uintptr_t params[] = { p1, p2, p3, p4, p5 };

	return zz_msg(chan, str_index, mask, params);
}

int cmsg6(enum console_channel chan, int str_index, uint32_t mask, uintptr_t p1,
	  uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5, uintptr_t p6)
{
	uintptr_t params[] = { p1, p2, p3, p4, p5, p6 };

	return zz_msg(chan, str_index, mask, params);
}

int cmsg7(enum console_channel chan, int str_index, uint32_t mask, uintptr_t p1,
	  uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5, uintptr_t p6,
	  uintptr_t p7)
{
	uintptr_t params[] = { p1, p2, p3, p4, p5, p6, p7 };

	return zz_msg(chan, str_index, mask, params);
}

int cmsg8(enum console_channel chan, int str_index, uint32_t mask, uintptr_t p1,
	  uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5, uintptr_t p6,
	  uintptr_t p7, uintptr_t p8)
{
	uintptr_t params[] = { p1, p2, p3, p4, p5, p6, p7, p8 };

	return zz_msg(chan, str_index, mask, params);
}
