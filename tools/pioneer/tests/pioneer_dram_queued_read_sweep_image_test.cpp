/* SPDX-License-Identifier: MIT */
// Execute emitted RV32 bytes against an independent asynchronous NIU model.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "../PioneerDramQueuedReadSweep.h"
#include "../PioneerDramWideQueuedReadSweep.h"
#include <deque>
#include <map>
#include <string>
#ifdef PIONEER_DRAM_WIDE_QUEUE_TEST
using Q=PioneerDramWideQueuedReadSweep;
static constexpr unsigned RawCapacity=0x40000,FrameVersion=3;
#else
using Q=PioneerDramQueuedReadSweep;
static constexpr unsigned RawCapacity=16384,FrameVersion=2;
#endif
struct QueuedMachine:Machine {
 enum Fault { None,Disabled,DirtyFifo,Overflow,InvalidFree,OtherBusy,OtherOutstanding,
  BadSource,BadDestination,StalledFifo,LostDoorbell,LateOverflow,FullFifo,
  ExtraResponse,StuckOutstanding,ChangedJob,FrozenClock,BackwardClock,UnstableClock };
 struct Packet {uint32_t source,dest,bytes;unsigned ready;};
 std::map<uint32_t,uint32_t> memory;
 std::deque<Packet> fifo;
 std::vector<Packet> flying;
 std::vector<uint32_t> raw;
 uint32_t command[10]={},responses=0xfffffffe,initial=responses;
 unsigned park,faultPark,textBytes,shift,depth,sweeps;
 unsigned requests=0,successful=0,ticks=0,csrReads=0,maxPending=0,minStack=0x70000;
 unsigned mmioReads=0,mmioWrites=0,reordered=0,commandWrites=0,delay=100;
 bool queueHidden=false;Fault fault=None;
 QueuedMachine(const std::vector<uint8_t>& boot,const std::vector<uint8_t>& text,
  unsigned good,unsigned bad,unsigned s,unsigned d,unsigned n=1)
  :Machine(),raw(RawCapacity/4,0xcafebabe),park(good),faultPark(bad),textBytes(unsigned(text.size())),shift(s),depth(d),sweeps(n){
  assert(boot.size()==4 && !text.empty() && !(text.size()&3));memory[0x6000]=Word(boot,0);
  for(unsigned i=0;i<text.size();i+=4)memory[0x64000+i]=Word(text,i);
  for(unsigned i=0;i<16;i++)memory[Q::Job+i*4]=memory[Q::Report+i*4]=0;
  const uint32_t frame[]={Q::Magic,FrameVersion,n,s,18|(12<<6),0x12345678,d};
  for(unsigned i=0;i<7;i++)memory[Q::Job+i*4]=frame[i];
 }
 bool CodeAddress(uint32_t a)const override{return a==0x6000 || (a>=0x64000 && a<0x64000+textBytes);}
 unsigned Index(uint32_t a)override{assert(CodeAddress(a) && !(a&3));l1[8191]=memory.at(a);return 8191;}
 unsigned StepLimit()const override{return 150000000;}
 void Instruction(uint32_t)override{assert(false && "memory-only leaf has no engine instructions");}
 static bool Range(uint32_t a,uint32_t base,unsigned bytes){return a>=base && a-base<bytes;}
 static uint32_t Pattern(uint32_t a){return (a*0x9e3779b9U)^((a>>11)*0x12345U)^0x5a69c3e7U;}
 void Tick(){
  ticks++;
  if(!fifo.empty() && ticks%3==0 && fault!=StalledFifo){
   auto p=fifo.front();fifo.pop_front();
   const unsigned slot=(p.dest-Q::Raw)>>shift;
   p.ready=ticks+delay+(depth-1-slot)*64;
   flying.push_back(p);
  }
  // Independently allow response reordering. Each packet retains the exact
  // captured command, not the mutable initiator registers or FIFO position.
  for(unsigned i=unsigned(flying.size());i>0;i--){
   const auto p=flying[i-1];
   if(p.ready>ticks)continue;
   if(i>1)reordered++;
   for(unsigned w=0;w<p.bytes/4;w++)raw[(p.dest-Q::Raw)/4+w]=Pattern(p.source+w*4);
   flying.erase(flying.begin()+i-1);successful++;responses++;
   if(fault==ChangedJob && successful==1)memory[Q::Job+20]^=1;
  }
 }
 uint32_t ReadCycleCsr(unsigned csr)override{
  assert(csr==0xb00 || csr==0xb80);csrReads++;
  uint64_t value=UINT64_C(0x1ffffe000)+steps;
  if(fault==FrozenClock)value=UINT64_C(0x200000000);
  if(fault==BackwardClock && csrReads>3)value=1;
  if(fault==UnstableClock && csr==0xb80)return csrReads;
  return csr==0xb00?uint32_t(value):uint32_t(value>>32);
 }
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));
  if(Range(a,0x6e000,8192) || Range(a,Q::Job,64) || Range(a,Q::Report,64))return memory[a];
  if(Range(a,Q::Raw,RawCapacity)){assert(fifo.empty() && flying.empty() && successful);return raw[(a-Q::Raw)/4];}
  mmioReads++;Tick();
  if(Range(a,0xffb20000,40)){
   const unsigned i=(a-0xffb20000)/4;
   if(!requests && commandWrites>10 && ((fault==BadSource && i==0) || (fault==BadDestination && i==3)))return command[i]^4;
   return command[i];
  }
  if(a==0xffb20100)return fault==Disabled?0x6005:0x16005;
  if(a==0xffb20068)return fault==Overflow || (fault==LateOverflow && requests)?1:0;
  if(a==0xffb20064){
   if(fault==DirtyFifo && !requests)return 0x1010100f;
   if(fault==InvalidFree)return 0x10101011;
   if(fault==FullFifo && requests)return 0x10101000;
   return 0x10101000|uint32_t(16-fifo.size());
  }
  for(unsigned i=0;i<4;i++)if(a==0xffb20040+i*0x800)return fault==OtherBusy && i==1?1:0;
  if(a==0xffb20208){
   if(!fifo.empty() && flying.empty())queueHidden=true;
   return responses+unsigned(fault==ExtraResponse && requests && fifo.empty() && flying.empty());
  }
  for(unsigned i=0;i<32;i++)if(a==0xffb20240+i*4){
   if(i==1 && fault==OtherOutstanding)return 1;
   return !i?uint32_t(flying.size())+unsigned(fault==StuckOutstanding && requests):0;
  }
  fprintf(stderr,"unexpected queued LOAD %08x pc%08x\n",a,pc);assert(false);return 0;
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3));
  if(Range(a,0x6e000,8192)){minStack=std::min(minStack,a);memory[a]=value;return;}
  if(Range(a,Q::Report,64)){memory[a]=value;return;}
  mmioWrites++;
  if(Range(a,0xffb20000,40)){command[(a-0xffb20000)/4]=value;commandWrites++;return;}
  assert(a==0xffb20040 && value==1 && fifo.size()<16);
  const unsigned bytes=1U<<shift,slot=requests&(depth-1);
  assert(command[0]==Q::Base+((requests<<shift)&(Q::Span-1)) && command[1]==0
   && command[2]==(18|(12<<6)) && command[3]==Q::Raw+slot*bytes
   && command[4]==0 && command[5]==129 && command[6]==0 && command[7]==0
   && command[8]==bytes && command[9]==0);
  assert(command[0]+bytes<=Q::Base+Q::Span && command[3]+bytes<=Q::Raw+RawCapacity);
  // A new batch must wait for every response of the previous batch. Within
  // a batch all live destination intervals must be pairwise disjoint.
  if(!slot)assert(fifo.empty() && flying.empty() && requests==successful);
  for(const auto& p:fifo)assert(p.dest+bytes<=command[3] || command[3]+bytes<=p.dest);
  for(const auto& p:flying)assert(p.dest+bytes<=command[3] || command[3]+bytes<=p.dest);
  requests++;
  if(fault!=LostDoorbell || requests!=1)fifo.push_back({command[0],command[3],bytes,0});
  maxPending=std::max(maxPending,unsigned(fifo.size()+flying.size()));assert(maxPending<=depth);
 }
 bool Execute(){
  while(pc!=park && pc!=faultPark)Step();
  assert(memory.at(pc)==0x0000006f && minStack>=0x6e000);
  const unsigned count=requests;for(unsigned i=0;i<32;i++)Step();assert(requests==count);
  if(pc==faultPark){assert(memory.at(Q::Report)==0x46524244);return false;}
  const auto r=[&](unsigned i){return memory.at(Q::Report+i*4);};
  assert(fifo.empty() && flying.empty() && requests==successful && requests==(Q::Span>>shift)*sweeps);
  assert(uint32_t(responses-initial)==requests && r(0)==Q::ReportMagic && r(1)==FrameVersion);
  assert(r(2)==0x12345678 && r(3)==(18|(12<<6)) && r(4)==1U<<shift && r(5)==sweeps);
  assert(r(6)==Q::Span*sweeps && r(7)==requests);
  const uint64_t start=(uint64_t(r(9))<<32)|r(8),end=(uint64_t(r(11))<<32)|r(10);
  assert(end>start && ((uint64_t(r(13))<<32)|r(12))==end-start);
  const unsigned bytes=1U<<shift;
  for(unsigned slot=0;slot<depth;slot++)for(unsigned w=0;w<bytes/4;w++)
   assert(raw[slot*bytes/4+w]==Pattern(Q::Base+Q::Span-depth*bytes+slot*bytes+w*4));
  for(unsigned w=depth*bytes/4;w<RawCapacity/4;w++)assert(raw[w]==0xcafebabe);
  assert(r(14)==Pattern(Q::Base+Q::Span-bytes) && r(15)==Pattern(Q::Base+Q::Span-4));
  if(depth>1)assert(maxPending>1 && reordered);
  assert(!matrixInstructions && !moveInstructions && !configWrites && !inputWrites && !vectorWrites);
  return true;
 }
};
int main(int argc,char** argv){
 assert(argc==5);const auto boot=Read(argv[1]),text=Read(argv[2]);
 const unsigned good=unsigned(std::stoul(argv[3],nullptr,16)),bad=unsigned(std::stoul(argv[4],nullptr,16));
 unsigned passed=0,refused=0;
 for(unsigned shift:{6U,10U,12U,14U})for(unsigned depth:{1U,2U,4U,8U,16U}){
  if((1U<<shift)*depth>RawCapacity || (shift==6 && depth!=1 && depth!=16))continue;
  QueuedMachine m(boot,text,good,bad,shift,depth);assert(m.Execute());passed++;
 }
 {QueuedMachine m(boot,text,good,bad,14,1,2);assert(m.Execute());passed++;}
 for(unsigned word=0;word<16;word++){
  QueuedMachine m(boot,text,good,bad,10,16);
  if(word==5)m.memory[Q::Job+20]=0;
  else m.memory[Q::Job+word*4]^=0x80000000;
  assert(!m.Execute() && !m.requests && !m.csrReads && !m.mmioReads && !m.mmioWrites);refused++;
 }
 for(const auto& change:std::vector<std::pair<unsigned,uint32_t>>{{2,0},{2,257},{2,UINT32_MAX},
  {3,0},{3,31},{3,32},{3,RawCapacity==16384?14U:15U},{4,0},{4,19|(12<<6)},{4,17|(13<<6)},
  {6,0},{6,3},{6,17},{6,32},{6,UINT32_MAX}}){
  QueuedMachine m(boot,text,good,bad,10,16);m.memory[Q::Job+change.first*4]=change.second;
  assert(!m.Execute() && !m.requests && !m.csrReads && !m.mmioReads && !m.mmioWrites);refused++;
 }
 {QueuedMachine m(boot,text,good,bad,10,16);m.memory[Q::Report+4]=1;
  assert(!m.Execute() && !m.requests && !m.mmioReads && !m.mmioWrites);refused++;}
 for(auto fault:{QueuedMachine::Disabled,QueuedMachine::DirtyFifo,QueuedMachine::Overflow,
  QueuedMachine::InvalidFree,QueuedMachine::OtherBusy,QueuedMachine::OtherOutstanding,
  QueuedMachine::BadSource,QueuedMachine::BadDestination,QueuedMachine::StalledFifo,
  QueuedMachine::LostDoorbell,QueuedMachine::LateOverflow,QueuedMachine::FullFifo,
  QueuedMachine::ExtraResponse,QueuedMachine::StuckOutstanding,QueuedMachine::ChangedJob,
  QueuedMachine::FrozenClock,QueuedMachine::BackwardClock,QueuedMachine::UnstableClock}){
  QueuedMachine m(boot,text,good,bad,10,16);m.fault=fault;
  const bool completed=m.Execute();
  if(completed)fprintf(stderr,"unexpected queued fault success enum%u\n",unsigned(fault));
  assert(!completed);
  if(fault<=QueuedMachine::BadDestination || fault==QueuedMachine::UnstableClock)assert(!m.requests);
  if(fault==QueuedMachine::StalledFifo || fault==QueuedMachine::LostDoorbell
   || fault==QueuedMachine::ExtraResponse || fault==QueuedMachine::StuckOutstanding)assert(m.requests==16);
  if(fault==QueuedMachine::LateOverflow || fault==QueuedMachine::FullFifo)assert(m.requests==1);
  refused++;
 }
 uint32_t job[16]={Q::Magic,FrameVersion,256,6,17|(12<<6),1,16};
 assert(Q::Shape(job) && Q::Span*job[2]==0x80000000U);
 printf("PASS actual queued RV32 valid%u/refused%u;FIFO capture/async returns/counter wrap/disjoint buffers/full address sequence/finite faults;NO hardware bandwidth/native ownership/Work claim\n",passed,refused);
}
