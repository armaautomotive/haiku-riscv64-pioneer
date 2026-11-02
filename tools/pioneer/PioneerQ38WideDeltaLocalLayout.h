/* SPDX-License-Identifier: MIT */
#if (defined(PIONEER_Q38_WIDE_DELTA_LOCAL_IMAGE) + defined(PIONEER_Q38_WIDE_DELTA_INSTRUCTION_TEST)) != 1 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Separate offline local wide-delta layout only; no native ownership grant
#endif
#ifndef PIONEER_Q38_WIDE_DELTA_LOCAL_LAYOUT_H
#define PIONEER_Q38_WIDE_DELTA_LOCAL_LAYOUT_H
namespace PioneerQ38WideDeltaLocalLayout {
enum { Code=0x72000,Descriptor=0x74000,Query=0x8e000,Key=0x8e200,
 Value=0x8e400,Decay=0x8e500,Beta=0x8e504,Delta=0x8e600,Output=0x8e800,
 Prior=0x90000,Next=0xa0000,Park=Code+0x38,FaultPark=Code+0x7c,
 Magic=0x57383351,Success=0x31574451,Fault=0x46574451 };
// The256B Delta extent is NEW. No old native L1 allowlist or image owner
// selects it. Dst remains EXACTLY512x16 FP32;row512 is never scratch.
static_assert(Delta>=Beta+4 && Delta+64*4<=Output,"distinct local delta scratch");
}
#endif
