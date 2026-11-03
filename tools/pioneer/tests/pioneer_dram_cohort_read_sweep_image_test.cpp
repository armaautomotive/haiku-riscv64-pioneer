/* SPDX-License-Identifier: MIT */
// ALL eight actual emitted RV32 programs on one asynchronous simulated fabric.
// No native controller, hardware clock, bandwidth or deployment proof.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "../PioneerDramCohortReadSweep.h"
#include <deque>
#include <map>
#include <memory>
#include <string>
using C=PioneerDramCohortReadSweep;
struct Fabric;
struct Core:Machine {
 Fabric& fabric;unsigned role,textBytes,park,bad;
 std::map<uint32_t,uint32_t> memory;
 std::vector<uint32_t> raw;
 uint32_t cmd[10]={},counter[64]={};
 struct Packet{uint32_t source,dest,sourceXY,destXY,bytes;bool write;uint64_t due;};
 std::deque<Packet> queued;std::vector<Packet> flying;
 unsigned requests=0,completed=0,mmioReads=0,mmioWrites=0,csrReads=0,minStack=0x70000;
 unsigned maxPending=0,reordered=0;
 uint64_t firstRead=0,lastRead=0;
 Core(Fabric&,unsigned,const std::vector<uint8_t>&,const std::vector<uint8_t>&,unsigned,unsigned,unsigned);
 bool CodeAddress(uint32_t a)const override{return a==0x6000 || (a>=0x64000 && a<0x64000+textBytes);}
 unsigned Index(uint32_t a)override{assert(CodeAddress(a) && !(a&3));l1[8191]=memory.at(a);return 8191;}
 unsigned StepLimit()const override{return 30000000;}
 void Instruction(uint32_t)override{assert(false && "no tensor engine in memory cohort");}
 uint32_t Load(uint32_t)override;
 void Store(uint32_t,uint32_t)override;
 uint32_t ReadCycleCsr(unsigned)override;
 bool Parked()const{return pc==park || pc==bad;}
 bool Local(uint32_t a)const{
  return (a>=0x6e000 && a<0x70000) || (a>=C::Job && a<C::Job+C::Words*4)
   || (a>=C::Report && a<C::Report+C::Words*4) || a==C::Ready || a==C::Start || a==C::Done
   || a==C::Payload || (a>=C::Scratch && a<C::Scratch+C::Words*4)
   || (a>=C::PeerReports && a<C::PeerReports+C::Peers*C::Words*4);
 }
};
struct Fabric {
 enum Fault{None,DirtyReport,DuplicateRoute,DisabledFifo,DirtyFifo,WrongTile,WrongStart,
  LostRead,ExtraResponse,ChangedJob,LostStartAck,CorruptPeerReport,BackwardRootClock,ChangedTile};
 std::vector<std::unique_ptr<Core>> cores;
 uint64_t tick=0;unsigned depth,faultRole=3,peakReaders=0;
 Fault fault=None;bool injected=false;
 static constexpr uint64_t ClockBase=UINT64_C(0x1fffffff0);
 static uint32_t Route(unsigned r){const uint32_t xy[]={17|(12<<6),18|(12<<6),17|(15<<6),
  18|(15<<6),17|(18<<6),18|(18<<6),17|(21<<6)};assert(r>=1 && r<=7);return xy[r-1];}
 static uint32_t Pattern(unsigned bank,uint32_t a){return (a*0x9e3779b9U)^((a>>11)*0x12345U)^(bank*0x7654321U);}
 unsigned Target(uint32_t xy){for(unsigned r=1;r<=7;r++)if(C::Worker(r)==xy)return r;assert(false);return 0;}
 Fabric(const std::vector<uint8_t>& boot,const std::vector<uint8_t>& text,unsigned park,unsigned bad,unsigned d):depth(d){
  for(unsigned r=0;r<=7;r++)cores.emplace_back(new Core(*this,r,boot,text,park,bad,d));
 }
 void Tick(){
  tick++;
  unsigned reading=0;
  for(auto& owned:cores){
   auto& m=*owned;
   if(m.role && m.firstRead && !m.memory[C::Done])reading++;
   if(!m.queued.empty() && tick%3==0){
    auto p=m.queued.front();m.queued.pop_front();
    m.counter[4]++;m.counter[16]++;
    if(p.write){m.counter[8]++;m.counter[10]++;m.counter[12]++;m.counter[32]++;}
    else{m.counter[5]++;m.counter[14]++;}
    // Delay separation exceeds actual interleaved command-issue time, so
    // later slots really complete first rather than merely intending to.
    p.due=tick+(m.role?4096+((depth-1)-((p.dest-C::Raw)/16384))*4096:90);
    m.flying.push_back(p);
   }
   for(unsigned i=unsigned(m.flying.size());i>0;i--){
    const auto p=m.flying[i-1];if(p.due>tick)continue;
    if(m.role && fault==LostRead && m.role==faultRole && m.completed==0){injected=true;continue;}
    if(!m.role){
     const unsigned target=Target(p.write?p.destXY:p.sourceXY);auto& remote=*cores[target];
     if(p.write){
      assert(p.source==C::Payload && p.dest==C::Start && p.bytes==4);
      remote.memory[p.dest]=m.memory.at(p.source);
      if(fault==LostStartAck && target==faultRole){injected=true;continue;}
      m.counter[1]++;m.counter[32]--;
     }else{
      assert(p.dest==C::Scratch && (p.source==C::Ready || p.source==C::Done || p.source==C::Job || p.source==C::Report));
      for(unsigned w=0;w<p.bytes/4;w++)m.memory[p.dest+w*4]=remote.memory.at(p.source+w*4);
      if(fault==CorruptPeerReport && target==faultRole && p.source==C::Report){m.memory[p.dest+12]^=1;injected=true;}
      m.counter[2]++;m.counter[3]+=(p.bytes+63)/64;
     }
    }else{
     assert(!p.write && p.sourceXY==Route(m.role) && p.destXY==C::Worker(m.role) && p.bytes==16384);
     for(unsigned w=0;w<p.bytes/4;w++)m.raw[(p.dest-C::Raw)/4+w]=Pattern(m.role,p.source+w*4);
     m.counter[2]++;m.counter[3]+=p.bytes/64;m.lastRead=tick;
     if(fault==ExtraResponse && m.role==faultRole && m.completed==0){m.counter[2]++;injected=true;}
     if(fault==ChangedJob && m.role==faultRole && m.completed==0){m.memory[C::Job+12]^=1;injected=true;}
    }
    if(i>1)m.reordered++;
    m.counter[16]--;m.completed++;m.flying.erase(m.flying.begin()+i-1);
   }
  }
  peakReaders=std::max(peakReaders,reading);
 }
 bool Execute(){
  for(unsigned round=0;round<30000000;round++){
   bool all=true;
   for(auto& owned:cores){
    auto& m=*owned;
    if(m.pc==m.bad){
     const unsigned requests=m.requests;
     for(unsigned i=0;i<32;i++)m.Step();
     assert(m.requests==requests && m.memory.at(m.pc)==0x0000006f);
     return false; // observer stops this whole HOST test; no native lease claim
    }
    if(m.pc==m.park)continue;
    all=false;Tick();m.Step();
   }
   if(all)return true;
  }
  assert(false && "cohort instruction observer bound");return false;
 }
 void Check(){
  assert(peakReaders==7);
  const auto& root=*cores[0];const auto r=[&](unsigned i){return root.memory.at(C::Report+i*4);};
  assert(r(0)==C::ReportMagic && r(1)==4 && r(2)==0 && r(3)==0x12345678 && r(4)==1 && r(5)==depth);
  assert(((uint64_t(r(7))<<32)|r(6))==uint64_t(C::Span)*7 && r(15)==512*7);
  const uint64_t a=(uint64_t(r(9))<<32)|r(8),b=(uint64_t(r(11))<<32)|r(10);
  assert(b>a && ((uint64_t(r(13))<<32)|r(12))==b-a);
  uint64_t release=0;
  for(unsigned role=1;role<=7;role++){
   const auto& m=*cores[role];assert(m.requests==512 && m.completed==512 && m.queued.empty() && m.flying.empty());
   assert(m.firstRead+ClockBase>a && m.lastRead+ClockBase<b && m.memory.at(C::Done)==0x12345678);
   assert(m.counter[2]==uint32_t(0xfffffff0U+512) && m.counter[3]==uint32_t(0xfffffff0U+512*256));
   const uint64_t now=(uint64_t(r(17+role*2))<<32)|r(16+role*2);
   assert(now>release && now<b-a);release=now;
   for(unsigned i=0;i<C::Words;i++)assert(root.memory.at(C::PeerReports+(role-1)*128+i*4)==m.memory.at(C::Report+i*4));
   for(unsigned w=0;w<depth*16384/4;w++)assert(m.raw[w]==Pattern(role,C::Base+C::Span-depth*16384+w*4));
   for(unsigned w=depth*16384/4;w<m.raw.size();w++)assert(m.raw[w]==0xcafebabe);
   if(depth>1)assert(m.maxPending>1 && m.reordered);
  }
  for(const auto& m:cores){
   assert(m->minStack>=0x6e000 && !m->matrixInstructions && !m->moveInstructions && !m->configWrites);
   assert(m->memory.at(m->pc)==0x0000006f);
  }
 }
};
Core::Core(Fabric& f,unsigned r,const std::vector<uint8_t>& boot,const std::vector<uint8_t>& text,unsigned good,unsigned fail,unsigned d)
 :Machine(),fabric(f),role(r),textBytes(unsigned(text.size())),park(good),bad(fail),raw(65536,0xcafebabe){
 assert(boot.size()==4 && text.size()%4==0);memory[0x6000]=Word(boot,0);
 for(unsigned i=0;i<text.size();i+=4)memory[0x64000+i]=Word(text,i);
 for(unsigned i=0;i<32;i++)memory[C::Job+i*4]=memory[C::Report+i*4]=0;
 const uint32_t j[]={C::Magic,4,r,0x12345678,1,14,d};
 for(unsigned i=0;i<7;i++)memory[C::Job+i*4]=j[i];
 for(unsigned i=1;i<=7;i++)memory[C::Job+(6+i)*4]=Fabric::Route(i);
 memory[C::Ready]=memory[C::Start]=memory[C::Done]=0;
 for(unsigned index:{1U,2U,3U,4U,5U,8U,10U,12U,14U})counter[index]=0xfffffff0;
}
uint32_t Core::ReadCycleCsr(unsigned csr){
 assert(csr==0xb00 || csr==0xb80);csrReads++;
 uint64_t value=fabric.tick+Fabric::ClockBase;
 if(!role && fabric.fault==Fabric::BackwardRootClock && csrReads>24){value=1;fabric.injected=true;}
 return csr==0xb00?uint32_t(value):uint32_t(value>>32);
}
uint32_t Core::Load(uint32_t a){
 assert(!(a&3));
 if(CodeAddress(a))return memory.at(a); // immutable emitted read-only tables
 if(Local(a)){
  if(a==C::Start && role==fabric.faultRole && fabric.fault==Fabric::WrongStart){fabric.injected=true;return 0xbad;}
  return memory[a];
 }
 if(a>=C::Raw && a<C::Raw+0x40000){assert(queued.empty() && flying.empty() && completed);return raw[(a-C::Raw)/4];}
 mmioReads++;
 if(a>=0xffb20000 && a<0xffb20028)return cmd[(a-0xffb20000)/4];
 if(a==0xffb20044){
  uint32_t xy=role?C::Worker(role):129U;
  if(role==fabric.faultRole && (fabric.fault==Fabric::WrongTile || (fabric.fault==Fabric::ChangedTile && completed))){xy^=1;fabric.injected=true;}
  return xy|(17U<<12)|(12U<<19);
 }
 if(a==0xffb20048)return 0x10000;
 if(a==0xffb20100)return role==fabric.faultRole && fabric.fault==Fabric::DisabledFifo?0x6005:0x16005;
 if(a==0xffb20064)return role==fabric.faultRole && fabric.fault==Fabric::DirtyFifo?0x1010100f:0x10101000|uint32_t(16-queued.size());
 if(a==0xffb20068)return 0;
 for(unsigned i=0;i<4;i++)if(a==0xffb20040+i*0x800)return 0;
 if(a>=0xffb20200 && a<0xffb20300)return counter[(a-0xffb20200)/4];
 fprintf(stderr,"cohort unexpected load role%u pc%x a%x\n",role,pc,a);assert(false);return 0;
}
void Core::Store(uint32_t a,uint32_t value){
 assert(!(a&3));
 if(Local(a)){
  assert(!(a>=C::Job && a<C::Job+128));
  if(a>=0x6e000 && a<0x70000)minStack=std::min(minStack,a);
  memory[a]=value;return;
 }
 mmioWrites++;
 if(a>=0xffb20000 && a<0xffb20028){cmd[(a-0xffb20000)/4]=value;return;}
 assert(a==0xffb20040 && value==1 && queued.size()<16);
 const bool write=cmd[7]==0x212;
 if(role){
  assert(!write && cmd[7]==0 && cmd[0]==C::Base+((requests*16384)&(C::Span-1))
   && cmd[2]==Fabric::Route(role) && cmd[3]==C::Raw+(requests&(fabric.depth-1))*16384
   && cmd[5]==C::Worker(role) && cmd[8]==16384);
  if(!(requests&(fabric.depth-1)))assert(queued.empty() && flying.empty() && requests==completed);
  if(!firstRead)firstRead=fabric.tick;
 }else{
  assert(cmd[7]==0 || write);
  assert((write?cmd[2]:cmd[5])==129 && (write?cmd[0]:cmd[3])==(write?C::Payload:C::Scratch));
 }
 assert(!cmd[1] && !cmd[4] && !cmd[6] && !cmd[9]);
 queued.push_back({cmd[0],cmd[3],cmd[2],cmd[5],cmd[8],write,0});requests++;
 maxPending=std::max(maxPending,unsigned(queued.size()+flying.size()));
 assert(maxPending<=(role?fabric.depth:1));
}
int main(int argc,char** argv){
 assert(argc==5);const auto boot=Read(argv[1]),text=Read(argv[2]);
 const unsigned good=unsigned(std::stoul(argv[3],nullptr,16)),bad=unsigned(std::stoul(argv[4],nullptr,16));
 unsigned valid=0,refused=0;
 for(unsigned depth:{1U,4U,16U}){Fabric f(boot,text,good,bad,depth);assert(f.Execute());f.Check();valid++;}
 for(unsigned word=0;word<32;word++){
  Fabric f(boot,text,good,bad,16);
  if(word==3)f.cores[0]->memory[C::Job+12]=0; // nonzero nonce changes are valid DATA
  else f.cores[0]->memory[C::Job+word*4]^=0x80000000;
  assert(!f.Execute() && !f.cores[0]->requests && !f.cores[0]->mmioReads && !f.cores[0]->mmioWrites && !f.cores[0]->csrReads);refused++;
 }
 {Fabric f(boot,text,good,bad,16);f.cores[0]->memory[C::Job+12]^=0x80000000;
  assert(!f.Execute() && f.cores[0]->requests); // real cross-participant ready mismatch
  for(unsigned role=1;role<=7;role++)assert(!f.cores[role]->requests);
  refused++;}
 for(auto fault:{Fabric::DirtyReport,Fabric::DuplicateRoute,Fabric::DisabledFifo,Fabric::DirtyFifo,Fabric::WrongTile,Fabric::WrongStart,
  Fabric::LostRead,Fabric::ExtraResponse,Fabric::ChangedJob,Fabric::LostStartAck,Fabric::CorruptPeerReport,Fabric::BackwardRootClock,Fabric::ChangedTile}){
  Fabric f(boot,text,good,bad,16);f.fault=fault;
  if(fault==Fabric::DirtyReport)f.cores[3]->memory[C::Report+4]=1;
  if(fault==Fabric::DuplicateRoute)f.cores[0]->memory[C::Job+32]=Fabric::Route(1);
  const bool accepted=f.Execute();if(accepted)fprintf(stderr,"unexpected cohort fault success %u\n",unsigned(fault));
  assert(!accepted && f.cores[0]->memory.at(C::Report)!=C::ReportMagic);
  if(fault>=Fabric::WrongStart)assert(f.injected);
  if(fault==Fabric::LostRead)assert(!f.cores[3]->memory.at(C::Done) && !f.cores[3]->flying.empty());
  refused++;
 }
 printf("PASS_COHORT_ACTUAL_RV32 valid%u refused%u all8virginEngines1 all7overlap1 commonCoordinatorInterval1 retainedCapturedCommands1 fullFinalBatch1 immutableCode1;ASYNCHRONOUS_SIMULATION_NOT_NATIVE_OR_HARDWARE_BANDWIDTH_FREQUENCY_MODEL_WORK\n",valid,refused);
}
