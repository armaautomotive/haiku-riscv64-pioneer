OUTPUT_FORMAT("elf64-littleriscv", "elf64-littleriscv", "elf64-littleriscv")
OUTPUT_ARCH(riscv)

ENTRY(_riscv_start)
SECTIONS
{
	/* Allow board firmware to select the physical load address. */
	. = DEFINED(BOOT_LOADER_BASE) ? BOOT_LOADER_BASE : 0x80000000;

	/* text/read-only data */
  .text : {
    *(.text.head)
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t.*)
  }

	. = ALIGN(0x8);
	__ctor_list = .;
	.init_array : { *(.init_array) }
	__ctor_end = .;
	__dtor_list = .;
	.fini_array : { *(.fini_array) }
	__dtor_end = .;

	.rodata : { *(.rodata .rodata.*) }

	/* writable data  */
	. = ALIGN(0x1000);
	__data_start = .;
	.data : {
		*(.data)
		*(.data.*)
		*(.gnu.linkonce.d.*)
	}
	.got : { *(.got.plt) *(.got) }
	.dynamic : { gDynamic = .; *(.dynamic) }

	/* uninitialized data (in same segment as writable data) */
	__bss_start = .;
	.bss : { *(.bss .bss.* .sbss .sbss.*) *(COMMON) }

	. = ALIGN(0x1000);
	_end = . ;

	.bss.stack : { *(.stack) }
	. = ALIGN(0x1000);
	_riscv_image_end = .;

	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.comment .note .eh_frame .dtors .stab .stabstr .debug*) }
}
