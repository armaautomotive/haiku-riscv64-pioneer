/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FACTORED_Q4_COMPARE_MACHINE_H
#define PIONEER_Q38_FACTORED_Q4_COMPARE_MACHINE_H
#include "pioneer_q38_q4_factored_operator_machine.h"
#include "pioneer_q38_cached_fpu_machine.h" // existing unchanged scalar Step
#define PIONEER_Q4_COMPACT_STREAM_LOADER_ONLY
#include "pioneer_q38_q4_fpu_compact_stream_loader_test.cpp"
#include "../PioneerQ38FactoredQ4Reference.h"
#include "../PioneerQ38Q4KernelTiming.h"
struct ComparePipeline:FactoredOperatorMachine {
 StreamIO* bus=nullptr;bool old=true;mutable unsigned readerStart=0;
 bool RawPublishesLast()const override{return old;}
 unsigned OperandOffset()const override{return (((old?inputLoads:groups)/8)&1)*0x10000;}
 bool OperandReady(unsigned a,unsigned bytes)const override{
  if(a==0x97000)return bytes==1024;
  const unsigned block=(old?inputLoads:groups)/8,slot=block&1;
  const bool valid=bus && bus->ready[slot] && bus->copies[slot]==SL::Packets && bus->dataBlock[slot]==block
   && a>=SL::Raw+slot*0x10000 && a+bytes<=0x963c0+slot*0x10000;
  if(valid && a==SL::Metadata+slot*0x10000 && !(compactImports%2)){bus->reader=slot;readerStart=old?matrixOps:groups;}
  return valid && bus->reader==slot;
 }
 void Op(unsigned op){
  const unsigned code=op>>24,mode=(op>>16)&15,address=op&1023;
  if(old && op==0x42000091 && (config[84]&65535)/16-4<=128){Q4FpuPipelineMachine::Op(op);return;}
  if(old && config[1]!=0xa03c8000 && (code==0x70 || code==0x72) && mode==3){
   if(code==0x72){Q4FpuPipelineMachine::Op(op);return;}
   Need(!Pending() && !packPending && enabled && vector.activeLanes==UINT32_MAX && !(address&1) && !(op&0xfc00));
   const unsigned reg=(op>>20)&15;Need((reg==0 || reg==4) && address>=128 && address<256);cycle++;
   for(unsigned lane=0;lane<32;lane++)vector.lanes[reg][lane]=FloatDst((address&~3U)+lane/8,(lane&7)*2+bool(address&2));
   vector.ready[reg]=cycle+1;return;
  }
  FactoredOperatorMachine::Op(op);
 }
};
struct CompareState {ComparePipeline native;};
// Same canonical L1/physical Dst/source banks for BOTH recipes. No model
// reset,host K loop,descriptor normalization or injected learned result.
struct FactoredCompareRV32:CompareState,ServiceMachine {
 StreamIO bus;unsigned width,drainedGroups=0,acks=0,timerStores=0,clockReads=0;bool awaiting=false;
 std::vector<unsigned> csrPc;
 FactoredCompareRV32(const char* folder,unsigned k):ServiceMachine(folder,native.l1),bus(k,17|(12<<6),&native.l1),width(k){
  assert(code.size()==11792);native.bus=&bus;native.delay[0]=100003;native.delay[1]=7;native.packDelay=100019;
  for(unsigned off=0;off<code.size();off+=4)if((::Word(code,off)&127)==0x73)csrPc.push_back(0x70000+off);
  assert(csrPc.size()==3 && csrPc[1]==csrPc[0]+4 && csrPc[2]==csrPc[0]+8);
  unsigned frame[8]={0x31435151,1,k,bus.route,1,0,0,0};for(unsigned i=0;i<8;i++)Put(SL::Frame+i*4,frame[i]);
 }
 bool InlineTensix()const override{return true;}
 unsigned Index(unsigned a)override{
  if(Range(a,SL::Output,256+64)){assert(!(a&3));return 6200+(a-SL::Output)/4;}
  return ServiceMachine::Index(a);
 }
 unsigned ReadL1(unsigned a)const{return ::Word(native.l1,a);}
 void WriteL1(unsigned a,unsigned v){Put(a,v);if(Range(a,SL::Output,320))l1[Index(a)]=v;}
 void Progress()override{
  if(bus.reader<2 && !awaiting && native.Clean()
   && (native.old?native.matrixOps==native.readerStart+128:drainedGroups==native.readerStart+8))bus.reader=2;
  bus.Progress();
 }
 void Instruction(unsigned op)override{
  if(op==0xa2101816){assert(!gpr63 && !awaiting);awaiting=true;}
  else if(op==0x4500017e){assert(awaiting && !gpr63);acks++;native.Drain();drainedGroups=native.groups;gpr63=1;awaiting=false;}
  else native.Op(op);Progress();
 }
 void Store(unsigned a,unsigned v)override{
  assert(!(a&3));
  if(a==0xffe40000){Instruction(v);return;}if(a==0xffe000fc){assert(!v);gpr63=0;return;}
  if(Range(a,0xffef0000,224*4)){native.Config((a-0xffef0000)/4,v);Progress();return;}
  if(SL::HeaderWord(a) || Range(a,0xffb20000,0x2000)){bus.Write(a,v);return;}
  if(Range(a,0x6e000,8192) || Range(a,0x17ffc0,12) || Range(a,SL::Output,320)){
   if(Range(a,0x6e000,8192))minStore=std::min(minStore,a);
   if(Range(a,SL::Output,320)){
    assert(native.Clean() && !awaiting && bus.reader==2 && bus.queued.empty() && bus.responses.empty());
    if(Range(a,SL::Output,256))stores++;else timerStores++;
   }
   WriteL1(a,v);Progress();return;
  }
  if(a==0x87ff0){assert(native.old && stores==32 && bus.jobs==2 && native.Clean() && bus.reader==2);native.old=false;}
  native.Write(a,v);Progress();
 }
 unsigned Load(unsigned a)override{
  assert(!(a&3));if(a==0xffe000fc)return gpr63;
  if(Range(a,0xffef0000,224*4)){assert(!native.Pending() && !native.mathPending);return native.config[(a-0xffef0000)/4];}
  if(SL::HeaderWord(a) || Range(a,0xffb20000,0x2000))return bus.Read(a);
  const unsigned row=(a-0xffbd8000)/64;
  if((native.old && (Range(a,0xffbd8800,64) || Range(a,0xffbd8a00,64)))
   || (!native.old && (Range(a,0xffbd9400,64) || Range(a,0xffbd9600,64)))){
   assert(native.Clean() && !awaiting && bus.reader==2);native.outputReads++;return native.FloatDst(row,(a%64)/4);
  }
  assert(CodeAddress(a) || Range(a,0x6e000,8192) || Range(a,SL::Frame,32) || Range(a,0x17ffc0,12));return ReadL1(a);
 }
 void TimingStep(){
  const unsigned w=l1[Index(pc)];if((w&127)!=0x73){Machine::Step();return;}
  const unsigned part=clockReads%3;assert(pc==csrPc[part] && (w&0x000ff07f)==0x2073);
  assert((w>>20)==(part==1?0xb00U:0xb80U));const unsigned rd=(w>>7)&31;assert(rd);
  const uint64_t synthetic=UINT64_C(0x100000000)-12345+uint64_t(steps)*8;
  regs[rd]=part==1?unsigned(synthetic):unsigned(synthetic>>32);regs[0]=0;pc+=4;steps++;clockReads++;
 }
 bool Execute(){
  while(pc!=0x70038 && pc!=0x70058){
   const unsigned w=l1[Index(pc)];TimingStep();if(w==0x0ff0000f)bus.Fence();Progress();
   if(regs[2]){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);}
   assert(regs[27]==0x13579bdf && steps<StepLimit());
  }
  if(pc==0x70058){assert(ReadL1(0x17ffc8)==0x45435151);return false;}
  assert(ReadL1(0x17ffc0)==0x7000c && ReadL1(0x17ffc4)==(0x04000000|width) && ReadL1(0x17ffc8)==0x31435151);
  assert(native.Clean() && !awaiting && bus.reader==2 && bus.jobs==2 && stores==64 && timerStores==16 && clockReads==12);
  assert(bus.issues==2*(width/256)*SL::Packets && bus.completed==bus.issues && native.outputReads==64);
  return true;
 }
};
struct FactoredCompareData {
 std::vector<Q4PipelineFixture> blocks;std::array<unsigned,32> expected{};
 FactoredCompareData(unsigned width,unsigned trial){
  for(unsigned block=0;block<width/256;block++)blocks.emplace_back(trial+block);
  for(const auto& block:blocks)for(unsigned row=0;row<32;row++){
   float weights[256];assert(PioneerDecodeQ4K(block.original.data()+row*144,weights));
   for(unsigned first=0;first<256;first+=16){
    uint16_t a[16],b[16];for(unsigned k=0;k<16;k++){a[k]=uint16_t(Round(NativeBfpMachine::Bits(weights[first+k]))>>16);b[k]=block.input[first+k];}
    for(unsigned phase=0;phase<4;phase++)assert(PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,expected[row],expected[row]));
   }
  }
 }
 void Stage(StreamIO& io)const{
  for(unsigned block=0;block<blocks.size();block++){
   std::array<uint8_t,4608> packed;assert(PioneerQ38Q4FpuCompactData::Prepare(blocks[block].original.data(),4608,packed.data(),4608));
   memcpy(io.dram.data()+block*4608,packed.data(),4608);
   for(unsigned k=0;k<256;k++){
    const unsigned a=SL::DramInput-SL::DramWeight+block*512+k*2,b=blocks[block].input[k];
    io.dram[a]=uint8_t(b);io.dram[a+1]=uint8_t(b>>8);
   }
  }
  memset(io.l1.data()+SL::Zero-64,0,1088);
 }
};
#endif
