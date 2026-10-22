/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_GRAPH_CODE_OWNER_H
#define PIONEER_Q38_FFN_GRAPH_CODE_OWNER_H
#include "PioneerQ38FfnGraphImages.h"
#include "PioneerQ38GraphCsrProbe.h"
// Exact role-specific program footprints. NO stack,manifest,output,NIU or
// caller address is part of this mapping/write policy.
static inline unsigned PioneerQ38FfnGraphCodeWords(unsigned w)
{return w<120?1+PioneerQ38FfnGraphPrefixBytes(w)/4+4096:0;}
static inline unsigned PioneerQ38FfnGraphCodeBase(unsigned w)
{return w<120?(w?14337+(w-1)*7169:0):UINT32_MAX;}
static inline uint32_t PioneerQ38FfnGraphCodeAddress(unsigned w,unsigned i)
{
 unsigned prefix=PioneerQ38FfnGraphPrefixBytes(w)/4;
 if(w>=120 || i>=PioneerQ38FfnGraphCodeWords(w))return UINT32_MAX;
 return !i?0x6000:i<=prefix?0x64000+(i-1)*4:0x70000+(i-prefix-1)*4;
}
struct PioneerQ38FfnGraphCodeResult {
 uint32_t version=1,started=0,passed=0,unsafe=0,phase=0,worker=0,word=0;
 uint32_t savedWords=0,installedWords=0,restoredWords=0;
};
class PioneerQ38FfnGraphCodeOwner;
template<class IO> bool PioneerQ38FfnGraphCodeRoundTrip(IO&,const tt_dram_samples_result&,
 PioneerResidentBanks&,PioneerQ38DispatchLease&,PioneerQ38FfnGraphCodeOwner&,PioneerQ38FfnGraphCodeResult&);
class PioneerQ38FfnGraphCodeOwner {
 template<class IO> friend bool PioneerQ38FfnGraphCodeRoundTrip(IO&,const tt_dram_samples_result&,
  PioneerResidentBanks&,PioneerQ38DispatchLease&,PioneerQ38FfnGraphCodeOwner&,PioneerQ38FfnGraphCodeResult&);
 const tt_dram_samples_result* proof=nullptr;
 PioneerResidentBanks* banks=nullptr;
 PioneerQ38DispatchLease* cohort=nullptr;
 bool started=false,unsafe=false,closed=false;
 unsigned sampled=0;
 uint32_t command[120][10]={};
 uint32_t counters[120][8]={}; // private physical MASTER baselines,not proof data
 uint32_t saved[867448]={}; // HEAP ONLY;ALL120 boots+panels+exact role prefixes
public:
 static constexpr unsigned Words=867448;
 PioneerQ38FfnGraphCodeOwner()=default;
 PioneerQ38FfnGraphCodeOwner(const PioneerQ38FfnGraphCodeOwner&)=delete;
 PioneerQ38FfnGraphCodeOwner& operator=(const PioneerQ38FfnGraphCodeOwner&)=delete;
 bool Unsafe()const{return unsafe;}
 bool Closed()const{return closed;}
};
static_assert(14337+119*7169==PioneerQ38FfnGraphCodeOwner::Words,"complete saved footprints");
// Private held-only install/readback/restore qualification,NOT a launch
// capability. Native caller MUST be the EXISTING cookie/global-lock owner
// before original bootstrap,excluding ALL other device writers. No new code
// ever runs here. There is deliberately NO start/executed/clean hint API;
// future FFN execution requires a separate actual producer/terminal owner.
// Keep this object,its mapping and the ENTIRE existing context on failure.
template<class IO> bool PioneerQ38FfnGraphCodeRoundTrip(IO& io,const tt_dram_samples_result& dram,
 PioneerResidentBanks& banks,PioneerQ38DispatchLease& cohort,PioneerQ38FfnGraphCodeOwner& owner,
 PioneerQ38FfnGraphCodeResult& r)
{
 if(owner.started || owner.unsafe || owner.closed || owner.proof || owner.banks || owner.cohort
  || r.version!=1 || r.started || r.passed || r.unsafe || r.phase || r.worker || r.word
  || r.savedWords || r.installedWords || r.restoredWords || cohort.unsafe || cohort.closed || cohort.rootRunning
  || cohort.prepared!=120 || cohort.bootAttempted || cohort.booted || !banks.Matches(dram))return false;
 owner.started=true;owner.proof=&dram;owner.banks=&banks;owner.cohort=&cohort;r.started=1;
 auto fail=[&](){owner.unsafe=true;r.unsafe=1;PioneerQ38DispatchQuarantine(cohort,banks);io.QuarantineHostPage();return false;};
 auto guard=[&](unsigned w){if(!(owner.proof==&dram && owner.banks==&banks && owner.cohort==&cohort
  && !owner.unsafe && !owner.closed && !cohort.bootAttempted && !cohort.booted
  && PioneerQ38GraphCsrGuard(io,dram,banks,cohort,w) && io.FfnGraphCodeMappingStable(w)))return false;
  // This owner emits NO NoC transaction. Any selected MASTER counter
  // change therefore invalidates the exclusive held-state proof.
  if(w<owner.sampled){
   for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=owner.command[w][i])return false;
   for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=owner.counters[w][i])return false;
  }
  return true;};
 // Save ALL participants BEFORE the FIRST image write. Old prepared code
 // is independently checked;the prefix's arbitrary original bytes are
 // never inferred from zeros or reconstructed from a different image.
 r.phase=1;
 for(unsigned w=0;w<120;w++){
  r.worker=w;if(!guard(w) || !PioneerQ38GraphCsrCode(io,w))return fail();
  for(unsigned i=0;i<10;i++)owner.command[w][i]=io.ResidentNocRead(i*4);
  for(unsigned i=0;i<8;i++){
   owner.counters[w][i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
   if(i==6 && owner.counters[w][2]!=owner.counters[w][6])return fail();
  }
  owner.sampled=w+1;
  const unsigned base=PioneerQ38FfnGraphCodeBase(w),words=PioneerQ38FfnGraphCodeWords(w);
  for(unsigned i=0;i<words;i++){r.word=i;owner.saved[base+i]=io.FfnGraphProgramRead(w,PioneerQ38FfnGraphCodeAddress(w,i));r.savedWords++;}
  if(!guard(w))return fail();
 }
 if(r.savedWords!=owner.Words || owner.sampled!=120)return fail();
 r.phase=2;
 for(unsigned w=0;w<120;w++){
  r.worker=w;if(!guard(w))return fail();
  const unsigned base=PioneerQ38FfnGraphCodeBase(w),words=PioneerQ38FfnGraphCodeWords(w);
  // Catch a changed saved footprint BEFORE writing this participant.
  for(unsigned i=0;i<words;i++){r.word=i;if(io.FfnGraphProgramRead(w,PioneerQ38FfnGraphCodeAddress(w,i))!=owner.saved[base+i])return fail();}
  for(unsigned i=0;i<words;i++){
   r.word=i;const uint32_t a=PioneerQ38FfnGraphCodeAddress(w,i),value=PioneerQ38FfnGraphProgramWord(w,a);
   if(!io.FfnGraphProgramWrite(w,a,value) || io.FfnGraphProgramRead(w,a)!=value)return fail();
   r.installedWords++;
  }
  if(!guard(w))return fail();
 }
 // Complete independent image proof BEFORE any restoration. Checking ALL
 // words (including peer scratch) is correct ONLY because nothing launched.
 r.phase=3;
 for(unsigned w=0;w<120;w++){
  r.worker=w;if(!guard(w))return fail();
  for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++){r.word=i;uint32_t a=PioneerQ38FfnGraphCodeAddress(w,i);
   if(io.FfnGraphProgramRead(w,a)!=PioneerQ38FfnGraphProgramWord(w,a))return fail();}
 }
 r.phase=4;
 for(unsigned w=0;w<120;w++){
  r.worker=w;if(!guard(w))return fail();
  const unsigned base=PioneerQ38FfnGraphCodeBase(w),words=PioneerQ38FfnGraphCodeWords(w);
  for(unsigned i=0;i<words;i++){r.word=i;uint32_t a=PioneerQ38FfnGraphCodeAddress(w,i),value=owner.saved[base+i];
   if(!io.FfnGraphProgramWrite(w,a,value) || io.FfnGraphProgramRead(w,a)!=value)return fail();
   r.restoredWords++;}
  if(!guard(w) || !PioneerQ38GraphCsrCode(io,w))return fail();
 }
 r.phase=5;
 for(unsigned w=0;w<120;w++){
  r.worker=w;if(!guard(w))return fail();
  const unsigned base=PioneerQ38FfnGraphCodeBase(w),words=PioneerQ38FfnGraphCodeWords(w);
  for(unsigned i=0;i<words;i++){r.word=i;if(io.FfnGraphProgramRead(w,PioneerQ38FfnGraphCodeAddress(w,i))!=owner.saved[base+i])return fail();}
 }
 if(r.installedWords!=owner.Words || r.restoredWords!=owner.Words || !guard(0))return fail();
 owner.closed=true;r.passed=1;return true;
}
#endif
