/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * EC-CR50 communication
 */
#include "common.h"
#include "console.h"
#include "crc8.h"
#include "ec_comm.h"
#include "gpio.h"
#include "hooks.h"
#include "system.h"
#include "task.h"
#include "timer.h"
#include "uartn.h"
#include "vboot.h"

#define CPRINTS(format, args...) cprints(CC_TASK, "EC-COMM: " format, ## args)

/*
 * EC communications state machine (FSM) is supposed to be active between TPM
 * reset (including power on) and the moment EC_IN_RW transition is latched.
 *
 * Each packet is supposed to be prepended by at least one synchronization
 * character (0xEC). If packet does not verify, the entire stream contents,
 * including all sync characters is delivered to the USB bridge.
 *
 * Meaning of the states included in the enum below is as follows:
 */
enum ec_comm_phase {
	PHASE_READY_COMM = 0,
	PHASE_RECEIVING_PREAMBLE,
	PHASE_RECEIVING_HEADER,
	PHASE_RECEIVING_DATA,
};

/*
 * Context of EC-CR50 communication
 */
static struct ec_comm_context_ {
	uint8_t uart;	/* Current UART ID in packet mode.        */
			/* UART_NULL if no UART is in packet mode */
	uint8_t phase;	/* enum ec_comm_phase */
	uint8_t preamble_count;
	uint8_t bytes_received;

	uint8_t bytes_expected;
	uint8_t reserved[1];

	uint16_t last_resp;

	union {
		struct cr50_comm_packet ph;
		uint8_t packet[CR50_COMM_MAX_PACKET_SIZE];
	};
} ec_comm_ctx;


/*
 * Initialize EC-CR50 communication context.
 */
static void ec_comm_init_(void)
{
	ec_comm_ctx.uart = UART_NULL;

	if (!board_has_ec_cr50_comm_support())
		return;

	CPRINTS("Initializtion");

	gpio_enable_interrupt(GPIO_EC_PACKET_MODE_EN);
	gpio_enable_interrupt(GPIO_EC_PACKET_MODE_DIS);

	/* If DIOB3 is already high, then enable the packet mode. */
	if (gpio_get_level(GPIO_EC_PACKET_MODE_EN))
		ec_comm_packet_mode_en(GPIO_EC_PACKET_MODE_EN);
}
DECLARE_HOOK(HOOK_INIT, ec_comm_init_, HOOK_PRIO_INIT_EC_CR50_COMM);

/*
 * Process the received packet.
 *
 * @param ph     Pointer to the received EC-Cr50 packet.
 * @param bytes  Total bytes of the received EC-Cr50 packet.
 * @return CR50_COMM_SUCCESS if the packet has been processed successfully,
 *         CR50_COMM_ERROR_CRC if CRC is incorrect,
 *         CR50_COMM_ERROR_UNDEFINED_CMD if the cmd is unknown,
 *         CR50_COMM_ERROR_SIZE if data size is not as expected, or
 *         CR50_COMM_ERROR_BAD_PAYLOAD if the given hash and the hash in NVM
 *                                     are not same.
 *         0 if it deosn't have to respond to EC.
 */
static uint16_t decode_packet_(const struct cr50_comm_packet *ph, int bytes)
{
	const int offset_cmd = offsetof(struct cr50_comm_packet, cmd);
	uint8_t crc8_calc;
	uint16_t response;

	/* Verify CRC. */
	crc8_calc = crc8((const uint8_t *)&ph->cmd, bytes - offset_cmd);
	if (crc8_calc != ph->crc)
		return CR50_COMM_ERROR_CRC;

	/* Execute the command. */
	switch (ph->cmd) {
	case CR50_COMM_CMD_SET_BOOT_MODE:
		response = ec_efs_set_boot_mode(ph->data, ph->size);
		break;

	case CR50_COMM_CMD_VERIFY_HASH:
		response = ec_efs_verify_hash(ph->data, ph->size);
		break;

	default:
		response = CR50_COMM_ERROR_UNDEFINED_CMD;
	}

	return response;
}

/*
 * Transfer a 'response' to 'uart' port using DIOB3 pin.
 *
 * @param response  Response code to return to EC. Should be one of
 *                  CR50_COMM_RESPONSE codes in include/vboot.h.
 */
static void transfer_response_to_ec_(uint16_t response)
{
	uint8_t *ptr_resp = (uint8_t *)&response;
	int uart = ec_comm_ctx.uart;

	/* Send the response to EC in little endian. */
	uartn_write_char(uart, ptr_resp[0]);
	uartn_write_char(uart, ptr_resp[1]);

	uartn_tx_flush(uart);  /* Flush from UART2_TX to DIOB3 */

	ec_comm_ctx.last_resp = response;
}

int ec_comm_process_packet(uint8_t ch)
{
	uint16_t response = 0;

	switch (ec_comm_ctx.phase) {
	case PHASE_READY_COMM:
		/*
		 * if it is not the preamble, then return 0 so that ch can be
		 * forwarded to USB.
		 */
		if (ch != CR50_COMM_PREAMBLE) {
			ec_comm_ctx.preamble_count = 0;
			return 0;
		}

		/*
		 * Forward ch to USB even if it is CR50_COMM_PREAMBLE, because
		 * it is not yet sure whether it is a preamble or not.
		 * Forwarding 0xec to USB is not harmful anyway.
		 */
		if (++ec_comm_ctx.preamble_count < MIN_LENGTH_PREAMBLE)
			return 0;

		ec_comm_ctx.phase = PHASE_RECEIVING_PREAMBLE;
		break;

	case PHASE_RECEIVING_PREAMBLE:
		if (ch == CR50_COMM_PREAMBLE) {
			++ec_comm_ctx.preamble_count;
			break;
		}

		/*
		 * First non-preamble character. Reset received bytes and
		 * fall through to receive header.
		 */
		ec_comm_ctx.bytes_received = 0;
		ec_comm_ctx.bytes_expected = sizeof(struct cr50_comm_packet);
		ec_comm_ctx.phase = PHASE_RECEIVING_HEADER;
		/* FALLTHROUGH */

	case PHASE_RECEIVING_HEADER:
		/*
		 * Note: EC-CR50 communication is designed to perform in
		 *       little endian.
		 */
		ec_comm_ctx.packet[ec_comm_ctx.bytes_received++] = ch;

		if (ec_comm_ctx.bytes_received < ec_comm_ctx.bytes_expected)
			break;

		/* The header has been received. Let's parse it. */
		if (ec_comm_ctx.ph.magic != CR50_COMM_MAGIC_WORD) {
			response = CR50_COMM_ERROR_MAGIC;
			break;
		}

		/* Check struct_version */
		/*
		 * Note: if CR50_COMM_VERSION gets bigger than 0x00,
		 *       you should implement how to handle backward
		 *       compatibility.
		 */
		if (ec_comm_ctx.ph.version != CR50_COMM_VERSION) {
			response = CR50_COMM_ERROR_STRUCT_VERSION;
			break;
		}

		if (ec_comm_ctx.ph.size == 0) {
			/* Data size zero, then process the packet now. */
			response = decode_packet_(&ec_comm_ctx.ph,
						  ec_comm_ctx.bytes_received);
		} else if ((ec_comm_ctx.ph.size + ec_comm_ctx.bytes_expected) >
			   CR50_COMM_MAX_PACKET_SIZE) {
			response = CR50_COMM_ERROR_SIZE;
		} else {
			ec_comm_ctx.bytes_expected += ec_comm_ctx.ph.size;
			ec_comm_ctx.phase = PHASE_RECEIVING_DATA;
		}
		break;

	case PHASE_RECEIVING_DATA:
		ec_comm_ctx.packet[ec_comm_ctx.bytes_received++] = ch;

		/* The EC is done sending the packet, let's process it. */
		if (ec_comm_ctx.bytes_received >= ec_comm_ctx.bytes_expected)
			response = decode_packet_(&ec_comm_ctx.ph,
						  ec_comm_ctx.bytes_received);
		break;

	default:
		/*
		 * It is in the unknown phase.
		 * Let's turn the phase back to READY_COMM, and
		 * return 0 so that ch can be forwarded to USB.
		 */
		ec_comm_ctx.phase = PHASE_READY_COMM;
		ec_comm_ctx.preamble_count = 0;
		return 0;
	}

	if (response) {
		transfer_response_to_ec_(response);

#ifdef CR50_RELAXED
		CPRINTS("decoded a packet");
		CPRINTS("header  : 0x%ph",
			HEX_BUF((uint8_t *)&ec_comm_ctx.ph,
				sizeof(struct cr50_comm_packet)));
		CPRINTS("body    : 0x%ph",
			HEX_BUF((uint8_t *)ec_comm_ctx.ph.data,
				ec_comm_ctx.ph.size));
		/* Let's response to EC */
		CPRINTS("response: 0x%04x", response);
#endif
		/*
		 * If it reaches here, EC comm is either broken or one packet
		 * was well-processed. Let's turn the phase back to READY_COMM.
		 */
		ec_comm_ctx.phase = PHASE_READY_COMM;
		ec_comm_ctx.preamble_count = 0;
	}

	return 1;
}

void ec_comm_packet_mode_en(enum gpio_signal unsed)
{
	disable_sleep(SLEEP_MASK_EC_CR50_COMM);

	/* Initialize packet context */
	ec_comm_ctx.phase = PHASE_READY_COMM;
	ec_comm_ctx.preamble_count = 0;
	ec_comm_ctx.uart = UART_EC;  /* Enable Packet Mode */
	ccd_update_state();
}

void ec_comm_packet_mode_dis(enum gpio_signal unsed)
{
	ec_comm_ctx.uart = UART_NULL;	 /* Disable Packet Mode. */
	ccd_update_state();

	enable_sleep(SLEEP_MASK_EC_CR50_COMM);
}

int ec_comm_is_uart_in_packet_mode(int uart)
{
	return uart == ec_comm_ctx.uart;
}

void ec_comm_block(int block)
{
	static int is_blocked;

	if (is_blocked == block)
		return;

	if (block) {
		gpio_disable_interrupt(GPIO_EC_PACKET_MODE_EN);
		gpio_disable_interrupt(GPIO_EC_PACKET_MODE_DIS);

		if (ec_comm_is_uart_in_packet_mode(UART_EC))
			ec_comm_packet_mode_dis(GPIO_EC_PACKET_MODE_DIS);
	} else {
		gpio_enable_interrupt(GPIO_EC_PACKET_MODE_EN);
		gpio_enable_interrupt(GPIO_EC_PACKET_MODE_DIS);
	}

	is_blocked = block;

	/* Note: ccd_update_state() should be called to change UART status. */
}

/*
 * A console command, printing EC-CR50-Comm status.
 */
static int command_ec_comm(int argc, char **argv)
{
	if (argc > 1) {
		if (!strcasecmp(argv[1], "corrupt")) {
			int result = ec_efs_corrupt_hash();

			if (result != EC_SUCCESS)
				return result;
		} else {
			return EC_ERROR_PARAM1;
		}
		/*
		 * let's keep processing so that we can see how the context
		 * values are changed.
		 */
	}

	/*
	 * EC Packet Context
	 */
	ccprintf("uart               : 0x%02x\n", ec_comm_ctx.uart);
	ccprintf("packet mode        : %sABLED\n",
		ec_comm_is_uart_in_packet_mode(UART_EC) ? "EN" : "DIS");

	ccprintf("phase              : %d\n", ec_comm_ctx.phase);
	ccprintf("preamble_count     : %d\n", ec_comm_ctx.preamble_count);
	ccprintf("bytes_received     : %d\n", ec_comm_ctx.bytes_received);
	ccprintf("bytes_expected     : %d\n", ec_comm_ctx.bytes_expected);
#ifdef CR50_RELAXED
	ccprintf("packet:\n");
	hexdump((uint8_t *)ec_comm_ctx.packet, CR50_COMM_MAX_PACKET_SIZE);
#endif  /* CR50_RELAXED */
	ccprintf("response           : 0x%04x\n", ec_comm_ctx.last_resp);
	ccprintf("\n");
	ec_efs_print_status();

	return EC_SUCCESS;
}
DECLARE_SAFE_CONSOLE_COMMAND(ec_comm, command_ec_comm,
			     "[corrupt]",
			     "Dump EC-CR50-comm status"
);
