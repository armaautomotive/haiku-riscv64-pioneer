/* SPDX-License-Identifier: MIT */
// TEST endpoints only. No native ownership,model products or processor history.
#define PIONEER_RESIDENT_READ_MOCK_ONLY
#define PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE 1
#include "pioneer_resident_read_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerComputeFleet.h"
#include <initializer_list>
#include <stdio.h>
int main(){
 unsigned checks=0;
 for(unsigned worker=0;worker<120;worker++){
  Mock io;io.nativePhysicalWindow=true;io.nativeWindowX=PioneerWorkerX(worker);io.nativeWindowY=PioneerWorkerY(worker);
  for(unsigned slot=0;slot<2;slot++){
   const uint32_t base=slot?0xffa00000U:0U;
   for(unsigned word=0;word<3;word++)io.DramTlbWrite(slot,word,BlackholeDramFixedTlbWord(base,io.nativeWindowX,io.nativeWindowY,false,word));
   assert(io.NativeWindowMatches(slot,base,1,2,false));io.Window(slot,base,1,2,false);checks++;
   // Every actual TLB bit remains checked. Selecting a physical expectation
   // never overwrites/repairs an actual register to make it match.
   for(unsigned word=0;word<3;word++)for(unsigned bit=0;bit<32;bit++){
    io.tlb[slot][word]^=1U<<bit;
    assert(!io.NativeWindowMatches(slot,base,1,2,false));checks++;
    io.tlb[slot][word]^=1U<<bit;
   }
   io.nativeWindowX^=1;assert(!io.NativeWindowMatches(slot,base,1,2,false));checks++;io.nativeWindowX^=1;
   io.nativeWindowY^=1;assert(!io.NativeWindowMatches(slot,base,1,2,false));checks++;io.nativeWindowY^=1;
   assert(!io.NativeWindowMatches(slot,base,1,2,true));checks++;
  }
  const unsigned writes=io.writes;
  for(uint32_t config:{0x6005U,0x16005U,0x6005U,0x14000U,0U,UINT32_MAX}){
   io.ResidentNocWrite(0x100,config);
   assert(io.source[0]==0x6005 && io.ResidentNocRead(0x100)==config
    && io.NativeFifoStatusWord(0x64)==((config&0x10000U)?0x10101010U:0U) && !io.NativeFifoStatusWord(0x68));
   checks++;
  }
  assert(io.writes==writes+6 && !io.requests && !io.postIssueWrites && !io.pauses);
 }
 printf("PASS native TEST endpoint contract/%u checks/120 physical endpoints/all6 TLB words+every bit+wrong coordinates/read-only/current NIU config not source baseline/FIFO enable-disable observations;NO native state/STOP/restore/END/hardware/Work/TPS grant\n",checks);
}
