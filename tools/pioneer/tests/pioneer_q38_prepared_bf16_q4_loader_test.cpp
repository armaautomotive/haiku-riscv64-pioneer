/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_prepared_bf16_q4_compare_machine.h"
using NewLoader=PioneerQ38PreparedBf16Q4StreamLoader;
static bool DrivePrepared(NewLoader& l,PreparedCompareIO& io){
 if(!l.Begin(io,io.width,io.route) || !l.Open(io,0,0))return false;
 for(unsigned s=0;s<3;s++)if(!l.Slice(io))return false;
 if(!l.Wait(io))return false;
 for(unsigned b=0;b<io.width/256;b++){
  const unsigned buffer=b&1;const bool next=b+1<io.width/256;
  if(!l.Acquire(b,buffer))return false;io.reader=buffer;
  if(next && !l.Open(io,b+1,buffer^1))return false;
  for(unsigned g=0;g<8;g++){
   if(next && g<3 && !l.Slice(io))return false;
   assert(io.reader==buffer && io.ready[buffer] && io.copies[buffer]==3 && io.dataBlock[buffer]==b);
   for(unsigned i=0;i<40;i++)io.Progress();
  }
  io.reader=2;if(!l.Release(b,buffer) || (next && !l.Wait(io)))return false;
 }
 return l.Complete(io);
}
static void NoMorePrepared(NewLoader& l,PreparedCompareIO& io){
 const unsigned before=io.events;
 assert(!l.Begin(io,io.width,io.route) && !l.Open(io,0,0) && !l.Slice(io) && !l.Wait(io)
  && !l.Acquire(0,0) && !l.Release(0,0) && !l.Complete(io) && io.events==before);
}
int main(){
 static_cast<void>(&NoMore);unsigned passed=0,refused=0;
 for(unsigned k:{5120U,17408U})for(unsigned x:{17U,18U})for(unsigned y:{12U,15U,18U,21U}){
  PreparedCompareIO io(k,nullptr,x|(y<<6));io.prepared=true;const auto original=io.dram;NewLoader l;
  assert(DrivePrepared(l,io));NoMorePrepared(l,io);
  assert(io.issues==k/256*3 && io.completed==io.issues && io.queued.empty() && io.responses.empty()
   && io.maxQueued<=16 && io.overlap && io.headerWrites==96 && io.headerReads==96 && io.dram==original);
  std::vector<uint8_t> expected(io.l1.size(),0xa7);
  for(unsigned b=0;b<2;b++){
   const unsigned block=k/256-2+b,base=b*0x10000;
   const unsigned source[]={0x80000+block*5632,0x80000+block*5632+4608,0x100000+block*512};
   const unsigned target[]={0x90000+base,0x8c000+base,0x96000+base},bytes[]={4608,1024,512};
   for(unsigned p=0;p<3;p++){memset(expected.data()+target[p]-64,0,64);memcpy(expected.data()+target[p],original.data()+source[p],bytes[p]);}
  }
  assert(io.l1==expected);passed++;
 }
 for(unsigned fault:{1U,2U,3U,4U,5U,6U,7U,8U,9U,10U,11U,12U,13U,16U}){
  PreparedCompareIO io(5120,nullptr);io.prepared=true;io.fault=fault;NewLoader l;
  assert(!DrivePrepared(l,io));NoMorePrepared(l,io);refused++;
 }
 for(unsigned drop=0;drop<10;drop++){
  PreparedCompareIO io(5120,nullptr);io.prepared=true;io.dropCommand=drop;NewLoader l;
  assert(!DrivePrepared(l,io) && !io.issues);NoMorePrepared(l,io);refused++;
 }
 for(unsigned bad:{0U,256U,5119U,5121U,6144U,17409U,~0U}){
  PreparedCompareIO io(5120,nullptr);io.prepared=true;NewLoader l;
  assert(!l.Begin(io,bad,io.route) && !io.events);NoMorePrepared(l,io);refused++;
 }
 for(unsigned bad:{0U,129U,16U|(12U<<6),17U|(14U<<6),~0U}){
  PreparedCompareIO io(5120,nullptr);io.prepared=true;NewLoader l;
  assert(!l.Begin(io,5120,bad) && !io.events);NoMorePrepared(l,io);refused++;
 }
 for(unsigned misuse=0;misuse<8;misuse++){
  PreparedCompareIO io(5120,nullptr);io.prepared=true;NewLoader l;assert(l.Begin(io,5120,io.route));
  if(misuse==0)assert(!l.Open(io,1,0));if(misuse==1)assert(!l.Acquire(0,0));if(misuse==2)assert(!l.Release(0,0));
  if(misuse==3)assert(!l.Wait(io));if(misuse==4)assert(!l.Slice(io));if(misuse==5)assert(!l.Complete(io));
  if(misuse==6){assert(l.Open(io,0,0));assert(!l.Open(io,0,1));}if(misuse==7){assert(l.Open(io,0,0));assert(!l.Wait(io));}
  NoMorePrepared(l,io);refused++;
 }
 assert(passed==16 && refused==44);
 printf("PASS_PREPARED_BF16_LOADER valid%u refused%u wholeL1UnownedBytesExact_sameQueueCounterCreditProof_threeContiguousReads96Flits_perK256_twoBufferOverlap_noScalarPayload_noDramWrite_noIOAfterTerminal;NOT_NATIVE_OR_HARDWARE_TIMING\n",passed,refused);
}
