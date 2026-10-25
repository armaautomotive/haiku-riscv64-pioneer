/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE
#include "PioneerQ38ResidentGdnOperatorImageRole.h"
#if PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE != 1 || !defined(PIONEER_Q38_RESIDENT_OPERATOR_IMAGE) || PIONEER_Q38_RESIDENT_OPERATOR_IMAGE != 1 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(_KERNEL_MODE)
#error Relocated GDN local mailbox requires exact separate returning service IMAGE
#endif
#endif
#if defined(PIONEER_Q38_DELTA_VECTOR_LAYOUT_H) && (defined(PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE) != defined(PIONEER_Q38_RESIDENT_GDN_LAYOUT_BOUND))
#error GDN local mailbox variant cannot change after layout binding
#endif
#ifndef PIONEER_Q38_DELTA_VECTOR_LAYOUT_H
#define PIONEER_Q38_DELTA_VECTOR_LAYOUT_H
// Fixed LOCAL addresses only. The probe variant fits wholly inside the
// existing saved Q38 L1 view; neither variant grants resident/DMA authority.
namespace PioneerQ38DeltaVectorLayout {
#ifdef PIONEER_Q38_DELTA_VECTOR_PROBE_IMAGE
enum { Code=0x72000,Query=0x8e000,Key=0x8e200,Value=0x8e400,
       Decay=0x8e500,Beta=0x8e504,Output=0x8e800 };
#else
enum { Code=0x64000,Query=0xb0000,Key=0xb0200,Value=0xb0400,
       Decay=0xb0500,Beta=0xb0504,Output=0xb0800 };
#endif
#ifdef PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE
#define PIONEER_Q38_RESIDENT_GDN_LAYOUT_BOUND 1
enum { Descriptor=0x74700 };
#else
enum { Descriptor=0x74000 };
#endif
enum { Prior=0x90000,Next=0xa0000,
       Done=Code+0xc,Park=Code+0x38,
#ifdef PIONEER_Q38_DELTA_VECTOR_PROBE_IMAGE
       FaultPark=Code+0x7c,
#else
       FaultPark=Code+0x58,
#endif
       Success=0x31564451,Fault=0x46564451,Magic=0x56383351 };
}
#endif
