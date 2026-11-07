/* SPDX-License-Identifier: MIT */
#include "PioneerQ38TidBoundaryRole.h"
#include "PioneerQ38HybridQueuedCountersRole.h"
#ifndef PIONEER_Q38_TID_QUIET_H
#define PIONEER_Q38_TID_QUIET_H
#if defined(PIONEER_Q38_TID_BOUNDARY_IMAGE) || defined(PIONEER_Q38_HYBRID_QUEUED_COUNTERS_IMAGE)
// Blackhole NoC/MemoryMap.md:CMD_CTRL==0 alone does NOT prove FIFO drain.
// This BUILD requires the genuine sealed NIU_CFG_0 FIFO-enable bit. It
// NEVER changes that mode or clears/recaptures counters. Every submitted
// command retains PACKET_TAG==0 and all existing expected-counter checks.
static __attribute__((noinline)) bool PioneerQ38TidInitiatorsQuiet(){
 for(unsigned i=0;i<4;i++)if(*reinterpret_cast<const volatile uint32_t*>(0xffb20040U+i*0x800))return false;
 // Return and TEST availability BEFORE issuing any transaction-counter
 // read;FIFO entries are not yet represented in those counters.
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb20064U)!=0x10101010U)return false;
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb20068U))return false;
 return true;
}
#ifdef PIONEER_Q38_TID_BOUNDARY_IMAGE
static __attribute__((noinline)) bool PioneerQ38TidPacketQuiet(){
 if(!PioneerQ38TidInitiatorsQuiet())return false;
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb20240U)&255)return false;
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb20280U)&255)return false;
 return true; // Only actual TID0 traffic;NOT a full inactive-ID audit.
}
#endif
static __attribute__((noinline)) bool PioneerQ38TidFullQuiet(){
 if(!PioneerQ38TidInitiatorsQuiet())return false;
 for(unsigned i=0;i<32;i++)if(*reinterpret_cast<const volatile uint32_t*>(0xffb20240U+i*4)&255)return false;
 return true;
}
#endif
#endif
