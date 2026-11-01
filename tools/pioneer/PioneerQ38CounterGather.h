/* SPDX-License-Identifier: MIT */
#include "PioneerQ38CounterGatherRole.h"
#ifndef PIONEER_Q38_COUNTER_GATHER_H
#define PIONEER_Q38_COUNTER_GATHER_H
#ifdef PIONEER_Q38_COUNTER_GATHER_IMAGE
#include "PioneerQ38TidQuiet.h"
#include "PioneerQ38MixedWorkerLayout.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
extern "C" void q38_mixed_root_packet_seal(const uint32_t*);
// Both actual callers already own this same 16KiB scratch reservation.
// Eight distinct cache lines preserve remote/local low-five-bit alignment.
// No wider MMIO, arbitrary address, write request or worker release exists.
namespace PioneerQ38CounterGather {
static inline volatile uint32_t* Words(unsigned at){return reinterpret_cast<volatile uint32_t*>(at);}
static inline unsigned Offset(unsigned i){
 static const uint32_t offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
 return offsets[i];
}
enum {Scratch=0x78000,Bytes=256};
static __attribute__((noinline,optimize("Os"))) void Read(const uint32_t* frame,uint32_t* tracked,unsigned w,uint32_t* out){
 if(w>=119 || !tracked || !out || tracked==out)scalar_fault_park();
 q38_mixed_root_packet_seal(frame);
 // FIFO mode is a CURRENT sealed source, never enabled/changed here.
 // Full quiet proves all16 slots free BEFORE the bounded eight submits.
 if(!PioneerQ38TidFullQuiet())scalar_fault_park();
 uint32_t expected[8],command[10];
 for(unsigned i=0;i<8;i++){
  if(*Words(0xffb20000U+Offset(i))!=tracked[i])scalar_fault_park();
  expected[i]=tracked[i]+(((0x4fU>>i)&1U)*8U);
 }
 if(expected[6]!=expected[2])scalar_fault_park();
 const unsigned route=PioneerQ38MixedWorkerLayout::Peer(w+1);
 command[1]=0;command[2]=route;command[4]=0;command[5]=129;
 command[6]=0;command[7]=0;command[8]=4;command[9]=0;
 for(unsigned i=0;i<8;i++){
  // Keep ALL16 live source checks before EACH request. Earlier read-only
  // metadata may precede a late fault;no result,release or commit follows.
  q38_mixed_root_packet_seal(frame);
  const unsigned remote=0xffb20000U+Offset(i),local=Scratch+i*32+(remote&31);
  command[0]=remote;command[3]=local;
  for(unsigned j=0;j<10;j++)Words(0xffb20000U)[j]=command[j];
  for(unsigned j=0;j<10;j++)if(Words(0xffb20000U)[j]!=command[j])scalar_fault_park();
  asm volatile("fence" ::: "memory");
  *Words(0xffb20040U)=1;(void)*Words(0xffb20040U);
  asm volatile("fence" ::: "memory");
 }
 for(unsigned poll=0;poll<65536;poll++){
  bool initiators=true;
  for(unsigned i=0;i<4;i++)if(*Words(0xffb20040U+i*0x800))initiators=false;
  // TEST this returned value BEFORE issuing the first counter read.
  // Queued commands are not represented in transaction/master counters.
  const unsigned available=*Words(0xffb20064U);
  if(*Words(0xffb20068U))scalar_fault_park();
  if(!initiators || available!=0x10101010U)continue;
  bool done=true;
  for(unsigned i=0;i<8;i++)if(*Words(0xffb20000U+Offset(i))!=expected[i])done=false;
  if(!done)continue;
  q38_mixed_root_packet_seal(frame);
  if(!PioneerQ38TidFullQuiet())scalar_fault_park();
  for(unsigned i=0;i<10;i++)if(Words(0xffb20000U)[i]!=command[i])scalar_fault_park();
  volatile uint32_t* retained=tracked;
  for(unsigned i=0;i<8;i++){
   out[i]=*Words(Scratch+i*32+(Offset(i)&31));
   retained[i]=expected[i];
  }
  return;
 }
 scalar_fault_park(); // UNKNOWN: no retry/reset/cleanup/later packet grant.
}
}
#endif
#endif
