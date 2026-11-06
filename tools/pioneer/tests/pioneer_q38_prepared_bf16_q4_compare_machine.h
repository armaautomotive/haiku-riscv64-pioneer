/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PREPARED_BF16_Q4_COMPARE_MACHINE_H
#define PIONEER_Q38_PREPARED_BF16_Q4_COMPARE_MACHINE_H
#include "pioneer_q38_prepared_bf16_q4_machine.h"
#include "pioneer_q38_factored_q4_compare_machine.h"
#include "../PioneerQ38PreparedBf16Q4StreamLoader.h"
#include "../PioneerQ38PreparedBf16Q4BlockData.h"
#include "../PioneerQ38PreparedBf16Q4Reference.h"
using PL=PioneerQ38PreparedBf16Q4StreamLayout;
// Keep existing register/counter/queue/credit observer. Only the DISTINCT
// new three-packet geometry/header set is decoded independently here.
struct PreparedCompareIO:StreamIO {
 bool prepared=false;
 PreparedCompareIO(unsigned k,std::vector<uint8_t>* shared,unsigned xy=17|(12<<6)):StreamIO(k,xy,shared){}
 bool Header(unsigned a)const{return prepared?PL::HeaderWord(a):SL::HeaderWord(a);}
 unsigned Packets()const{return prepared?3:26;}
 unsigned Read(unsigned a){
  if(prepared && PL::HeaderWord(a)){events++;reads++;Progress();headerReads++;return l1[a]|unsigned(l1[a+1])<<8|unsigned(l1[a+2])<<16|unsigned(l1[a+3])<<24;}
  return StreamIO::Read(a);
 }
 void Write(unsigned a,unsigned v){
  if(!prepared){StreamIO::Write(a,v);return;}
  if(PL::HeaderWord(a)){
   events++;writes++;Progress();
   if(a==0x8ffc0){assert(queued.empty() && responses.empty() && reader==2);jobStart=issues;jobs++;copies[0]=copies[1]=0;ready[0]=ready[1]=false;}
   assert(!v && issues==jobStart);headerWrites++;if(fault!=12 || headerWrites!=1)memset(l1.data()+a,0,4);return;
  }
  if(a!=0xffb20040){StreamIO::Write(a,v);return;}
  events++;writes++;Progress();assert(v==1 && queued.size()<16);
  const unsigned block=(issues-jobStart)/3,p=(issues-jobStart)%3,buffer=block&1,base=buffer*0x10000;
  // Independent literal geometry, not PL::Packet/proxy address repair.
  SL::Read expected=p==0?SL::Read{0x04080000+block*5632,0x90000+base,4608}:
   p==1?SL::Read{0x04080000+block*5632+4608,0x8c000+base,1024}:SL::Read{0x04100000+block*512,0x96000+base,512};
  assert(command[0]==expected.source && command[3]==expected.destination && command[8]==expected.bytes && command[2]==route
   && !command[1] && !command[4] && command[5]==129 && !command[6] && !command[7] && !command[9] && reader!=buffer);
  counterSeen=creditSeen=0;emptySeen=busySeen=false;
  if(!p){copies[buffer]=0;ready[buffer]=false;dataBlock[buffer]=block;}
  queued.push_back({expected,0,buffer,block});issues++;maxQueued=std::max(maxQueued,unsigned(queued.size()));
 }
 void Fence(){
  StreamIO::Fence();
  if(prepared && issues && queued.empty() && responses.empty() && emptySeen && busySeen && counterSeen==511 && creditSeen==~0U){
   const unsigned indices[]={1,2,3,4,5,8,10,12,14};bool stable=true;for(unsigned i=0;i<9;i++)stable&=observed[i]==counters[indices[i]];
   if(stable)for(unsigned b=0;b<2;b++)if(copies[b]==3)ready[b]=true;
  }
 }
};
struct PreparedComparePipeline:PreparedBf16Q4Machine {
 PreparedCompareIO* bus=nullptr;bool old=true;mutable unsigned readerStart=0;
 bool RawPublishesLast()const override{return old;}
 unsigned OperandOffset()const override{return old?((inputLoads/8)&1)*0x10000:buffer*0x10000;}
 bool OperandReady(unsigned a,unsigned bytes)const override{
  if(a==0x97000)return bytes==1024;
  if(!old)return PreparedSourceReady(a,bytes);
  const unsigned block=inputLoads/8,slot=block&1;
  const bool valid=bus && !bus->prepared && bus->ready[slot] && bus->copies[slot]==26 && bus->dataBlock[slot]==block
   && a>=SL::Raw+slot*0x10000 && a+bytes<=0x963c0+slot*0x10000;
  if(valid && a==SL::Metadata+slot*0x10000 && !(compactImports%2)){bus->reader=slot;readerStart=matrixOps;}
  return valid && bus->reader==slot;
 }
 bool PreparedSourceReady(unsigned a,unsigned bytes)const override{
  const unsigned block=groups/8,slot=block&1,off=slot*0x10000;
  const bool span=(a>=0x90000+off && uint64_t(a)+bytes<=0x91200+off)
   || (a>=0x8c000+off && uint64_t(a)+bytes<=0x8c400+off)
   || (a>=0x96000+off && uint64_t(a)+bytes<=0x96200+off);
  const bool valid=bus && bus->prepared && !old && span && bus->ready[slot] && bus->copies[slot]==3 && bus->dataBlock[slot]==block;
  if(valid && a==0x90000+off && !(compactImports%2)){bus->reader=slot;readerStart=groups;}
  return valid && bus->reader==slot;
 }
 void Op(unsigned op){
  if(!old){PreparedBf16Q4Machine::Op(op);return;}
  const unsigned code=op>>24,mode=(op>>16)&15,address=op&1023;
  if(op==0x42000091 && (config[84]&65535)/16-4<=128){Q4FpuPipelineMachine::Op(op);return;}
  if(config[1]!=0xa03c8000 && (code==0x70 || code==0x72) && mode==3){
   if(code==0x72){Q4FpuPipelineMachine::Op(op);return;}
   Need(!Pending() && !packPending && enabled && vector.activeLanes==UINT32_MAX && !(address&1) && !(op&0xfc00));
   const unsigned reg=(op>>20)&15;Need((reg==0 || reg==4) && address>=128 && address<256);cycle++;
   for(unsigned lane=0;lane<32;lane++)vector.lanes[reg][lane]=FloatDst((address&~3U)+lane/8,(lane&7)*2+bool(address&2));
   vector.ready[reg]=cycle+1;return;
  }
  FactoredOperatorMachine::Op(op);
 }
};
struct PreparedCompareState {PreparedComparePipeline native;};
struct PreparedCompareRV32:PreparedCompareState,ServiceMachine {
 PreparedCompareIO bus;unsigned width,acks=0,timerStores=0,clockReads=0;bool awaiting=false;
 std::vector<unsigned> csrPc;unsigned lateFault=0;
 PreparedCompareRV32(const char* folder,unsigned k):ServiceMachine(folder,native.l1),bus(k,&native.l1),width(k){
  assert(code.size()==14788);native.bus=&bus;native.delay[0]=100003;native.delay[1]=7;
  for(unsigned off=0;off<code.size();off+=4)if((::Word(code,off)&127)==0x73)csrPc.push_back(0x70000+off);
  assert(csrPc.size()==3 && csrPc[1]==csrPc[0]+4 && csrPc[2]==csrPc[0]+8);
  const unsigned frame[]={0x31425151,1,k,bus.route,1,0,0,0};for(unsigned i=0;i<8;i++)Put(SL::Frame+i*4,frame[i]);
 }
 bool InlineTensix()const override{return true;}
 unsigned Index(unsigned a)override{if(Range(a,SL::Output,320)){assert(!(a&3));return 6200+(a-SL::Output)/4;}return ServiceMachine::Index(a);}
 unsigned ReadL1(unsigned a)const{return ::Word(native.l1,a);}
 void WriteL1(unsigned a,unsigned v){Put(a,v);if(Range(a,SL::Output,320))l1[Index(a)]=v;}
 void Progress()override{
  if(bus.reader<2 && !awaiting && native.Clean() && (native.old?native.matrixOps==native.readerStart+128:native.groups==native.readerStart+8))bus.reader=2;
  bus.Progress();
 }
 void Instruction(unsigned op)override{
  if(op==0xa2101816){assert(!gpr63 && !awaiting);awaiting=true;}
  else if(op==0x4500017e){assert(awaiting && !gpr63);acks++;native.Drain();gpr63=1;awaiting=false;}
  else native.Op(op);Progress();
 }
 void Store(unsigned a,unsigned v)override{
  assert(!(a&3));
  if(a==0xffe40000){Instruction(v);return;}if(a==0xffe000fc){assert(!v);gpr63=0;return;}
  if(Range(a,0xffef0000,224*4)){native.Config((a-0xffef0000)/4,v);Progress();return;}
  if(a==0x87ff0){assert(v==0x32425151 && native.old && stores==32 && bus.jobs==1 && native.Clean() && bus.reader==2);native.old=false;bus.prepared=true;bus.fault=lateFault;WriteL1(a,v);return;}
  if(bus.Header(a) || Range(a,0xffb20000,0x2000)){bus.Write(a,v);return;}
  if(Range(a,0x6e000,8192) || Range(a,0x17ffc0,12) || Range(a,SL::Output,320)){
   if(Range(a,0x6e000,8192))minStore=std::min(minStore,a);
   if(Range(a,SL::Output,320)){
    assert(native.Clean() && !awaiting && bus.reader==2 && bus.queued.empty() && bus.responses.empty());
    if(Range(a,SL::Output,256))stores++;else timerStores++;
   }
   WriteL1(a,v);Progress();return;
  }
  if(!native.old && Range(a,0x8a000,512) && (a-0x8a000)%128<64)native.EscapeWord((a-0x8a000)/128,(a%128)/4,v);
  else native.Write(a,v);Progress();
 }
 unsigned Load(unsigned a)override{
  assert(!(a&3));if(a==0xffe000fc)return gpr63;
  if(Range(a,0xffef0000,224*4)){assert(!native.Pending() && !native.mathPending);return native.config[(a-0xffef0000)/4];}
  if(bus.Header(a) || Range(a,0xffb20000,0x2000))return bus.Read(a);
  if(!native.old && (Range(a,0x8c000,1024) || Range(a,0x9c000,1024))){
   const unsigned b=a>=0x9c000,rel=a-0x8c000-b*0x10000,panel=(rel%512)/32;
   return native.MaskWord(b,panel/2,(rel/512)*2+panel%2,(rel%32)/4);
  }
  if((native.old && (Range(a,0xffbd8800,64) || Range(a,0xffbd8a00,64)))
   || (!native.old && (Range(a,0xffbd9400,64) || Range(a,0xffbd9600,64)))){
   assert(native.Clean() && !awaiting && bus.reader==2);native.outputReads++;return native.FloatDst((a-0xffbd8000)/64,(a%64)/4);
  }
  assert(CodeAddress(a) || Range(a,0x6e000,8192) || Range(a,SL::Frame,32) || Range(a,0x17ffc0,12));return ReadL1(a);
 }
 void TimingStep(){
  const unsigned w=l1[Index(pc)];if((w&127)!=0x73){Machine::Step();return;}
  const unsigned part=clockReads%3;assert(pc==csrPc[part] && (w&0x000ff07f)==0x2073 && (w>>20)==(part==1?0xb00U:0xb80U));
  const unsigned rd=(w>>7)&31;assert(rd);const uint64_t synthetic=UINT64_C(0x100000000)-12345+uint64_t(steps)*8;
  regs[rd]=part==1?unsigned(synthetic):unsigned(synthetic>>32);regs[0]=0;pc+=4;steps++;clockReads++;
 }
 bool Execute(){
  while(pc!=0x70038 && pc!=0x70058){
   const unsigned w=l1[Index(pc)];TimingStep();if(w==0x0ff0000f){bus.Fence();native.SealData();}Progress();
   if(regs[2]){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);}
   assert(regs[27]==0x13579bdf && steps<StepLimit());
  }
  if(pc==0x70058){assert(ReadL1(0x17ffc8)==0x45425151);return false;}
  assert(ReadL1(0x17ffc0)==0x7000c && ReadL1(0x17ffc4)==(0x04000000|width) && ReadL1(0x17ffc8)==0x31425151);
  assert(native.Clean() && !awaiting && bus.reader==2 && bus.jobs==2 && stores==64 && timerStores==16 && clockReads==12);
  assert(bus.issues==(26+3)*(width/256) && bus.completed==bus.issues && native.outputReads==64 && !native.packs && !native.codeStores);
  return true;
 }
};
struct PreparedCompareData:FactoredCompareData {
 using FactoredCompareData::FactoredCompareData;
 void Stage(PreparedCompareIO& io)const{
  FactoredCompareData::Stage(io);
  for(unsigned b=0;b<blocks.size();b++){
   std::array<uint8_t,5632> data{};
   assert(PioneerQ38PreparedBf16Q4BlockData::Prepare(blocks[b].original.data(),4608,data.data(),data.size()));
   memcpy(io.dram.data()+0x80000+b*5632,data.data(),data.size());
  }
 }
};
#endif
