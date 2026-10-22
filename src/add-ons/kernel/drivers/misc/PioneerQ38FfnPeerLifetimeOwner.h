/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_PEER_LIFETIME_OWNER_H
#define PIONEER_Q38_FFN_PEER_LIFETIME_OWNER_H
#include <stddef.h>
#include "PioneerQ38FfnGraphCodeOwner.h"
#include "PioneerQ38FfnPersistentShape.h"
// PRIVATE worker execution-lifetime qualifier,NOT full FFN/model compute.
// The actual native cookie/global call lock must exclude EVERY other writer
// throughout this ONE call. Retain BOTH heap owners and the whole native
// context/map/host/clock/cold latch on any failure. No native route selects
// this helper yet;IO test callbacks are NOT ownership or launch authority.
// Root NEVER runs. No weights,input,GDDR,host DMA or NoC packet is touched.
// ALL119 NEW workers boot,receive ONLY internally constructed STOP(seq1),
// prove current physical terminals BEFORE ANY hold,then restore ALL saved
// code/commands. There is NO caller job/address/clean/completed hint API.
struct PioneerQ38FfnPeerLifetimeResult {
 uint32_t version=1,started=0,passed=0,unsafe=0,phase=0,worker=0,word=0;
 uint32_t savedWords=0,installedWords=0,restoredWords=0,armed=0,released=0,ready=0,stopIssued=0,stopped=0,held=0;
 uint32_t savedFrameWords=0,restoredFrameWords=0;
};
class PioneerQ38FfnPeerLifetimeOwner {
 PioneerQ38FfnGraphCodeOwner* code=nullptr;
 const tt_dram_samples_result* proof=nullptr;
 PioneerResidentBanks* banks=nullptr;
 PioneerQ38DispatchLease* cohort=nullptr;
 uint32_t reset[120]={};
 uint32_t frames[119][96]={}; // HEAP ONLY:74000..74180 is OUTSIDE code snapshot
 bool running[120]={},bootCommand[120]={};
 bool started=false,unsafe=false,closed=false;
 static unsigned Route(unsigned w){return PioneerWorkerX(w)|(PioneerWorkerY(w)<<6);}
 template<class IO> bool Fail(IO& io,PioneerQ38FfnPeerLifetimeResult& r) {
  unsafe=true;r.unsafe=1;code->unsafe=true;PioneerQ38DispatchQuarantine(*cohort,*banks);io.QuarantineHostPage();return false;
 }
 template<class IO> bool Guard(IO& io,unsigned w) {
  if(w>=120 || unsafe || closed || !code || code->unsafe || code->closed || code->proof!=proof
   || code->banks!=banks || code->cohort!=cohort || cohort->unsafe || cohort->closed || cohort->rootRunning
   || cohort->prepared!=120 || cohort->bootAttempted || cohort->booted || !banks->Matches(*proof))return false;
  auto& n=cohort->worker[w];const auto& store=n.Store();
  if(!n.image.saved || !n.image.ready || n.image.unsafe || n.image.running!=running[w]
   || !store.ready || store.pending || store.recovery_required || n.noc.recovery_required
   || !io.SelectRuntimeCore(w) || !io.Q38PanelMappingStable() || !io.FfnGraphCodeMappingStable(w)
   || io.ResetRead()!=reset[w] || io.ResetRead()!=reset[w])return false;
  for(unsigned i=0;i<16;i++)if(io.ResidentNocRead(BlackholeDramSourceOffset(i))!=cohort->authority.source[w][i])return false;
  for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)if(io.SourceRead(i)!=proof->source[i])return false;
  for(unsigned p=0;p<2;p++)if(!PioneerQ38GraphCsrPciQuiet(io,p))return false;
  if(!PioneerQ38GraphCsrIdle(io))return false;
  if(w<code->sampled) {
   for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=(bootCommand[w]?(i==5?Route(w):0):code->command[w][i]))return false;
   for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=code->counters[w][i])return false;
  }
  return true;
 }
 template<class IO> bool Image(IO& io,unsigned w,bool full=false) {
  for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++) {
   uint32_t a=PioneerQ38FfnGraphCodeAddress(w,i);
   if(full || PioneerQ38FfnGraphProgramImmutable(w,a)){uint32_t value;
    if(!io.FfnGraphProgramReadChecked(w,a,value) || value!=PioneerQ38FfnGraphProgramWord(w,a))return false;}
  }
  return true;
 }
 template<class IO> bool Stage(IO& io,uint32_t a,const uint32_t* words,unsigned n) {
  for(unsigned i=0;i<n;i++){io.Q38PanelWrite(a+i*4,words[i]);if(io.Q38PanelRead(a+i*4)!=words[i])return false;}return true;
 }
 // -1 is FIRST observed invalid field,0 is known-code transition,1 is a
 // CURRENT physical WAIT/STOP. A previously observed ready bit is no grant.
 template<class IO> int Status(IO& io,unsigned w,bool stop) {
  uint32_t marker=io.Q38PanelRead(0x7414c);
  if(marker!=TT_Q38_FFN_PERSISTENT_MARKER && (stop || (marker && marker!=0xdeadbeef)))return -1;
  bool published=marker==TT_Q38_FFN_PERSISTENT_MARKER,done=false;
  if(published) {
   if(io.Q38PanelRead(0x74148))return -1;
   // Finish is written AFTER start. Sample finish FIRST so forward
   // progress between non-atomic PCIe reads cannot appear as b>a.
   uint32_t b=io.Q38PanelRead(0x74144);if(b>unsigned(stop))return -1;
   uint32_t a=io.Q38PanelRead(0x74140);if(a>unsigned(stop) || b>a)return -1;
   uint32_t op=io.Q38PanelRead(0x74150);if(op && (!stop || op!=2))return -1;
   for(unsigned i=5;i<16;i++)if(io.Q38PanelRead(0x74140+i*4))return -1;
   done=stop?a==1 && b==1 && op==2:!a && !b && !op;
  }
  bool terminal=true;
  for(unsigned i=0;i<2;i++) {
   io.Pause();uint32_t pc=io.RuntimePcRead();
   if(!PioneerQ38FfnGraphExecutableAddress(w,pc) || pc==TT_Q38_PEER_FAULT_ENTRY || pc==TT_Q38_PEER_FAULT_PARK
    || (!stop && PioneerQ38PeerTerminalPc(pc)))return -1;
   // Earlier status samples may precede completion while this PC sample
   // already sees STOP. That is pending,not permission OR corruption;
   // re-sample the full receipt until status+physical PC agree together.
   terminal &= stop?PioneerQ38PeerTerminalPc(pc):pc==TT_Q38_PEER_WAIT_LOAD || pc==TT_Q38_PEER_WAIT_BRANCH;
  }
  return done && terminal?1:0;
 }
 template<class IO> bool Frame(IO& io,unsigned w,bool stop) {
  for(unsigned i=0;i<8;i++)if(io.Q38PanelRead(0x74040+i*4)!=(i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?w:i==2?Route(w):i==3?cohort->authority.source[w][0]:0))return false;
  for(unsigned i=0;i<16;i++) {
   uint32_t expected=!stop?0:i==10?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==11?w:i==12?1:i==13?2:0;
   if(io.Q38PanelRead(0x74000+i*4)!=expected)return false;
  }
  return io.Q38PanelRead(0x74100)==unsigned(stop);
 }
 template<class IO> bool WaitAll(IO& io,bool stop,PioneerQ38FfnPeerLifetimeResult& r) {
  bool ready[119]={};unsigned count=0;uint64_t began=io.Q38CohortNow();
  if(began>UINT64_MAX-UINT64_C(30000000))return false;
  for(unsigned attempt=0;attempt<65536 && count<119;attempt++) {
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return false;
   for(unsigned w=1;w<120;w++)if(!ready[w-1]) {
    r.worker=w;if(!Guard(io,w))return false;
    int state=Status(io,w,stop);if(state<0)return false;
    if(state==1){if(!Image(io,w) || !Frame(io,w,stop) || !Guard(io,w))return false;
     ready[w-1]=true;count++;if(stop)r.stopped++;else r.ready++;}
   }
   io.Pause();
  }
  return count==119;
 }
public:
 PioneerQ38FfnPeerLifetimeOwner()=default;
 PioneerQ38FfnPeerLifetimeOwner(const PioneerQ38FfnPeerLifetimeOwner&)=delete;
 PioneerQ38FfnPeerLifetimeOwner& operator=(const PioneerQ38FfnPeerLifetimeOwner&)=delete;
 bool Unsafe()const{return unsafe;}
 bool Closed()const{return closed;}
 template<class IO> bool Qualify(IO& io,const tt_dram_samples_result& dram,PioneerResidentBanks& bankOwner,
  PioneerQ38DispatchLease& participants,PioneerQ38FfnGraphCodeOwner& codeOwner,PioneerQ38FfnPeerLifetimeResult& r) {
  if(started || unsafe || closed || code || codeOwner.started || codeOwner.unsafe || codeOwner.closed
   || codeOwner.proof || codeOwner.banks || codeOwner.cohort || participants.unsafe || participants.closed
   || participants.rootRunning || participants.prepared!=120 || participants.bootAttempted || participants.booted
   || r.version!=1 || r.started || r.passed || r.unsafe || r.phase || r.worker || r.word || r.savedWords
   || r.installedWords || r.restoredWords || r.armed || r.released || r.ready || r.stopIssued || r.stopped || r.held
   || r.savedFrameWords || r.restoredFrameWords
   || !bankOwner.Matches(dram))return false;
  // An existing uncertain/partial native parent gets NO further IO,even
  // if a caller omitted its aggregate unsafe bit. These CPU checks are
  // refusals only,not substitutes for the physical proofs below.
  for(unsigned w=0;w<120;w++){auto& n=participants.worker[w];const auto& s=n.Store();
   if(!n.image.saved || !n.image.ready || n.image.unsafe || n.image.running || !s.ready
    || s.pending || s.recovery_required || n.noc.recovery_required)return false;}
  started=true;code=&codeOwner;proof=&dram;banks=&bankOwner;cohort=&participants;r.started=1;
  code->started=true;code->proof=proof;code->banks=banks;code->cohort=cohort;
  for(auto& value:reset)value=0x47800;
  r.phase=1;
  for(unsigned w=0;w<120;w++) {
   r.worker=w;if(!Guard(io,w) || !PioneerQ38GraphCsrCode(io,w))return Fail(io,r);
   for(unsigned i=0;i<10;i++)code->command[w][i]=io.ResidentNocRead(i*4);
   for(unsigned i=0;i<8;i++){code->counters[w][i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
    if(i==6 && code->counters[w][2]!=code->counters[w][6])return Fail(io,r);}
   code->sampled=w+1;
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++){r.word=i;uint32_t value;
    if(!io.FfnGraphProgramReadChecked(w,PioneerQ38FfnGraphCodeAddress(w,i),value))return Fail(io,r);
    code->saved[PioneerQ38FfnGraphCodeBase(w)+i]=value;r.savedWords++;}
   if(w)for(unsigned i=0;i<96;i++){frames[w-1][i]=io.Q38PanelRead(0x74000+i*4);r.savedFrameWords++;}
   if(!Guard(io,w))return Fail(io,r);
  }
  if(r.savedWords!=code->Words || r.savedFrameWords!=119*96)return Fail(io,r);
  r.phase=2;
  for(unsigned w=0;w<120;w++) {
   r.worker=w;if(!Guard(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++){r.word=i;uint32_t value;
    if(!io.FfnGraphProgramReadChecked(w,PioneerQ38FfnGraphCodeAddress(w,i),value) || value!=code->saved[PioneerQ38FfnGraphCodeBase(w)+i])return Fail(io,r);}
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++){r.word=i;uint32_t a=PioneerQ38FfnGraphCodeAddress(w,i),v=PioneerQ38FfnGraphProgramWord(w,a);
    uint32_t actual;if(!io.FfnGraphProgramWrite(w,a,v) || !io.FfnGraphProgramReadChecked(w,a,actual) || actual!=v)return Fail(io,r);
    r.installedWords++;}
   if(!Guard(io,w))return Fail(io,r);
  }
  r.phase=3;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !Image(io,w,true))return Fail(io,r);}
  r.phase=4;
  for(unsigned w=1;w<120;w++) {
   r.worker=w;if(!Guard(io,w) || !Image(io,w))return Fail(io,r);
   for(unsigned i=0;i<96;i++)if(io.Q38PanelRead(0x74000+i*4)!=frames[w-1][i])return Fail(io,r);
   uint32_t boot[]={TT_Q38_FFN_PERSISTENT_MARKER,w,Route(w),cohort->authority.source[w][0],0,0,0,0},zero[16]={},sentinel[16];
   for(auto& v:sentinel)v=0xdeadbeef;
   if(!Stage(io,0x74040,boot,8) || !Stage(io,0x74000,zero,16) || !Stage(io,0x74100,zero,16) || !Stage(io,0x74140,sentinel,16))return Fail(io,r);
   for(unsigned i=0;i<10;i++){uint32_t v=i==5?Route(w):0;io.ResidentNocWrite(i*4,v);if(io.ResidentNocRead(i*4)!=v)return Fail(io,r);}
   bootCommand[w]=true;if(!Guard(io,w))return Fail(io,r);
  }
  if(!Guard(io,0) || !Image(io,0))return Fail(io,r);
  r.phase=5;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);
   reset[w]=0x87ffff;r.armed++;io.RuntimeResetWrite(reset[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);}
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);
   running[w]=cohort->worker[w].image.running=true;reset[w]=0x46800;r.released++;
   io.RuntimeResetWrite(reset[w]);if(!Guard(io,w))return Fail(io,r);}
  if(!WaitAll(io,false,r) || !Guard(io,0) || !Image(io,0))return Fail(io,r);
  // Independently current ALL119 idle/code/frame proofs BEFORE first STOP.
  r.phase=6;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w) || !Image(io,w) || !Frame(io,w,false) || Status(io,w,false)!=1)return Fail(io,r);}
  for(unsigned w=1;w<120;w++) {
   r.worker=w;if(!Guard(io,w))return Fail(io,r);
   uint32_t job[16]={};job[10]=TT_Q38_FFN_PERSISTENT_MARKER;job[11]=w;job[12]=1;job[13]=2;
   if(!Stage(io,0x74000,job,16))return Fail(io,r);
  }
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);
   r.stopIssued++;io.Q38PanelWrite(0x74100,1);if(io.Q38PanelRead(0x74100)!=1)return Fail(io,r);}
  if(!WaitAll(io,true,r) || !Guard(io,0) || !Image(io,0))return Fail(io,r);
  // ALL terminals BEFORE ANY hold. Cached ready/stopped counts never grant
  // this audit;every selected participant is re-proved again before hold.
  r.phase=7;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w) || !Image(io,w) || !Frame(io,w,true) || Status(io,w,true)!=1)return Fail(io,r);}
  for(unsigned w=1;w<120;w++) {
   r.worker=w;if(!Guard(io,w) || !Image(io,w) || !Frame(io,w,true) || Status(io,w,true)!=1)return Fail(io,r);
   reset[w]=0x87ffff;io.RuntimeResetWrite(reset[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   reset[w]=0x47800;io.RuntimeResetWrite(reset[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   running[w]=cohort->worker[w].image.running=false;r.held++;
  }
  if(r.armed!=119 || r.released!=119 || r.ready!=119 || r.stopIssued!=119 || r.stopped!=119 || r.held!=119)return Fail(io,r);
  r.phase=8;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !Image(io,w))return Fail(io,r);}
  for(unsigned w=0;w<120;w++) {
   r.worker=w;if(!Guard(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++){r.word=i;uint32_t a=PioneerQ38FfnGraphCodeAddress(w,i),v=code->saved[PioneerQ38FfnGraphCodeBase(w)+i];
    uint32_t actual;if(!io.FfnGraphProgramWrite(w,a,v) || !io.FfnGraphProgramReadChecked(w,a,actual) || actual!=v)return Fail(io,r);
    r.restoredWords++;}
   if(w)for(unsigned i=0;i<96;i++){io.Q38PanelWrite(0x74000+i*4,frames[w-1][i]);
    if(io.Q38PanelRead(0x74000+i*4)!=frames[w-1][i])return Fail(io,r);
    r.restoredFrameWords++;}
   if(w){for(unsigned i=0;i<10;i++){uint32_t v=code->command[w][i];io.ResidentNocWrite(i*4,v);if(io.ResidentNocRead(i*4)!=v)return Fail(io,r);}bootCommand[w]=false;}
   if(!Guard(io,w) || !PioneerQ38GraphCsrCode(io,w))return Fail(io,r);
  }
  r.phase=9;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++){r.word=i;uint32_t value;
    if(!io.FfnGraphProgramReadChecked(w,PioneerQ38FfnGraphCodeAddress(w,i),value) || value!=code->saved[PioneerQ38FfnGraphCodeBase(w)+i])return Fail(io,r);}
   if(w)for(unsigned i=0;i<96;i++)if(io.Q38PanelRead(0x74000+i*4)!=frames[w-1][i])return Fail(io,r);}
  if(r.installedWords!=code->Words || r.restoredWords!=code->Words || r.restoredFrameWords!=119*96 || !Guard(io,0))return Fail(io,r);
  closed=code->closed=true;r.passed=1;return true;
 }
};
static_assert(sizeof(PioneerQ38FfnPeerLifetimeOwner)>45000 && sizeof(PioneerQ38FfnPeerLifetimeOwner)<49152,"HEAP ONLY:complete119 control frames plus retained execution metadata");
#endif
