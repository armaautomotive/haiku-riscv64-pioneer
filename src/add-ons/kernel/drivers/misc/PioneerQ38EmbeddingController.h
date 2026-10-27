/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
#include "PioneerQ38RecurrentBlockNativeRole.h"
#endif
#ifndef PIONEER_Q38_EMBEDDING_CONTROLLER_H
#define PIONEER_Q38_EMBEDDING_CONTROLLER_H
#include "PioneerQ38ProjectionMutableBanks.h"
#include "PioneerQ38EmbeddingFootprint.h"
#include "PioneerQ38SpanController.h"
// PRIVATE actual QPE1 execution lifecycle,not the HELD-ONLY graph memory
// helper. ALL119 peers stay physically held;save the COMPLETE ROOT before
// FIRST write,run actual learned embedding,prove physical terminal+NoC,
// hold,restore EVERYTHING and only then privately publish its GDDR input.
// Native adapter MUST bind same source/cookie/global lock/clock/ALL hosts.
// No caller clean/completed/epoch hint,public producer setter or CPU math.
struct PioneerQ38EmbeddingResult {
 uint32_t version=1,started=0,phase=0,word=0,unsafe=0,closed=0,completed=0,held=0;
 uint32_t savedWords=0,verifiedWords=0,installedWords=0,restoredWords=0;
 uint32_t before[8]={},after[8]={},summary[32]={},pc[2]={};
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE
 uint32_t validationPages=0,validationWords=0;
#endif
};
class PioneerQ38EmbeddingController {
 friend class PioneerQ38ProjectionGraphOwner;
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
 friend class PioneerQ38RecurrentBlockNativeState;
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
 friend class PioneerQ38MixedLiveNativeState;
#endif
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
 friend class PioneerQ38LearnedRmsController;
#endif
 const tt_dram_samples_result* proof=nullptr;
 const tt_compute_noc_result* noc=nullptr;
 PioneerResidentBanks* banks=nullptr;PioneerQ38ProjectionMutableBanks* graph=nullptr;
 PioneerQ38ProjectionBanks* state=nullptr;const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const uint8_t* learned=nullptr;unsigned learnedPages=0;
 PioneerQ38ProjectionIdentity identity={};PioneerQ38EmbeddingPlan embedding;
 uint32_t saved[73737],originalCommand[10]={},command[10]={},counts[8]={},source[16]={};
 unsigned token=UINT32_MAX,position=UINT32_MAX,bank=7;
 bool attempted=false,started=false,running=false,traffic=false,sampled=false,unsafe=false,closed=false,completed=false;
 static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
 template<class IO> bool Fail(IO& io,PioneerResidentLease& store,PioneerQ38EmbeddingResult& r){
  unsafe=true;completed=false;r.unsafe=1;store.recovery_required=true;
  if(graph)graph->Fault();
  if(banks)banks->Quarantine();
  io.QuarantineHostPage();io.EmbeddingRetainHostOwners();return false; // MEMORY ONLY;NO later IO/hold/retry/END/restore
 }
 template<class IO> static bool PciQuiet(IO& io,unsigned peer){
  if(io.HostPciRead(peer,4,2)!=0x402)return false;
  uint32_t cap=io.HostPciRead(peer,0x34,1);uint64_t visited=0;bool found=false;
  for(unsigned hop=0;cap && hop<48;hop++){
   if(cap<0x40 || cap>0xfc || (cap&3) || (visited&(UINT64_C(1)<<(cap/4))))return false;
   visited|=UINT64_C(1)<<(cap/4);uint32_t header=io.HostPciRead(peer,cap,2);
   if((header&255)==0x10){
    if(found || cap>0xf4)return false;
    uint32_t version=io.HostPciRead(peer,cap+2,2)&15;if(version!=1 && version!=2)return false;
    uint32_t status=io.HostPciRead(peer,cap+0xa,2);if(status>0xffff || (status&0x2f))return false;found=true;
   }
   cap=(header>>8)&255;
  }
  return !cap && found;
 }
 template<class IO> bool Guard(IO& io,PioneerResidentLease& store,uint32_t reset){
  if(!started || unsafe || closed || !proof || !noc || !banks || !graph || !canonical || !state
   || !MatchesNativeOwner(*proof,*noc,*banks,*graph,*canonical,identity,learned,learnedPages)
   || !io.EmbeddingNativeScope(*this,*graph,*canonical,identity,learned,learnedPages)
   || !store.saved || !store.ready || store.pending || store.recovery_required
   || !io.EmbeddingOtherCoresHeld() || !io.SelectRuntimeCore(0) || !io.EmbeddingMemoryMappingStable()
   || io.ResetRead()!=reset || io.ResetRead()!=reset)return false;
  for(unsigned i=0;i<16;i++)if(io.ResidentNocRead(BlackholeDramSourceOffset(i))!=source[i])return false;
  for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)if(io.SourceRead(i)!=proof->source[i])return false;
  for(unsigned peer=0;peer<2;peer++)if(!PciQuiet(io,peer))return false;
  if(traffic)return true; // sole actual ROOT may own active commands/counters;terminal proof below OWED
  for(unsigned i=0;i<4;i++)if(io.ResidentNocRead(0x40+i*0x800))return false;
  for(unsigned i=0;i<32;i++)if(io.ResidentNocRead(0x240+i*4)&255)return false;
  if(sampled){
   for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=command[i])return false;
   for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(Counter(i))!=counts[i])return false;
  }
  return true;
 }
 template<class IO> bool Original(IO& io,PioneerQ38EmbeddingResult& r){
  for(unsigned i=0;i<73737;i++){r.word=i;uint32_t value;
   if(!io.EmbeddingReadChecked(PioneerQ38EmbeddingSnapshotAddress(i),value) || value!=saved[i])return false;
   r.verifiedWords++;}
  return true;
 }
 template<class IO> bool Image(IO& io){
  for(unsigned i=0;i<10241;i++){uint32_t value;
   if(!io.EmbeddingReadChecked(PioneerQ38EmbeddingProgramAddress(i),value) || value!=PioneerQ38EmbeddingProgramWord(i))return false;}
  return true;
 }
 template<class IO> bool Write(IO& io,uint32_t a,uint32_t value){
  uint32_t actual;return io.EmbeddingWrite(a,value) && io.EmbeddingReadChecked(a,actual) && actual==value;
 }
 template<class IO> bool Pc(IO& io,PioneerQ38EmbeddingResult& r){
  for(unsigned i=0;i<2;i++){io.Pause();r.pc[i]=io.RuntimePcRead();if(r.pc[i]!=0x64038 && r.pc[i]!=0x6403c)return false;}return true;
 }
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE
 // DEBUG ONLY. Read back the actual private GDDR input,not the old local
 // computation buffer. All5 fixed pages,poisoned local destination first.
 // Same exclusive source/controller/held-core lifetime as the producer.
 // NO BME/host DMA/new executable/other mutable addresses/CPU arithmetic.
 template<class IO> bool ValidationReadback(IO& io,PioneerResidentLease& store,uint32_t* output,PioneerQ38EmbeddingResult& r){
  if(!output || !state || !state->OwnsEmbedding(identity,this) || running || r.validationPages || r.validationWords)return false;
  const uint32_t* d=embedding.Descriptor();PioneerQ38ProjectionExtent e;
  if(!state->InitializeSpan(identity,0,0,5,e) || e.bank!=bank || e.address!=d[4] || e.route!=d[8] || e.pages!=5 || e.bytes!=20480)return false;
  for(unsigned page=0;page<5;page++){
   if(!Guard(io,store,0x47800))return false;
   for(unsigned w=0;w<1024;w++)if(!Write(io,0x80000+page*4096+w*4,0x7fc10000+page*1024+w))return false;
   const uint32_t next[]={e.address+page*4096,0,e.route,0x80000+page*4096,0,129,0,0,4096,0};
   for(unsigned i=0;i<10;i++){io.ResidentNocWrite(i*4,next[i]);if(io.ResidentNocRead(i*4)!=next[i])return false;command[i]=next[i];}
   if(!Guard(io,store,0x47800))return false;
   uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return false;
   uint32_t expected[8];const unsigned delta[]={1,64,1,1,0,0,1,0};
   for(unsigned i=0;i<8;i++)expected[i]=counts[i]+delta[i];
   traffic=true;io.ResidentNocWrite(0x40,1); // same private traffic lifetime as ROM packets,not legacy lease.pending
   bool done=false;
   for(unsigned poll=0;poll<65536;poll++){
    if(!Guard(io,store,0x47800))return false;
    uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return false;
    bool exact=io.ResidentNocRead(0x40)==0;
    for(unsigned i=0;i<8;i++)exact &= io.ResidentNocRead(Counter(i))==expected[i];
    if(exact){done=true;break;}io.Pause();
   }
   if(!done)return false;
   for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=next[i])return false;
   for(unsigned i=0;i<8;i++)counts[i]=expected[i];
   traffic=false;
   if(!Guard(io,store,0x47800))return false;
   for(unsigned w=0;w<1024;w++){
    uint32_t bits;if(!io.EmbeddingReadChecked(0x80000+page*4096+w*4,bits) || (bits&0x7fffffff)>=0x7f800000)return false;
    output[page*1024+w]=bits;r.validationWords++;
   }
   r.validationPages++;if(!Guard(io,store,0x47800))return false;
  }
  return r.validationPages==5 && r.validationWords==5120;
 }
#endif
 // Opaque retained object identity,not transferable descriptor/epoch DATA.
 // Only the future full projection owner can consume this private proof,
 // after its OWN current source/physical/exclusive lifetime checks.
 bool MatchesCompleted(const PioneerQ38ProjectionMutableBanks& owner,const PioneerQ38CanonicalModelPlan& plan,
  const PioneerQ38ProjectionIdentity& id,unsigned selectedToken,unsigned selectedPosition,unsigned selectedBank)const{
  return Completed() && graph==&owner && canonical==&plan && identity==id && token==selectedToken
   && position==selectedPosition && bank==selectedBank && state && state->OwnsEmbedding(id,this) && state->embeddingPublished;
 }
public:
 PioneerQ38EmbeddingController()=default;
 PioneerQ38EmbeddingController(const PioneerQ38EmbeddingController&)=delete;
 PioneerQ38EmbeddingController& operator=(const PioneerQ38EmbeddingController&)=delete;
 bool Unsafe()const{return unsafe;}
 bool Closed()const{return closed;}
 bool Running()const{return running;}
 bool Completed()const{return completed && closed && !unsafe;} // CPU metadata ONLY,not execution capability
 void Fault(){unsafe=true;completed=false;} // MEMORY ONLY;native retains all maps/snapshots
 bool MatchesNativeOwner(const tt_dram_samples_result& dram,const tt_compute_noc_result& root,
  const PioneerResidentBanks& resident,const PioneerQ38ProjectionMutableBanks& mutableOwner,const PioneerQ38CanonicalModelPlan& plan,
  const PioneerQ38ProjectionIdentity& id,const uint8_t* initialized,unsigned pages)const{
  return started && !unsafe && proof==&dram && noc==&root && banks==&resident && graph==&mutableOwner && canonical==&plan
   && identity==id && learned==initialized && learnedPages==pages && pages==PioneerResidentBanks::Pages && state
   && state->OwnsEmbedding(id,this) && mutableOwner.CanonicalModel(id)==&plan
   && mutableOwner.MatchInitializedEmbedding(id,embedding,token,position,bank,source,1,initialized,pages);
 }
 template<class IO> bool RunOnce(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& root,
  PioneerResidentBanks& resident,PioneerQ38ProjectionMutableBanks& mutableOwner,const PioneerQ38CanonicalModelPlan& plan,
  const PioneerQ38ProjectionIdentity& id,const uint8_t* initialized,unsigned pages,PioneerResidentLease& store,
  unsigned selectedToken,unsigned selectedPosition,unsigned selectedBank,PioneerQ38EmbeddingResult& r
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE
  ,uint32_t* validation=nullptr
#endif
  ){
  if(attempted || started || unsafe || closed || r.version!=1 || r.started || r.phase || r.word || r.unsafe || r.closed || r.completed
   || r.held || r.savedWords || r.verifiedWords || r.installedWords || r.restoredWords || !id.Valid()
   || root.recovery_required || root.version!=1 || root.state!=TT_COMPUTE_NOC_PASSED || !root.tlb_restored
   || !resident.Matches(dram) || mutableOwner.CanonicalModel(id)!=&plan || !initialized || pages!=PioneerResidentBanks::Pages)return false;
  for(auto value:r.before)if(value)return false;
  for(auto value:r.after)if(value)return false;
  for(auto value:r.summary)if(value)return false;
  for(auto value:r.pc)if(value)return false;
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE
  if(r.validationPages || r.validationWords)return false;
#endif
  attempted=true;
  for(unsigned i=0;i<16;i++)source[i]=i?root.source[0][i]:store.config|0x10000;
  if(!embedding.Build(plan,selectedToken,selectedPosition,selectedBank,id,source,1)
   || !mutableOwner.MatchInitializedEmbedding(id,embedding,selectedToken,selectedPosition,selectedBank,source,1,initialized,pages))return false;
  state=mutableOwner.Projection(id);if(!state || !state->ClaimEmbedding(id,this,selectedToken,selectedPosition))return false;
  started=true;proof=&dram;noc=&root;banks=&resident;graph=&mutableOwner;canonical=&plan;identity=id;
  learned=initialized;learnedPages=pages;token=selectedToken;position=selectedPosition;bank=selectedBank;r.started=1;
  auto fail=[&](){return Fail(io,store,r);};
  r.phase=1;if(!Guard(io,store,0x47800))return fail();
  for(unsigned i=0;i<10;i++)originalCommand[i]=command[i]=io.ResidentNocRead(i*4);
  for(unsigned i=0;i<8;i++){counts[i]=r.before[i]=io.ResidentNocRead(Counter(i));if(i==6 && counts[2]!=counts[6])return fail();}
  sampled=true;
  for(unsigned i=0;i<73737;i++){r.word=i;uint32_t value;
   if(!io.EmbeddingReadChecked(PioneerQ38EmbeddingSnapshotAddress(i),value))return fail();
   saved[i]=value;r.savedWords++;}
  if(!Guard(io,store,0x47800))return fail();
  r.phase=2;if(!Original(io,r) || !Guard(io,store,0x47800))return fail(); // ALL original words SECOND pass before FIRST write
  bool oldDirty=store.l1_dirty,oldRestored=store.l1_restored;store.l1_dirty=true;store.l1_restored=false;
  r.phase=3;
  for(unsigned i=0;i<10241;i++){r.word=i;if(!Write(io,PioneerQ38EmbeddingProgramAddress(i),PioneerQ38EmbeddingProgramWord(i)))return fail();r.installedWords++;}
  const uint32_t* d=embedding.Descriptor();
  for(unsigned i=0;i<64;i++)if(!Write(io,TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4,d[i]))return fail();
  for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);if(io.RuntimeStatusRead(i))return fail();}
  if(!Image(io) || !Guard(io,store,0x47800))return fail();
  r.phase=4;running=true;io.RuntimeResetWrite(0x87ffff);io.Pause();if(!Guard(io,store,0x87ffff))return fail();
  uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return fail();
  traffic=true;io.RuntimeResetWrite(0x46800);
  bool done=false;
  for(unsigned poll=0;poll<65536;poll++){
   if(!Guard(io,store,0x46800))return fail();
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return fail();
   uint32_t marker=io.RuntimeStatusRead(2);if(marker && marker!=TT_Q38_EMBEDDING_MARKER)return fail();
   if(marker){
    if(io.RuntimeStatusRead(0)!=0x6400c || io.RuntimeStatusRead(1)!=5120 || !Pc(io,r))return fail();
    done=true;break;
   }
   io.Pause();uint32_t pc=io.RuntimePcRead();
   if((pc&3) || pc<0x64000 || pc>=0x64000+sizeof(PioneerQ38EmbeddingCode) || (pc>=0x6403c && pc<=0x64058))return fail();
  }
  if(!done)return fail();
  r.phase=5;if(!Guard(io,store,0x46800) || !Image(io))return fail();
  for(unsigned i=0;i<64;i++){uint32_t value;if(!io.EmbeddingReadChecked(TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4,value) || value!=d[i])return fail();}
  for(unsigned i=0;i<32;i++)if(!io.EmbeddingReadChecked(TT_Q38_FFN_RESIDENT_SUMMARY+i*4,r.summary[i]))return fail();
  for(unsigned i=0;i<8;i++)r.after[i]=io.ResidentNocRead(Counter(i));
  for(unsigned i=0;i<8;i++)if(r.before[i]!=counts[i])return fail();
  if(!PioneerQ38EmbeddingSummary(r.summary,d,counts,r.after))return fail();
  const uint32_t last[]={0x84000,0,129,d[4]+16384,0,d[8],0,0x212,4096,0};
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=last[i])return fail();
  for(unsigned i=0;i<8;i++)counts[i]=r.after[i];
  for(unsigned i=0;i<10;i++)command[i]=last[i];
  traffic=false;
  if(!Guard(io,store,0x46800) || !Pc(io,r))return fail(); // ALL completion/source/image proofs BEFORE FIRST hold
  io.RuntimeResetWrite(0x87ffff);io.Pause();if(!Guard(io,store,0x87ffff))return fail();
  io.RuntimeResetWrite(0x47800);io.Pause();if(!Guard(io,store,0x47800))return fail();running=false;r.held=1;
  r.phase=6;if(!Image(io) || !Guard(io,store,0x47800))return fail();
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE
  if(validation && !ValidationReadback(io,store,validation,r))return fail();
#endif
  for(unsigned i=0;i<73737;i++){r.word=i;if(!Write(io,PioneerQ38EmbeddingSnapshotAddress(i),saved[i]))return fail();r.restoredWords++;}
  for(unsigned i=0;i<10;i++){io.ResidentNocWrite(i*4,originalCommand[i]);if(io.ResidentNocRead(i*4)!=originalCommand[i])return fail();command[i]=originalCommand[i];}
  r.phase=7;if(!Original(io,r) || !Guard(io,store,0x47800) || r.savedWords!=73737 || r.verifiedWords!=147474
   || r.installedWords!=10241 || r.restoredWords!=73737 || !state->PublishEmbedding(id,this,token,position))return fail();
  store.l1_dirty=oldDirty;store.l1_restored=oldRestored;completed=closed=true;r.completed=r.closed=1;return true;
 }
};
static_assert(sizeof(PioneerQ38EmbeddingController)>294948 && sizeof(PioneerQ38EmbeddingController)<303104,"complete embedding execution owner HEAP ONLY");
#endif
