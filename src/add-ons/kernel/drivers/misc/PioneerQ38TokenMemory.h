/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_MEMORY_H
#define PIONEER_Q38_TOKEN_MEMORY_H
#include <stddef.h>
#include "../../../../../tools/pioneer/PioneerQ38TokenPrograms.h"
#include "../../../../../tools/pioneer/PioneerQ38TokenStartupPlan.h"
#include "PioneerQ38GraphCsrProbe.h"
// DISTINCT full-token held L1 journal. Not the three-layer prefix journal,
// an embedding producer,initialized state,model-source/clock/host permission,
// a launched owner or a caller-supplied STOP/clean capability. Genuine native
// IO must bind this SAME object+startup+source+FD+thread+generation lifetime.
// No conventional driver route includes this header yet.
struct PioneerQ38TokenMemoryResult {
 uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,installed=0,closed=0;
 uint32_t savedWords=0,verifiedOriginalWords=0,installedWords=0,restoredWords=0;
};
class PioneerQ38TokenNativeOwner;
class PioneerQ38TokenMemory {
 friend class PioneerQ38TokenNativeOwner; // only this future DISTINCT owner may bind a real launch
 const tt_dram_samples_result* proof=nullptr;PioneerResidentBanks* banks=nullptr;
 PioneerQ38DispatchLease* cohort=nullptr;const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const PioneerQ38TokenStartupPlan* startup=nullptr;const uint8_t* ledger=nullptr;
 PioneerQ38GdnIdentity identity={};uint32_t command[120][10]={},counters[120][8]={};
 uint32_t saved[PioneerQ38TokenL1::SavedWords]; // ALL arbitrary originals;HEAP ONLY
 unsigned sampled=0;bool started=false,installed=false,closed=false,unsafe=false,launched=false;
 template<class IO> bool Fail(IO& io,PioneerQ38TokenMemoryResult& result){
  unsafe=true;installed=false;result.unsafe=1;
  if(cohort && banks)PioneerQ38DispatchQuarantine(*cohort,*banks);
  io.QuarantineHostPage();io.TokenMemoryRetainHostOwners();return false; // MEMORY ONLY,no retry/repair/restore
 }
 template<class IO> bool Scope(IO& io)const{
  return started && !unsafe && !closed && !launched && proof && banks && cohort && canonical && startup && ledger
   && MatchesNativeOwner(*proof,*banks,*cohort,*canonical,*startup,identity,ledger,PioneerResidentBanks::Pages)
   && io.TokenMemoryScope(*this,*canonical,*startup,identity,ledger,PioneerResidentBanks::Pages);
 }
 template<class IO> bool Guard(IO& io,unsigned worker){
  if(worker>=120 || !Scope(io) || !PioneerQ38GraphCsrGuard(io,*proof,*banks,*cohort,worker)
   || !io.TokenMemoryMappingStable(worker))return false;
  if(worker<sampled){
   for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=command[worker][i])return false;
   for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=counters[worker][i])return false;
  }
  return true;
 }
 template<class IO> bool VerifyOriginal(IO& io,unsigned worker,PioneerQ38TokenMemoryResult& result){
  namespace L=PioneerQ38TokenL1;
  const unsigned count=L::SnapshotWords(worker),base=L::SnapshotBase(worker);
  for(unsigned i=0;i<count;i++){
   result.word=i;uint32_t actual;
   if(!io.TokenMemoryReadChecked(worker,L::SnapshotAddress(worker,i),actual) || actual!=saved[base+i])return false;
   result.verifiedOriginalWords++;
  }
  return true;
 }
 template<class IO> bool VerifyImage(IO& io,unsigned worker,PioneerQ38TokenMemoryResult& result){
  namespace P=PioneerQ38TokenPrograms;
  for(unsigned i=0;i<P::CodeWords(worker);i++){
   result.word=i;uint32_t address=P::CodeAddress(worker,i),actual,expected;
   if(!P::ProgramWord(worker,address,expected) || !io.TokenMemoryReadChecked(worker,address,actual) || actual!=expected)return false;
  }
  return true;
 }
public:
 PioneerQ38TokenMemory()=default;
 PioneerQ38TokenMemory(const PioneerQ38TokenMemory&)=delete;
 PioneerQ38TokenMemory& operator=(const PioneerQ38TokenMemory&)=delete;
 bool Unsafe()const{return unsafe;}
 bool Installed()const{return installed && !unsafe && !closed && !launched;}
 bool Closed()const{return closed && !unsafe && !launched;}
 void Fault(){unsafe=true;installed=false;} // MEMORY ONLY;retain every native source/map/host/clock owner
 bool MatchesNativeOwner(const tt_dram_samples_result& dram,const PioneerResidentBanks& owner,const PioneerQ38DispatchLease& participants,
  const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38TokenStartupPlan& data,const PioneerQ38GdnIdentity& id,
  const uint8_t* initialized,unsigned pages)const{
  if(!started || unsafe || closed || launched || proof!=&dram || banks!=&owner || cohort!=&participants || canonical!=&plan
   || startup!=&data || ledger!=initialized || !initialized || pages!=PioneerResidentBanks::Pages || !(identity==id) || !id.Valid()
   || !plan.Ready() || !owner.Matches(dram) || participants.unsafe || participants.closed || participants.rootRunning
   || participants.prepared!=120 || participants.bootAttempted || participants.booted)return false;
  for(unsigned worker=0;worker<120;worker++){
   const auto& node=participants.worker[worker];const auto& store=node.master?*node.master:node.store;
   if(!node.image.saved || !node.image.ready || node.image.unsafe || node.image.running || !store.ready || store.pending
    || store.recovery_required || node.noc.recovery_required)return false;
  }
  return true; // necessary SAME private binding only,not a native source/phase capability
 }
 template<class IO> bool PrepareHeld(IO& io,const tt_dram_samples_result& dram,PioneerResidentBanks& owner,
  PioneerQ38DispatchLease& participants,const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38TokenStartupPlan& data,
  const PioneerQ38GdnIdentity& id,const uint8_t* initialized,unsigned pages,PioneerQ38TokenMemoryResult& result){
  namespace L=PioneerQ38TokenL1;namespace P=PioneerQ38TokenPrograms;
  static_assert(P::CancelBoundary==1,"full native candidate requires DISTINCT cancellable FP32 firmware");
  if(started || unsafe || installed || closed || launched || proof || banks || cohort || canonical || startup || ledger
   || !id.Valid() || !initialized || pages!=PioneerResidentBanks::Pages || !plan.Ready() || !owner.Matches(dram)
   || participants.unsafe || participants.closed || participants.rootRunning || participants.prepared!=120
   || participants.bootAttempted || participants.booted || result.version!=1 || result.started || result.phase || result.worker || result.word
   || result.unsafe || result.installed || result.closed || result.savedWords || result.verifiedOriginalWords || result.installedWords || result.restoredWords)return false;
  for(unsigned worker=0;worker<120;worker++){
   const auto& node=participants.worker[worker];const auto& store=node.master?*node.master:node.store;
   if(!node.image.saved || !node.image.ready || node.image.unsafe || node.image.running || !store.ready || store.pending
    || store.recovery_required || node.noc.recovery_required)return false;
  }
  if(!data.MatchesBinding(plan,data.Bank(),id,&participants.authority.source[0][0]) || !data.LearnedPagesPresent(initialized,pages))return false;
  started=true;proof=&dram;banks=&owner;cohort=&participants;canonical=&plan;startup=&data;identity=id;ledger=initialized;
  result.started=1;result.phase=1;
  for(unsigned worker=0;worker<120;worker++){
   result.worker=worker;if(!Guard(io,worker) || !PioneerQ38GraphCsrCode(io,worker))return Fail(io,result);
   for(unsigned i=0;i<10;i++)command[worker][i]=io.ResidentNocRead(i*4);
   for(unsigned i=0;i<8;i++){
    counters[worker][i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
    if(i==6 && counters[worker][2]!=counters[worker][6])return Fail(io,result);
   }
   sampled=worker+1;
   for(unsigned i=0;i<L::SnapshotWords(worker);i++){
    result.word=i;uint32_t actual;
    if(!io.TokenMemoryReadChecked(worker,L::SnapshotAddress(worker,i),actual))return Fail(io,result);
    saved[L::SnapshotBase(worker)+i]=actual;result.savedWords++;
   }
   if(!Guard(io,worker))return Fail(io,result);
  }
  if(result.savedWords!=L::SavedWords || sampled!=120)return Fail(io,result);
  // SECOND complete pass over EVERY participant BEFORE the FIRST write.
  result.phase=2;
  for(unsigned worker=0;worker<120;worker++){
   result.worker=worker;if(!Guard(io,worker) || !VerifyOriginal(io,worker,result) || !Guard(io,worker))return Fail(io,result);
  }
  if(result.verifiedOriginalWords!=L::SavedWords)return Fail(io,result);
  result.phase=3;
  for(unsigned worker=0;worker<120;worker++){
   result.worker=worker;if(!Guard(io,worker))return Fail(io,result);
   for(unsigned i=0;i<P::CodeWords(worker);i++){
    result.word=i;uint32_t address=P::CodeAddress(worker,i),expected,actual;
    if(!P::ProgramWord(worker,address,expected) || !io.TokenMemoryWrite(worker,address,expected)
     || !io.TokenMemoryReadChecked(worker,address,actual) || actual!=expected)return Fail(io,result);
    result.installedWords++;
   }
   if(!Guard(io,worker))return Fail(io,result);
  }
  result.phase=4;
  for(unsigned worker=0;worker<120;worker++){
   result.worker=worker;if(!Guard(io,worker) || !VerifyImage(io,worker,result) || !Guard(io,worker))return Fail(io,result);
  }
  if(result.installedWords!=P::CodeWords(0)+119*P::CodeWords(1) || !Guard(io,0))return Fail(io,result);
  installed=true;result.installed=1;return true;
 }
 // NEVER-launched journal only. No public clean/STOP hint is accepted.
 // Launched restoration belongs to the future DISTINCT native token owner.
 template<class IO> bool CloseHeldNoLaunch(IO& io,PioneerQ38TokenMemoryResult& result){
  namespace L=PioneerQ38TokenL1;
  if(!Installed() || !result.installed || result.closed || result.unsafe || result.restoredWords)return false;
  result.phase=5;
  for(unsigned worker=0;worker<120;worker++){
   result.worker=worker;if(!Guard(io,worker) || !VerifyImage(io,worker,result) || !Guard(io,worker))return Fail(io,result);
  }
  result.phase=6;
  for(unsigned worker=0;worker<120;worker++){
   result.worker=worker;if(!Guard(io,worker))return Fail(io,result);
   for(unsigned i=0;i<L::SnapshotWords(worker);i++){
    result.word=i;uint32_t address=L::SnapshotAddress(worker,i),actual;
    if(!io.TokenMemoryWrite(worker,address,saved[L::SnapshotBase(worker)+i])
     || !io.TokenMemoryReadChecked(worker,address,actual) || actual!=saved[L::SnapshotBase(worker)+i])return Fail(io,result);
    result.restoredWords++;
   }
   if(!Guard(io,worker) || !PioneerQ38GraphCsrCode(io,worker))return Fail(io,result);
  }
  result.phase=7;
  for(unsigned worker=0;worker<120;worker++){
   result.worker=worker;if(!Guard(io,worker) || !VerifyOriginal(io,worker,result) || !Guard(io,worker))return Fail(io,result);
  }
  if(result.restoredWords!=L::SavedWords || result.verifiedOriginalWords!=2*L::SavedWords || !Guard(io,0))return Fail(io,result);
  closed=true;installed=false;result.closed=1;return true;
 }
};
static_assert(sizeof(PioneerQ38TokenMemory)>PioneerQ38TokenL1::SavedBytes
 && sizeof(PioneerQ38TokenMemory)<PioneerQ38TokenL1::SavedBytes+16384,"complete token L1 journal HEAP ONLY,not an old owner's storage");
#endif
