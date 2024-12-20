/* Copyright 2015 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Interrupt based USART RX driver for STM32L */

#include "atomic.h"
#include "common.h"
#include "queue.h"
#include "registers.h"
#include "usart.h"

static void usart_rx_init(struct usart_config const *config)
{
	intptr_t base = config->hw->base;

#ifdef STM32_USART_CR2_RTOEN
	/*
	 * Enable RX timeout interrupt if the RX line stays idle for the time it
	 * takes to transfer 20 bits (two characters) at the current baud rate.
	 *
	 * This is long enough that we can be quite sure that MCU at the other
	 * end has emptied its FIFO and does not immediately have more data to
	 * send, and short enough to not introduce significant delay
	 * transmitting the USB data.
	 */
	STM32_USART_CR1(base) |= STM32_USART_CR1_RTOIE;
	STM32_USART_RTOR(base) = 20;
	STM32_USART_CR2(base) |= STM32_USART_CR2_RTOEN;

	/*
	 * Signal that the consumer is allowed to buffer characters
	 * indefinitely, and is only strictly required to empty the queue when
	 * queue_flush() is invoked in the future.
	 */
	queue_enable_buffered_mode(config->producer.queue);
#endif

	STM32_USART_CR1(base) |= STM32_USART_CR1_RXNEIE;
	STM32_USART_CR1(base) |= STM32_USART_CR1_RE;
}

static void usart_rx_interrupt_handler(struct usart_config const *config)
{
	intptr_t base = config->hw->base;
	int32_t status = STM32_USART_SR(base);

	/*
	 * We have to check and clear the overrun error flag on STM32L because
	 * we can't disable it.
	 */
	if (status & STM32_USART_SR_ORE) {
#ifdef STM32_USART_ICR_ORECF
		/*
		 * Newer series (STM32L4xx and STM32L5xx) have an explicit
		 * "interrupt clear" register.
		 *
		 * ST reference code does blind write to this register, as is
		 * usual with the "write 1 to clear" convention, despite the
		 * datasheet listing the bits as "keep at reset value", (which
		 * we assume is due to copying from the description of
		 * reserved bits in read/write registers.)
		 */
		STM32_USART_ICR(config->hw->base) = STM32_USART_ICR_ORECF;
#else
		/*
		 * On the older series STM32L1xx, the overrun bit is cleared
		 * by a read of the status register, followed by a read of the
		 * data register.
		 *
		 * In the unlikely event that the overrun error bit was set but
		 * the RXNE bit was not (possibly because a read was done from
		 * RDR without first reading the status register) we do a read
		 * here to clear the overrun error bit.
		 */
		if (!(status & STM32_USART_SR_RXNE))
			(void)STM32_USART_RDR(config->hw->base);
#endif

		atomic_add((atomic_t *)&(config->state->rx_overrun), 1);
	}

	while (status & STM32_USART_SR_RXNE) {
		uint8_t byte = STM32_USART_RDR(base);

		if (!queue_add_unit(config->producer.queue, &byte))
			atomic_add((atomic_t *)&(config->state->rx_dropped), 1);

#ifdef STM32_USART_CR1_FIFOEN
		/* UART has FIFO, see if there are more bytes ready. */
		status = STM32_USART_SR(base);
#else
		/*
		 * Do not loop.  If a second character has arrived in the short
		 * span since above, we will enter this IRQ again.
		 */
		break;
#endif
	}

#ifdef STM32_USART_CR2_RTOEN
	if (status & STM32_USART_SR_RTOF) {
		/*
		 * UART RX line has been idle for a while, tell the consumer to
		 * flush the queue of previously received characters.
		 */
		STM32_USART_ICR(base) = STM32_USART_ICR_RTOCF;
		queue_flush(config->producer.queue);
	}
#endif
}

struct usart_rx const usart_rx_interrupt = {
	.producer_ops = {
		/*
		 * Nothing to do here, we either had enough space in the queue
		 * when a character came in or we dropped it already.
		 */
		.read = NULL,
	},

	.init      = usart_rx_init,
	.interrupt = usart_rx_interrupt_handler,
	.info      = NULL,
};
