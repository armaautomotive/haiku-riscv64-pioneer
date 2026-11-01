/* SPDX-License-Identifier: MIT */
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "../PioneerDramReadSweep.h"
#include <map>
#include <string>
struct SweepMachine:Machine {
 enum Fault { None, BadConfig, BadSource, BadBusy, ExcessResponse, MissingResponse,
  StuckOutstanding, DirtyInitial, ChangedJob, FrozenClock, BackwardClock, UnstableClock,
  UnstableEndClock, RolloverSample };
 std::map<uint32_t,uint32_t> memory;
 std::vector<uint32_t> raw;
 uint32_t command[10]={},responses=0xfffffffeU,initial=responses;
 unsigned successful=0,requests=0,polls=0,csrReads=0,commandWrites=0,minStack=0x70000;
 unsigned mmioReads=0,mmioWrites=0;
 unsigned park=0,faultPark=0,textBytes=0;
 unsigned delay=2,faultAt=1;
 Fault fault=None;
 bool pending=false,active=false,doorbellRead=false;
 explicit SweepMachine(const std::vector<uint8_t>& boot,const std::vector<uint8_t>& text,
  unsigned goodPark,unsigned badPark,unsigned shift=14,unsigned sweeps=1)
  :Machine(),raw(4096,0xcafebabe),park(goodPark),faultPark(badPark),textBytes(unsigned(text.size())) {
  assert(boot.size()==4 && !text.empty() && !(text.size()&3));
  memory[0x6000]=Word(boot,0);
  for(unsigned i=0;i<text.size();i+=4)memory[0x64000+i]=Word(text,i);
  for(unsigned i=0;i<16;i++)memory[PioneerDramReadSweep::Job+i*4]=memory[PioneerDramReadSweep::Report+i*4]=0;
  const uint32_t frame[]={PioneerDramReadSweep::Magic,1,sweeps,shift,17|(12<<6),0x12345678};
  for(unsigned i=0;i<6;i++)memory[PioneerDramReadSweep::Job+i*4]=frame[i];
 }
 bool CodeAddress(uint32_t a)const override {return a==0x6000 || (a>=0x64000 && a<0x64000+textBytes);}
 unsigned Index(uint32_t a) override {
  assert(CodeAddress(a) && !(a&3));l1[8191]=memory.at(a);return 8191;
 }
 unsigned StepLimit()const override{return 50000000;}
 void Instruction(uint32_t)override{assert(false && "no Tensix/inline instructions in memory-only diagnostic");}
 static uint32_t Pattern(uint32_t a){return (a*0x9e3779b9U)^((a>>11)*0x12345U)^0x5a69c3e7U;}
 static bool Range(uint32_t a,uint32_t base,unsigned bytes){return a>=base && a-base<bytes;}
 uint32_t ReadCycleCsr(unsigned csr) override {
  assert(csr==0xb00 || csr==0xb80);csrReads++;
  uint64_t ticks=UINT64_C(0x1ffffe000)+steps;
  if(fault==FrozenClock)ticks=UINT64_C(0x200000000);
  if(fault==BackwardClock && csrReads>3)ticks=1;
  if((fault==UnstableClock || (fault==UnstableEndClock && csrReads>3)) && csr==0xb80)return csrReads;
  if(fault==RolloverSample && csrReads==1)return uint32_t(ticks>>32)-1;
  return csr==0xb00?uint32_t(ticks):uint32_t(ticks>>32);
 }
 uint32_t Load(uint32_t a)override {
  assert(!(a&3));
  if(Range(a,0x6e000,8192) || Range(a,PioneerDramReadSweep::Job,64)
   || Range(a,PioneerDramReadSweep::Report,64))return memory[a];
  if(Range(a,PioneerDramReadSweep::Raw,16384)){
   assert(!pending && successful);return raw[(a-PioneerDramReadSweep::Raw)/4];
  }
  mmioReads++;
  if(Range(a,0xffb20000,40)){
   unsigned i=(a-0xffb20000)/4;
   if((fault==BadConfig && i==8 && !requests)
    || (fault==BadSource && i==0 && commandWrites>10 && !requests))return command[i]^4;
   return command[i];
  }
  for(unsigned i=0;i<4;i++)if(a==0xffb20040+i*0x800){
   if(fault==DirtyInitial && !i && !active)return 1;
   if(i)return 0;
   doorbellRead=true;
   if(pending && ++polls>delay && !(fault==MissingResponse && requests==faultAt)){
    for(unsigned w=0;w<command[8]/4;w++)raw[w]=Pattern(command[0]+w*4);
    pending=false;successful++;responses++;
    if(fault==ChangedJob && requests==faultAt)memory[PioneerDramReadSweep::Job+20]^=1;
   }
   if(fault==BadBusy && requests==faultAt)return 2;
   return pending?1:0;
  }
  if(a==0xffb20208){
   if(fault==ExcessResponse && requests==faultAt)return responses+2;
   return responses;
  }
  for(unsigned i=0;i<32;i++)if(a==0xffb20240+i*4){
   if(!i && fault==StuckOutstanding && requests==faultAt)return 1;
   return !i && pending?1:0;
  }
  fprintf(stderr,"unexpected LOAD %08x pc%08x\n",a,pc);assert(false);return 0;
 }
 void Store(uint32_t a,uint32_t v) override {
  assert(!(a&3));
  if(Range(a,0x6e000,8192)){minStack=std::min(minStack,a);memory[a]=v;return;}
  if(Range(a,PioneerDramReadSweep::Report,64)){memory[a]=v;return;}
  mmioWrites++;
  if(Range(a,0xffb20000,40)){
   assert(!pending);unsigned i=(a-0xffb20000)/4;command[i]=v;commandWrites++;
   if(!i && active){
    const unsigned shift=memory.at(PioneerDramReadSweep::Job+12);
    assert(v==PioneerDramReadSweep::Base+((requests<<shift)&(PioneerDramReadSweep::Span-1)));
   }
   return;
  }
  assert(a==0xffb20040 && v==1 && !pending);
  const unsigned shift=memory.at(PioneerDramReadSweep::Job+12);
  assert(command[0]>=PioneerDramReadSweep::Base && command[0]+command[8]<=PioneerDramReadSweep::Base+PioneerDramReadSweep::Span);
  assert(command[1]==0 && command[2]==(17|(12<<6)) && command[3]==PioneerDramReadSweep::Raw
   && command[4]==0 && command[5]==129 && command[6]==0 && command[7]==0
   && command[8]==(1U<<shift) && command[9]==0 && !(command[0]&63));
  assert(requests==successful);requests++;pending=true;active=true;doorbellRead=false;polls=0;
 }
 bool Execute(){
  while(pc!=park && pc!=faultPark){Step();}
  assert(memory.at(pc)==0x0000006f && minStack>=0x6e000);
  if(pc==faultPark){assert(memory.at(PioneerDramReadSweep::Report)==0x46524244);return false;}
  const auto r=[&](unsigned i){return memory.at(PioneerDramReadSweep::Report+i*4);};
  const unsigned shift=memory.at(PioneerDramReadSweep::Job+12),sweeps=memory.at(PioneerDramReadSweep::Job+8);
  assert(!pending && requests==successful && requests==(PioneerDramReadSweep::Span>>shift)*sweeps);
  assert(uint32_t(responses-initial)==requests && r(0)==PioneerDramReadSweep::ReportMagic && r(1)==1);
  assert(r(2)==0x12345678 && r(3)==(17|(12<<6)) && r(4)==1U<<shift && r(5)==sweeps);
  assert(r(6)==PioneerDramReadSweep::Span*sweeps && r(7)==requests);
  const uint64_t begin=(uint64_t(r(9))<<32)|r(8),end=(uint64_t(r(11))<<32)|r(10);
  assert(end>begin && ((uint64_t(r(13))<<32)|r(12))==end-begin);
  assert(r(14)==Pattern(PioneerDramReadSweep::Base+PioneerDramReadSweep::Span-(1U<<shift)));
  assert(r(15)==Pattern(PioneerDramReadSweep::Base+PioneerDramReadSweep::Span-4));
  assert(!matrixInstructions && !moveInstructions && !configWrites && !inputWrites && !vectorWrites);
  return true;
 }
};
int main(int argc,char** argv){
 assert(argc==5);auto boot=Read(argv[1]),text=Read(argv[2]);
 const unsigned good=unsigned(std::stoul(argv[3],nullptr,16)),bad=unsigned(std::stoul(argv[4],nullptr,16));
 unsigned passes=0,failures=0;
 for(unsigned shift:{6U,10U,12U,14U}){
  SweepMachine m(boot,text,good,bad,shift);assert(m.Execute());passes++;
 }
 {SweepMachine m(boot,text,good,bad,14,2);assert(m.Execute());passes++;}
 {SweepMachine m(boot,text,good,bad);m.fault=SweepMachine::RolloverSample;
  assert(m.Execute() && m.csrReads==9);passes++;}
 for(unsigned word=0;word<16;word++){
  SweepMachine m(boot,text,good,bad);
  if(word==5)m.memory[PioneerDramReadSweep::Job+word*4]=0;
  else m.memory[PioneerDramReadSweep::Job+word*4]^=word==3?1:0x80000000;
  assert(!m.Execute() && !m.requests && !m.csrReads && !m.active && !m.mmioReads && !m.mmioWrites);failures++;
 }
 for(auto mutation:std::vector<std::pair<unsigned,uint32_t>>{{2,0},{2,257},{2,0xffffffff},
  {3,0},{3,31},{3,32},{4,0},{4,17|(13<<6)},{4,19|(12<<6)}}){
  SweepMachine m(boot,text,good,bad);m.memory[PioneerDramReadSweep::Job+mutation.first*4]=mutation.second;
  assert(!m.Execute() && !m.requests && !m.csrReads && !m.mmioReads && !m.mmioWrites);failures++;
 }
 {SweepMachine m(boot,text,good,bad);m.memory[PioneerDramReadSweep::Report+4]=1;
  assert(!m.Execute() && !m.requests && !m.csrReads && !m.mmioReads && !m.mmioWrites);failures++;}
 for(auto fault:{SweepMachine::BadConfig,SweepMachine::BadSource,SweepMachine::BadBusy,
  SweepMachine::ExcessResponse,SweepMachine::MissingResponse,SweepMachine::StuckOutstanding,
  SweepMachine::DirtyInitial,SweepMachine::ChangedJob,SweepMachine::FrozenClock,
  SweepMachine::BackwardClock,SweepMachine::UnstableClock,SweepMachine::UnstableEndClock}){
  SweepMachine m(boot,text,good,bad);m.fault=fault;
  assert(!m.Execute());
  if(fault==SweepMachine::BadBusy || fault==SweepMachine::ExcessResponse
   || fault==SweepMachine::MissingResponse || fault==SweepMachine::StuckOutstanding)assert(m.requests==1);
  if(fault==SweepMachine::BadConfig || fault==SweepMachine::BadSource
   || fault==SweepMachine::DirtyInitial || fault==SweepMachine::UnstableClock)assert(!m.requests);
  if(fault==SweepMachine::UnstableClock)assert(m.csrReads==12);
  if(fault==SweepMachine::UnstableEndClock)assert(m.csrReads==15);
  failures++;
 }
 {uint32_t job[16]={PioneerDramReadSweep::Magic,1,256,6,17|(12<<6),1};
  assert(PioneerDramReadSweep::Shape(job) && PioneerDramReadSweep::Span*job[2]==0x80000000U);}
 puts("OFFLINE actual RV32 READ-sweep boot/issue/wait/clock/result/fault-park execution;NOT hardware timing or native lifecycle");
 printf("PASS valid%u rejected%u;8MiB complete address sequence/response wrap/finite uncertainty/no next packet/no GDDR writes\n",passes,failures);
}
