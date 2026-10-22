/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_GRAPH_MEMORY_H
#define PIONEER_Q38_PROJECTION_GRAPH_MEMORY_H
#include "PioneerQ38ProjectionGraphFootprint.h"
#include "PioneerQ38CanonicalModelPlan.h"
#include "PioneerQ38GdnIdentity.h"
#include "PioneerQ38GraphCsrProbe.h"
// PRIVATE HEAP memory component for the new complete projection lifetime.
// Snapshot ALL code/stacks/runtime/trailers/new ROOT manifests BEFORE ANY
// write,then install exact code ONCE. No NoC packet/reset/start/host DMA
// exists in this class. The full execution controller is STILL REQUIRED.
// Scope is a real native cookie/global-lock/source/model/clock/host check,
// not a caller ready bit. Invalid CPU preconditions are inert. First IO
// failure retains ALL objects/maps and permits NO later read/write/retry.
struct PioneerQ38ProjectionGraphMemoryResult {
 uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,installed=0,closed=0;
 uint32_t savedWords=0,verifiedOriginalWords=0,installedWords=0,restoredWords=0;
};
class PioneerQ38ProjectionGraphOwner;
class PioneerQ38ProjectionGraphMemory {
 friend class PioneerQ38ProjectionGraphOwner;
 const tt_dram_samples_result* proof=nullptr;
 PioneerResidentBanks* banks=nullptr;
 PioneerQ38DispatchLease* cohort=nullptr;
 const PioneerQ38CanonicalModelPlan* plan=nullptr;
 const uint8_t* ledger=nullptr;
 PioneerQ38GdnIdentity identity={};
 uint32_t command[120][10]={},counters[120][8]={};
 uint32_t saved[8134072]; // HEAP ONLY;arbitrary original words,NOT reconstructed padding
 unsigned sampled=0;
 bool started=false,installed=false,closed=false,unsafe=false,launched=false;
 template<class IO> bool Fail(IO& io,PioneerQ38ProjectionGraphMemoryResult& r){
  unsafe=true;installed=false;r.unsafe=1;
  if(cohort && banks)PioneerQ38DispatchQuarantine(*cohort,*banks);
  io.QuarantineHostPage();io.ProjectionGraphRetainHostOwners();return false; // MEMORY ONLY
 }
 template<class IO> bool Scope(IO& io)const{
  return started && !unsafe && !closed && !launched && proof && banks && cohort && plan && ledger
   && MatchesNativeOwner(*proof,*banks,*cohort,*plan,identity,ledger,PioneerResidentBanks::Pages)
   && io.ProjectionGraphMemoryScope(*this,*plan,identity,ledger,PioneerResidentBanks::Pages);
 }
 template<class IO> bool Guard(IO& io,unsigned w){
  if(w>=120 || !Scope(io) || !PioneerQ38GraphCsrGuard(io,*proof,*banks,*cohort,w)
   || !io.ProjectionGraphMemoryMappingStable(w))return false;
  // This component emits NO NoC traffic. A changed command/counter is
  // foreign or unknown work,NEVER an excuse to repair a baseline.
  if(w<sampled){
   for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=command[w][i])return false;
   for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=counters[w][i])return false;
  }
  return true;
 }
 template<class IO> bool VerifyOriginal(IO& io,unsigned w,PioneerQ38ProjectionGraphMemoryResult& r){
  const unsigned count=PioneerQ38ProjectionGraphSnapshotWords(w),base=PioneerQ38ProjectionGraphSnapshotBase(w);
  for(unsigned i=0;i<count;i++){
   r.word=i;uint32_t actual;
   if(!io.ProjectionGraphMemoryReadChecked(w,PioneerQ38ProjectionGraphSnapshotAddress(w,i),actual) || actual!=saved[base+i])return false;
   r.verifiedOriginalWords++;
  }
  return true;
 }
 template<class IO> bool VerifyImage(IO& io,unsigned w,PioneerQ38ProjectionGraphMemoryResult& r){
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(w);i++){
   r.word=i;
   uint32_t a=PioneerQ38ProjectionGraphCodeAddress(w,i),actual;
   if(!io.ProjectionGraphMemoryReadChecked(w,a,actual) || actual!=PioneerQ38ProjectionGraphProgramWord(w,a))return false;
  }
  return true;
 }
public:
 PioneerQ38ProjectionGraphMemory()=default;
 PioneerQ38ProjectionGraphMemory(const PioneerQ38ProjectionGraphMemory&)=delete;
 PioneerQ38ProjectionGraphMemory& operator=(const PioneerQ38ProjectionGraphMemory&)=delete;
 bool Unsafe()const{return unsafe;}
 bool Closed()const{return closed;}
 void Fault(){unsafe=true;installed=false;} // MEMORY ONLY;native context retains every snapshot/map
 bool Installed()const{return installed && !unsafe && !closed && !launched;}
 // Structural CPU binding only. Native Scope MUST ALSO authenticate the
 // real retained source/context and actual clock/hosts/maps/exclusivity.
 // This local-memory component proves NO generic GDDR initialization or
 // input producer,NO learned arithmetic or native execution capability.
 bool MatchesNativeOwner(const tt_dram_samples_result& dram,const PioneerResidentBanks& owner,
  const PioneerQ38DispatchLease& participants,const PioneerQ38CanonicalModelPlan& canonical,
  const PioneerQ38GdnIdentity& id,const uint8_t* initialized,unsigned pages)const{
  if(!started || unsafe || closed || launched || proof!=&dram || banks!=&owner || cohort!=&participants
   || plan!=&canonical || !(identity==id) || !id.Valid() || ledger!=initialized || !initialized
   || pages!=PioneerResidentBanks::Pages || !canonical.Ready() || !owner.Matches(dram)
   || participants.unsafe || participants.closed || participants.rootRunning || participants.prepared!=120
   || participants.bootAttempted || participants.booted)return false;
  for(unsigned w=0;w<120;w++){
   const auto& n=participants.worker[w];const auto& s=n.master?*n.master:n.store;
   if(!n.image.saved || !n.image.ready || n.image.unsafe || n.image.running || !s.ready || s.pending
    || s.recovery_required || n.noc.recovery_required)return false;
  }
  return true;
 }
 template<class IO> bool PrepareHeld(IO& io,const tt_dram_samples_result& dram,PioneerResidentBanks& owner,
  PioneerQ38DispatchLease& participants,const PioneerQ38CanonicalModelPlan& canonical,const PioneerQ38GdnIdentity& id,
  const uint8_t* initialized,unsigned pages,PioneerQ38ProjectionGraphMemoryResult& r){
  if(started || unsafe || installed || closed || launched || proof || banks || cohort || plan || ledger
   || !id.Valid() || !initialized || pages!=PioneerResidentBanks::Pages || !canonical.Ready() || !owner.Matches(dram)
   || participants.unsafe || participants.closed || participants.rootRunning || participants.prepared!=120
   || participants.bootAttempted || participants.booted || r.version!=1 || r.started || r.phase || r.worker || r.word
   || r.unsafe || r.installed || r.closed || r.savedWords || r.verifiedOriginalWords || r.installedWords || r.restoredWords)return false;
  for(unsigned w=0;w<120;w++){
   const auto& n=participants.worker[w];const auto& s=n.master?*n.master:n.store;
   if(!n.image.saved || !n.image.ready || n.image.unsafe || n.image.running || !s.ready || s.pending
    || s.recovery_required || n.noc.recovery_required)return false;
  }
  started=true;proof=&dram;banks=&owner;cohort=&participants;plan=&canonical;identity=id;ledger=initialized;r.started=1;r.phase=1;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !PioneerQ38GraphCsrCode(io,w))return Fail(io,r);
   for(unsigned i=0;i<10;i++)command[w][i]=io.ResidentNocRead(i*4);
   for(unsigned i=0;i<8;i++){
    counters[w][i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));if(i==6 && counters[w][2]!=counters[w][6])return Fail(io,r);
   }
   sampled=w+1;
   unsigned count=PioneerQ38ProjectionGraphSnapshotWords(w),base=PioneerQ38ProjectionGraphSnapshotBase(w);
   for(unsigned i=0;i<count;i++){
    r.word=i;uint32_t actual;
    if(!io.ProjectionGraphMemoryReadChecked(w,PioneerQ38ProjectionGraphSnapshotAddress(w,i),actual))return Fail(io,r);
    saved[base+i]=actual;r.savedWords++;
   }
   if(!Guard(io,w))return Fail(io,r);
  }
  if(r.savedWords!=8134072 || sampled!=120)return Fail(io,r);
  // Complete SECOND physical pass before FIRST write,not only a digest
  // or per-participant precheck after earlier participants were changed.
  r.phase=2;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !VerifyOriginal(io,w,r) || !Guard(io,w))return Fail(io,r);}
  if(r.verifiedOriginalWords!=8134072)return Fail(io,r);
  r.phase=3;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(w);i++){
    r.word=i;uint32_t a=PioneerQ38ProjectionGraphCodeAddress(w,i),expected=PioneerQ38ProjectionGraphProgramWord(w,a),actual;
    if(!io.ProjectionGraphMemoryWrite(w,a,expected) || !io.ProjectionGraphMemoryReadChecked(w,a,actual) || actual!=expected)return Fail(io,r);
    r.installedWords++;
   }
   if(!Guard(io,w))return Fail(io,r);
  }
  r.phase=4;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !VerifyImage(io,w,r) || !Guard(io,w))return Fail(io,r);}
  if(r.installedWords!=989304 || !Guard(io,0))return Fail(io,r);
  installed=true;r.installed=1;return true;
 }
 // ONLY for a never-launched held session. The future actual graph owner
 // has private access and must implement its OWN independent physical
 // STOP/counters/source/ALL-holds handoff. There is NO caller "clean" hint
 // or public restore-after-compute method. This MUST NOT become another
 // hardware boot/STOP diagnostic instead of full arithmetic integration.
 template<class IO> bool CloseHeldNoLaunch(IO& io,PioneerQ38ProjectionGraphMemoryResult& r){
  if(!Installed() || !r.installed || r.closed || r.unsafe || r.restoredWords)return false;
  r.phase=5;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !VerifyImage(io,w,r) || !Guard(io,w))return Fail(io,r);}
  r.phase=6;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w))return Fail(io,r);
   unsigned count=PioneerQ38ProjectionGraphSnapshotWords(w),base=PioneerQ38ProjectionGraphSnapshotBase(w);
   for(unsigned i=0;i<count;i++){
    r.word=i;uint32_t a=PioneerQ38ProjectionGraphSnapshotAddress(w,i),actual;
    if(!io.ProjectionGraphMemoryWrite(w,a,saved[base+i]) || !io.ProjectionGraphMemoryReadChecked(w,a,actual)
     || actual!=saved[base+i])return Fail(io,r);
    r.restoredWords++;
   }
   if(!Guard(io,w) || !PioneerQ38GraphCsrCode(io,w))return Fail(io,r);
  }
  r.phase=7;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !VerifyOriginal(io,w,r) || !Guard(io,w))return Fail(io,r);}
  if(r.restoredWords!=8134072 || r.verifiedOriginalWords!=2*8134072 || !Guard(io,0))return Fail(io,r);
  closed=true;installed=false;r.closed=1;return true;
 }
};
static_assert(sizeof(PioneerQ38ProjectionGraphMemory)>32536288 && sizeof(PioneerQ38ProjectionGraphMemory)<32600000,"complete retained graph memory is HEAP ONLY");
#endif
