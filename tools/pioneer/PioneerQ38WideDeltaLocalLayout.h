/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_WIDE_GDN_RETURNING_IMAGE
#include "PioneerQ38WideGdnReturningImageRole.h"
#endif
#if defined(PIONEER_Q38_WIDE_DELTA_LOCAL_LAYOUT_H) && (defined(PIONEER_Q38_WIDE_GDN_RETURNING_IMAGE) != defined(PIONEER_Q38_WIDE_GDN_RETURNING_LAYOUT_BOUND))
#error Standalone and returning wide-GDN layouts cannot change after binding
#endif
#if defined(PIONEER_Q38_WIDE_GDN_RETURNING_LAYOUT_BOUND) && !defined(PIONEER_Q38_WIDE_DELTA_LOCAL_LAYOUT_H)
#error Private wide-GDN returning-layout witness cannot be supplied by a caller
#endif
#if (defined(PIONEER_Q38_WIDE_DELTA_LOCAL_IMAGE) + defined(PIONEER_Q38_WIDE_DELTA_INSTRUCTION_TEST)) != 1 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Separate offline local wide-delta layout only; no native ownership grant
#endif
#ifndef PIONEER_Q38_WIDE_DELTA_LOCAL_LAYOUT_H
#define PIONEER_Q38_WIDE_DELTA_LOCAL_LAYOUT_H
namespace PioneerQ38WideDeltaLocalLayout {
#ifdef PIONEER_Q38_WIDE_GDN_RETURNING_IMAGE
#define PIONEER_Q38_WIDE_GDN_RETURNING_LAYOUT_BOUND 1
// Preserve the existing returning service's74700 mailbox DATA format.
// Shared magic is NOT code identity or permission to reuse its native owner.
enum { Code=0x72000,Descriptor=0x74700,Query=0x8e000,Key=0x8e200,
 Value=0x8e400,Decay=0x8e500,Beta=0x8e504,Delta=0x8e600,Output=0x8e800,
 Prior=0x90000,Next=0xa0000,Park=Code+0x38,FaultPark=Code+0x7c,
 Magic=0x56383351,Success=0x31574451,Fault=0x46574451 };
#else
enum { Code=0x72000,Descriptor=0x74000,Query=0x8e000,Key=0x8e200,
 Value=0x8e400,Decay=0x8e500,Beta=0x8e504,Delta=0x8e600,Output=0x8e800,
 Prior=0x90000,Next=0xa0000,Park=Code+0x38,FaultPark=Code+0x7c,
 Magic=0x57383351,Success=0x31574451,Fault=0x46574451 };
#endif
// The256B Delta use is NEW. It fits inside QTP3's already captured padding,
// but that GEOMETRY alone grants no new image/owner or execution permission.
// Dst remains EXACTLY512x16 FP32;row512 is never scratch.
static_assert(Delta>=Beta+4 && Delta+64*4<=Output,"distinct local delta scratch");
}
#endif
