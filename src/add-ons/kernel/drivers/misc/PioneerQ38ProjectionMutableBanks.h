/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_MUTABLE_BANKS_H
#define PIONEER_Q38_PROJECTION_MUTABLE_BANKS_H
#include "PioneerQ38GdnBanks.h"
#include "PioneerQ38ProjectionBanks.h"
#include "PioneerQ38ProjectionJobs.h"
// NEW private HEAP canonical joint owner,NOT an extension of the old
// GDN+FFN74-page reservation. No FFN child/old source ownership promotion.
// Reserve ALL recurrent regions+FULL64MiB canonical workspace envelope in
// ONE atomic call. Source authentication/exclusive native cookie/clock/
// hosts/ALL code participants and completed input producers remain owed.
class PioneerQ38ProjectionMutableBanks {
 PioneerResidentBanks* banks=nullptr;
 const tt_dram_samples_result* baseline=nullptr;
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const uint8_t* canonicalLedger=nullptr;unsigned ledgerPages=0;
 PioneerQ38ProjectionIdentity identity={};
 PioneerQ38GdnBanks recurrent;
 PioneerQ38ProjectionBanks projection;
 PioneerResidentMutableRange claims[673]={};
 bool unsafe=false;
 bool Matches(const PioneerQ38ProjectionIdentity& id)const{
  return banks && baseline && canonical && canonical->Ready() && !unsafe && id==identity
   && !recurrent.Unsafe() && !projection.Unsafe() && banks->Matches(*baseline);
 }
public:
 PioneerQ38ProjectionMutableBanks()=default;
 PioneerQ38ProjectionMutableBanks(const PioneerQ38ProjectionMutableBanks&)=delete;
 PioneerQ38ProjectionMutableBanks& operator=(const PioneerQ38ProjectionMutableBanks&)=delete;
 bool BindCanonical(PioneerResidentBanks& owner,const tt_dram_samples_result& proof,const PioneerQ38ProjectionIdentity& id,
  const uint8_t* learned,unsigned pages,const PioneerQ38CanonicalModelPlan& plan,unsigned bank){
  if(banks || unsafe || !id.Valid() || !plan.Ready() || bank>=7 || !learned || pages!=PioneerResidentBanks::Pages || !owner.Matches(proof))return false;
  const auto* workspace=plan.Workspace(bank);unsigned count=plan.RecurrentCount();
  if(!workspace || workspace->bank!=bank || workspace->bytes!=67108864 || (workspace->offset&4095)
   || uint64_t(workspace->offset)>=owner.Capacity || UINT64_C(67108864)>owner.Capacity-workspace->offset
   || !recurrent.StageRegions(plan.RecurrentRegions(),count))return false;
  for(unsigned i=0;i<count;i++){
   const auto& r=recurrent.claims[i];
   if(r.bank==bank && uint64_t(r.firstPage)*4096<uint64_t(workspace->offset)+67108864
    && uint64_t(workspace->offset)<uint64_t(r.firstPage+r.pages)*4096)return false;
   claims[i]=r;
  }
  claims[count]={bank,workspace->offset/4096,PioneerQ38ProjectionBanks::ReservedPages};
  if(!owner.ReserveMutable(proof,this,claims,count+1,learned,pages))return false;
  banks=&owner;baseline=&proof;canonical=&plan;canonicalLedger=learned;ledgerPages=pages;identity=id;
  if(!recurrent.AttachReservation(owner,proof,id,this,count)){Fault();return false;}
  projection.AttachReservation(owner,proof,id,this,bank,workspace->offset);return true;
 }
 PioneerQ38GdnBanks* Recurrent(const PioneerQ38ProjectionIdentity& id){return Matches(id)?&recurrent:nullptr;}
 PioneerQ38ProjectionBanks* Projection(const PioneerQ38ProjectionIdentity& id){return Matches(id)?&projection:nullptr;}
 const PioneerQ38CanonicalModelPlan* CanonicalModel(const PioneerQ38ProjectionIdentity& id)const{return Matches(id)?canonical:nullptr;}
 bool MatchesInitializationOwner(const PioneerResidentBanks& owner,const tt_dram_samples_result& proof,const PioneerQ38ProjectionIdentity& id)const{
  return banks==&owner && baseline==&proof && Matches(id);
 }
 // Necessary CPU-only canonical geometry/upload/zero-ledger checks. This
 // does NOT grant model execution or turn zero init into a completed F32
 // model-input producer. Future controller MUST retain the ACTUAL producer
 // object/epoch/span and prove all current native source/code/terminal state.
 bool MatchInitializedProjection(const PioneerQ38ProjectionIdentity& id,const PioneerQ38ProjectionJobs& jobs,
  unsigned layer,unsigned role,uint64_t token,const uint8_t* learned,unsigned pages)const{
  return learned==canonicalLedger && pages==ledgerPages && Matches(id) && projection.InitializationPublished(id)
   && jobs.Matches(*canonical,layer,role,token) && jobs.LearnedPagesPresent(learned,pages);
 }
 void Fault(){unsafe=true;recurrent.Fault();projection.Fault();if(banks)banks->Quarantine();} // MEMORY ONLY
 bool Unsafe()const{return unsafe || recurrent.Unsafe() || projection.Unsafe() || (banks && banks->Quarantined());}
};
static_assert(sizeof(PioneerQ38ProjectionMutableBanks)>32768 && sizeof(PioneerQ38ProjectionMutableBanks)<131072,"complete joint owner is HEAP ONLY");
#endif
