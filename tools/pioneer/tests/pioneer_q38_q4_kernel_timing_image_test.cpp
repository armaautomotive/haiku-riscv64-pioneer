/* SPDX-License-Identifier: MIT */
#define PIONEER_Q4_COMPACT_STREAM_IMAGE_ONLY
#include "pioneer_q38_q4_fpu_compact_stream_image_test.cpp"
#include "../PioneerQ38Q4KernelTiming.h"

// Exactly the same arithmetic/NoC/FENCE machine as the untimed test.
// Only the three audited CSRRS words and16 timestamp stores are added.
// The clock is synthetic;neither its steps nor these outputs are timings.
struct Q4TimingRV32:CompactStreamRV32 {
 using T=PioneerQ38Q4KernelTiming;
 enum Clock {Monotonic,Unstable,Backward,Zero};
 Clock clock=Monotonic;unsigned clockReads=0,timerStores=0;
 std::vector<unsigned> csrPc;
 Q4TimingRV32(const char* directory,unsigned k):CompactStreamRV32(directory,k,true){
  for(unsigned i=0;i<code.size();i+=4){
   const unsigned word=::Word(code,i);
   if((word&0x7f)==0x73){
    assert((word&0x000ff07f)==0x2073 && ((word>>7)&31));
    csrPc.push_back(0x70000+i);
   }
  }
  assert(csrPc.size()==3 && csrPc[1]==csrPc[0]+4 && csrPc[2]==csrPc[0]+8);
  assert((::Word(code,csrPc[0]-0x70000)>>20)==0xb80
   && (::Word(code,csrPc[1]-0x70000)>>20)==0xb00
   && (::Word(code,csrPc[2]-0x70000)>>20)==0xb80);
  WriteL1(SL::Frame,0x31544351); // Actual timed wire frame,not a ready flag.
 }
 unsigned Index(unsigned a)override{
  if(Range(a,T::Address,T::Words*4)){assert(!(a&3));return 4172+(a-T::Address)/4;}
  return CompactStreamRV32::Index(a);
 }
 void Store(unsigned a,unsigned value)override{
  if(!Range(a,T::Address,T::Words*4)){CompactStreamRV32::Store(a,value);return;}
  assert(!(a&3) && native.Clean() && !awaiting && bus.queued.empty() && bus.responses.empty());
  // Canonical same L1,not a detached timer/result supplied to the guest.
  for(unsigned i=0;i<4;i++)native.l1[a+i]=uint8_t(value>>(i*8));
  l1[Index(a)]=value;timerStores++;Progress();
 }
 void TimingStep(){
  const unsigned word=l1[Index(pc)];
  if((word&0x7f)!=0x73){Machine::Step();return;}
  const unsigned part=clockReads%3;
  assert(pc==csrPc[part] && (word&0x000ff07f)==0x2073);
  const unsigned rd=(word>>7)&31;assert(rd);
  // Cross the 32-bit boundary over a request. This is not a hardware clock.
  uint64_t cycle=UINT64_C(0x100000000)-12345+uint64_t(steps)*8;
  if(clock==Zero)cycle=0;
  if(clock==Backward && ((clockReads/3)&1))cycle=1;
  unsigned value=part==1?unsigned(cycle):unsigned(cycle>>32);
  if(clock==Unstable && part==2)value++;
  regs[rd]=value;regs[0]=0;pc+=4;steps++;clockReads++;
  assert(steps<=StepLimit());
 }
 bool Execute(){
  while(pc!=0x70038 && pc!=0x70058){
   const unsigned instruction=l1[Index(pc)];TimingStep();
   if(instruction==0x0ff0000f)bus.Fence();Progress();
   if(regs[2])stackSeen=true;
   if(stackSeen){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);}
  }
  assert(l1[Index(pc)]==0x0000006f && minStore>=0x6e000);
  if(pc==0x70058){assert(ReadL1(0x17ffc8)==0x45544351);return false;}
  assert(ReadL1(0x17ffc0)==0x7000c && ReadL1(0x17ffc4)==(0x04000000|width) && ReadL1(0x17ffc8)==0x31544351);
  assert(native.Clean() && stores==64 && native.outputReads==64 && timerStores==16
   && native.zeroLoads==10 && native.extraZeroLoads==14 && native.compactImports==4*(width/256)
   && !native.metadataLoads && native.matrixOps==2*(width/256)*128
   && bus.jobs==2 && bus.issues==2*(width/256)*SL::Packets && bus.completed==bus.issues
   && bus.queued.empty() && bus.responses.empty() && bus.overlap);
  for(unsigned pass=0;pass<2;pass++){
   uint32_t record[8];for(unsigned i=0;i<8;i++)record[i]=ReadL1(T::Address+pass*32+i*4);
   assert(T::State(record,width,pass)==(clock==Monotonic?1U:2U));
  }
  assert(clockReads==(clock==Unstable?48U:12U));
  return true;
 }
};

#ifndef PIONEER_Q4_KERNEL_TIMING_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3);static_cast<void>(&NoMore);setvbuf(stdout,nullptr,_IOLBF,0);
 const unsigned k=unsigned(strtoul(argv[2],nullptr,10));assert(SL::Width(k));
 CompactStreamData data(k,0);
 for(auto mode:{Q4TimingRV32::Monotonic,Q4TimingRV32::Unstable,Q4TimingRV32::Backward,Q4TimingRV32::Zero}){
  auto image=std::make_unique<Q4TimingRV32>(argv[1],k);image->clock=mode;data.Stage(image->bus);
  assert(image->Execute() && image->Matches(data.expected));
  printf("PASS_Q4_TIMING_ACTUAL_LINKED_RV32 width%u clockCase%u exactWords64 csrReads%u timerStores16 stack%u;SYNTHETIC_CLOCK_NOT_CARD_TIMING\n",
   k,unsigned(mode),image->clockReads,0x70000-image->minSP);
 }
}
#endif
