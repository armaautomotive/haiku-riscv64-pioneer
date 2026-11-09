/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x70000 : { KEEP(*(.text.start)) *(.text .text.*) *(.rodata .rodata.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4,"probe boot must be one word")
 ASSERT(SIZEOF(.text)<=8192,"probe exceeds private 8KiB ROM")
 ASSERT(srca_probe_park<0x72000,"probe park outside private ROM")
 ASSERT(srca_probe_fault_park<0x72000,"probe fault park outside private ROM")
}
