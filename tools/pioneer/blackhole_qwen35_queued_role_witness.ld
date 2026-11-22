/* SPDX-License-Identifier: MIT */
/* Link-only witness: no boot image, loader, reset, or service ownership. */
OUTPUT_ARCH(riscv)
SECTIONS {
    .text ROLE_ADDRESS : {
        *(.text .text.* .rodata .rodata.* .srodata .srodata.*)
        . = ALIGN(4);
    }
    .forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
    /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
    ASSERT(ROLE_ADDRESS < ROLE_LIMIT, "role code region must be nonempty")
    ASSERT(SIZEOF(.text) <= ROLE_LIMIT - ROLE_ADDRESS, "role code exceeds its planned region")
    ASSERT(SIZEOF(.forbidden) == 0, "role witness cannot use writable globals")
}
