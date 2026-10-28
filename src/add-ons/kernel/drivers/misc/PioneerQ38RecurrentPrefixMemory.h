/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_PREFIX_MEMORY_H
#define PIONEER_Q38_RECURRENT_PREFIX_MEMORY_H
#include <stddef.h>
#include "../../../../../tools/pioneer/PioneerQ38RecurrentPrefixPrograms.h"
#include "PioneerQ38CanonicalModelPlan.h"
#include "PioneerQ38GdnIdentity.h"
#include "PioneerQ38GraphCsrProbe.h"
// DISTINCT QPX1/QMW1 retained L1 journal, NOT the old QRB1 owner. The old dense memory type/scope/
// capacity is NOT an extended grant. This object is HEAP ONLY and includes
// the separate64000..6c000 ROOT Conv window and arbitrary untouched padding.
// No reset/start/NoC submission/host DMA/clock/END exists in this component.
// IO must implement the NEW genuine locked source/cookie/model/clock/host
// scope and revalidate its current alias on EVERY checked read/write.
// CPU fixture scopes are NOT evidence that such a native context exists.
// No public post-launch restore or caller-supplied clean/STOP hint exists.
struct PioneerQ38RecurrentPrefixMemoryResult {
 uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,installed=0,closed=0;
 uint32_t savedWords=0,verifiedOriginalWords=0,installedWords=0,restoredWords=0;
};
class PioneerQ38RecurrentPrefixOwner;
class PioneerQ38RecurrentPrefixMemory {
 friend class PioneerQ38RecurrentPrefixOwner;
 const tt_dram_samples_result* proof=nullptr;
 PioneerResidentBanks* banks=nullptr;
 PioneerQ38DispatchLease* cohort=nullptr;
 const PioneerQ38CanonicalModelPlan* plan=nullptr;
 const uint8_t* ledger=nullptr;
 PioneerQ38GdnIdentity identity={};
 uint32_t command[120][10]={},counters[120][8]={};
 uint32_t saved[TT_Q38_RECURRENT_PREFIX_SAVED_WORDS]; // never reconstructed zero/padding
 unsigned sampled=0;
 bool started=false,installed=false,closed=false,unsafe=false,launched=false;
 template<class IO> bool Fail(IO& io,PioneerQ38RecurrentPrefixMemoryResult& r){
  unsafe=true;installed=false;r.unsafe=1;
  if(cohort && banks)PioneerQ38DispatchQuarantine(*cohort,*banks);
  io.QuarantineHostPage();io.RecurrentPrefixRetainHostOwners();return false; // MEMORY ONLY
 }
 template<class IO> bool Scope(IO& io)const{
  return started && !unsafe && !closed && !launched && proof && banks && cohort && plan && ledger
   && MatchesNativeOwner(*proof,*banks,*cohort,*plan,identity,ledger,PioneerResidentBanks::Pages)
   && io.RecurrentPrefixMemoryScope(*this,*plan,identity,ledger,PioneerResidentBanks::Pages);
 }
 template<class IO> bool Guard(IO& io,unsigned w){
  if(w>=120 || !Scope(io) || !PioneerQ38GraphCsrGuard(io,*proof,*banks,*cohort,w)
   || !io.RecurrentPrefixMemoryMappingStable(w))return false;
  // No command/counter changes are caused by this held journal. Foreign
  // work is not permission to repair/recapture a baseline or attempt END.
  if(w<sampled){
   for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=command[w][i])return false;
   for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=counters[w][i])return false;
  }
  return true;
 }
 template<class IO> bool VerifyOriginal(IO& io,unsigned w,PioneerQ38RecurrentPrefixMemoryResult& r){
  const unsigned count=PioneerQ38RecurrentPrefixSnapshotWords(w),base=PioneerQ38RecurrentPrefixSnapshotBase(w);
  for(unsigned i=0;i<count;i++){
   r.word=i;uint32_t actual;
   if(!io.RecurrentPrefixMemoryReadChecked(w,PioneerQ38RecurrentPrefixSnapshotAddress(w,i),actual) || actual!=saved[base+i])return false;
   r.verifiedOriginalWords++;
  }
  return true;
 }
 template<class IO> bool VerifyImage(IO& io,unsigned w,PioneerQ38RecurrentPrefixMemoryResult& r){
  for(unsigned i=0;i<PioneerQ38RecurrentPrefixCodeWords(w);i++){
   r.word=i;uint32_t a=PioneerQ38RecurrentPrefixCodeAddress(w,i),actual,expected;
   if(!PioneerQ38RecurrentPrefixProgramWord(w,a,expected)
    || !io.RecurrentPrefixMemoryReadChecked(w,a,actual) || actual!=expected)return false;
  }
  return true;
 }
public:
 PioneerQ38RecurrentPrefixMemory()=default;
 PioneerQ38RecurrentPrefixMemory(const PioneerQ38RecurrentPrefixMemory&)=delete;
 PioneerQ38RecurrentPrefixMemory& operator=(const PioneerQ38RecurrentPrefixMemory&)=delete;
 bool Unsafe()const{return unsafe;}
 bool Closed()const{return closed;}
 bool Installed()const{return installed && !unsafe && !closed && !launched;}
 void Fault(){unsafe=true;installed=false;} // MEMORY ONLY;native context retains every owner/map
 // Binding is necessary,not sufficient: initialized learned/state/input,
 // original-source identity and actual native scope are separate proofs.
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
  const uint8_t* initialized,unsigned pages,PioneerQ38RecurrentPrefixMemoryResult& r){
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
   unsigned count=PioneerQ38RecurrentPrefixSnapshotWords(w),base=PioneerQ38RecurrentPrefixSnapshotBase(w);
   for(unsigned i=0;i<count;i++){
    r.word=i;uint32_t actual;
    if(!io.RecurrentPrefixMemoryReadChecked(w,PioneerQ38RecurrentPrefixSnapshotAddress(w,i),actual))return Fail(io,r);
    saved[base+i]=actual;r.savedWords++;
   }
   if(!Guard(io,w))return Fail(io,r);
  }
  if(r.savedWords!=TT_Q38_RECURRENT_PREFIX_SAVED_WORDS || sampled!=120)return Fail(io,r);
  // SECOND complete physical pass over ALL120 BEFORE the FIRST write.
  // A per-core precheck after earlier cores were changed is insufficient.
  r.phase=2;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !VerifyOriginal(io,w,r) || !Guard(io,w))return Fail(io,r);}
  if(r.verifiedOriginalWords!=TT_Q38_RECURRENT_PREFIX_SAVED_WORDS)return Fail(io,r);
  r.phase=3;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38RecurrentPrefixCodeWords(w);i++){
    r.word=i;uint32_t a=PioneerQ38RecurrentPrefixCodeAddress(w,i),expected,actual;
    if(!PioneerQ38RecurrentPrefixProgramWord(w,a,expected) || !io.RecurrentPrefixMemoryWrite(w,a,expected)
     || !io.RecurrentPrefixMemoryReadChecked(w,a,actual) || actual!=expected)return Fail(io,r);
    r.installedWords++;
   }
   if(!Guard(io,w))return Fail(io,r);
  }
  r.phase=4;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !VerifyImage(io,w,r) || !Guard(io,w))return Fail(io,r);}
  if(r.installedWords!=TT_Q38_RECURRENT_PREFIX_CODE_WORDS || !Guard(io,0))return Fail(io,r);
  installed=true;r.installed=1;return true;
 }
 // NEVER launched only. The future actual QPX1 graph owner is a DISTINCT
 // private friend and must prove genuine STOP/all120 holds/restoration,
 // not call this with a fabricated clean bit or transplant an old owner.
 template<class IO> bool CloseHeldNoLaunch(IO& io,PioneerQ38RecurrentPrefixMemoryResult& r){
  if(!Installed() || !r.installed || r.closed || r.unsafe || r.restoredWords)return false;
  r.phase=5;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !VerifyImage(io,w,r) || !Guard(io,w))return Fail(io,r);}
  r.phase=6;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w))return Fail(io,r);
   unsigned count=PioneerQ38RecurrentPrefixSnapshotWords(w),base=PioneerQ38RecurrentPrefixSnapshotBase(w);
   for(unsigned i=0;i<count;i++){
    r.word=i;uint32_t a=PioneerQ38RecurrentPrefixSnapshotAddress(w,i),actual;
    if(!io.RecurrentPrefixMemoryWrite(w,a,saved[base+i]) || !io.RecurrentPrefixMemoryReadChecked(w,a,actual)
     || actual!=saved[base+i])return Fail(io,r);
    r.restoredWords++;
   }
   if(!Guard(io,w) || !PioneerQ38GraphCsrCode(io,w))return Fail(io,r);
  }
  r.phase=7;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !VerifyOriginal(io,w,r) || !Guard(io,w))return Fail(io,r);}
  if(r.restoredWords!=TT_Q38_RECURRENT_PREFIX_SAVED_WORDS || r.verifiedOriginalWords!=2*TT_Q38_RECURRENT_PREFIX_SAVED_WORDS || !Guard(io,0))return Fail(io,r);
  closed=true;installed=false;r.closed=1;return true;
 }
};
static_assert(sizeof(PioneerQ38RecurrentPrefixMemory)>TT_Q38_RECURRENT_PREFIX_SAVED_WORDS*4
 && sizeof(PioneerQ38RecurrentPrefixMemory)<TT_Q38_RECURRENT_PREFIX_SAVED_WORDS*4+63712,"ALL recurrent arbitrary originals HEAP ONLY");
#endif
