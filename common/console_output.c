/* Copyright 2012 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Console output module for Chrome EC */

#include "console.h"
#include "uart.h"
#include "usb_console.h"
#include "util.h"

/* Default to all channels active */
#ifndef CC_DEFAULT
#define CC_DEFAULT CC_ALL
#endif

/* Any consoles not enabled by default are restricted */
#ifndef CC_RESTRICTED
#define CC_RESTRICTED ((CC_ALL ^ CC_DEFAULT) & CC_ALL)
#endif

uint32_t channel_mask = CC_DEFAULT;
static uint32_t channel_mask_saved = CC_DEFAULT;

void console_disable_output(void)
{
	channel_mask_saved = channel_mask;
	channel_mask = 0;
}

void console_enable_output(void)
{
	channel_mask = channel_mask_saved;
}

/*
 * List of channel names;
 *
 * We could try to get clever with #ifdefs or board-specific lists of channel
 * names, so that for example boards without port80 support don't waste binary
 * size on the channel name string for "port80".  Pruning the channel list
 * might also become more important if we have >32 channels - for example, if
 * we decide to replace enum console_channel with enum module_id.
 */
static const char * const channel_names[] = {
	#define CONSOLE_CHANNEL(enumeration, string) string,
	#include "include/console_channel.inc"
	#undef CONSOLE_CHANNEL
};
BUILD_ASSERT(ARRAY_SIZE(channel_names) == CC_CHANNEL_COUNT);
/* ensure that we are not silently masking additional channels */
BUILD_ASSERT(CC_CHANNEL_COUNT <= 8*sizeof(uint32_t));

#ifndef CONFIG_EXTRACT_PRINTF_STRINGS

/*****************************************************************************/
/* Channel-based console output */

int cputs(enum console_channel channel, const char *outstr)
{
	int rv1, rv2;

	/* Filter out inactive channels */
	if (!(CC_MASK(channel) & channel_mask))
		return EC_SUCCESS;

	rv1 = usb_puts(outstr);
	rv2 = uart_puts(outstr);

	return rv1 == EC_SUCCESS ? rv2 : rv1;
}

int cprintf(enum console_channel channel, const char *format, ...)
{
	int rv1, rv2;
	va_list args;

	/* Filter out inactive channels */
	if (!(CC_MASK(channel) & channel_mask))
		return EC_SUCCESS;

	usb_va_start(args, format);
	rv1 = usb_vprintf(format, args);
	usb_va_end(args);

	va_start(args, format);
	rv2 = uart_vprintf(format, args);
	va_end(args);

	return rv1 == EC_SUCCESS ? rv2 : rv1;
}

int cprints(enum console_channel channel, const char *format, ...)
{
	int r, rv;
	va_list args;

	/* Filter out inactive channels */
	if (!(CC_MASK(channel) & channel_mask))
		return EC_SUCCESS;

	rv = cprintf(channel, "[%pT ", PRINTF_TIMESTAMP_NOW);

	va_start(args, format);
	r = uart_vprintf(format, args);
	if (r)
		rv = r;
	va_end(args);

	usb_va_start(args, format);
	r = usb_vprintf(format, args);
	if (r)
		rv = r;
	usb_va_end(args);

	r = cputs(channel, "]\n");
	return r ? r : rv;
}
#endif /* ^^^^^^^^ CONFIG_EXTRACT_PRINTF_STRINGS NOT defined. */

void cflush(void)
{
	uart_flush_output();
}

/*****************************************************************************/
/* Console commands */

static int update_channel_mask(uint32_t new_mask)
{
	if (console_is_restricted() && (new_mask & CC_RESTRICTED)) {
		ccprintf("restricted chan: 0x%08x\n", new_mask & CC_RESTRICTED);
		return EC_ERROR_ACCESS_DENIED;
	}
	channel_mask = new_mask;
	return EC_SUCCESS;
}

/* Set active channels */
static int command_ch(int argc, char **argv)
{
	int i;
	char *e;

	/* If one arg, save / restore, or set the mask */
	if (argc == 2) {

		if (strcasecmp(argv[1], "save") == 0) {
			/* Only save unrestricted channels. */
			channel_mask_saved = (channel_mask & CC_DEFAULT);
			return EC_SUCCESS;
		} else if (strcasecmp(argv[1], "restore") == 0) {
			return update_channel_mask(channel_mask_saved);
		}
		for (i = 0; i < CC_CHANNEL_COUNT; i++)
			if (strcasecmp(argv[1], channel_names[i]) == 0) {
				uint32_t new_mask = channel_mask;

				new_mask ^= CC_MASK(i);
				/* Don't disable command output. */
				new_mask |= CC_MASK(CC_COMMAND);
				ccprintf("setting %s to %s\n", argv[1],
					 (new_mask & CC_MASK(i)) ?
					 "on" : "off");
				return update_channel_mask(new_mask);
			}

		/* Set the mask */
		i = strtoi(argv[1], &e, 0);
		if (*e)
			return EC_ERROR_PARAM1;

		/* No disabling the command output channel */
		return update_channel_mask(i | CC_MASK(CC_COMMAND));
	}

	/* Print the list of channels */
	ccputs(" # Mask     E Channel\n");
	for (i = 0; i < CC_CHANNEL_COUNT; i++) {
		ccprintf("%2d %08x %c %s\n",
			 i, CC_MASK(i),
			 (channel_mask & CC_MASK(i)) ? '*' : ' ',
			 channel_names[i]);
		cflush();
	}
	return EC_SUCCESS;
};
DECLARE_SAFE_CONSOLE_COMMAND(chan, command_ch,
			     "[ save | restore | <chan_name> | <mask> ]",
			     "Save, restore, get or set console channel mask");
