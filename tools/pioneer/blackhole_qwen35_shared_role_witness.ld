/* SPDX-License-Identifier: MIT */
/* Offline-only link witness: no boot vector, reset or physical owner. */
OUTPUT_ARCH(riscv)
SECTIONS {
    .shim ROLE_SHIM_ADDRESS : {
        *(.text.qwen35_shared_reader_start .text.qwen35_shared_writer_start)
        . = ALIGN(4);
    }
    .text ROLE_ADDRESS : {
        *(.text .text.* .rodata .rodata.* .srodata .srodata.*)
        . = ALIGN(4);
    }
    .forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
    /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
    ASSERT(ROLE_SHIM_ADDRESS < ROLE_SHIM_LIMIT, "shim region must be nonempty")
    ASSERT(SIZEOF(.shim) > 0 && SIZEOF(.shim) <= ROLE_SHIM_LIMIT - ROLE_SHIM_ADDRESS,
        "startup shim exceeds its planned region")
    ASSERT(ROLE_ADDRESS < ROLE_LIMIT && SIZEOF(.text) <= ROLE_LIMIT - ROLE_ADDRESS,
        "role code exceeds its planned region")
    ASSERT(SIZEOF(.forbidden) == 0, "role witness cannot use writable globals")
}
