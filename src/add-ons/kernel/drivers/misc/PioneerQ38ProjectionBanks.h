/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_BANKS_H
#define PIONEER_Q38_PROJECTION_BANKS_H
#ifndef PIONEER_Q38_GDN_MUTABLE_STATE
#error Complete projections require the single-owner mutable reservation interlock
#endif
#include "PioneerResidentBanks.h"
#include "PioneerQ38GdnIdentity.h"
#include "PioneerQ38ProjectionMatrixShape.h"
using PioneerQ38ProjectionIdentity=PioneerQ38GdnIdentity;
struct PioneerQ38ProjectionExtent {uint32_t bank,address,route,index,pages,bytes;};
struct PioneerResidentLease;struct PioneerBulkUploadLease;struct PioneerOutputBufferLease;struct PioneerWideOutputLease;
struct PioneerQ38ProjectionZeroLease;struct PioneerQ38ProjectionZeroResult;
class PioneerQ38EmbeddingController;
class PioneerQ38ProjectionGraphOwner;
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
class PioneerQ38LearnedRmsController;
#endif
class PioneerQ38ProjectionBanks {
 friend class PioneerQ38ProjectionMutableBanks;
 friend class PioneerQ38EmbeddingController;
 friend class PioneerQ38ProjectionGraphOwner;
#ifdef PIONEER_Q38_RESIDENT_DECODER_OWNER
 friend class PioneerQ38ResidentDecoderOwner;
#endif
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
 friend class PioneerQ38LearnedRmsController;
#endif
 template<class IO> friend bool PioneerQ38ProjectionZeroPrepare(IO&,const tt_dram_samples_result&,const tt_compute_noc_result&,
  PioneerQ38ProjectionBanks&,PioneerResidentBanks&,const PioneerQ38ProjectionIdentity&,const tt_host_page_result&,
  PioneerResidentLease&,PioneerBulkUploadLease&,PioneerOutputBufferLease&,PioneerWideOutputLease&,PioneerQ38ProjectionZeroLease&);
 template<class IO> friend bool PioneerQ38ProjectionZeroRun(IO&,const tt_dram_samples_result&,const tt_compute_noc_result&,
  PioneerQ38ProjectionBanks&,PioneerResidentBanks&,const tt_host_page_result&,PioneerResidentLease&,PioneerBulkUploadLease&,
  PioneerOutputBufferLease&,PioneerWideOutputLease&,PioneerQ38ProjectionZeroLease&,const PioneerQ38ProjectionIdentity&,
  unsigned,unsigned,unsigned,PioneerQ38ProjectionZeroResult&);
 template<class IO> friend bool PioneerQ38ProjectionZeroClose(IO&,const tt_dram_samples_result&,const tt_compute_noc_result&,
  PioneerQ38ProjectionBanks&,PioneerResidentBanks&,const tt_host_page_result&,PioneerResidentLease&,PioneerBulkUploadLease&,
  PioneerOutputBufferLease&,PioneerWideOutputLease&,PioneerQ38ProjectionZeroLease&,bool);
 PioneerResidentBanks* banks=nullptr;
 const tt_dram_samples_result* baseline=nullptr;
 const void* reservationOwner=nullptr;
 PioneerQ38ProjectionIdentity identity={};
 const PioneerQ38ProjectionZeroLease* initializationLease=nullptr;
 uint32_t bank=7,offset=0;
 uint64_t initialized[8]={};
 bool unsafe=false,published=false;
 const PioneerQ38EmbeddingController* embeddingOwner=nullptr;
 uint32_t embeddingToken=UINT32_MAX,embeddingPosition=UINT32_MAX;
 bool embeddingPublished=false;
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
 const PioneerQ38LearnedRmsController* rmsOwner=nullptr;
 bool rmsPublished=false;
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
 const PioneerQ38ProjectionGraphOwner* executionOwner=nullptr;
 bool executionClosed=false;
 bool ClaimExecution(const PioneerQ38ProjectionIdentity& id,const PioneerQ38ProjectionGraphOwner* owner,const PioneerQ38LearnedRmsController* input){
  if(!owner || executionOwner || executionClosed || !rmsPublished || !OwnsLearnedRms(id,input,embeddingOwner))return false;
  executionOwner=owner;return true;
 }
 bool OwnsExecution(const PioneerQ38ProjectionIdentity& id,const PioneerQ38ProjectionGraphOwner* owner,const PioneerQ38LearnedRmsController* input)const{
  return owner && executionOwner==owner && !executionClosed && rmsPublished && OwnsLearnedRms(id,input,embeddingOwner);
 }
 bool CloseExecution(const PioneerQ38ProjectionIdentity& id,const PioneerQ38ProjectionGraphOwner* owner,const PioneerQ38LearnedRmsController* input){
  if(!OwnsExecution(id,owner,input))return false;
  executionClosed=true;return true; // retain exact owner identity;no second claimant or ready facade
 }
#endif
 bool ClaimLearnedRms(const PioneerQ38ProjectionIdentity& id,const PioneerQ38LearnedRmsController* owner,
  const PioneerQ38EmbeddingController* input,unsigned token,unsigned position){
  if(!owner || rmsOwner || !embeddingPublished || !OwnsEmbedding(id,input) || token!=embeddingToken || position!=embeddingPosition)return false;
  rmsOwner=owner;embeddingPublished=false;return true; // consume raw proof BEFORE any possible input mutation
 }
 bool OwnsLearnedRms(const PioneerQ38ProjectionIdentity& id,const PioneerQ38LearnedRmsController* owner,const PioneerQ38EmbeddingController* input)const{
  return owner && rmsOwner==owner && !embeddingPublished && OwnsEmbedding(id,input);
 }
 bool PublishLearnedRms(const PioneerQ38ProjectionIdentity& id,const PioneerQ38LearnedRmsController* owner,
  const PioneerQ38EmbeddingController* input,unsigned token,unsigned position){
  if(rmsPublished || !OwnsLearnedRms(id,owner,input) || token!=embeddingToken || position!=embeddingPosition)return false;
  rmsPublished=true;return true; // only exact RMS execution owner's verified physical restore CLOSE
 }
#endif
 bool Matches(const PioneerQ38ProjectionIdentity& id)const{
  return banks && baseline && reservationOwner && !unsafe && id==identity && banks->Matches(*baseline);
 }
 bool Recorded(unsigned page)const{return (initialized[page/64]>>(page%64))&1;}
 bool ClaimEmbedding(const PioneerQ38ProjectionIdentity& id,const PioneerQ38EmbeddingController* owner,unsigned token,unsigned position){
  if(!owner || embeddingOwner || !InitializationPublished(id) || initializationLease || token>=248320 || position>=8192)return false;
  embeddingOwner=owner;embeddingToken=token;embeddingPosition=position;return true;
 }
 bool OwnsEmbedding(const PioneerQ38ProjectionIdentity& id,const PioneerQ38EmbeddingController* owner)const{
  return owner && embeddingOwner==owner && InitializationPublished(id) && !initializationLease;
 }
 bool PublishEmbedding(const PioneerQ38ProjectionIdentity& id,const PioneerQ38EmbeddingController* owner,unsigned token,unsigned position){
  if(embeddingPublished || !OwnsEmbedding(id,owner) || embeddingToken!=token || embeddingPosition!=position)return false;
  embeddingPublished=true;return true; // ONLY exact controller's known-clean physical restore close
 }
 bool ClaimInitialization(const PioneerQ38ProjectionIdentity& id,const PioneerQ38ProjectionZeroLease* lease){
  if(!lease || initializationLease || published || !Matches(id))return false;
  initializationLease=lease;return true;
 }
 bool CloseInitialization(const PioneerQ38ProjectionIdentity& id,const PioneerQ38ProjectionZeroLease* lease){
  if(!lease || initializationLease!=lease || !Matches(id))return false;
  initializationLease=nullptr;return true; // ONLY after known-clean exact code/scratch/source restore
 }
 bool CommitInitialization(const PioneerQ38ProjectionIdentity& id,unsigned region,unsigned first,unsigned count){
  PioneerQ38ProjectionExtent e;if(!FreshInitializationSpan(id,region,first,count,e))return false;
  for(unsigned p=0;p<count;p++)initialized[(first+p)/64]|=UINT64_C(1)<<((first+p)%64);
  return true; // controller-private,AFTER independent physical completion/hold/source proofs
 }
 bool PublishInitialization(const PioneerQ38ProjectionIdentity& id){
  unsigned pages=0;if(published || !InitializationProgress(id,pages) || pages!=Pages)return false;
  published=true;return true; // ONLY complete known-clean zero CLOSE,NOT a model-input producer
 }
 void AttachReservation(PioneerResidentBanks& owner,const tt_dram_samples_result& proof,const PioneerQ38ProjectionIdentity& id,
  const void* reservation,unsigned workspaceBank,uint32_t workspaceOffset){
  banks=&owner;baseline=&proof;identity=id;reservationOwner=reservation;bank=workspaceBank;offset=workspaceOffset;
 }
public:
 // Initialize the exact runtime area now;reserve the complete canonical
 // envelope ONCE so future resident decoder schedules/metadata do not
 // require growing an already-sticky reservation. Uninitialized metadata
 // is NOT exposed by InitializeSpan and is NOT made ready by zero CLOSE.
 enum {Pages=512,Bytes=2097152,ReservedPages=16384,ReservedBytes=67108864};
 PioneerQ38ProjectionBanks()=default;
 PioneerQ38ProjectionBanks(const PioneerQ38ProjectionBanks&)=delete;
 PioneerQ38ProjectionBanks& operator=(const PioneerQ38ProjectionBanks&)=delete;
 // No standalone Bind/raw coordinates/public shared-owner hint. Only the
 // new canonical joint factory can attach this role AFTER atomic claims.
 bool MatchesInitializationOwner(const PioneerResidentBanks& owner,const tt_dram_samples_result& proof,const PioneerQ38ProjectionIdentity& id)const{
  return banks==&owner && baseline==&proof && Matches(id);
 }
 bool InitializeSpan(const PioneerQ38ProjectionIdentity& id,unsigned region,unsigned first,unsigned count,PioneerQ38ProjectionExtent& output)const{
  if(!Matches(id) || region || !count || first>=Pages || count>Pages-first)return false;
  PioneerQ38ProjectionExtent e={bank,0,0,0,count,count*4096};
  for(unsigned p=0;p<count;p++){
   uint32_t a=0,r=0,i=0;
   if(!banks->MutablePage(*baseline,reservationOwner,bank,uint64_t(offset)+uint64_t(first+p)*4096,a,r,i))return false;
   if(!p){e.address=a;e.route=r;e.index=i;}else if(a!=e.address+p*4096 || r!=e.route || i!=e.index+p)return false;
  }
  output=e;return true; // initialization extent ONLY,NOT compute/read/producer authority
 }
 bool FreshInitializationSpan(const PioneerQ38ProjectionIdentity& id,unsigned region,unsigned first,unsigned count,PioneerQ38ProjectionExtent& output)const{
  if(published || region || !count || first>=Pages || count>Pages-first)return false;
  for(unsigned p=0;p<count;p++)if(Recorded(first+p))return false;
  return InitializeSpan(id,region,first,count,output);
 }
 bool InitializationProgress(const PioneerQ38ProjectionIdentity& id,unsigned& pages)const{
  if(!Matches(id))return false;
  unsigned n=0;for(unsigned p=0;p<Pages;p++)n+=Recorded(p);pages=n;return true;
 }
 bool InitializationPublished(const PioneerQ38ProjectionIdentity& id)const{return Matches(id) && published;}
 bool OwnsInitializationLease(const PioneerQ38ProjectionZeroLease* lease)const{return lease && initializationLease==lease;}
 void Fault(){unsafe=true;published=false;if(banks)banks->Quarantine();} // MEMORY ONLY;no release/reset/rebind
 bool Unsafe()const{return unsafe || (banks && banks->Quarantined());}
};
static_assert(TT_Q38_PROJECTION_OUTPUT_OFFSET+248320*4<=PioneerQ38ProjectionBanks::Bytes,"complete vocabulary F32 output fits reserved pages");
static_assert(sizeof(PioneerQ38ProjectionBanks)<256,"private page ledger,NOT a fake producer/ready object");
#endif
