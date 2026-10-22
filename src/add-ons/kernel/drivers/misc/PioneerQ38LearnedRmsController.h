/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_LEARNED_RMS_CONTROLLER_H
#define PIONEER_Q38_LEARNED_RMS_CONTROLLER_H
#if !defined(PIONEER_Q38_LEARNED_RMS_NATIVE) || PIONEER_Q38_LEARNED_RMS_NATIVE != 1 || !defined(PIONEER_Q38_EMBEDDING_NATIVE) || PIONEER_Q38_EMBEDDING_NATIVE != 1 || defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE)
#error Private no-reference learned RMS execution owner ONLY,not old/embedding-validation promotion
#endif
#include "PioneerQ38EmbeddingController.h"
#include "PioneerQ38LearnedRmsPlan.h"
#include "PioneerQ38LearnedRmsCode.h"
// First hidden norm ONLY:layer0/role0,consuming exact completed embedding.
// Further layers require actual residual/input producers,not caller hints.
struct PioneerQ38LearnedRmsResult {
 uint32_t version=1,started=0,phase=0,word=0,unsafe=0,closed=0,completed=0,held=0;
 uint32_t savedWords=0,verifiedWords=0,installedWords=0,restoredWords=0;
 uint32_t before[8]={},after[8]={},summary[32]={},pc[2]={};
};
class PioneerQ38LearnedRmsController {
 friend class PioneerQ38ProjectionGraphOwner;
 const tt_dram_samples_result* proof=nullptr;const tt_compute_noc_result* noc=nullptr;
 PioneerResidentBanks* banks=nullptr;PioneerQ38ProjectionMutableBanks* graph=nullptr;PioneerQ38ProjectionBanks* state=nullptr;
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;const PioneerQ38EmbeddingController* producer=nullptr;
 const uint8_t* learned=nullptr;unsigned learnedPages=0;
 PioneerQ38ProjectionIdentity identity={};PioneerQ38LearnedRmsPlan rms;
 uint32_t saved[73737],originalCommand[10]={},command[10]={},counts[8]={},source[16]={};
 unsigned token=UINT32_MAX,position=UINT32_MAX,bank=7;
 bool attempted=false,started=false,running=false,traffic=false,sampled=false,unsafe=false,closed=false,completed=false;
 static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
 template<class IO> bool Fail(IO& io,PioneerResidentLease& store,PioneerQ38LearnedRmsResult& r){
  unsafe=true;completed=false;r.unsafe=1;store.recovery_required=true;
  if(graph)graph->Fault();
  if(banks)banks->Quarantine();
  io.QuarantineHostPage();io.EmbeddingRetainHostOwners();return false; // MEMORY ONLY;NO subsequent I/O/hold/END/restore
 }
 template<class IO> bool Guard(IO& io,PioneerResidentLease& store,uint32_t reset){
  if(!started || unsafe || closed || !proof || !noc || !banks || !graph || !canonical || !state || !producer
   || !MatchesNativeOwner(*proof,*noc,*banks,*graph,*canonical,*producer,identity,learned,learnedPages)
   || !io.LearnedRmsNativeScope(*this,*graph,*canonical,*producer,identity,learned,learnedPages)
   || !store.saved || !store.ready || store.pending || store.recovery_required
   || !io.LearnedRmsOtherCoresHeld() || !io.SelectRuntimeCore(0) || !io.EmbeddingMemoryMappingStable()
   || io.ResetRead()!=reset || io.ResetRead()!=reset)return false;
  for(unsigned i=0;i<16;i++)if(io.ResidentNocRead(BlackholeDramSourceOffset(i))!=source[i])return false;
  for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)if(io.SourceRead(i)!=proof->source[i])return false;
  for(unsigned p=0;p<2;p++)if(!PioneerQ38EmbeddingController::PciQuiet(io,p))return false;
  if(traffic)return true;
  for(unsigned i=0;i<4;i++)if(io.ResidentNocRead(0x40+i*0x800))return false;
  for(unsigned i=0;i<32;i++)if(io.ResidentNocRead(0x240+i*4)&255)return false;
  if(sampled){for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=command[i])return false;
   for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(Counter(i))!=counts[i])return false;}
  return true;
 }
 template<class IO> bool Original(IO& io,PioneerQ38LearnedRmsResult& r){
  for(unsigned i=0;i<73737;i++){r.word=i;uint32_t value;
   if(!io.EmbeddingReadChecked(PioneerQ38EmbeddingSnapshotAddress(i),value) || value!=saved[i])return false;
   r.verifiedWords++;}return true;
 }
 template<class IO> bool Image(IO& io){
  for(unsigned i=0;i<10241;i++){uint32_t value;
   if(!io.EmbeddingReadChecked(PioneerQ38EmbeddingProgramAddress(i),value) || value!=PioneerQ38LearnedRmsProgramWord(i))return false;}return true;
 }
 template<class IO> bool Write(IO& io,uint32_t a,uint32_t value){uint32_t actual;return io.EmbeddingWrite(a,value) && io.EmbeddingReadChecked(a,actual) && actual==value;}
 template<class IO> bool Pc(IO& io,PioneerQ38LearnedRmsResult& r){
  for(unsigned i=0;i<2;i++){io.Pause();r.pc[i]=io.RuntimePcRead();if(r.pc[i]!=0x64038 && r.pc[i]!=0x6403c)return false;}return true;
 }
 bool MatchesCompleted(const PioneerQ38ProjectionMutableBanks& owner,const PioneerQ38CanonicalModelPlan& plan,
  const PioneerQ38ProjectionIdentity& id,unsigned selectedToken,unsigned selectedPosition,unsigned selectedBank)const{
  return Completed() && graph==&owner && canonical==&plan && identity==id && token==selectedToken && position==selectedPosition && bank==selectedBank
   && state && state->OwnsLearnedRms(id,this,producer) && state->rmsPublished;
 }
public:
 PioneerQ38LearnedRmsController()=default;
 PioneerQ38LearnedRmsController(const PioneerQ38LearnedRmsController&)=delete;
 PioneerQ38LearnedRmsController& operator=(const PioneerQ38LearnedRmsController&)=delete;
 bool Unsafe()const{return unsafe;}bool Closed()const{return closed;}bool Running()const{return running;}
 bool Claimed()const{return started;}bool Completed()const{return completed && closed && !unsafe;} // metadata,NOT an input grant
 void Fault(){unsafe=true;completed=false;} // MEMORY ONLY
 bool MatchesNativeOwner(const tt_dram_samples_result& dram,const tt_compute_noc_result& root,const PioneerResidentBanks& resident,
  const PioneerQ38ProjectionMutableBanks& mutableOwner,const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38EmbeddingController& input,
  const PioneerQ38ProjectionIdentity& id,const uint8_t* initialized,unsigned pages)const{
  return started && !unsafe && proof==&dram && noc==&root && banks==&resident && graph==&mutableOwner && canonical==&plan && producer==&input
   && identity==id && learned==initialized && learnedPages==pages && pages==PioneerResidentBanks::Pages && state
   && state->OwnsLearnedRms(id,this,&input) && input.Completed()
   && input.MatchesNativeOwner(dram,root,resident,mutableOwner,plan,id,initialized,pages)
   && rms.Matches(plan,0,0,position,bank,id,source,2) && rms.LearnedPagesPresent(initialized,pages);
 }
 template<class IO> bool RunOnce(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& root,
  PioneerResidentBanks& resident,PioneerQ38ProjectionMutableBanks& mutableOwner,const PioneerQ38CanonicalModelPlan& plan,
  const PioneerQ38EmbeddingController& input,const PioneerQ38ProjectionIdentity& id,const uint8_t* initialized,unsigned pages,PioneerResidentLease& store,
  unsigned selectedToken,unsigned selectedPosition,unsigned selectedBank,PioneerQ38LearnedRmsResult& r){
  if(attempted || started || unsafe || closed || r.version!=1 || r.started || r.phase || r.word || r.unsafe || r.closed || r.completed || r.held
   || r.savedWords || r.verifiedWords || r.installedWords || r.restoredWords || !id.Valid() || !resident.Matches(dram)
   || root.version!=1 || root.state!=TT_COMPUTE_NOC_PASSED || !root.tlb_restored || root.recovery_required
   || !initialized || pages!=PioneerResidentBanks::Pages || !input.MatchesCompleted(mutableOwner,plan,id,selectedToken,selectedPosition,selectedBank))return false;
  for(auto value:r.before)if(value)return false;
  for(auto value:r.after)if(value)return false;
  for(auto value:r.summary)if(value)return false;
  for(auto value:r.pc)if(value)return false;
  attempted=true;for(unsigned i=0;i<16;i++)source[i]=i?root.source[0][i]:store.config|0x10000;
  if(!rms.Build(plan,0,0,selectedPosition,selectedBank,id,source,2) || !rms.LearnedPagesPresent(initialized,pages))return false;
  state=mutableOwner.Projection(id);
  if(!state || !state->ClaimLearnedRms(id,this,&input,selectedToken,selectedPosition))return false;
  started=true;proof=&dram;noc=&root;banks=&resident;graph=&mutableOwner;canonical=&plan;producer=&input;identity=id;
  learned=initialized;learnedPages=pages;token=selectedToken;position=selectedPosition;bank=selectedBank;r.started=1;
  auto fail=[&](){return Fail(io,store,r);};
  r.phase=1;if(!Guard(io,store,0x47800))return fail();
  for(unsigned i=0;i<10;i++)originalCommand[i]=command[i]=io.ResidentNocRead(i*4);
  for(unsigned i=0;i<8;i++){counts[i]=r.before[i]=io.ResidentNocRead(Counter(i));if(i==6 && counts[2]!=counts[6])return fail();}
  sampled=true;
  for(unsigned i=0;i<73737;i++){r.word=i;uint32_t value;if(!io.EmbeddingReadChecked(PioneerQ38EmbeddingSnapshotAddress(i),value))return fail();saved[i]=value;r.savedWords++;}
  r.phase=2;if(!Guard(io,store,0x47800) || !Original(io,r) || !Guard(io,store,0x47800))return fail();
  bool oldDirty=store.l1_dirty,oldRestored=store.l1_restored;store.l1_dirty=true;store.l1_restored=false;r.phase=3;
  for(unsigned i=0;i<10241;i++){r.word=i;if(!Write(io,PioneerQ38EmbeddingProgramAddress(i),PioneerQ38LearnedRmsProgramWord(i)))return fail();r.installedWords++;}
  const uint32_t* d=rms.Descriptor();for(unsigned i=0;i<64;i++)if(!Write(io,TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4,d[i]))return fail();
  for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);if(io.RuntimeStatusRead(i))return fail();}
  if(!Image(io) || !Guard(io,store,0x47800))return fail();
  r.phase=4;running=true;io.RuntimeResetWrite(0x87ffff);io.Pause();if(!Guard(io,store,0x87ffff))return fail();
  uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return fail();traffic=true;io.RuntimeResetWrite(0x46800);
  bool done=false;
  for(unsigned poll=0;poll<65536;poll++){
   if(!Guard(io,store,0x46800))return fail();
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return fail();
   uint32_t marker=io.RuntimeStatusRead(2);if(marker && marker!=TT_Q38_RMS_MARKER)return fail();
   if(marker){if(io.RuntimeStatusRead(0)!=0x6400c || io.RuntimeStatusRead(1)!=5120 || !Pc(io,r))return fail();done=true;break;}
   io.Pause();uint32_t pc=io.RuntimePcRead();if((pc&3) || pc<0x64000 || pc>=0x64000+sizeof(PioneerQ38LearnedRmsCode) || (pc>=0x6403c && pc<=0x64058))return fail();
  }
  if(!done)return fail();
  r.phase=5;if(!Guard(io,store,0x46800) || !Image(io))return fail();
  for(unsigned i=0;i<64;i++){uint32_t value;if(!io.EmbeddingReadChecked(TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4,value) || value!=d[i])return fail();}
  for(unsigned i=0;i<32;i++)if(!io.EmbeddingReadChecked(TT_Q38_FFN_RESIDENT_SUMMARY+i*4,r.summary[i]))return fail();
  for(unsigned i=0;i<8;i++)r.after[i]=io.ResidentNocRead(Counter(i));
  for(unsigned i=0;i<8;i++)if(r.before[i]!=counts[i])return fail();
  if(!PioneerQ38LearnedRmsSummary(r.summary,d,counts,r.after))return fail();
  const uint32_t last[]={0x94000,0,129,d[4]+16384,0,d[8],0,0x212,4096,0};
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=last[i])return fail();
  for(unsigned i=0;i<8;i++)counts[i]=r.after[i];
  for(unsigned i=0;i<10;i++)command[i]=last[i];
  traffic=false;
  if(!Guard(io,store,0x46800) || !Pc(io,r))return fail();
  io.RuntimeResetWrite(0x87ffff);io.Pause();if(!Guard(io,store,0x87ffff))return fail();
  io.RuntimeResetWrite(0x47800);io.Pause();if(!Guard(io,store,0x47800))return fail();running=false;r.held=1;r.phase=6;
  if(!Image(io) || !Guard(io,store,0x47800))return fail();
  for(unsigned i=0;i<73737;i++){r.word=i;if(!Write(io,PioneerQ38EmbeddingSnapshotAddress(i),saved[i]))return fail();r.restoredWords++;}
  for(unsigned i=0;i<10;i++){io.ResidentNocWrite(i*4,originalCommand[i]);if(io.ResidentNocRead(i*4)!=originalCommand[i])return fail();command[i]=originalCommand[i];}
  r.phase=7;if(!Original(io,r) || !Guard(io,store,0x47800) || r.savedWords!=73737 || r.verifiedWords!=147474 || r.installedWords!=10241
   || r.restoredWords!=73737 || !state->PublishLearnedRms(id,this,&input,token,position))return fail();
  store.l1_dirty=oldDirty;store.l1_restored=oldRestored;completed=closed=true;r.completed=r.closed=1;return true;
 }
};
static_assert(sizeof(PioneerQ38LearnedRmsController)>294948 && sizeof(PioneerQ38LearnedRmsController)<303104,"full ROOT RMS execution snapshot HEAP ONLY");
#endif
