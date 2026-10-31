/* SPDX-License-Identifier: MIT */
// HOST byte endpoints ONLY. Never a native journal,ready flag or launch grant.
#if !defined(PIONEER_Q38_NATIVE_PUSHED_FABRIC_TEST) || PIONEER_Q38_NATIVE_PUSHED_FABRIC_TEST != 1 \
 || !defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) || PIONEER_Q38_TOKEN_PUSHED_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE)
#error Explicit offline pushed byte-fabric role required;never device/Work code
#endif
#if defined(_KERNEL_MODE) && (!defined(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1)
#error Only the simulated compile-only native fixture may import pushed TEST endpoints
#endif
#ifndef PIONEER_Q38_NATIVE_PUSHED_TEST_FABRIC_H
#define PIONEER_Q38_NATIVE_PUSHED_TEST_FABRIC_H
#include <array>
#include <cassert>
#include "../PioneerQ38PushedWorkerLayout.h"
struct Q38NativePushedTestFabric {
 using Word=uint32_t&(*)(void*,unsigned,uint32_t);
 void* const context;const Word word;
 std::array<uint32_t,4> slave={{0xfffffff0U,0xfffffff1U,0xfffffff2U,0xfffffff3U}};
 std::array<unsigned,4> tags={{~0U,~0U,~0U,~0U}};
 std::array<std::array<uint32_t,4>,4> cache={};
 std::array<unsigned,119> delivered={};
 unsigned messages=0,hits=0,misses=0,stale=0,fences=0;
 unsigned hostWrites=0;std::array<uint32_t,3> hostPosted={};
 Q38NativePushedTestFabric(void* c,Word w):context(c),word(w){assert(c && w);}
 static bool Contains(unsigned address){return address>=PioneerQ38PushedWorkerLayout::Mailbox
  && address-PioneerQ38PushedWorkerLayout::Mailbox<119*256;}
 static unsigned CounterOffset(unsigned i){const unsigned offsets[]={0x2f0,0x2e0,0x2e8,0x2c4};assert(i<4);return offsets[i];}
 void Fence(){tags.fill(~0U);fences++;}
 uint32_t Read(unsigned address){
  assert(Contains(address) && !(address&3));const unsigned line=address&~15U,index=(address>>4)&3;
  if(tags[index]!=line){tags[index]=line;misses++;for(unsigned i=0;i<4;i++)cache[index][i]=word(context,0,line+i*4);}
  else hits++;
  return cache[index][(address>>2)&3];
 }
 void Store(unsigned address,uint32_t value){
  assert(Contains(address) && !(address&3));word(context,0,address)=value;
  if(tags[(address>>4)&3]==(address&~15U))tags[(address>>4)&3]=~0U;
 } // the actual ROOT store invalidates its matching resident line,not all L0
 void IncomingHostWord(unsigned address,uint32_t value,bool posted){
  assert(address<0x180000 && !(address&3));word(context,0,address)=value;hostWrites++;
  if(posted)for(auto& counter:hostPosted)counter++;
  else for(auto& counter:slave)counter++;
 } // one actual modeled host RAW32 packet:posted has no nonposted counter/ACK event
 void Incoming(unsigned worker,const uint32_t* packet,const uint32_t* raw){
  namespace W=PioneerQ38PushedWorkerLayout;
  assert(worker && worker<=119 && packet && raw);
  const unsigned count=packet[8],slot=W::Slot(worker),ordinal=delivered[worker-1];
  const uint32_t expected[]={W::PushBuffer,0,W::Peer(worker),slot,0,W::Root,0,0x212,(ordinal&1)?4U:256U,0};
  for(unsigned i=0;i<10;i++)assert(packet[i]==expected[i]);
  assert(!word(context,0,slot) && raw[0]==((ordinal&1)?unsigned(W::ReceiptMagic):0U));
  for(unsigned i=0;i<count/4;i++){
   const unsigned address=slot+i*4,index=(address>>4)&3;
   if(tags[index]==(address&~15U) && cache[index][(address>>2)&3]!=raw[i])stale++;
   word(context,0,address)=raw[i]; // a remote write deliberately does NOT invalidate ROOT's L0
  }
  slave[0]++;slave[1]+=(count+63)/64;slave[2]++;slave[3]++;messages++;delivered[worker-1]++;
 } // actual raw NoC payload/commit delivery+ACK events ONLY;no processor/private state inputs
};
#endif
