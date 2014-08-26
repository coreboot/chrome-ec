# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Command automation for Blizzard LM4F chip

# Program internal flash

proc flash_lm4 {path offset} {
	#set firstsect [expr {$offset / 1024}];
	#set lastsect [expr {($offset + $size) / 1024 - 1}];
	reset halt;
	flash write_image erase $path $offset;
	reset
}

proc flash_link { } {
	flash_lm4 ../../../build/link/ec.bin 0
}

proc flash_link_ro { } {
	flash_lm4 ../../../build/link/ec.RO.flat 0
}

# Link originally had 80KB images, but TOT builds 128KB images
proc flash_link_rw { } {
	flash_lm4 ../../../build/link/ec.RW.bin 131072
}

proc flash_bds { } {
	flash_lm4 ../../../build/bds/ec.bin 0
}

proc flash_cranky { } {
	flash_lm4 ../../../build/cranky/ec.bin 0
}

proc flash_enguarde { } {
	flash_lm4 ../../../build/enguarde/ec.bin 0
}

proc flash_enguarde_ro { } {
	flash_lm4 ../../../build/enguarde/ec.RO.flat 0
}

proc flash_enguarde_rw { } {
	flash_lm4 ../../../build/enguarde/ec.RW.bin 131072
}

proc flash_glimmer { } {
	flash_lm4 ../../../build/glimmer/ec.bin 0
}

proc flash_glimmer_ro { } {
	flash_lm4 ../../../build/glimmer/ec.RO.flat 0
}

proc flash_glimmer_rw { } {
	flash_lm4 ../../../build/glimmer/ec.RW.bin 131072
}

proc flash_rambi { } {
	flash_lm4 ../../../build/rambi/ec.bin 0
}

proc flash_expresso { } {
	flash_lm4 ../../../build/express/ec.bin 0
}

proc flash_clapper { } {
	flash_lm4 ../../../build/clapper/ec.bin 0
}

proc flash_samus { } {
	flash_lm4 ../../../build/samus/ec.bin 0
}

proc flash_samus_ro { } {
	flash_lm4 ../../../build/samus/ec.RO.flat 0
}

proc flash_samus_rw { } {
	flash_lm4 ../../../build/samus/ec.RW.bin 131072
}

proc flash_squawks { } {
	flash_lm4 ../../../build/squawks/ec.bin 0
}

proc flash_squawks_ro { } {
	flash_lm4 ../../../build/squawks/ec.RO.flat 0
}

proc flash_quawks { } {
	flash_lm4 ../../../build/quawks/ec.bin 0
}

proc flash_quawks_ro { } {
	flash_lm4 ../../../build/quawks/ec.RO.flat 0
}

proc flash_falco { } {
	flash_lm4 ../../../build/falco/ec.bin 0
}

proc flash_peppy { } {
	flash_lm4 ../../../build/peppy/ec.bin 0
}

proc flash_rambi_ro { } {
	flash_lm4 ../../../build/rambi/ec.RO.flat 0
}

proc flash_rambi_rw { } {
	flash_lm4 ../../../build/rambi/ec.RW.bin 131072
}

proc flash_cranky_ro { } {
	flash_lm4 ../../../build/cranky/ec.RO.flat 0
}

proc flash_cranky_rw { } {
	flash_lm4 ../../../build/cranky/ec.RW.bin 131072
}

proc flash_expresso_ro { } {
	flash_lm4 ../../../build/expresso/ec.RO.flat 0
}

proc flash_expresso_rw { } {
	flash_lm4 ../../../build/expresso/ec.RW.bin 131072
}

proc flash_clapper_ro { } {
	flash_lm4 ../../../build/clapper/ec.RO.flat 0
}

proc flash_clapper_rw { } {
	flash_lm4 ../../../build/clapper/ec.RW.bin 131072
}

proc flash_winky { } {
	flash_lm4 ../../../build/winky/ec.bin 0
}

proc flash_winky_ro { } {
	flash_lm4 ../../../build/winky/ec.RO.flat 0
}

proc flash_winky_rw { } {
	flash_lm4 ../../../build/winky/ec.RW.bin 131072
}

proc flash_kip { } {
	flash_lm4 ../../../build/kip/ec.bin 0
}

proc flash_kip_ro { } {
	flash_lm4 ../../../build/kip/ec.RO.flat 0
}

proc flash_kip_rw { } {
	flash_lm4 ../../../build/kip/ec.RW.bin 131072
}

proc flash_swanky { } {
	flash_lm4 ../../../build/swanky/ec.bin 0
}

proc flash_swanky_ro { } {
	flash_lm4 ../../../build/swanky/ec.RO.flat 0
}

proc flash_swanky_rw { } {
	flash_lm4 ../../../build/swanky/ec.RW.bin 131072
}

proc flash_gnawty { } {
	flash_lm4 ../../../build/gnawty/ec.bin 0
}

proc flash_gnawty_ro { } {
	flash_lm4 ../../../build/gnawty/ec.RO.flat 0
}

proc flash_gnawty_rw { } {
	flash_lm4 ../../../build/gnawty/ec.RW.bin 131072
}

proc flash_tiny { } {
	flash_lm4 ../../../build/tiny/ec.bin 0
}

proc flash_tiny_ro { } {
	flash_lm4 ../../../build/tiny/ec.RO.flat 0
}

proc flash_tiny_rw { } {
	flash_lm4 ../../../build/tiny/ec.RW.bin 131072
}

proc flash_candy { } {
	flash_lm4 ../../../build/candy/ec.bin 0
}

proc flash_candy_ro { } {
	flash_lm4 ../../../build/candy/ec.RO.flat 0
}

proc flash_candy_rw { } {
	flash_lm4 ../../../build/candy/ec.RW.bin 131072
}

# link has pstate in last sector
proc unprotect_link { } {
	reset halt
	flash erase_sector 0 254 255
	reset
}

# Peppy/falco have pstate following RO
proc unprotect_peppy { } {
	reset halt
	flash erase_sector 0 126 127
	reset
}

# Boot a software using internal RAM only

proc ramboot_lm4 {path} {
	reset halt
	load_image $path 0x20000000 bin
	reg 15 0x20000400
	resume
}

proc ramboot_link { } {
	ramboot_lm4 ../../../build/link/ec.RO.flat
}

proc ramboot_bds { } {
	ramboot_lm4 ../../../build/bds/ec.RO.flat
}

proc flash_emerged_link { } {
	set firmware_image ../../../../../../chroot/build/link/firmware/ec.bin

	flash_lm4 $firmware_image 0
}
