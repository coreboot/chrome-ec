/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * MEC1322 SoC little FW
 *
 */

/* Memory Spaces Definitions */
MEMORY
{
    VECTOR(r )  : ORIGIN =  0x100000, LENGTH = 24
    SRAM (xrw)  : ORIGIN =  0x100018, LENGTH = 0x1000 - LENGTH(VECTOR)
}

/*
 * The entry point is informative, for debuggers and simulators,
 * since the Cortex-M vector points to it anyway.
 */
ENTRY(lfw_main)

/* Sections Definitions */

SECTIONS
{

    /*
     * The vector table goes first
     */
    .intvector :
    {
        . = ALIGN(4);
        KEEP(*(.intvector))
    } > VECTOR

    /*
     * The program code is stored in the .text section,
     * which goes to FLASH.
     */

    .text :
    {
        *(.text .text.*)                        /* all remaining code */
        *(.rodata .rodata.*)            /* read-only data (constants) */
    } >SRAM

    . = ALIGN(4);

    .data  :
    {
        . = ALIGN(4);

        __data_start__ = . ;
        *(.data_begin .data_begin.*)

        *(.data .data.*)

        *(.data_end .data_end.*)
        . = ALIGN(4);

        __data_end__ = . ;

    } > SRAM


    /*
     * The uninitialised data section. NOLOAD is used to avoid
     * the "section `.bss' type changed to PROGBITS" warning
     */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start__ = .;      /* standard newlib definition */
        *(.bss_begin .bss_begin.*)

        *(.bss .bss.*)
        *(COMMON)

        *(.bss_end .bss_end.*)
        . = ALIGN(4);
        __bss_end__ = .;        /* standard newlib definition */
    } >SRAM

    /* Padding */

    .fill : {
        FILL(0xFF);
        . = ORIGIN(SRAM) + LENGTH(SRAM) - 1;
        BYTE(0xFF);   /* emit at least a byte to make linker happy */
    }
}
