/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GRAPH_MUTABLE_BANKS_H
#define PIONEER_Q38_GRAPH_MUTABLE_BANKS_H
#include "PioneerQ38GdnBanks.h"
#include "PioneerQ38FfnBanks.h"
#include "PioneerQ38CanonicalModelPlan.h"
#include "PioneerQ38FfnCanonicalJobs.h"
// KERNEL-PRIVATE,HEAP ONLY. ONE sticky reservation for the complete GDN
// state AND the resident FFN,not two independent mutable owners. Native
// integration must derive these proposals from the authenticated retained
// model and hold the same exclusive global store lock. No native operation
// selects this factory yet. It grants NO initialization/producer/launch,
// terminal,clock or host-DMA authority. Each role keeps its private ledger.
class PioneerQ38GraphMutableBanks {
 PioneerResidentBanks* banks=nullptr;
 const tt_dram_samples_result* baseline=nullptr;
 PioneerQ38GdnIdentity identity={};
 const PioneerQ38CanonicalModelPlan* canonical=nullptr; // exact borrowed geometry;native context retains it
 const uint8_t* canonicalLedger=nullptr; // SAME retained upload bitmap,not an equal caller copy
 unsigned canonicalLedgerPages=0;
 PioneerQ38GdnBanks recurrent;
 PioneerQ38FfnBanks ffn;
 PioneerResidentMutableRange claims[673]={};
 bool unsafe=false;
 bool Matches(const PioneerQ38GdnIdentity& id)const {
  return banks && baseline && !unsafe && id==identity && !recurrent.Unsafe() && !ffn.Unsafe()
   && banks->Matches(*baseline);
 }
public:
 PioneerQ38GraphMutableBanks()=default;
 PioneerQ38GraphMutableBanks(const PioneerQ38GraphMutableBanks&)=delete;
 PioneerQ38GraphMutableBanks& operator=(const PioneerQ38GraphMutableBanks&)=delete;
 // Kernel-derived fixed model geometry,not public caller coordinates.
 // Still only an atomic reservation:source authentication and every
 // initialization/producer/execution proof remain the native caller's job.
 bool BindCanonical(PioneerResidentBanks& owner,const tt_dram_samples_result& proof,const PioneerQ38GdnIdentity& id,
  const uint8_t* learned,unsigned learnedPages,const PioneerQ38CanonicalModelPlan& plan,unsigned ffnBank) {
  const auto* workspace=plan.Workspace(ffnBank);
  if(!workspace || !Bind(owner,proof,id,learned,learnedPages,plan.RecurrentRegions(),plan.RecurrentCount(),ffnBank,workspace->offset))return false;
  canonical=&plan;canonicalLedger=learned;canonicalLedgerPages=learnedPages;return true;
 }
 bool Bind(PioneerResidentBanks& owner,const tt_dram_samples_result& proof,const PioneerQ38GdnIdentity& id,
  const uint8_t* learned,unsigned learnedPages,const PioneerQ38GdnRegion* regions,unsigned count,
  unsigned ffnBank,uint64_t workspaceOffset) {
  if(banks || unsafe || !id.Valid() || ffnBank>=7 || (workspaceOffset&4095)
   || workspaceOffset>=owner.Capacity || UINT64_C(67108864)>owner.Capacity-workspaceOffset
   || !owner.Matches(proof) || !recurrent.StageRegions(regions,count))return false;
  // The full64MiB FFN envelope must be disjoint from recurrent state,
  // even though the resident FFN currently owns only its first74 pages.
  for(unsigned i=0;i<count;i++) {
   const auto& r=recurrent.claims[i];
   if(r.bank==ffnBank && uint64_t(r.firstPage)*4096<workspaceOffset+67108864
    && workspaceOffset<uint64_t(r.firstPage+r.pages)*4096)return false;
   claims[i]=r;
  }
  claims[count]={ffnBank,uint32_t(workspaceOffset/4096),PioneerQ38FfnBanks::Pages};
  // ReserveMutable checks EVERY page,learned collision and pair overlap
  // before publishing ANY claim. A refusal leaves both roles unbound.
  if(!owner.ReserveMutable(proof,this,claims,count+1,learned,learnedPages))return false;
  banks=&owner;baseline=&proof;identity=id; // retain exact owner after claim
  if(!recurrent.AttachReservation(owner,proof,id,this,count)){Fault();return false;}
  ffn.AttachReservation(owner,proof,id,this,ffnBank,workspaceOffset);
  return true;
 }
 // BORROWED typed role handles:NEVER delete separately or put them into
 // an old native context's individually deleted owner pointers. Retain
 // this complete heap owner until that context dies. They share a page
 // reservation,NOT code/operation ownership or readiness:the native caller
 // MUST serialize/exclude BOTH roles' controllers under one real owner.
 // Every zero/compute controller still requires all its own proofs.
 PioneerQ38GdnBanks* Recurrent(const PioneerQ38GdnIdentity& id){return Matches(id)?&recurrent:nullptr;}
 PioneerQ38FfnBanks* Ffn(const PioneerQ38GdnIdentity& id){return Matches(id)?&ffn:nullptr;}
 const PioneerQ38CanonicalModelPlan* CanonicalModel(const PioneerQ38GdnIdentity& id)const {
  return Matches(id) && canonical && canonical->Ready()?canonical:nullptr;
 }
 // Necessary CPU-only preflight,NOT input/source authentication or any
 // launch grant. The real native caller still owes the initial F32
 // producer,exclusive code/operation/host/clock and current terminals.
 // Zero-filled allocation or equal-but-foreign geometry cannot pass.
 bool MatchInitializedFfn(const PioneerQ38GdnIdentity& id,const PioneerQ38FfnCanonicalJobs& jobs,
  unsigned layer,uint64_t position,const uint8_t* learned,unsigned pages)const {
  return learned==canonicalLedger && pages==canonicalLedgerPages && CanonicalModel(id)
   && ffn.InitializationPublished(id) && jobs.Matches(*canonical,layer,position)
   && jobs.LearnedPagesPresent(learned,pages);
 }
 bool MatchesInitializationOwner(const PioneerResidentBanks& owner,const tt_dram_samples_result& proof,
  const PioneerQ38GdnIdentity& id)const{return banks==&owner && baseline==&proof && Matches(id);}
 void Fault(){unsafe=true;recurrent.Fault();ffn.Fault();if(banks)banks->Quarantine();} // memory ONLY,never release/rebind
 bool Unsafe()const{return unsafe || recurrent.Unsafe() || ffn.Unsafe() || (banks && banks->Quarantined());}
};
static_assert(sizeof(PioneerQ38GraphMutableBanks)>32768 && sizeof(PioneerQ38GraphMutableBanks)<131072,"bounded HEAP-ONLY graph reservation owner");
#endif
