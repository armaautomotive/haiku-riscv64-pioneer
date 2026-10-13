/* Copyright 2026. Distributed under the terms of the MIT License. */
OUTPUT_ARCH(riscv)
ENTRY(_probe_image_start)
SECTIONS
{
	. = DEFINED(PROBE_LINK_BASE) ? PROBE_LINK_BASE : 0x2200000;
	.text : { KEEP(*(.text.head)) *(.text .text.*) }
	.rodata : { *(.rodata .rodata.*) }
	.data ALIGN(4096) : { *(.data.stack) *(.data .data.*) . = ALIGN(4096); }
	_probe_image_end = .;
	.bss (NOLOAD) : { *(.bss .bss.* COMMON) }
	.riscv.attributes 0 : { *(.riscv.attributes) }
	/DISCARD/ : { *(.comment .note* .eh_frame*) }
}
ASSERT(_probe_entry - _probe_image_start == 64, "Image header must be 64 bytes")
ASSERT(SIZEOF(.bss) == 0, "Probe must not require unrepresented BSS")
ASSERT(_probe_image_end - _probe_image_start <= 131072, "Probe exceeds 128 KiB limit")
