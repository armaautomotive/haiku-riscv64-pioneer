/* SPDX-License-Identifier: MIT */
/* Fixed-address restricted qualification shim; no other code or data. */
OUTPUT_ARCH(riscv)
SECTIONS {
    .shim ROLE_SHIM_ADDRESS : {
        *(.text.qwen35_restore_probe_reader_start .text.qwen35_restore_probe_writer_start)
        . = ALIGN(4);
    }
    .forbidden : { *(.text .text.* .rodata .rodata.* .data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
    /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
    ASSERT(ROLE_SHIM_ADDRESS < ROLE_SHIM_LIMIT, "invalid probe shim range")
    ASSERT(SIZEOF(.shim) > 0 && SIZEOF(.shim) <= ROLE_SHIM_LIMIT - ROLE_SHIM_ADDRESS,
        "probe shim exceeds its planned range")
    ASSERT(SIZEOF(.forbidden) == 0, "probe shim has unexpected code or data")
}
