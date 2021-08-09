SECTIONS
{
	.text.fips :  ALIGN(4)  SUBALIGN(4)
	{
		. = ALIGN(4);
		*(SORT_BY_NAME(.text*) .gnu.linkonce.t.*)
		. = ALIGN(4);
	} =0xffffffff

	/* Special EC sections should be placed
         * in the properly named sections. */
	.rodata.HOOK_INIT : {
		*(.rodata.HOOK_INIT)
	}
	.rodata.cmds.fips : {
		*(.rodata.cmds.*)
	}
	.rodata.extensioncmds : {
		*(.rodata.extensioncmds)
	}
	.rodata.irqprio : {
		*(.rodata.irqprio)
	}

	/* FIPS integrity placeholder should be in separate section. */
	.rodata.fips.checksum : {
		*(.rodata.fips.checksum)
	}

	/* Rest of the FIPS module data. */
	.rodata.fips : {
		/* Combine read-only data. */
		*(SORT_BY_NAME(.rodata*) SORT_BY_NAME(.srodata*) \
		 .gnu.linkonce.r.*)
	} =0xffffffff
	.data.fips :
	{
		*(SORT_BY_NAME(.data*) \
		  SORT_BY_NAME(.sdata*) \
		  SORT_BY_NAME(.ramfunc*))
	}
	ASSERT(SIZEOF(.data.fips) == 0, "No .data for FIPS module is allowed")
	.bss.fips (NOLOAD) :
	{
		. = ALIGN(4);
		*(SORT_BY_NAME(.sbss*) SORT_BY_NAME(.bss*))
		*(COMMON)
		. = ALIGN(4);
	}
	/* Debug data; this is stripped from the final binary. */
	.debug_abbrev 0 : { *(.debug_abbrev) }
	.debug_aranges 0 : { *(.debug_aranges) }
	.debug_frame 0 : { *(.debug_frame) }
	.debug_info 0 : { *(.debug_info) }
	.debug_line 0 : { *(.debug_line) }
	.debug_loc 0 : { *(.debug_loc) }
	.debug_ranges 0 : { *(.debug_ranges) }
	.debug_str 0 : { *(.debug_str) }
	.debug_macinfo 0 : { *(.debug_macinfo) }
	.debug_pubnames 0 : { *(.debug_pubnames) }
	.debug_pubtypes 0 : { *(.debug_pubtypes) }
	.symtab 0 : { *(.symtab) }
	.symtab_shndx 0 : { *(.symtab_shndx) }
	.shstrtab 0 : { *(.shstrtab) }
	.strtab 0 : { *(.strtab) }
	/* .stack_sizes (INFO)  makes the section not allocatable so
	 * it won't be loaded into memory. */
	.stack_sizes 0 :
	{
		KEEP(*(.stack_sizes));
	}
	/DISCARD/ :
	{
	*(.eh_frame);
	*(.comment*)
	*(.ARM.attributes*)
	*(.riscv.attributes*)
	*(.llvm_addrsig*)
	*(.note.GNU-stack*)
	*(.note.gnu.build-id)
	}
}
