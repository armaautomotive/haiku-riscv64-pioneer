/* SPDX-License-Identifier: MIT */
/* Q4 full-offload ROOT: preserve all original fixed regions while placing
 * cold control routines in the unused SRAM gap immediately after STOP. */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .rootspill 0x60008 : {
  *(.text._ZN11GlobalChainL7RunHeadEv)
  *(.text._ZN23PioneerQ38TokenMetadataL3GdnEPjjjj)
  *(.text._ZN29PioneerQ38PreciseWorkerLayout8JobShapeEPKjS1_jj)
  *(.text._ZN11GlobalChainL5ScopeEb)
  *(.text._ZN11GlobalChainL8MetadataEjj)
  *(.text._ZN34PioneerQ38MixedLiveBlockRootLayoutL8RunShapeEPKj)
  *(.text.q38_token_cancellation_point)
  *(.text._ZN11GlobalChainL8FeedbackEPKjjPj)
  *(.text._ZN11GlobalChainL4WaitEPKjjb)
  *(.text._ZL24PioneerQ38BlockFfnRecipePKjS0_S0_PA8_S_S0_.constprop.0)
  *(.text._ZL31PioneerQ38DenseProjectionRecipePKjS0_S0_PA8_S_S0_b.constprop.0)
  *(.text._ZL30PioneerQ38MixedRecurrentRecipePKjS0_S0_PA8_S_S0_.constprop.0)
  *(.text._ZN11GlobalChainL11CommitStateEPKjb)
  *(.text._ZN11GlobalChainL12PublishTokenEPKjjjb)
  *(.conv.cancel)
  . = ALIGN(4);
 }
 .text 0x40000 : { KEEP(*(.text.start)) *(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .terminal 0x60000 : { KEEP(*(.terminal)) }
 .conv 0x64000 : { *(.conv*) . = ALIGN(4); }
 .forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4 && scalar_entry==0x40000,"exact full-offload entry")
 ASSERT(SIZEOF(.text)<=0x20000 && SIZEOF(.conv)<=0x8000,"fixed ROOT/Conv bounds")
 ASSERT(SIZEOF(.terminal)==8 && scalar_fault_park==0x60000 && q38_global_chain_stopped==0x60004,"unchanged STOP ABI")
 ASSERT(SIZEOF(.rootspill)>0 && SIZEOF(.rootspill)<=0x3ff8,"bounded adjacent root code span")
 ASSERT(SIZEOF(.forbidden)==0,"no writable globals/GOT")
}
