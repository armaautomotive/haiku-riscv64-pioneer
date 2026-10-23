/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_CONV_HISTORY_BANKS_H
#define PIONEER_Q38_CONV_HISTORY_BANKS_H
#if !defined(PIONEER_Q38_DECODER_RUNTIME) || PIONEER_Q38_DECODER_RUNTIME != 1
#error Convolution history requires the separate exact decoder runtime reservation role
#endif
#include "PioneerQ38CanonicalModelPlan.h"
struct PioneerResidentLease;struct PioneerBulkUploadLease;struct PioneerOutputBufferLease;struct PioneerWideOutputLease;
struct PioneerQ38ConvHistoryZeroLease;struct PioneerQ38ConvHistoryZeroResult;
struct PioneerQ38ConvHistoryExtent {uint32_t bank,address,route,index,pages,bytes;};
#ifdef PIONEER_Q38_RESIDENT_DECODER_OWNER
class PioneerQ38ResidentDecoderOwner;
#endif
class PioneerQ38ConvHistoryBanks {
 friend class PioneerQ38ProjectionMutableBanks;
#ifdef PIONEER_Q38_RESIDENT_DECODER_OWNER
 friend class PioneerQ38ResidentDecoderOwner;
 const PioneerQ38ResidentDecoderOwner* decoderOwner=nullptr;
#endif
 template<class IO> friend bool PioneerQ38ConvHistoryZeroPrepare(IO&,const tt_dram_samples_result&,const tt_compute_noc_result&,
  PioneerQ38ConvHistoryBanks&,PioneerResidentBanks&,const PioneerQ38GdnIdentity&,const tt_host_page_result&,
  PioneerResidentLease&,PioneerBulkUploadLease&,PioneerOutputBufferLease&,PioneerWideOutputLease&,PioneerQ38ConvHistoryZeroLease&);
 template<class IO> friend bool PioneerQ38ConvHistoryZeroRun(IO&,const tt_dram_samples_result&,const tt_compute_noc_result&,
  PioneerQ38ConvHistoryBanks&,PioneerResidentBanks&,const tt_host_page_result&,PioneerResidentLease&,PioneerBulkUploadLease&,
  PioneerOutputBufferLease&,PioneerWideOutputLease&,PioneerQ38ConvHistoryZeroLease&,const PioneerQ38GdnIdentity&,
  unsigned,unsigned,unsigned,PioneerQ38ConvHistoryZeroResult&);
 template<class IO> friend bool PioneerQ38ConvHistoryZeroClose(IO&,const tt_dram_samples_result&,const tt_compute_noc_result&,
  PioneerQ38ConvHistoryBanks&,PioneerResidentBanks&,const tt_host_page_result&,PioneerResidentLease&,PioneerBulkUploadLease&,
  PioneerOutputBufferLease&,PioneerWideOutputLease&,PioneerQ38ConvHistoryZeroLease&,bool);
 struct Region {uint32_t part,layer,first,channels,bank,offset;};
 PioneerResidentBanks* banks=nullptr;const tt_dram_samples_result* baseline=nullptr;
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;const void* reservationOwner=nullptr;
 const PioneerQ38ConvHistoryZeroLease* initializationLease=nullptr;
 PioneerQ38GdnIdentity identity={};Region regions[336]={};uint64_t initialized[27]={};
 unsigned canonicalCount=0;bool unsafe=false,published=false;
 bool Geometry()const{
  if(!canonical || !canonical->Ready() || canonical->Count()!=canonicalCount)return false;
  for(const auto& r:regions){
   const auto* p=canonical->Part(r.part);
   if(!p || p->kind!=canonical->ConvKind || p->layer!=r.layer || p->role || p->type || p->bank!=r.bank
    || p->firstRow!=r.first || p->rows!=r.channels || p->rowBytes!=12 || p->bytes!=r.channels*12
    || p->offset!=r.offset || p->allocated!=RegionPages*4096)return false;
  }
  return true;
 }
 bool Matches(const PioneerQ38GdnIdentity& id)const{
  return banks && baseline && reservationOwner && !unsafe && id==identity && Geometry() && banks->Matches(*baseline);
 }
 bool Recorded(unsigned page)const{return (initialized[page/64]>>(page%64))&1;}
 bool StageRegions(const PioneerQ38CanonicalModelPlan& plan){
  if(banks || unsafe || !plan.Ready())return false;
  unsigned channels[48]={},bankMask[48]={},count=0;
  for(unsigned i=0;i<plan.Count();i++){
   const auto* p=plan.Part(i);if(p->kind!=plan.ConvKind)continue;
   if(count>=Regions || p->layer>=64 || p->layer%4==3)return false;
   unsigned ordinal=p->layer-p->layer/4;
   if(p->role || p->type || p->bank>=7 || (bankMask[ordinal]&(1U<<p->bank))
    || p->firstRow!=channels[ordinal] || !p->rows || (p->rows&63) || p->rows>10240-p->firstRow
    || p->rowBytes!=12 || p->bytes!=p->rows*12 || p->allocated!=RegionPages*4096
    || (p->offset&4095) || p->offset>=plan.Capacity || p->allocated>plan.Capacity-p->offset)return false;
   regions[count++]={i,p->layer,p->firstRow,p->rows,p->bank,p->offset};
   channels[ordinal]+=p->rows;bankMask[ordinal]|=1U<<p->bank;
  }
  for(unsigned layer=0;layer<48;layer++)if(channels[layer]!=10240 || bankMask[layer]!=127)return false;
  if(count!=Regions)return false;
  canonical=&plan;canonicalCount=plan.Count();return true; // staged DATA only;no bank reservation or progress
 }
 bool AttachReservation(PioneerResidentBanks& owner,const tt_dram_samples_result& proof,const PioneerQ38GdnIdentity& id,const void* reservation){
  if(banks || !canonical || !id.Valid() || !reservation || !owner.Matches(proof))return false;
  banks=&owner;baseline=&proof;identity=id;reservationOwner=reservation;return true;
 }
 bool ClaimInitialization(const PioneerQ38GdnIdentity& id,const PioneerQ38ConvHistoryZeroLease* lease){
  if(!lease || initializationLease || published || !Matches(id))return false;
  initializationLease=lease;return true;
 }
 bool CloseInitialization(const PioneerQ38GdnIdentity& id,const PioneerQ38ConvHistoryZeroLease* lease){
  if(!lease || initializationLease!=lease || !Matches(id))return false;
  initializationLease=nullptr;return true; // only controller's known-clean physical CLOSE
 }
 bool CommitInitialization(const PioneerQ38GdnIdentity& id,unsigned region,unsigned first,unsigned count){
  PioneerQ38ConvHistoryExtent e;if(!FreshInitializationSpan(id,region,first,count,e))return false;
  for(unsigned p=0;p<count;p++){unsigned at=region*RegionPages+first+p;initialized[at/64]|=UINT64_C(1)<<(at%64);}
  return true; // only zero controller AFTER completion and physical hold proofs
 }
 bool PublishInitialization(const PioneerQ38GdnIdentity& id){
  unsigned pages=0;if(published || !InitializationProgress(id,pages) || pages!=Pages)return false;
  published=true;return true; // zero-init/CLOSE seal,NOT a current token-state/compute grant
 }
public:
 enum {Regions=336,RegionPages=5,Pages=Regions*RegionPages,Bytes=Pages*4096};
 PioneerQ38ConvHistoryBanks()=default;
 PioneerQ38ConvHistoryBanks(const PioneerQ38ConvHistoryBanks&)=delete;
 PioneerQ38ConvHistoryBanks& operator=(const PioneerQ38ConvHistoryBanks&)=delete;
 bool MatchesInitializationOwner(const PioneerResidentBanks& owner,const tt_dram_samples_result& proof,const PioneerQ38GdnIdentity& id)const{
  return banks==&owner && baseline==&proof && Matches(id);
 }
 bool InitializationRegions(const PioneerQ38GdnIdentity& id,unsigned& count)const{if(!Matches(id))return false;count=Regions;return true;}
 bool InitializationPages(const PioneerQ38GdnIdentity& id,unsigned region,unsigned& pages)const{
  if(!Matches(id) || region>=Regions)return false;
  pages=RegionPages;return true;
 }
 bool InitializeSpan(const PioneerQ38GdnIdentity& id,unsigned region,unsigned first,unsigned count,PioneerQ38ConvHistoryExtent& output)const{
  if(!Matches(id) || region>=Regions || !count || first>=RegionPages || count>RegionPages-first)return false;
  const auto& r=regions[region];const auto* p=canonical->Part(r.part);
  if(!p || p->kind!=canonical->ConvKind || p->layer!=r.layer || p->role || p->type || p->bank!=r.bank
   || p->firstRow!=r.first || p->rows!=r.channels || p->rowBytes!=12 || p->bytes!=r.channels*12
   || p->offset!=r.offset || p->allocated!=RegionPages*4096)return false;
  PioneerQ38ConvHistoryExtent e={r.bank,0,0,0,count,count*4096};
  for(unsigned page=0;page<count;page++){
   uint32_t a=0,route=0,index=0;
   if(!banks->MutablePage(*baseline,reservationOwner,r.bank,uint64_t(r.offset)+uint64_t(first+page)*4096,a,route,index))return false;
   if(!page){e.address=a;e.route=route;e.index=index;}else if(a!=e.address+page*4096 || route!=e.route || index!=e.index+page)return false;
  }
  output=e;return true; // initialization-only;no read/current-history/producer authority
 }
 bool FreshInitializationSpan(const PioneerQ38GdnIdentity& id,unsigned region,unsigned first,unsigned count,PioneerQ38ConvHistoryExtent& output)const{
  if(published || region>=Regions || !count || first>=RegionPages || count>RegionPages-first)return false;
  for(unsigned p=0;p<count;p++)if(Recorded(region*RegionPages+first+p))return false;
  return InitializeSpan(id,region,first,count,output);
 }
 bool InitializationProgress(const PioneerQ38GdnIdentity& id,unsigned& pages)const{
  if(!Matches(id))return false;
  unsigned n=0;for(unsigned p=0;p<Pages;p++)n+=Recorded(p);pages=n;return true;
 }
 bool InitializationPublished(const PioneerQ38GdnIdentity& id)const{return Matches(id) && published;}
 bool OwnsInitializationLease(const PioneerQ38ConvHistoryZeroLease* lease)const{return lease && initializationLease==lease;}
 void Fault(){unsafe=true;published=false;if(banks)banks->Quarantine();} // retain reservation/lease;NO IO/reset/release
 bool Unsafe()const{return unsafe || (banks && banks->Quarantined());}
};
static_assert(sizeof(PioneerQ38ConvHistoryBanks)>8192 && sizeof(PioneerQ38ConvHistoryBanks)<12288,"real history regions/page ledger are HEAP ONLY");
#endif
