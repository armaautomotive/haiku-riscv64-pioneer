/* SPDX-License-Identifier: MIT */
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
enum { Descriptor=0x74000,Prior=0x90000,Next=0xa0000,
       Done=Code+0xc,Park=Code+0x38,
#ifdef PIONEER_Q38_DELTA_VECTOR_PROBE_IMAGE
       FaultPark=Code+0x7c,
#else
       FaultPark=Code+0x58,
#endif
       Success=0x31564451,Fault=0x46564451,Magic=0x56383351 };
}
#endif
