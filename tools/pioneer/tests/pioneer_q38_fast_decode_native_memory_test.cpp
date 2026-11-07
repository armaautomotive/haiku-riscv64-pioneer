/* SPDX-License-Identifier: MIT */
// Actual complete journal algorithm, MOCK PCI/FIFO/source/HOLD/L1 only.
// Exercise the new helper region, not an old lease or a physical launch.
#include "fast_decode_native_role_defines.inc"
#define main FullTokenMemoryOriginalMain
#include "pioneer_q38_token_memory_test.cpp"
#undef main
int main(){
 assert(TOKL::SavedBytes==53915328 && TOKP::FastDecodeProfile==1);
 // The unchanged positive test verifies every arbitrary original, all120
 // code views and the entire journal after install/readback/restore.
 char name[]="test";char* args[]={name,nullptr};assert(FullTokenMemoryOriginalMain(1,args)==0);
 unsigned cases=0;
 auto fault=[&](unsigned kind,unsigned phase,unsigned address){
  TokenMemoryFixture f;f.io.badKind=kind;f.io.badPhase=phase;f.io.badWorker=0;
  if(phase==3 || phase==4 || phase==5){
   unsigned index=0;while(index<TOKP::CodeWords(0) && TOKP::CodeAddress(0,index)!=address)index++;
   assert(index<TOKP::CodeWords(0));f.io.badOrdinal=index;
  }else f.io.badOrdinal=TokenMemoryIO::Index(0,address);
  if(phase<=4){assert(!f.PrepareMemory());if(phase<3)assert(!f.io.writes);}
  else{assert(f.PrepareMemory());assert(!f.CloseMemory());}
  f.Unknown();cases++;
 };
 for(unsigned address:{0x6c000U,0x6c5d4U}){
  fault(1,1,address);fault(2,2,address);
  fault(3,3,address);fault(5,3,address);fault(2,3,address);
  fault(1,4,address);fault(2,4,address);
  fault(2,5,address);
  fault(3,6,address);fault(5,6,address);fault(2,7,address);
 }
 printf("PASS fast decode journal: complete positive plus%u new-region first/last save,verify,install,silent-drop,readback,final-code,restore,final-original faults;sticky retain/no retry or following IO;MOCK endpoints,NOT physical launch or END\n",cases);
}
