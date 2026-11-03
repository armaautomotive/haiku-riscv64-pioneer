/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DRAM_COHORT_READ_SWEEP_OWNER_H
#define PIONEER_DRAM_COHORT_READ_SWEEP_OWNER_H
#include "BlackholeComputeNocProbe.h"
#include "BlackholeDramCohortReadSweepImage.h"
#include "../../../../../tools/pioneer/PioneerDramCohortReadSweep.h"
#include "../../../../../tools/pioneer/PioneerDramCohortHostBracket.h"
// PRIVATE algorithm component. No native context/public opcode selects it.
// Every IO method still needs a SAME live native lease/FD/thread/generation,
// actual owned mappings and per-access bounds. Fixture scope/clock callbacks
// are NOT that native proof. Never use this component as a model/Work fallback.
struct PioneerDramCohortReadSweepResult {
 uint32_t version=6,started=0,phase=0,unsafe=0,launched=0,observed=0,held=0,closed=0;
 uint32_t savedWords=0,restoredWords=0,comparedWords=0,report[8][32]={};
 uint32_t reserved=0;
 PioneerDramCohortHostTiming timing;
};
class PioneerDramCohortReadSweepOwner {
 using I=BlackholeDramCohortReadSweepImage;
 using P=PioneerDramCohortReadSweep;
 const tt_dram_samples_result* proof=nullptr;
 tt_dram_samples_result routes={}; // heap-owned with the owner,not a kernel stack copy
 uint32_t* saved=nullptr;uint32_t* scratch=nullptr;
 uint32_t tlb[2][3]={},command[8][10]={},before[8][9]={},frame[8][32]={};
 uint32_t reset[8]={0x47800,0x47800,0x47800,0x47800,0x47800,0x47800,0x47800,0x47800};
 bool fifo[8]={},attempted=false,unsafe=false;
 uint64_t began=0,lastTime=0;
 PioneerDramCohortHostBracket bracket;
 static unsigned X(unsigned role){return role?(P::Worker(role)&63):1;}
 static unsigned Offset(unsigned role){return role?I::RootSavedWords+(role-1)*I::ReaderSavedWords:0;}
 template<class IO> bool Fail(IO& io,PioneerDramCohortReadSweepResult& r){
  unsafe=true;r.unsafe=1;io.CohortQuarantineMemoryOnly();return false;
 } // FIRST uncertainty: no MMIO/OS cleanup/reset/retry/clock-close afterward
 template<class IO> bool Scope(IO& io){
  return attempted && !unsafe && proof && saved && scratch
   && io.CohortNativeScope(this,proof,saved,scratch,I::CohortSavedWords,I::RawWords);
 }
 template<class IO> bool Sources(IO& io){
  if(!Scope(io))return false;
  for(unsigned i=0;i<16;i++)if(io.SourceRead(i)!=routes.source[i])return false;
  for(unsigned p=0;p<2;p++){
   if(io.HostPciRead(p,4,2)!=0x402)return false;
   uint32_t cap=io.HostPciRead(p,0x34,1);uint64_t visited=0;bool found=false;
   for(unsigned hop=0;cap && hop<48;hop++){
    if(cap<0x40 || cap>0xfc || (cap&3) || (visited&(UINT64_C(1)<<(cap/4))))return false;
    visited|=UINT64_C(1)<<(cap/4);const uint32_t h=io.HostPciRead(p,cap,2);
    if((h&255)==0x10){
     if(found || cap>0xf4)return false;
     const unsigned v=io.HostPciRead(p,cap+2,2)&15;if(v!=1 && v!=2)return false;
     const uint32_t s=io.HostPciRead(p,cap+0xa,2);if(s>0xffff || (s&0x2f))return false;
     found=true;
    }
    cap=(h>>8)&255;
   }
   if(cap || !found)return false;
  }
  return true;
 }
 template<class IO> bool Set(IO& io,unsigned slot,uint32_t base,unsigned x,unsigned y,bool ro){
  if(!Scope(io))return false;
  for(unsigned i=0;i<3;i++){
   const uint32_t v=BlackholeDramFixedTlbWord(base,x,y,ro,i);
   io.DramTlbWrite(slot,i,v);if(io.DramTlbRead(slot,i)!=v)return false;
  }
  return true;
 }
 template<class IO> bool Select(IO& io,unsigned role){
  return role<8 && Sources(io) && Set(io,0,0,X(role),2,false)
   && Set(io,1,0xffa00000,X(role),2,false);
 }
 template<class IO> bool Idle(IO& io,unsigned role){
  for(unsigned i=0;i<4;i++)if(io.CohortNocRead(role,0x40+i*0x800))return false;
  if(io.CohortNocRead(role,0x64)!=(fifo[role]?0x10101010U:0U)
   || (io.CohortNocRead(role,0x68)&15))return false;
  for(unsigned i=0;i<32;i++)if(io.CohortNocRead(role,0x240+i*4)&255)return false;
  return true;
 }
 template<class IO> bool Guard(IO& io,unsigned role,bool idle,
  uint32_t base=0,unsigned x=0,unsigned y=2,bool ro=false){
  if(role>=8 || !Sources(io))return false;
  if(!x)x=X(role);
  for(unsigned i=0;i<3;i++)if(io.DramTlbRead(0,i)!=BlackholeDramFixedTlbWord(base,x,y,ro,i)
   || io.DramTlbRead(1,i)!=BlackholeDramFixedTlbWord(0xffa00000,X(role),2,false,i))return false;
  if(io.CohortResetRead(role)!=reset[role] || io.CohortResetRead(role)!=reset[role])return false;
  const uint32_t entry=io.CohortEntryRead(role);
  if((entry&1) || io.CohortEntryRead(role)!=entry)return false;
  for(unsigned i=0;i<16;i++)if(io.CohortNocRead(role,BlackholeDramSourceOffset(i))
   !=(i?routes.source[i]:routes.source[0]|(fifo[role]?0x10000U:0U)))return false;
  const uint32_t physical=BlackholeDramSourceX(routes.source,X(role))
   |(BlackholeDramSourceY(routes.source,2)<<6);
  const uint32_t node=io.CohortNocRead(role,0x44);
  if((node&0xfff)!=physical || ((node>>12)&127)!=17 || ((node>>19)&127)!=12
   || (io.CohortNocRead(role,0x48)&0xffffff00)!=0x10000
   || (io.CohortNocRead(role,0x148)&0xfff)!=(X(role)|(2U<<6)))return false;
  return !idle || Idle(io,role);
 }
 template<class IO> bool AllHeld(IO& io){
  const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
  for(unsigned x:columns)for(unsigned y=2;y<=11;y++)if(!Sources(io)
   || !Set(io,1,0xffa00000,x,y,true) || io.CohortHeldRead(x,y)!=0x47800
   || io.CohortHeldRead(x,y)!=0x47800)return false;
  return Select(io,0);
 }
 template<class IO> bool CodeFrame(IO& io,unsigned role){
  uint32_t v=0;
  for(unsigned i=0;i<I::CodeWords;i++)if(!io.CohortRead(role,I::Address(role==0,i),v) || v!=I::Code(i))return false;
  for(unsigned i=0;i<32;i++)if(!io.CohortRead(role,P::Job+i*4,v) || v!=frame[role][i])return false;
  return true;
 }
 template<class IO> bool BeginDeadline(IO& io){
  return Scope(io) && io.CohortHostTime(began) && began && (lastTime=began)!=0;
 }
 template<class IO> bool Deadline(IO& io,uint64_t budget){
  uint64_t now=0;if(!Scope(io) || !io.CohortHostTime(now) || now<lastTime || now-began>budget)return false;
  lastTime=now;return true;
 }
 template<class IO> bool Progress(IO& io){
  const uint32_t address[4]={P::Ready,P::Start,P::Done,P::Payload};
  for(unsigned i=0;i<4;i++){
   uint64_t before=0,after=0;uint32_t value=0;
   if(!Scope(io) || !io.CohortHostTime(before) || before<lastTime
    || !io.CohortRead(0,address[i],value) || !io.CohortHostTime(after)
    || !bracket.Observe(i,value,before,after))return false;
   lastTime=after;
  }
  return true;
 }
 template<class IO> bool Counters(IO& io,unsigned role,const uint32_t* r){
  const uint32_t reads=role?r[15]:r[14],flits=role?reads*256:r[17],writes=role?0:7;
  const uint32_t delta[9]={writes,reads,flits,reads+writes,reads,writes,writes,writes,reads};
  for(unsigned i=0;i<9;i++)if(uint32_t(io.CohortNocRead(role,P::CoordinatorLink::Offset(i))-before[role][i])!=delta[i])return false;
  return true;
 }
 bool Report(unsigned role,const uint32_t* r)const{
  const uint64_t a=(uint64_t(r[9])<<32)|r[8],b=(uint64_t(r[11])<<32)|r[10];
  const uint64_t bytes=uint64_t(P::Span)*frame[role][4]*(role?1:7);
  if(r[0]!=P::ReportMagic || r[1]!=P::Version || r[2]!=role || r[3]!=frame[role][3]
   || r[4]!=frame[role][4] || r[5]!=frame[role][6] || ((uint64_t(r[7])<<32)|r[6])!=bytes
   || b<=a || ((uint64_t(r[13])<<32)|r[12])!=b-a
   || r[15]!=(P::Span/16384)*frame[role][4]*(role?1:7))return false;
  if(role){
   if(r[14]!=frame[role][6+role])return false;
   for(unsigned i=18;i<32;i++)if(r[i])return false;
  }else{
   if(r[14]<28 || r[14]>21+7*P::PollLimit || r[16]!=7 || r[17]!=r[14]+14)return false;
   uint64_t last=0;
   for(unsigned p=1;p<=7;p++){
    const uint64_t release=(uint64_t(r[17+p*2])<<32)|r[16+p*2];
    if(release<=last || release>=b-a)return false;
    last=release;
   }
  }
  return true;
 }
public:
 using SnapshotImage=I;
 using DeviceProgram=P;
 static constexpr unsigned Allocation=I::CohortSavedWords+I::RawWords;
 template<class IO> bool Execute(IO& io,const tt_dram_samples_result& d,unsigned sweeps,unsigned depth,
  uint32_t nonce,PioneerDramCohortReadSweepResult& r){
  static_assert(P::ExternalStartWatchdogRequired==1 && P::Version==6,"owned startup waits require external deadline");
  static_assert(sizeof(PioneerDramCohortReadSweepResult)==1144,"no hidden padding in pristine DATA comparison");
  const PioneerDramCohortReadSweepResult pristine;
  const auto* a=reinterpret_cast<const uint8_t*>(&r);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
  for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
  if(attempted || !nonce || !sweeps || sweeps>256 || !depth || depth>16 || (depth&(depth-1))
   || d.version!=1 || d.state!=TT_DRAM_SAMPLE_ROUTES_OK || d.routes_verified!=7 || !d.tlb_restored
   || !d.data_restored || d.recovery_required || (d.source[0]&0x10000))return false;
  routes=d;if(!BlackholeDramBuildRoutes(routes))return false;
  for(unsigned bank=0;bank<7;bank++)for(unsigned i=0;i<2;i++)if(routes.translated[bank][i]!=d.translated[bank][i])return false;
  for(unsigned role=0;role<8;role++){
   auto* f=frame[role];f[0]=P::Magic;f[1]=P::Version;f[2]=role;f[3]=nonce;f[4]=sweeps;f[5]=14;f[6]=depth;
   for(unsigned i=0;i<7;i++)f[7+i]=routes.translated[i][0]|(routes.translated[i][1]<<6);
   if(!P::Shape(f))return false;
  }
  if(io.CohortAllocationWords()!=Allocation)return false;
  saved=io.CohortSavedWords();scratch=io.CohortScratchWords();
  if(!saved || !scratch || scratch!=saved+I::CohortSavedWords)return false;
  proof=&d;attempted=true;if(!Scope(io))return false;
  r.started=1;r.phase=1;
  if(!Sources(io) || !io.CohortClockBegin())return Fail(io,r);
  for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++){
   tlb[s][i]=io.DramTlbRead(s,i);if(i==2 && (tlb[s][i]&0xffffc000))return Fail(io,r);
  }
  for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++)if(io.DramTlbRead(s,i)!=tlb[s][i])return Fail(io,r);
  if(!AllHeld(io))return Fail(io,r);
  r.phase=2;
  for(unsigned role=0;role<8;role++){
   if(!Select(io,role) || !Guard(io,role,true) || (io.CohortNocRead(role,0x178)&0xffff))return Fail(io,r);
   for(unsigned i=0;i<10;i++){command[role][i]=io.CohortNocRead(role,i*4);if(command[role][i])return Fail(io,r);}
   for(unsigned i=0;i<9;i++)before[role][i]=io.CohortNocRead(role,P::CoordinatorLink::Offset(i));
   for(unsigned i=0;i<I::SavedWords(role==0);i++){
    if(!(i&255) && !Guard(io,role,true))return Fail(io,r);
    if(!io.CohortRead(role,I::Address(role==0,i),saved[Offset(role)+i]))return Fail(io,r);
    r.savedWords++;
   }
   for(unsigned i=0;i<I::SavedWords(role==0);i++){
    uint32_t v=0;if(!(i&255) && !Guard(io,role,true))return Fail(io,r);
    if(!io.CohortRead(role,I::Address(role==0,i),v) || v!=saved[Offset(role)+i])return Fail(io,r);
   }
  }
  r.phase=3;
  for(unsigned role=0;role<8;role++){
   if(!Select(io,role) || !Guard(io,role,true))return Fail(io,r);
   io.CohortNocWrite(role,0x100,routes.source[0]|0x10000);fifo[role]=true;
   if(!Guard(io,role,true))return Fail(io,r);
   for(unsigned i=1;i<I::CodeWords;i++)if(!io.CohortWrite(role,I::Address(role==0,i),I::Code(i)))return Fail(io,r);
   for(unsigned i=0;i<32;i++)if(!io.CohortWrite(role,P::Job+i*4,frame[role][i])
    || !io.CohortWrite(role,P::Report+i*4,0))return Fail(io,r);
   for(unsigned signal=0;signal<3;signal++)if(!io.CohortWrite(role,P::Ready+signal*32,0))return Fail(io,r);
   if(!role){
    if(!io.CohortWrite(0,P::Payload,0))return Fail(io,r);
    for(unsigned i=0;i<32;i++)if(!io.CohortWrite(0,P::Scratch+i*4,0))return Fail(io,r);
    for(unsigned i=0;i<224;i++)if(!io.CohortWrite(0,P::PeerReports+i*4,0))return Fail(io,r);
   }
   if(!io.CohortWrite(role,0x6000,I::Code(0)) || !CodeFrame(io,role))return Fail(io,r);
   for(unsigned i=0;i<32;i++){uint32_t v=1;if(!io.CohortRead(role,P::Report+i*4,v) || v)return Fail(io,r);}
   for(unsigned signal=0;signal<3;signal++){
    const unsigned addr=P::Ready+signal*32;
    uint32_t v=1;if(!io.CohortRead(role,addr,v) || v)return Fail(io,r);
   }
  }
  r.phase=4;
  for(unsigned role=0;role<8;role++){
   if(!Select(io,role) || !Guard(io,role,true))return Fail(io,r);
   reset[role]=0x87ffff;io.CohortResetWrite(role,reset[role]);if(!Guard(io,role,true))return Fail(io,r);
   for(unsigned i=0;i<9;i++)before[role][i]=io.CohortNocRead(role,P::CoordinatorLink::Offset(i));
  }
  r.phase=5;
  for(unsigned role=1;role<8;role++){
   if(!BeginDeadline(io) || !Select(io,role) || !Guard(io,role,true) || !CodeFrame(io,role))return Fail(io,r);
   r.launched|=1U<<role;reset[role]=0x46800;io.CohortResetWrite(role,reset[role]);
   bool ready=false;
   for(unsigned poll=0;poll<P::PollLimit;poll++){
    if(!Deadline(io,UINT64_C(10000000)) || !Guard(io,role,true))return Fail(io,r);
    uint32_t v=0;if(!io.CohortRead(role,P::Ready,v) || (v && v!=nonce))return Fail(io,r);
    if(v==nonce){ready=true;break;}
    io.CohortPause();
   }
   if(!ready || !CodeFrame(io,role))return Fail(io,r);
  }
  if(!Select(io,0) || !Guard(io,0,true) || !CodeFrame(io,0) || !BeginDeadline(io))return Fail(io,r);
  if(!bracket.Arm(nonce,began))return Fail(io,r);
  r.launched|=1;reset[0]=0x46800;io.CohortResetWrite(0,reset[0]);
  bool done=false;
  for(unsigned poll=0;poll<P::PollLimit;poll++){
   if(!Deadline(io,UINT64_C(120000000)) || !Guard(io,0,false) || !Progress(io))return Fail(io,r);
   uint32_t v=0;if(!io.CohortRead(0,P::Report,v) || (v && v!=P::ReportMagic))return Fail(io,r);
   if(v==P::ReportMagic){done=true;break;}io.CohortPause();
  }
  if(!done)return Fail(io,r);
  for(unsigned role=0;role<8;role++){
   if(!Select(io,role))return Fail(io,r);
   bool parked=false;
   for(unsigned poll=0;poll<P::PollLimit;poll++){
    if(!Deadline(io,UINT64_C(120000000)) || !Guard(io,role,false))return Fail(io,r);
    uint32_t marker=0;if(!io.CohortRead(role,P::Report,marker) || (marker && marker!=P::ReportMagic))return Fail(io,r);
    if(marker){
     const uint32_t pc=io.CohortPcRead(role),q=io.CohortPcRead(role);
     if((pc==I::Park || pc==I::Park+4) && (q==I::Park || q==I::Park+4)){parked=true;break;}
    }
    io.CohortPause();
   }
   if(!parked || !Guard(io,role,true) || !CodeFrame(io,role))return Fail(io,r);
   for(unsigned i=0;i<32;i++)if(!io.CohortRead(role,P::Report+i*4,r.report[role][i]))return Fail(io,r);
   if(!Report(role,r.report[role]) || !Counters(io,role,r.report[role]))return Fail(io,r);
   if(role)for(unsigned signal=0;signal<3;signal++){
    const unsigned addr=P::Ready+signal*32;
    uint32_t v=0;if(!io.CohortRead(role,addr,v) || v!=nonce)return Fail(io,r);
   }
   r.observed|=1U<<role;
  }
  if(!Select(io,0) || !Guard(io,0,true))return Fail(io,r);
  for(unsigned i=0;i<4;i++){
   const uint32_t address[4]={P::Ready,P::Start,P::Done,P::Payload};uint32_t v=0;
   if(!io.CohortRead(0,address[i],v) || v!=(i==3?~nonce:nonce))return Fail(io,r);
  }
  for(unsigned role=1;role<8;role++)for(unsigned i=0;i<32;i++){
   uint32_t v=0;if(!io.CohortRead(0,P::PeerReports+(role-1)*128+i*4,v) || v!=r.report[role][i])return Fail(io,r);
  }
  r.phase=6;
  for(unsigned role=0;role<8;role++){
   if(!Select(io,role) || !Guard(io,role,true))return Fail(io,r);
   reset[role]=0x47800;io.CohortResetWrite(role,reset[role]);
   if(!Guard(io,role,true))return Fail(io,r);
   r.held|=1U<<role;
  }
  if(!AllHeld(io))return Fail(io,r);
  r.phase=7;
  const unsigned batch=depth*16384,words=batch/4;
  for(unsigned role=1;role<8;role++){
   if(!Select(io,role) || !Guard(io,role,true))return Fail(io,r);
   for(unsigned i=0;i<words;i++)if(!io.CohortRead(role,P::Raw+i*4,scratch[i]))return Fail(io,r);
   if(scratch[0]!=r.report[role][16] || scratch[words-1]!=r.report[role][17])return Fail(io,r);
   const unsigned x=routes.translated[role-1][0],y=routes.translated[role-1][1];
   const uint32_t last=P::Base+P::Span-batch,page=last&~0x1fffffU;
   if(!Set(io,0,page,x,y,true) || !Guard(io,role,true,page,x,y,true))return Fail(io,r);
   for(unsigned i=0;i<words;i++){
    uint32_t v=0;if(!io.CohortWindowRead(role,last-page+i*4,v) || v!=scratch[i])return Fail(io,r);
    r.comparedWords++;
   }
   if(!Guard(io,role,true,page,x,y,true))return Fail(io,r);
  }
  r.phase=8;
  for(unsigned role=0;role<8;role++){
   if(!Select(io,role) || !Guard(io,role,true) || !CodeFrame(io,role) || !Counters(io,role,r.report[role]))return Fail(io,r);
   const unsigned count=I::SavedWords(role==0),offset=Offset(role);
   for(unsigned i=1;i<count;i++){
    uint32_t v=0;if(!(i&255) && !Guard(io,role,true))return Fail(io,r);
    if(!io.CohortWrite(role,I::Address(role==0,i),saved[offset+i])
     || !io.CohortRead(role,I::Address(role==0,i),v) || v!=saved[offset+i])return Fail(io,r);
    r.restoredWords++;
   }
   uint32_t v=0;if(!io.CohortWrite(role,0x6000,saved[offset]) || !io.CohortRead(role,0x6000,v) || v!=saved[offset])return Fail(io,r);
   r.restoredWords++;
   for(unsigned i=0;i<count;i++){
    if(!(i&255) && !Guard(io,role,true))return Fail(io,r);
    if(!io.CohortRead(role,I::Address(role==0,i),v) || v!=saved[offset+i])return Fail(io,r);
   }
   for(unsigned i=0;i<10;i++){
    io.CohortNocWrite(role,i*4,command[role][i]);if(io.CohortNocRead(role,i*4)!=command[role][i])return Fail(io,r);
   }
   io.CohortNocWrite(role,0x100,routes.source[0]);fifo[role]=false;
   if(!Guard(io,role,true))return Fail(io,r);
  }
  r.phase=9;
  for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++){
   io.DramTlbWrite(s,i,tlb[s][i]);if(io.DramTlbRead(s,i)!=tlb[s][i])return Fail(io,r);
  }
  if(!Sources(io) || !io.CohortClockEnd() || !Sources(io))return Fail(io,r);
  for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++)if(io.DramTlbRead(s,i)!=tlb[s][i])return Fail(io,r);
  bracket.Finish((uint64_t(r.report[0][13])<<32)|r.report[0][12],
   (uint64_t(r.report[0][7])<<32)|r.report[0][6],r.timing);
  r.phase=10;r.closed=1;return true;
 }
};
#endif
