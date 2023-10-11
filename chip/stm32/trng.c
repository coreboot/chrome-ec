/* Copyright 2017 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Hardware Random Number Generator */

#include "common.h"
#include "console.h"
#include "host_command.h"
#include "panic.h"
#include "printf.h"
#include "registers.h"
#include "system.h"
#include "task.h"
#include "trng.h"
#include "util.h"

uint32_t trng_rand(void)
{
	int tries = 300;
	/* Wait for a valid random number */
	while (!(STM32_RNG_SR & STM32_RNG_SR_DRDY) && --tries)
		;
	/* we cannot afford to feed the caller with an arbitrary number */
	if (!tries)
		software_panic(PANIC_SW_BAD_RNG, task_get_current());
	/* Finally the 32-bit of entropy */
	return STM32_RNG_DR;
}

test_mockable void trng_init(void)
{
#ifdef CHIP_FAMILY_STM32L4
	/* Enable the 48Mhz internal RC oscillator */
	STM32_RCC_CRRCR |= STM32_RCC_CRRCR_HSI48ON;
	/* no timeout: we watchdog if the oscillator doesn't start */
	while (!(STM32_RCC_CRRCR & STM32_RCC_CRRCR_HSI48RDY))
		;

	/* Clock the TRNG using the HSI48 */
	STM32_RCC_CCIPR = (STM32_RCC_CCIPR & ~STM32_RCC_CCIPR_CLK48SEL_MASK) |
			  (0 << STM32_RCC_CCIPR_CLK48SEL_SHIFT);
#elif defined(CHIP_FAMILY_STM32H7)
	/* Enable the 48Mhz internal RC oscillator */
	STM32_RCC_CR |= STM32_RCC_CR_HSI48ON;
	/* no timeout: we watchdog if the oscillator doesn't start */
	while (!(STM32_RCC_CR & STM32_RCC_CR_HSI48RDY))
		;

	/* Clock the TRNG using the HSI48 */
	STM32_RCC_D2CCIP2R =
		(STM32_RCC_D2CCIP2R & ~STM32_RCC_D2CCIP2_RNGSEL_MASK) |
		STM32_RCC_D2CCIP2_RNGSEL_HSI48;
#elif defined(CHIP_FAMILY_STM32F4)
	/*
	 * The RNG clock is the same as the SDIO/USB OTG clock, already set at
	 * 48 MHz during clock initialisation. Nothing to do.
	 */
#else
#error "Please add support for CONFIG_RNG on this chip family."
#endif
	/* Enable the RNG logic */
	STM32_RCC_AHB2ENR |= STM32_RCC_AHB2ENR_RNGEN;
	/* Start the random number generation */
	STM32_RNG_CR |= STM32_RNG_CR_RNGEN;
}

test_mockable void trng_exit(void)
{
	STM32_RNG_CR &= ~STM32_RNG_CR_RNGEN;
	STM32_RCC_AHB2ENR &= ~STM32_RCC_AHB2ENR_RNGEN;
#ifdef CHIP_FAMILY_STM32L4
	STM32_RCC_CRRCR &= ~STM32_RCC_CRRCR_HSI48ON;
#elif defined(CHIP_FAMILY_STM32H7)
	STM32_RCC_CR &= ~STM32_RCC_CR_HSI48ON;
#elif defined(CHIP_FAMILY_STM32F4)
	/* Nothing to do */
#endif
}
