/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_NATIVE_OWNER_H
#define PIONEER_Q38_TOKEN_NATIVE_OWNER_H
#include "PioneerQ38TokenMemory.h"
#include "../../../../../tools/pioneer/PioneerQ38TokenRuntimePlan.h"
#include "../../../../../tools/pioneer/PioneerQ38TokenTerminalShape.h"
#include "../../../../../tools/pioneer/PioneerQ38TokenRecordShape.h"
// DISTINCT full-token owner. Held staging,fresh QGW1 bootstrap and ROOT
// startup/cancel request,terminal/worker STOP observation,owned HOLD,
// committed readout,original-L1 and earlier-dispatch image restoration.
// No CPU products,old-prefix ownership,NIU/CSR close or END grant.
struct PioneerQ38TokenRuntimeResult {
 uint32_t version=1,started=0,phase=0,worker=0,chunk=0,word=0,unsafe=0,staged=0;
 uint32_t packets=0,writes=0,reads=0,bytesWritten=0,bytesRead=0,boots=0,reserved[2]={};
};
struct PioneerQ38TokenBootstrapResult {
 uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,ready=0,armed=0;
 uint32_t released=0,workers=0,polls=0,firstPC=0,secondPC=0,reserved[3]={};
};
struct PioneerQ38TokenExecutionResult {
 uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,armed=0,released=0;
 uint32_t acknowledged=0,cancelRequested=0,firstPC=0,secondPC=0,reserved[4]={};
};
struct PioneerQ38TokenTerminalResult {
 uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,ready=0,cancelled=0;
 uint32_t committed=0,position=0,completedLayers=0,stage=0,firstPC=0,secondPC=0,reserved[2]={};
};
struct PioneerQ38TokenHoldResult {
 uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,ready=0,held=0;
 uint32_t firstPC=0,secondPC=0,reserved[6]={};
};
struct PioneerQ38TokenReadResult {
 uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,ready=0,firstRecord=0;
 uint32_t recordCount=0,packets=0,bytesRead=0,drained=0,firstPC=0,secondPC=0,reserved[2]={};
};
struct PioneerQ38TokenRestoreResult {
 uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,ready=0,restoredCores=0;
 uint32_t savedWords=0,restoredWords=0,verifiedWords=0,programWords=0,firstPC=0,secondPC=0,reserved[2]={};
};
struct PioneerQ38TokenDispatchRestoreResult {
 uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,ready=0,restoredCores=0;
 uint32_t savedWords=0,restoredWords=0,verifiedWords=0,entryWords=0,firstPC=0,secondPC=0,reserved[2]={};
};
class PioneerQ38TokenNativeOwner {
 const tt_dram_samples_result* proof=nullptr;PioneerResidentBanks* banks=nullptr;PioneerQ38DispatchLease* cohort=nullptr;
 PioneerQ38TokenMemory* memory=nullptr;const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const PioneerQ38TokenStartupPlan* startup=nullptr;const uint8_t* ledger=nullptr;PioneerQ38GdnIdentity identity={};
 PioneerQ38TokenRuntimePlan plan;uint32_t staging[4096]={};
 bool started=false,unsafe=false,staged=false,traffic=false,bootAttempted=false,booted=false,launched=false;
 bool running[120]={};uint32_t reset[120]={};
 bool rootAttempted=false,rootStarted=false,rootAcknowledged=false,cancelRequested=false;
 bool terminalObserved=false,holding=false,held=false;
 uint32_t observedTerminal[64]={},terminalCounters[120][8]={},terminalNonces[120]={};
 bool readoutPending=false,readoutDrained=false,readoutCommandKnown=false;
 uint32_t readoutCursor=0,readoutInput=0,readoutCommand[10]={};
 bool restoring=false,l1Restored=false;
 const uint32_t* dispatchSaved[120]={}; // SAME earlier dispatch originals,captured BEFORE native launch
 bool dispatchRestoring=false,dispatchRestored=false;
 bool Bound()const{
  if(!started || unsafe || !proof || !banks || !cohort || !memory || !canonical || !startup || !ledger
   || !plan.MatchesBinding(*canonical,*startup,identity,&cohort->authority.source[0][0]))return false;
  if(!launched)return memory->Installed()
   && memory->MatchesNativeOwner(*proof,*banks,*cohort,*canonical,*startup,identity,ledger,PioneerResidentBanks::Pages);
  // SAME private journal after its first uncertain ARM,never a public ready
  // hint or the legacy bootstrap policy. No arbitrary counter recapture.
  if(!bootAttempted || !staged || traffic || !memory->started || (!memory->installed && !l1Restored) || !memory->launched
   || memory->unsafe || memory->closed || memory->proof!=proof || memory->banks!=banks || memory->cohort!=cohort
   || memory->canonical!=canonical || memory->startup!=startup || memory->ledger!=ledger || !(memory->identity==identity)
   || !canonical->Ready() || !banks->Matches(*proof) || cohort->unsafe || cohort->closed || cohort->rootRunning!=running[0]
   || cohort->prepared!=120 || cohort->bootAttempted || cohort->booted || ((!holding && !held) && running[0]!=rootStarted)
   || ((holding || held) && (!terminalObserved || !rootAcknowledged))
   || ((restoring || l1Restored) && (!held || holding || !readoutDrained || readoutPending || readoutCursor!=observedTerminal[3]))
   || ((dispatchRestoring || dispatchRestored) && (!l1Restored || restoring || !held || holding))
   || (rootStarted?(!rootAttempted || !booted || reset[0]!=((holding || held) && !running[0]?0x47800U:0x46800U))
    :reset[0]!=(rootAttempted?0x87ffffU:0x47800U)))return false;
  for(unsigned w=0;w<120;w++){
   const auto& n=cohort->worker[w];const auto& store=n.master?*n.master:n.store;
   if(!n.image.saved || n.image.ready==dispatchRestored || n.image.unsafe || n.image.running!=running[w] || !store.ready
    || store.pending || store.recovery_required || n.noc.recovery_required
    || ((holding || held) && reset[w]!=(running[w]?0x46800U:0x47800U)) || (held && running[w]))return false;
  }
  return true;
 }
 template<class IO> bool Scope(IO& io)const{
  return Bound() && io.TokenRuntimeScope(*this,*memory,*canonical,*startup,identity,ledger,PioneerResidentBanks::Pages);
 }
 template<class IO> bool FailOwner(IO& io){
  unsafe=true;staged=booted=false;if(memory)memory->Fault();
  if(cohort && banks)PioneerQ38DispatchQuarantine(*cohort,*banks);
  io.QuarantineHostPage();io.TokenMemoryRetainHostOwners();return false; // MEMORY ONLY,no following IO/cleanup/retry
 }
 template<class IO> bool Fail(IO& io,PioneerQ38TokenRuntimeResult& r){r.unsafe=1;r.staged=0;return FailOwner(io);}
 template<class IO> bool Fail(IO& io,PioneerQ38TokenBootstrapResult& r){r.unsafe=1;r.ready=0;return FailOwner(io);}
 template<class IO> bool Fail(IO& io,PioneerQ38TokenExecutionResult& r){r.unsafe=1;r.acknowledged=r.cancelRequested=0;return FailOwner(io);}
 template<class IO> bool Fail(IO& io,PioneerQ38TokenTerminalResult& r){r.unsafe=1;r.ready=0;return FailOwner(io);}
 template<class IO> bool Fail(IO& io,PioneerQ38TokenHoldResult& r){r.unsafe=1;r.ready=0;return FailOwner(io);}
 template<class IO> bool Fail(IO& io,PioneerQ38TokenReadResult& r){r.unsafe=1;r.ready=0;return FailOwner(io);}
 template<class IO> bool Fail(IO& io,PioneerQ38TokenRestoreResult& r){r.unsafe=1;r.ready=0;return FailOwner(io);}
 template<class IO> bool Fail(IO& io,PioneerQ38TokenDispatchRestoreResult& r){r.unsafe=1;r.ready=0;return FailOwner(io);}
 template<class IO> bool Sources(IO& io,unsigned w){
  if(w>=120 || !Scope(io) || !io.SelectRuntimeCore(w) || !io.TokenMemoryMappingStable(w)
   || io.ResetRead()!=reset[w] || io.ResetRead()!=reset[w])return false;
  for(unsigned i=0;i<16;i++)if(io.ResidentNocRead(BlackholeDramSourceOffset(i))!=cohort->authority.source[w][i])return false;
  for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)if(io.SourceRead(i)!=proof->source[i])return false;
  for(unsigned p=0;p<2;p++)if(!PioneerQ38GraphCsrPciQuiet(io,p))return false;
  return true;
 } // active device NoC traffic is NOT a host/bus-master grant or an idle receipt
 template<class IO> bool Guard(IO& io,unsigned w){
  if(w>=120 || !Scope(io))return false;
  if(launched){
   if(traffic || rootStarted || !Sources(io,w))return false;
   if(!PioneerQ38GraphCsrIdle(io))return false;
   for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=memory->command[w][i])return false;
   for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=memory->counters[w][i])return false;
   return true;
  }
  if(!traffic)return memory->Guard(io,w);
  // ONLY this owner's fixed ROOT packet may be in flight. All host/BME,
  // reset,source,mapping and native lifetime checks still hold. The caller
  // independently bounds EVERY actual counter delta before adoption.
  if(w || !io.SelectRuntimeCore(0) || !io.TokenMemoryMappingStable(0) || io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800)return false;
  for(unsigned i=0;i<16;i++)if(io.ResidentNocRead(BlackholeDramSourceOffset(i))!=cohort->authority.source[0][i])return false;
  for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)if(io.SourceRead(i)!=proof->source[i])return false;
  for(unsigned p=0;p<2;p++)if(!PioneerQ38GraphCsrPciQuiet(io,p))return false;
  return true;
 }
 template<class IO,class Result> bool Exact(IO& io,unsigned w,uint32_t address,const uint32_t* words,unsigned count,Result& r){
  if(!Guard(io,w))return false;
  for(unsigned i=0;i<count;i++){uint32_t actual;r.word=i;
   if(!io.TokenMemoryReadChecked(w,address+i*4,actual) || actual!=words[i])return false;}
  return Guard(io,w);
 }
 template<class IO> bool BootTime(IO& io,uint64_t began,uint64_t& previous)const{
  if(!Scope(io))return false;
  const uint64_t now=io.Q38CohortNow();
  if(now<previous || now<began || now-began>=UINT64_C(30000000))return false;
  previous=now;return true;
 }
 template<class IO> bool Local(IO& io,unsigned worker,uint32_t address,const uint32_t* words,unsigned count,PioneerQ38TokenRuntimeResult& r){
  if(!Guard(io,worker))return false;
  for(unsigned i=0;i<count;i++){r.word=i;if(!io.TokenMemoryWrite(worker,address+i*4,words[i]))return false;}
  for(unsigned i=0;i<count;i++){uint32_t actual;r.word=i;
   if(!io.TokenMemoryReadChecked(worker,address+i*4,actual) || actual!=words[i])return false;}
  return Guard(io,worker);
 }
 template<class IO> bool Packet(IO& io,const PioneerQ38TokenRuntimePlan::Chunk& d,bool write,PioneerQ38TokenRuntimeResult& r){
  namespace C=PioneerQ38GlobalChainLayout;const auto scratch=uint32_t(C::R::Scratch);
  if(d.bank!=plan.Bank() || !d.bytes || (d.address|d.bytes)&63 || d.bytes>16384 || d.address<plan.Workspace()
   || d.address>plan.Workspace()+C::WorkspaceBytes-d.bytes || !Guard(io,0))return false;
  const uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return false;
  uint32_t before[8],expected[8];for(unsigned i=0;i<8;i++){
   before[i]=memory->counters[0][i];expected[i]=before[i]+(i==(write?5U:1U)?d.bytes/64:
    i==2 || i==6 || i==(write?4U:0U) || i==(write?7U:3U)?1U:0U);
  }
  const unsigned route=PioneerQ38ProjectionRoute(d.bank);
  const uint32_t command[]={write?scratch:d.address,0,write?129U:route,write?d.address:scratch,0,write?route:129U,0,write?0x212U:0,d.bytes,0};
  for(unsigned i=0;i<10;i++){
   memory->command[0][i]=command[i];io.ResidentNocWrite(i*4,command[i]);if(io.ResidentNocRead(i*4)!=command[i])return false;
  }
  traffic=true;io.ResidentNocWrite(0x40,1);r.packets++;bool done=false;
  for(unsigned poll=0;poll<65536;poll++){
   if(!Guard(io,0))return false;
   const uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return false;
   uint32_t busy=io.ResidentNocRead(0x40);if(busy>1)return false;done=!busy;
   for(unsigned i=0;i<8;i++){
    const uint32_t observed=io.ResidentNocRead(PioneerQ38PanelCounter(i));
    if(uint32_t(observed-before[i])>uint32_t(expected[i]-before[i]))return false;
    done &= observed==expected[i];
   }
   if(done)break;
   io.Pause();
  }
  if(!done)return false;
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=command[i])return false;
  for(unsigned i=0;i<8;i++)memory->counters[0][i]=expected[i]; // ONLY complete exact receipts,NEVER arbitrary recapture
  traffic=false;if(!Guard(io,0))return false;
  if(write){r.writes++;r.bytesWritten+=d.bytes;}else{r.reads++;r.bytesRead+=d.bytes;}
  return true;
 }
public:
 PioneerQ38TokenNativeOwner()=default;
 PioneerQ38TokenNativeOwner(const PioneerQ38TokenNativeOwner&)=delete;
 PioneerQ38TokenNativeOwner& operator=(const PioneerQ38TokenNativeOwner&)=delete;
 bool Unsafe()const{return unsafe;}
 bool Staged()const{return staged && !unsafe && !traffic;}
 bool Pending()const{return traffic || readoutPending;}
 bool WorkersReady()const{return booted && bootAttempted && launched && Staged() && Bound();}
 bool ControlsCohort(const PioneerQ38DispatchLease* participants)const{return participants==cohort && Staged() && Bound();}
 bool OwnsRunningRoot(const PioneerQ38DispatchLease* participants)const{
  return !unsafe && participants && participants==cohort && rootAttempted && rootStarted && booted && launched
   && running[0] && reset[0]==0x46800 && participants->rootRunning;
 } // cheap private lifecycle binding ONLY;full native scope+sources still required before IO
 bool ExecutionStarted()const{return rootAcknowledged && OwnsRunningRoot(cohort) && Bound();}
 bool TerminalObserved()const{return terminalObserved && !unsafe && (ExecutionStarted() || ((holding || held) && Bound()));}
 bool HeldAfterTerminal()const{return held && !holding && TerminalObserved() && Bound();} // NOT restored/closed/END
 bool CommittedRecordsRead()const{return readoutDrained && !readoutPending && HeldAfterTerminal() && readoutCursor==observedTerminal[3];}
 bool OriginalL1Restored()const{return l1Restored && !restoring && CommittedRecordsRead() && Bound();} // dispatch/CSR leases STILL owned
 bool DispatchImagesRestored()const{return dispatchRestored && !dispatchRestoring && OriginalL1Restored();} // NIU/scratch/TLB/END still owned
 bool DispatchOutput(const void* output,size_t bytes)const{
  const uintptr_t out=reinterpret_cast<uintptr_t>(output);
  const size_t savedBytes=(size_t(TT_Q38_PANEL_VIEW_WORDS)+1)*4;
  if(!output || !bytes || out>UINTPTR_MAX-bytes)return false;
  for(unsigned w=0;w<120;w++){
   const uintptr_t p=reinterpret_cast<uintptr_t>(dispatchSaved[w]);
   if(!p || p>UINTPTR_MAX-savedBytes || (out<p+savedBytes && p<out+bytes))return false;
  }
  return true; // memory-only alias exclusion,NOT lifecycle/restore authority
 }
 void Fault(){unsafe=true;staged=false;}
 bool MatchesRuntimeBinding(const tt_dram_samples_result& dram,const PioneerResidentBanks& b,const PioneerQ38DispatchLease& participants,
  const PioneerQ38TokenMemory& m,const PioneerQ38CanonicalModelPlan& c,const PioneerQ38TokenStartupPlan& data,
  const PioneerQ38GdnIdentity& id,const uint8_t* initialized,unsigned pages)const{
  return proof==&dram && banks==&b && cohort==&participants && memory==&m && canonical==&c && startup==&data && identity==id
   && ledger==initialized && pages==PioneerResidentBanks::Pages && Bound();
 } // necessary same held binding ONLY,not a completed model or native lifetime permission
 template<class IO> bool PrepareHeldRuntime(IO& io,const tt_dram_samples_result& dram,PioneerResidentBanks& b,PioneerQ38DispatchLease& participants,
  PioneerQ38TokenMemory& mem,const PioneerQ38CanonicalModelPlan& c,const PioneerQ38TokenStartupPlan& data,
  const PioneerQ38GdnIdentity& id,const uint8_t* initialized,unsigned pages,PioneerQ38TokenRuntimeResult& r){
  const PioneerQ38TokenRuntimeResult pristine;const auto* a=reinterpret_cast<const uint8_t*>(&r);const auto* p=reinterpret_cast<const uint8_t*>(&pristine);
  for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=p[i])return false;
  if(started || unsafe || staged || traffic || !mem.Installed() || !mem.MatchesNativeOwner(dram,b,participants,c,data,id,initialized,pages)
   || !data.LearnedPagesPresent(initialized,pages) || !plan.Bind(c,data,id,&participants.authority.source[0][0]))return false;
  started=true;proof=&dram;banks=&b;cohort=&participants;memory=&mem;canonical=&c;startup=&data;identity=id;ledger=initialized;r.started=1;r.phase=1;
  for(auto& value:reset)value=0x47800;
  for(unsigned w=0;w<120;w++){r.worker=w;PioneerQ38TokenMemoryResult image;
   if(!Guard(io,w) || !memory->VerifyImage(io,w,image))return Fail(io,r);
   dispatchSaved[w]=io.Q38PanelSavedWords();
   const uintptr_t saved=reinterpret_cast<uintptr_t>(dispatchSaved[w]);
   const size_t savedBytes=(size_t(TT_Q38_PANEL_VIEW_WORDS)+1)*4;
   if(!saved || (saved&3) || saved>UINTPTR_MAX-savedBytes)return Fail(io,r);
   for(unsigned p=0;p<w;p++){
    const uintptr_t other=reinterpret_cast<uintptr_t>(dispatchSaved[p]);
    if(saved<other+savedBytes && other<saved+savedBytes)return Fail(io,r);
   }
  }
  namespace C=PioneerQ38GlobalChainLayout;namespace W=PioneerQ38GlobalWorkerLayout;namespace T=PioneerQ38TokenLayout;
  r.phase=2;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!data.ReadBoot(w,staging) || !Local(io,w,w?unsigned(W::Boot):unsigned(C::R::Boot),staging,64,r))return Fail(io,r);
   for(unsigned i=0;i<128;i++)staging[i]=0;
   if(!Local(io,w,w?unsigned(W::Job):unsigned(C::R::Outer),staging,w?64U:128U,r)
    || !Local(io,w,w?unsigned(W::Doorbell):unsigned(C::R::Doorbell),staging,1,r)
    || !Local(io,w,w?unsigned(W::Status):unsigned(C::R::Status),staging,16,r))return Fail(io,r);
   r.boots++;
  }
  r.worker=0;
  if(!Local(io,0,T::CancelLocal,staging,16,r) || !Local(io,0,T::RecordLocal,staging,64,r))return Fail(io,r);
  r.phase=3;
  for(unsigned n=0;n<PioneerQ38TokenRuntimePlan::Chunks;n++){
   r.chunk=n;PioneerQ38TokenRuntimePlan::Chunk d;auto* raw=reinterpret_cast<uint8_t*>(staging);
   if(!plan.Describe(n,d) || !plan.Read(n,raw,d.bytes))return Fail(io,r);
   for(unsigned i=0;i<d.bytes/4;i++)staging[i]=uint32_t(raw[i*4])|uint32_t(raw[i*4+1])<<8|uint32_t(raw[i*4+2])<<16|uint32_t(raw[i*4+3])<<24;
   if(!Local(io,0,C::R::Scratch,staging,d.bytes/4,r) || !Packet(io,d,true,r) || !Packet(io,d,false,r))return Fail(io,r);
   for(unsigned i=0;i<d.bytes/4;i++){uint32_t actual;r.word=i;
    if(!io.TokenMemoryReadChecked(0,C::R::Scratch+i*4,actual) || actual!=staging[i])return Fail(io,r);
   }
  }
  if(r.packets!=4616 || r.writes!=2308 || r.reads!=2308 || r.bytesWritten!=PioneerQ38TokenRuntimePlan::PayloadBytes
   || r.bytesRead!=r.bytesWritten || !data.ReadPublishedPackage(0,0,reinterpret_cast<uint8_t*>(staging),512))return Fail(io,r);
  {auto* raw=reinterpret_cast<uint8_t*>(staging);
   for(unsigned i=0;i<128;i++)staging[i]=uint32_t(raw[i*4])|uint32_t(raw[i*4+1])<<8|uint32_t(raw[i*4+2])<<16|uint32_t(raw[i*4+3])<<24;}
  if(!Local(io,0,C::R::Outer,staging,128,r))return Fail(io,r);
  staging[0]=1;
  if(!Local(io,0,C::R::Doorbell,staging,1,r))return Fail(io,r);
  r.phase=4;
  for(unsigned w=0;w<120;w++){r.worker=w;PioneerQ38TokenMemoryResult image;
   if(!Guard(io,w) || !memory->VerifyImage(io,w,image))return Fail(io,r);
  }
  staged=true;r.staged=1;return true;
 } // empty KV prefix/initial session DATA ONLY;NO boot,model arithmetic,token commit or clean-END capability
 template<class IO> bool BootstrapWorkers(IO& io,PioneerQ38TokenBootstrapResult& r){
  const PioneerQ38TokenBootstrapResult pristine;const auto* a=reinterpret_cast<const uint8_t*>(&r);
  const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
  for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
  if(!Staged() || bootAttempted || booted || launched || !Scope(io))return false;
  namespace W=PioneerQ38GlobalWorkerLayout;namespace C=PioneerQ38GlobalChainLayout;namespace T=PioneerQ38TokenLayout;
  bootAttempted=true;r.started=1;r.phase=1;
  // Full immutable programs,boot envelopes and pristine control mailboxes
  // are checked BEFORE the first ARM. ROOT's real template stays held.
  for(unsigned w=0;w<120;w++){
   r.worker=w;PioneerQ38TokenMemoryResult image;
   if(!Guard(io,w) || !memory->VerifyImage(io,w,image) || !startup->ReadBoot(w,staging)
    || !Exact(io,w,w?unsigned(W::Boot):unsigned(C::R::Boot),staging,64,r))return Fail(io,r);
   for(unsigned i=0;i<128;i++)staging[i]=0;
   if(w && (!Exact(io,w,W::Job,staging,64,r) || !Exact(io,w,W::Doorbell,staging,1,r)))return Fail(io,r);
   if(!Exact(io,w,w?unsigned(W::Status):unsigned(C::R::Status),staging,16,r))return Fail(io,r);
   if(!w && (!Exact(io,0,T::CancelLocal,staging,16,r) || !Exact(io,0,T::RecordLocal,staging,64,r)))return Fail(io,r);
  }
  if(!startup->ReadPublishedPackage(0,0,reinterpret_cast<uint8_t*>(staging),512))return Fail(io,r);
  {auto* raw=reinterpret_cast<uint8_t*>(staging);
   for(unsigned i=0;i<128;i++)staging[i]=uint32_t(raw[i*4])|uint32_t(raw[i*4+1])<<8|uint32_t(raw[i*4+2])<<16|uint32_t(raw[i*4+3])<<24;}
  if(!Exact(io,0,C::R::Outer,staging,128,r))return Fail(io,r);
  staging[0]=1;if(!Exact(io,0,C::R::Doorbell,staging,1,r))return Fail(io,r);
  r.phase=2;
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!Guard(io,w))return Fail(io,r);
   // Firmware requires this exact self-route at boot. These known writes
   // advance the journal,never sample a new arbitrary command baseline.
   for(unsigned i=0;i<10;i++){
    const uint32_t value=i==5?W::Peer(w):0;memory->command[w][i]=value;
    io.ResidentNocWrite(i*4,value);if(io.ResidentNocRead(i*4)!=value)return Fail(io,r);
   }
   if(!Guard(io,w))return Fail(io,r);
  }
  if(!Scope(io))return Fail(io,r);
  const uint64_t began=io.Q38CohortNow();uint64_t previous=began;
  if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  // All119 ARM before ANY release. Mark launched BEFORE the first possible
  // uncertain write so held-only restoration can never reclaim this owner.
  launched=memory->launched=true;r.phase=3;
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!BootTime(io,began,previous) || !Guard(io,w))return Fail(io,r);
   cohort->worker[w].image.dirty=true;cohort->worker[w].Store().l1_dirty=true;
   cohort->worker[w].Store().l1_restored=false;reset[w]=0x87ffff;
   io.RuntimeResetWrite(reset[w]);if(!Scope(io))return Fail(io,r);
   io.Pause();
   if(!Guard(io,w) || !BootTime(io,began,previous))return Fail(io,r);
   r.armed++;
  }
  r.phase=4;
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!BootTime(io,began,previous) || !Guard(io,w))return Fail(io,r);
   running[w]=cohort->worker[w].image.running=true;reset[w]=0x46800;
   io.RuntimeResetWrite(reset[w]);
   if(!Guard(io,w) || !BootTime(io,began,previous))return Fail(io,r);
   r.released++;
  }
  r.phase=5;bool ready[120]={};
  for(unsigned poll=0;poll<65536 && r.workers<119;poll++){
   r.polls=poll+1;if(!BootTime(io,began,previous))return Fail(io,r);
   for(unsigned w=1;w<120;w++)if(!ready[w]){
    r.worker=w;if(!Guard(io,w) || !BootTime(io,began,previous))return Fail(io,r);
    uint32_t marker=0;
    for(unsigned i=0;i<16;i++){uint32_t actual;r.word=i;
     if(!io.TokenMemoryReadChecked(w,W::Status+i*4,actual) || (i==3?(actual && actual!=W::Magic):actual!=0))return Fail(io,r);
     if(i==3)marker=actual;
    }
    if(!marker)continue;
    io.Pause();r.firstPC=io.RuntimePcRead();io.Pause();r.secondPC=io.RuntimePcRead();
    if(!Guard(io,w) || !BootTime(io,began,previous))return Fail(io,r);
    if((r.firstPC!=0x2004c && r.firstPC!=0x20050) || (r.secondPC!=0x2004c && r.secondPC!=0x20050))continue;
    for(unsigned i=0;i<64;i++)staging[i]=0;
    if(!Exact(io,w,W::Job,staging,64,r) || !Exact(io,w,W::Doorbell,staging,1,r))return Fail(io,r);
    ready[w]=true;r.workers++;
   }
   if(r.workers<119)io.Pause();
  }
  if(r.armed!=119 || r.released!=119 || r.workers!=119)return Fail(io,r);
  r.phase=6;
  // Audit every already-accepted worker again;an early worker must not
  // silently change while a later one is still booting.
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!BootTime(io,began,previous) || !Guard(io,w))return Fail(io,r);
   for(unsigned i=0;i<64;i++)staging[i]=0;
   if(!Exact(io,w,W::Job,staging,64,r) || !Exact(io,w,W::Doorbell,staging,1,r))return Fail(io,r);
   staging[3]=W::Magic;if(!Exact(io,w,W::Status,staging,16,r))return Fail(io,r);
   io.Pause();r.firstPC=io.RuntimePcRead();io.Pause();r.secondPC=io.RuntimePcRead();
   if((r.firstPC!=0x2004c && r.firstPC!=0x20050) || (r.secondPC!=0x2004c && r.secondPC!=0x20050)
    || !Guard(io,w))return Fail(io,r);
  }
  if(!Guard(io,0) || !BootTime(io,began,previous))return Fail(io,r);
  booted=true;r.phase=7;r.ready=1;return true;
 } // genuine fresh workers ONLY;ROOT held,NO model RUN/cancel/STOP/END/Work permission
 template<class IO> bool StartRoot(IO& io,PioneerQ38TokenExecutionResult& r){
  namespace W=PioneerQ38GlobalWorkerLayout;namespace C=PioneerQ38GlobalChainLayout;namespace T=PioneerQ38TokenLayout;
  const PioneerQ38TokenExecutionResult pristine;const auto* a=reinterpret_cast<const uint8_t*>(&r);
  const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
  for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
  if(rootAttempted || rootStarted || rootAcknowledged || cancelRequested || !WorkersReady() || !Scope(io))return false;
  r.started=1;r.phase=1;
  // A fresh call must prove the CURRENT119 idle workers,not trust only the
  // previous call's successful bootstrap result or a caller-ready value.
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!startup->ReadBoot(w,staging) || !Exact(io,w,W::Boot,staging,64,r))return Fail(io,r);
   for(unsigned i=0;i<64;i++)staging[i]=0;
   if(!Exact(io,w,W::Job,staging,64,r) || !Exact(io,w,W::Doorbell,staging,1,r))return Fail(io,r);
   staging[3]=W::Magic;if(!Exact(io,w,W::Status,staging,16,r))return Fail(io,r);
   io.Pause();r.firstPC=io.RuntimePcRead();io.Pause();r.secondPC=io.RuntimePcRead();
   if((r.firstPC!=0x2004c && r.firstPC!=0x20050) || (r.secondPC!=0x2004c && r.secondPC!=0x20050)
    || !Guard(io,w))return Fail(io,r);
  }
  r.worker=0;PioneerQ38TokenMemoryResult image;
  if(!Guard(io,0) || !memory->VerifyImage(io,0,image) || !startup->ReadBoot(0,staging)
   || !Exact(io,0,C::R::Boot,staging,64,r)
   || !startup->ReadPublishedPackage(0,0,reinterpret_cast<uint8_t*>(staging),512))return Fail(io,r);
  {auto* raw=reinterpret_cast<uint8_t*>(staging);
   for(unsigned i=0;i<128;i++)staging[i]=uint32_t(raw[i*4])|uint32_t(raw[i*4+1])<<8|uint32_t(raw[i*4+2])<<16|uint32_t(raw[i*4+3])<<24;}
  if(!Exact(io,0,C::R::Outer,staging,128,r))return Fail(io,r);
  staging[0]=1;if(!Exact(io,0,C::R::Doorbell,staging,1,r))return Fail(io,r);
  for(unsigned i=0;i<64;i++)staging[i]=0;
  if(!Exact(io,0,T::CancelLocal,staging,16,r) || !Exact(io,0,T::RecordLocal,staging,64,r)
   || !Exact(io,0,C::R::Status,staging,16,r))return Fail(io,r);
  // Clear stale original receipt/progress bytes while ROOT is STILL HELD.
  // Only actual firmware publication may subsequently acknowledge startup.
  r.phase=2;
  for(unsigned region=0;region<2;region++){
   const unsigned address=region?unsigned(C::Progress):unsigned(T::TerminalLocal);
   if(!Guard(io,0))return Fail(io,r);
   for(unsigned i=0;i<64;i++)if(!io.TokenMemoryWrite(0,address+i*4,0))return Fail(io,r);
   if(!Exact(io,0,address,staging,64,r))return Fail(io,r);
  }
  if(!Scope(io))return Fail(io,r);
  const uint64_t began=io.Q38CohortNow();uint64_t previous=began;
  if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  rootAttempted=true;reset[0]=0x87ffff;cohort->worker[0].image.dirty=true;
  cohort->worker[0].Store().l1_dirty=true;cohort->worker[0].Store().l1_restored=false;r.phase=3;
  io.RuntimeResetWrite(reset[0]);if(!Scope(io))return Fail(io,r);
  io.Pause();if(!Guard(io,0) || !BootTime(io,began,previous))return Fail(io,r);
  r.armed=1;rootStarted=running[0]=cohort->worker[0].image.running=cohort->rootRunning=true;
  reset[0]=0x46800;r.phase=4;io.RuntimeResetWrite(reset[0]);
  if(!Sources(io,0) || !BootTime(io,began,previous))return Fail(io,r);
  r.released=1;
  // From release onward device NoC traffic is legitimate. Never recapture
  // arbitrary commands/counters or pretend active traffic is an idle proof.
  for(unsigned poll=0;poll<65536;poll++){
   if(!Sources(io,0) || !BootTime(io,began,previous))return Fail(io,r);
   uint32_t marker=0,error=0;r.word=3;
   // Other status fields are legitimate changing layer progress after
   // publication. Startup must not require catching an all-zero instant.
   if(!io.TokenMemoryReadChecked(0,C::R::Status+12,marker) || (marker && marker!=T::Magic)
    || !io.TokenMemoryReadChecked(0,C::R::Status+8,error) || error)return Fail(io,r);
   if(marker){
    if(!Sources(io,0) || !BootTime(io,began,previous))return Fail(io,r);
    uint32_t word;r.firstPC=io.RuntimePcRead();
    if(r.firstPC==0x60000 || !PioneerQ38TokenPrograms::ProgramWord(0,r.firstPC,word) || !Scope(io))return Fail(io,r);
    r.secondPC=io.RuntimePcRead();
    if(r.secondPC==0x60000 || !PioneerQ38TokenPrograms::ProgramWord(0,r.secondPC,word) || !Sources(io,0))return Fail(io,r);
    rootAcknowledged=true;r.acknowledged=1;r.phase=5;return true;
   }
   if(!Scope(io))return Fail(io,r);
   io.Pause();
  }
  return Fail(io,r);
 } // entered actual token ROM ONLY;NO completed layer/token/STOP/END or CPU-reference Work route
 template<class IO> bool RequestCancel(IO& io,PioneerQ38TokenExecutionResult& r){
  namespace T=PioneerQ38TokenLayout;
  const PioneerQ38TokenExecutionResult pristine;const auto* a=reinterpret_cast<const uint8_t*>(&r);
  const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
  for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
  if(cancelRequested || terminalObserved || !ExecutionStarted() || !Scope(io))return false;
  r.started=1;r.phase=6;
  if(!Sources(io,0))return Fail(io,r);
  for(unsigned i=0;i<16;i++){uint32_t actual;r.word=i;
   if(!io.TokenMemoryReadChecked(0,T::CancelLocal+i*4,actual) || actual)return Fail(io,r);}
  // One fixed monotonic RAW32 request,not a STOP acknowledgment. Never
  // rewrite it,reset a worker,copy a model product or authorize disposal.
  cancelRequested=true;
  if(!io.TokenMemoryWrite(0,T::CancelLocal,T::CancelMagic))return Fail(io,r);
  uint32_t actual;if(!Scope(io) || !io.TokenMemoryReadChecked(0,T::CancelLocal,actual)
   || actual!=T::CancelMagic || !Sources(io,0))return Fail(io,r);
  r.cancelRequested=1;r.phase=7;return true;
 }
 template<class IO> bool ObserveRoot(IO& io,PioneerQ38TokenExecutionResult& r){
  const PioneerQ38TokenExecutionResult pristine;const auto* a=reinterpret_cast<const uint8_t*>(&r);
  const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
  for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
  if(!ExecutionStarted() || !Scope(io))return false;
  r.started=1;r.phase=8;
  if(!Sources(io,0))return Fail(io,r);
  uint32_t marker,error,word;
  if(!io.TokenMemoryReadChecked(0,PioneerQ38GlobalChainLayout::R::Status+12,marker) || marker!=PioneerQ38TokenLayout::Magic
   || !io.TokenMemoryReadChecked(0,PioneerQ38GlobalChainLayout::R::Status+8,error) || error)return Fail(io,r);
  r.firstPC=io.RuntimePcRead();
  if(r.firstPC==0x60000 || !PioneerQ38TokenPrograms::ProgramWord(0,r.firstPC,word) || !Scope(io))return Fail(io,r);
  r.secondPC=io.RuntimePcRead();
  if(r.secondPC==0x60000 || !PioneerQ38TokenPrograms::ProgramWord(0,r.secondPC,word) || !Sources(io,0))return Fail(io,r);
  r.acknowledged=1;r.cancelRequested=cancelRequested;return true;
 } // observation ONLY;NO NoC command/counter recapture,layer/token/frontier/STOP/END completion grant
private:
 template<class IO> __attribute__((noinline)) bool InspectTerminal(IO& io,PioneerQ38TokenTerminalResult& r,bool recheck){
  namespace C=PioneerQ38GlobalChainLayout;namespace R=C::R;namespace T=PioneerQ38TokenLayout;namespace W=PioneerQ38GlobalWorkerLayout;
  const PioneerQ38TokenTerminalResult pristine;const auto* a=reinterpret_cast<const uint8_t*>(&r);
  const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
  for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
  if(terminalObserved!=recheck || holding || held || !ExecutionStarted() || !Scope(io))return false;
  r.started=1;r.phase=9;
  if(!Sources(io,0))return Fail(io,r);
  uint32_t terminal[64],boot[64],job[64],status[16],counters[8],progress[3],cancel=0;
  uint32_t marker=0,error=0;
  if(!io.TokenMemoryReadChecked(0,R::Status+12,marker) || marker!=T::Magic
   || !io.TokenMemoryReadChecked(0,R::Status+8,error) || error)return Fail(io,r);
  uint32_t word=0;
  r.firstPC=io.RuntimePcRead();
  if(r.firstPC==0x60000 || !PioneerQ38TokenPrograms::ProgramWord(0,r.firstPC,word) || !Sources(io,0))return Fail(io,r);
  r.secondPC=io.RuntimePcRead();
  if(r.secondPC==0x60000 || !PioneerQ38TokenPrograms::ProgramWord(0,r.secondPC,word) || !Sources(io,0))return Fail(io,r);
  if(!io.TokenMemoryReadChecked(0,T::TerminalLocal,terminal[0]))return Fail(io,r);
  if(!terminal[0])return Sources(io,0) ? true : Fail(io,r); // actual unpublished marker: pending,NOT ready
  if(terminal[0]!=T::TerminalMagic)return Fail(io,r);
  for(unsigned i=1;i<64;i++){r.word=i;if(!io.TokenMemoryReadChecked(0,T::TerminalLocal+i*4,terminal[i]))return Fail(io,r);}
  if(r.firstPC!=0x60004 || r.secondPC!=0x60004)return true; // publication may precede STOP;no terminal grant
  if(!PioneerQ38GraphCsrIdle(io) || !startup->ReadBoot(0,boot))return Fail(io,r);
  for(unsigned i=0;i<64;i++){uint32_t actual=0;r.word=i;
   if(!io.TokenMemoryReadChecked(0,R::Boot+i*4,actual) || actual!=boot[i])return Fail(io,r);}
  for(unsigned i=0;i<3;i++)if(!io.TokenMemoryReadChecked(0,C::Progress+i*4,progress[i]))return Fail(io,r);
  auto* raw=reinterpret_cast<uint8_t*>(staging);
  if(!startup->ReadSession(0,raw,512))return Fail(io,r);
  for(unsigned i=0;i<128;i++)staging[i]=uint32_t(raw[i*4])|uint32_t(raw[i*4+1])<<8|uint32_t(raw[i*4+2])<<16|uint32_t(raw[i*4+3])<<24;
  if(!T::Session(staging,boot,plan.Bank(),plan.Workspace())
   || !PioneerQ38TokenTerminalShape(terminal,boot,progress,staging[5],staging[6],staging[32]!=0,cancelRequested))return Fail(io,r);
  for(unsigned i=0;i<16;i++)if(!io.TokenMemoryReadChecked(0,T::CancelLocal+i*4,cancel)
   || cancel!=(i==0 && cancelRequested?unsigned(T::CancelMagic):0U))return Fail(io,r);
  for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=terminal[36+i])return Fail(io,r);
  const unsigned stopLayer=terminal[2]==3 && terminal[10]==1?terminal[6]-1:64;
  for(unsigned w=1;w<120;w++){
   r.phase=10;r.worker=w;
   if(!Sources(io,0) || !startup->ReadBoot(w,boot))return Fail(io,r);
   uint32_t nonce=0;
   if(!io.TokenMemoryReadChecked(0,R::Nonces+(w-1)*4,nonce) || !nonce || nonce>=0x7fffffff
    || (recheck && nonce!=terminalNonces[w]))return Fail(io,r);
   for(unsigned i=0;i<64;i++){uint32_t cached=0;r.word=i;
    if(!io.TokenMemoryReadChecked(0,R::PeerBoots+(w-1)*256+i*4,cached) || cached!=boot[i]
     || !io.TokenMemoryReadChecked(0,R::PeerJobs+(w-1)*256+i*4,job[i]))return Fail(io,r);}
   if(!W::JobShape(job,boot,nonce-1,0) || job[1]!=nonce || job[2]!=W::Stop || job[3]!=w || job[4]!=W::Peer(w)
    || job[5]!=stopLayer || job[6]!=terminal[4] || job[7]!=(terminal[2]==3?9998U:9999U))return Fail(io,r);
   for(unsigned i=0;i<8;i++)if(!io.TokenMemoryReadChecked(0,R::Counters+(w-1)*32+i*4,counters[i]))return Fail(io,r);
   if(counters[2]!=counters[6] || !Sources(io,w) || !PioneerQ38GraphCsrIdle(io))return Fail(io,r);
   for(unsigned i=0;i<64;i++){uint32_t actual=0;r.word=i;
    if(!io.TokenMemoryReadChecked(w,W::Boot+i*4,actual) || actual!=boot[i]
     || !io.TokenMemoryReadChecked(w,W::Job+i*4,actual) || actual!=job[i])return Fail(io,r);}
   uint32_t doorbell=0;if(!io.TokenMemoryReadChecked(w,W::Doorbell,doorbell) || doorbell!=nonce)return Fail(io,r);
   for(unsigned i=0;i<16;i++)if(!io.TokenMemoryReadChecked(w,W::Status+i*4,status[i]))return Fail(io,r);
   if(status[0]!=nonce || status[1]!=nonce || status[2] || status[3]!=W::Magic || status[4]!=W::Stop || status[5]
    || status[6]!=stopLayer || status[7]!=terminal[4])return Fail(io,r);
   for(unsigned i=8;i<16;i++)if(status[i])return Fail(io,r);
   r.firstPC=io.RuntimePcRead();r.secondPC=io.RuntimePcRead();
   if((r.firstPC!=0x20060 && r.firstPC!=0x20064) || (r.secondPC!=0x20060 && r.secondPC!=0x20064))return Fail(io,r);
   for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=counters[i])return Fail(io,r);
   if(!Sources(io,w) || !PioneerQ38GraphCsrIdle(io))return Fail(io,r);
   for(unsigned i=0;i<8;i++){
    if(recheck && terminalCounters[w][i]!=counters[i])return Fail(io,r);
    terminalCounters[w][i]=counters[i];
   } // authenticated current closed STOP journal ONLY;rechecks cannot adopt a changed journal
   terminalNonces[w]=nonce;
  }
  r.phase=11;r.worker=0;
  if(!Sources(io,0) || !PioneerQ38GraphCsrIdle(io))return Fail(io,r);
  for(unsigned i=0;i<64;i++){uint32_t actual=0;r.word=i;
   if(!io.TokenMemoryReadChecked(0,T::TerminalLocal+i*4,actual) || actual!=terminal[i])return Fail(io,r);}
  for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=terminal[36+i])return Fail(io,r);
  r.firstPC=io.RuntimePcRead();r.secondPC=io.RuntimePcRead();
  if(r.firstPC!=0x60004 || r.secondPC!=0x60004 || !Sources(io,0))return Fail(io,r);
  for(unsigned i=0;i<64;i++){
   if(recheck && observedTerminal[i]!=terminal[i])return Fail(io,r);
   observedTerminal[i]=terminal[i];
  }
  for(unsigned i=0;i<8;i++)terminalCounters[0][i]=terminal[36+i];
  terminalObserved=true;
  r.ready=1;r.cancelled=terminal[2]==3;r.committed=terminal[3];r.position=terminal[4];r.completedLayers=terminal[6];r.stage=terminal[10];
  return true;
 } // READ-ONLY genuine STOP observation. NO HOLD/reset/NIU commands,counter recapture,restore/END/disposal or reusable state grant.
 template<class IO,class Result> bool HoldGuard(IO& io,unsigned w,Result& r){
  if(!Sources(io,w) || !PioneerQ38GraphCsrIdle(io))return false;
  for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=terminalCounters[w][i])return false;
  if(running[w]){
   r.firstPC=io.RuntimePcRead();r.secondPC=io.RuntimePcRead();
   if(w?((r.firstPC!=0x20060 && r.firstPC!=0x20064) || (r.secondPC!=0x20060 && r.secondPC!=0x20064))
    :(r.firstPC!=0x60004 || r.secondPC!=0x60004))return false;
  } // PCs of already HELD cores are never treated as execution proof
  return Sources(io,w) && PioneerQ38GraphCsrIdle(io);
 }
 template<class IO> bool ReadIdle(IO& io,PioneerQ38TokenReadResult& r){
  if(readoutPending || !HeldAfterTerminal() || !HoldGuard(io,0,r))return false;
  if(readoutCommandKnown)for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=readoutCommand[i])return false;
  return Sources(io,0) && PioneerQ38GraphCsrIdle(io);
 }
 template<class IO> bool ReadFrontier(IO& io,const uint32_t* boot,PioneerQ38TokenReadResult& r){
  if(!ReadIdle(io,r))return false;
  for(unsigned i=0;i<64;i++){uint32_t actual=0;r.word=i;
   if(!io.TokenMemoryReadChecked(0,PioneerQ38TokenLayout::TerminalLocal+i*4,actual) || actual!=observedTerminal[i]
    || !io.TokenMemoryReadChecked(0,PioneerQ38GlobalChainLayout::R::Boot+i*4,actual) || actual!=boot[i])return false;
  }
  return ReadIdle(io,r);
 }
 template<class IO> bool ReadRecordPacket(IO& io,unsigned records,PioneerQ38TokenReadResult& r){
  namespace T=PioneerQ38TokenLayout;namespace C=PioneerQ38GlobalChainLayout;
  const unsigned bytes=records*T::RecordBytes;
  const uint64_t wide=uint64_t(plan.Workspace())+T::OutputOffset+uint64_t(readoutCursor)*T::RecordBytes;
  if(!records || records>64 || wide>UINT32_MAX || wide+bytes>uint64_t(plan.Workspace())+C::WorkspaceBytes
   || !ReadIdle(io,r))return false;
  const uint32_t address=uint32_t(wide),route=PioneerQ38ProjectionRoute(plan.Bank());
  const uint32_t command[]={address,0,route,C::R::Scratch,0,129,0,0,bytes,0};
  uint32_t before[8],expected[8];
  for(unsigned i=0;i<8;i++){
   before[i]=terminalCounters[0][i];
   expected[i]=before[i]+(i==1?bytes/64:i==0 || i==2 || i==3 || i==6?1U:0U);
  }
  // All processors are genuinely HELD and NoC is idle. The first command
  // replaces the completed ROM's inert command words,NOT its counter epoch.
  // Subsequent commands must match this owner's own exact prior journal.
  readoutCommandKnown=true;
  for(unsigned i=0;i<10;i++){
   readoutCommand[i]=command[i];io.ResidentNocWrite(i*4,command[i]);
   if(!Sources(io,0) || io.ResidentNocRead(i*4)!=command[i])return false;
  }
  const uint64_t began=io.Q38CohortNow();uint64_t previous=began;
  if(began>UINT64_MAX-UINT64_C(30000000) || !ReadIdle(io,r))return false;
  readoutPending=true;io.ResidentNocWrite(0x40,1);r.packets++;bool done=false;
  for(unsigned poll=0;poll<65536;poll++){
   if(!BootTime(io,began,previous) || !Sources(io,0))return false;
   const uint32_t busy=io.ResidentNocRead(0x40);if(busy>1)return false;done=!busy;
   for(unsigned i=0;i<8;i++){
    const uint32_t actual=io.ResidentNocRead(PioneerQ38PanelCounter(i));
    if(uint32_t(actual-before[i])>uint32_t(expected[i]-before[i]))return false;
    done &= actual==expected[i];
   }
   if(done)break;
   io.Pause();
  }
  if(!done || !Sources(io,0) || !PioneerQ38GraphCsrIdle(io))return false;
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=command[i])return false;
  // ONLY exact completed known READ deltas advance the current journal.
  // Historical token-record counters never participate in this update.
  for(unsigned i=0;i<8;i++)terminalCounters[0][i]=expected[i];
  readoutPending=false;r.bytesRead+=bytes;return ReadIdle(io,r);
 }
 bool PromptToken(unsigned position,uint32_t& output)const{
  if(position>=PioneerQ38TokenLayout::Context)return false;
  uint8_t raw[64];const unsigned offset=position*4;
  if(!startup->ReadPrompt(offset&~63U,raw,64))return false;
  const unsigned at=offset&63;uint32_t value=0;
  for(unsigned i=0;i<4;i++)value|=uint32_t(raw[at+i])<<(i*8);
  if(value>=248320)return false;
  output=value;return true;
 }
public:
 template<class IO> __attribute__((noinline)) bool ObserveTerminal(IO& io,PioneerQ38TokenTerminalResult& r){return InspectTerminal(io,r,false);}
 template<class IO> bool HoldStopped(IO& io,PioneerQ38TokenHoldResult& r){
  const PioneerQ38TokenHoldResult pristine;const auto* a=reinterpret_cast<const uint8_t*>(&r);
  const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
  for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
  if(holding || held || !TerminalObserved() || !ExecutionStarted() || !Scope(io))return false;
  r.started=1;r.phase=1;PioneerQ38TokenTerminalResult fresh;
  // Re-prove the SAME terminal and every worker in THIS call BEFORE the
  // first HOLD. Cached ready/counters alone can never authorize a reset.
  if(!InspectTerminal(io,fresh,true) || !fresh.ready)return Fail(io,r);
  const uint64_t began=io.Q38CohortNow();uint64_t previous=began;
  if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  holding=true;r.phase=2;
  for(unsigned w=0;w<120;w++){
   r.worker=w;
   if(!BootTime(io,began,previous) || !HoldGuard(io,w,r))return Fail(io,r);
   // ROOT first;one known HOLD per core. Mark before uncertain writes so
   // any failure retains the entire cohort without retry/repair/release.
   running[w]=cohort->worker[w].image.running=false;reset[w]=0x47800;
   if(!w)cohort->rootRunning=false;
   io.RuntimeResetWrite(reset[w]);
   if(!BootTime(io,began,previous) || !HoldGuard(io,w,r))return Fail(io,r);
   io.Pause();if(!BootTime(io,began,previous) || !HoldGuard(io,w,r))return Fail(io,r);
   r.held++;
  }
  r.phase=3;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(running[w] || !BootTime(io,began,previous) || !HoldGuard(io,w,r))return Fail(io,r);
  }
  r.worker=0;
  if(r.held!=120 || !Sources(io,0))return Fail(io,r);
  for(unsigned i=0;i<64;i++){uint32_t actual=0;r.word=i;
   if(!io.TokenMemoryReadChecked(0,PioneerQ38TokenLayout::TerminalLocal+i*4,actual) || actual!=observedTerminal[i])return Fail(io,r);
  }
  if(!HoldGuard(io,0,r))return Fail(io,r);
  held=true;holding=false;r.ready=1;r.phase=4;return true;
 } // actual120 HOLD only. No L1/CSR restore,NIU command,token delivery,END/disposal or current-state reuse grant.
 template<class IO> bool ReadCommitted(IO& io,unsigned records,uint32_t* output,PioneerQ38TokenReadResult& r){
  namespace T=PioneerQ38TokenLayout;namespace C=PioneerQ38GlobalChainLayout;
  const PioneerQ38TokenReadResult pristine;const auto* a=reinterpret_cast<const uint8_t*>(&r);
  const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
  for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
  if(restoring || l1Restored || readoutPending || readoutDrained || !HeldAfterTerminal() || records>64 || readoutCursor>observedTerminal[3]
   || records>observedTerminal[3]-readoutCursor || (records?!output:(output || observedTerminal[3] || readoutCursor))
   || !Scope(io))return false;
  r.started=1;r.phase=1;r.firstRecord=readoutCursor;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!HoldGuard(io,w,r))return Fail(io,r);}
  r.worker=0;uint32_t boot[64],session[128];auto* raw=reinterpret_cast<uint8_t*>(session);
  if(!startup->ReadBoot(0,boot) || !startup->ReadSession(0,raw,512))return Fail(io,r);
  for(unsigned i=0;i<128;i++)session[i]=uint32_t(raw[i*4])|uint32_t(raw[i*4+1])<<8|uint32_t(raw[i*4+2])<<16|uint32_t(raw[i*4+3])<<24;
  if(!T::Session(session,boot,plan.Bank(),plan.Workspace()) || !ReadFrontier(io,boot,r))return Fail(io,r);
  if(!records){
   readoutDrained=true;r.ready=1;r.drained=1;r.phase=4;return true;
  } // genuinely cancelled BEFORE any commit;NO GDDR record or token invented
  r.phase=2;
  if(!ReadRecordPacket(io,records,r))return Fail(io,r);
  for(unsigned i=0;i<records*64;i++){r.word=i;
   if(!io.TokenMemoryReadChecked(0,C::R::Scratch+i*4,staging[i]))return Fail(io,r);
  }
  r.phase=3;uint32_t input=readoutInput;
  if(!readoutCursor && !PromptToken(0,input))return Fail(io,r);
  for(unsigned n=0;n<records;n++){
   r.word=n;const unsigned position=readoutCursor+n;const uint32_t* record=staging+n*64;
   if(!PioneerQ38TokenRecordShape(record,boot,observedTerminal,position,input,session[5],session[6],session[32]!=0,session[33],session[34]))return Fail(io,r);
   input=record[4];
   if(position+1<session[6] && !PromptToken(position+1,input))return Fail(io,r);
  }
  for(unsigned w=0;w<120;w++){r.worker=w;if(!HoldGuard(io,w,r))return Fail(io,r);}
  r.worker=0;if(!ReadFrontier(io,boot,r))return Fail(io,r);
  // Trusted non-aliased kernel output,atomic publication after the ENTIRE
  // batch proves valid. No CPU model product is computed or supplied back.
  for(unsigned i=0;i<records*64;i++)output[i]=staging[i];
  readoutCursor+=records;readoutInput=input;readoutDrained=readoutCursor==observedTerminal[3];
  r.ready=1;r.recordCount=records;r.drained=readoutDrained;r.phase=4;return true;
 } // immutable generated DATA only;NO current-state reuse,L1/CSR restore,END/disposal or running-ROOT NIU access.
 template<class IO> bool RestoreOriginalL1(IO& io,PioneerQ38TokenRestoreResult& r){
  namespace L=PioneerQ38TokenL1;namespace P=PioneerQ38TokenPrograms;
  const PioneerQ38TokenRestoreResult pristine;const auto* a=reinterpret_cast<const uint8_t*>(&r);
  const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
  for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
  if(restoring || l1Restored || readoutPending || !CommittedRecordsRead() || !Scope(io))return false;
  r.started=1;r.phase=1;uint32_t boot[64];PioneerQ38TokenReadResult frontier;
  if(!startup->ReadBoot(0,boot) || !ReadFrontier(io,boot,frontier))return Fail(io,r);
  const uint64_t began=io.Q38CohortNow();uint64_t previous=began;
  if(began>UINT64_MAX-UINT64_C(300000000))return Fail(io,r);
  auto time=[&](){
   if(!Scope(io))return false;
   const uint64_t now=io.Q38CohortNow();
   if(now<previous || now<began || now-began>=UINT64_C(300000000))return false;
   previous=now;return true;
  };
  // Complete immutable image proof over ALL participants BEFORE any L1
  // original is written. No cached HOLD or caller clean bit substitutes.
  for(unsigned w=0;w<120;w++){
   r.worker=w;PioneerQ38TokenMemoryResult image;
   if(!time() || !HoldGuard(io,w,r) || !memory->VerifyImage(io,w,image) || !HoldGuard(io,w,r))return Fail(io,r);
   r.programWords+=P::CodeWords(w);
  }
  if(r.programWords!=P::CodeWords(0)+119*P::CodeWords(1))return Fail(io,r);
  restoring=true;r.phase=2;r.savedWords=L::SavedWords;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!time() || !HoldGuard(io,w,r))return Fail(io,r);
   for(unsigned i=0;i<L::SnapshotWords(w);i++){
    r.word=i;const uint32_t address=L::SnapshotAddress(w,i),expected=memory->saved[L::SnapshotBase(w)+i];uint32_t actual=0;
    if(!io.TokenMemoryWrite(w,address,expected) || !io.TokenMemoryReadChecked(w,address,actual) || actual!=expected)return Fail(io,r);
    r.restoredWords++;
   }
   if(!time() || !HoldGuard(io,w,r) || !PioneerQ38GraphCsrCode(io,w))return Fail(io,r);
   r.restoredCores++;
  }
  r.phase=3;
  for(unsigned w=0;w<120;w++){
   r.worker=w;PioneerQ38TokenMemoryResult original;
   if(!time() || !HoldGuard(io,w,r) || !memory->VerifyOriginal(io,w,original) || !HoldGuard(io,w,r))return Fail(io,r);
   r.verifiedWords+=original.verifiedOriginalWords;
  }
  if(r.restoredCores!=120 || r.restoredWords!=L::SavedWords || r.verifiedWords!=L::SavedWords || !time())return Fail(io,r);
  r.worker=0;if(!HoldGuard(io,0,r))return Fail(io,r);
  l1Restored=true;restoring=false;memory->installed=false;r.ready=1;r.phase=4;return true;
 } // complete SAME original L1 journal restored;NO counter rewind/NIU command/CSR or dispatch close/END/unmap/host release.
 template<class IO> bool RestoreDispatchImages(IO& io,PioneerQ38TokenDispatchRestoreResult& r){
  const PioneerQ38TokenDispatchRestoreResult pristine;const auto* a=reinterpret_cast<const uint8_t*>(&r);
  const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
  for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
  if(dispatchRestoring || dispatchRestored || !OriginalL1Restored() || !DispatchOutput(&r,sizeof(r)) || !Scope(io))return false;
  r.started=1;r.phase=1;const uint64_t began=io.Q38CohortNow();uint64_t previous=began;
  if(began>UINT64_MAX-UINT64_C(300000000))return Fail(io,r);
  auto time=[&](){
   if(!Scope(io))return false;
   const uint64_t now=io.Q38CohortNow();
   if(now<previous || now<began || now-began>=UINT64_C(300000000))return false;
   previous=now;return true;
  };
  // The complete earlier dispatch images must be present on ALL120 before
  // the first original is written. This is NOT the broad legacy close.
  for(unsigned w=0;w<120;w++){
   r.worker=w;
   if(!time() || !HoldGuard(io,w,r) || io.Q38PanelSavedWords()!=dispatchSaved[w]
    || !PioneerQ38GraphCsrCode(io,w) || !HoldGuard(io,w,r))return Fail(io,r);
  }
  dispatchRestoring=true;r.phase=2;r.savedWords=120U*(TT_Q38_PANEL_VIEW_WORDS+1U);
  for(unsigned end=120;end;end--){
   const unsigned w=end-1;r.worker=w;
   if(!time() || !HoldGuard(io,w,r) || io.Q38PanelSavedWords()!=dispatchSaved[w])return Fail(io,r);
   for(unsigned i=0;i<TT_Q38_PANEL_VIEW_WORDS;i++){
    r.word=i;const uint32_t address=TT_Q38_PANEL_VIEW_BASE+i*4,expected=dispatchSaved[w][1+i];uint32_t actual=0;
    if(!io.TokenDispatchImageWrite(w,address,expected) || !io.TokenDispatchImageReadChecked(w,address,actual) || actual!=expected)return Fail(io,r);
    r.restoredWords++;
   }
   uint32_t actual=0;r.word=TT_Q38_PANEL_VIEW_WORDS;
   if(!io.TokenDispatchImageWrite(w,0x6000,dispatchSaved[w][0])
    || !io.TokenDispatchImageReadChecked(w,0x6000,actual) || actual!=dispatchSaved[w][0]
    || !time() || !HoldGuard(io,w,r))return Fail(io,r);
   r.restoredWords++;r.entryWords++;r.restoredCores++;
  }
  r.phase=3;
  for(unsigned w=0;w<120;w++){
   r.worker=w;
   if(!time() || !HoldGuard(io,w,r) || io.Q38PanelSavedWords()!=dispatchSaved[w])return Fail(io,r);
   for(unsigned i=0;i<TT_Q38_PANEL_VIEW_WORDS+1U;i++){
    r.word=i;uint32_t actual=0;const uint32_t address=i?TT_Q38_PANEL_VIEW_BASE+(i-1)*4:0x6000;
    if(!io.TokenDispatchImageReadChecked(w,address,actual) || actual!=dispatchSaved[w][i])return Fail(io,r);
    r.verifiedWords++;
   }
   if(!HoldGuard(io,w,r))return Fail(io,r);
  }
  if(r.restoredCores!=120 || r.entryWords!=120 || r.restoredWords!=r.savedWords || r.verifiedWords!=r.savedWords
   || !time() || !HoldGuard(io,0,r))return Fail(io,r);
  // CPU-only atomic publication under the SAME driver lock;no intermediate
  // node-ready hint can authorize cleanup/reuse of this launched owner.
  for(auto& node:cohort->worker){node.image.ready=false;node.image.dirty=false;}
  dispatchRestored=true;dispatchRestoring=false;r.ready=1;r.phase=4;return true;
 } // earlier dispatch L1 originals ONLY;NIU/scratch/TLB/clock/maps/hosts still owned. NO END.
};
static_assert(sizeof(PioneerQ38TokenRuntimeResult)==64,"all runtime result bytes explicit");
static_assert(sizeof(PioneerQ38TokenBootstrapResult)==64,"all bootstrap result bytes explicit");
static_assert(sizeof(PioneerQ38TokenExecutionResult)==64,"all execution result bytes explicit");
static_assert(sizeof(PioneerQ38TokenTerminalResult)==64,"all terminal-observation result bytes explicit");
static_assert(sizeof(PioneerQ38TokenHoldResult)==64,"all terminal-HOLD result bytes explicit");
static_assert(sizeof(PioneerQ38TokenReadResult)==64,"all committed-readout result bytes explicit");
static_assert(sizeof(PioneerQ38TokenRestoreResult)==64,"all L1-restoration result bytes explicit");
static_assert(sizeof(PioneerQ38TokenDispatchRestoreResult)==64,"all dispatch-restoration result bytes explicit");
static_assert(sizeof(PioneerQ38TokenNativeOwner)>16384 && sizeof(PioneerQ38TokenNativeOwner)<32768,"native owner staging buffer HEAP ONLY");
#endif
