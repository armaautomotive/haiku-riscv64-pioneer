/* SPDX-License-Identifier: MIT */
#include "PioneerQ38ManualTTSyncRole.h"
#ifndef PIONEER_Q38_MANUAL_TTSYNC_H
#define PIONEER_Q38_MANUAL_TTSYNC_H
#if !defined(PIONEER_Q38_MANUAL_TTSYNC_IMAGE) || !defined(__riscv) || __riscv_xlen != 32
#error Offboard RV32 returning-worker prototype only;no host/native helper
#endif
// Blackhole ManualTTSync.md:the STORE is discarded but orders earlier
// instruction-buffer stores. The LOAD waits for THIS Tensix thread's
// complete in-flight work;its returned bits are UNDEFINED,not an ACK.
// The dependent ANDI is mandatory:without it,another PCBuf/mailbox read
// can trigger the documented overlapping-load hardware bug. The final
// fence preserves the existing RISC-facing boundary before any consumer.
//
// REQUIRED independently owned launch invariant:PCBuf OverrideEn/Busy
// must NOT bypass completion. This helper does NOT establish that invariant
// or ownership,source,deadline,STOP,HOLD or cleanup. It cannot grant launch.
// A missing completion can BLOCK the hardware load indefinitely. There is
// no dishonest local polling bound;future native integration must retain
// its actual external watchdog and uncertain-resource quarantine.
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/BabyRISCV/ManualTTSync.md
static __attribute__((always_inline)) inline void PioneerQ38ManualTTSync()
{
 unsigned discarded;
 const unsigned address=0xffe80004U;
 asm volatile("sw zero,0(%1)\n\tlw %0,0(%1)\n\tandi %0,%0,0\n\tfence"
  // Keep conservative original call clobbers. This annotation alone is
  // NOT a proof that the compiler leaves whole callers byte-identical.
  : "=&r"(discarded) : "r"(address) : "a3", "memory");
}
#endif
