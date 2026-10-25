/* SPDX-License-Identifier: MIT */
#include "PioneerQ38ResidentLayerOwner.h" // revalidate role/runtime even after inclusion
#if !defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT) || PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT != 1
#error Exact separate full-layer native context required,not a component or old output scope
#endif
#ifndef PIONEER_Q38_RESIDENT_LAYER_NATIVE_STATE_H
#define PIONEER_Q38_RESIDENT_LAYER_NATIVE_STATE_H
#include "PioneerQ38EmbeddingNativeState.h"
#include "PioneerQ38ResidentOutputLearned.h" // ALL56 fixed first-layer DATA only,never old native ownership
#include "PioneerQ38ConvHistoryZeroController.h"
#include "PioneerQ38StateZeroController.h"
struct PioneerStoreContext;
struct PioneerSessionIO;
[[maybe_unused]] static bool BindQ38ResidentLayerNativeState(PioneerStoreContext&);
[[maybe_unused]] static bool InitializeQ38ResidentLayerRecurrent(PioneerStoreContext&,PioneerQ38StateZeroResult&);
[[maybe_unused]] static bool InitializeQ38ResidentLayerHistory(PioneerStoreContext&,PioneerQ38ConvHistoryZeroResult&);
[[maybe_unused]] static bool PrepareQ38ResidentLayerNative(PioneerStoreContext&,PioneerQ38ResidentLayerResult&);
[[maybe_unused]] static bool RunQ38ResidentLayerNative(PioneerStoreContext&,PioneerQ38ResidentLayerResult&);
[[maybe_unused]] static bool CloseQ38ResidentLayerNative(PioneerStoreContext&,PioneerQ38ResidentLayerResult&);
[[maybe_unused]] static bool CloseQ38ResidentLayerNativeForValidation(PioneerStoreContext&,PioneerQ38ResidentLayerResult&,
 uint32_t*,unsigned,PioneerQ38ResidentLayerReadbackResult&);
// A distinct retained NATIVE context, borrowing the genuine prefix. No
// public construction/copy, phase setter, readiness bit or destructor.
// Binding alone is CPU metadata, NEVER initialization/compute permission.
// Initializers and the complete layer use this context's private leases,
// journal and new IO scopes. Actual execution/original-model/physical
// qualification remain required;only a refusing context object may compile.
class PioneerQ38ResidentLayerNativeState {
 friend struct PioneerStoreContext;
 friend struct PioneerSessionIO;
 friend bool BindQ38ResidentLayerNativeState(PioneerStoreContext&);
 friend bool InitializeQ38ResidentLayerRecurrent(PioneerStoreContext&,PioneerQ38StateZeroResult&);
 friend bool InitializeQ38ResidentLayerHistory(PioneerStoreContext&,PioneerQ38ConvHistoryZeroResult&);
 friend bool PrepareQ38ResidentLayerNative(PioneerStoreContext&,PioneerQ38ResidentLayerResult&);
 friend bool RunQ38ResidentLayerNative(PioneerStoreContext&,PioneerQ38ResidentLayerResult&);
 friend bool CloseQ38ResidentLayerNative(PioneerStoreContext&,PioneerQ38ResidentLayerResult&);
 friend bool CloseQ38ResidentLayerNativeForValidation(PioneerStoreContext&,PioneerQ38ResidentLayerResult&,
  uint32_t*,unsigned,PioneerQ38ResidentLayerReadbackResult&);
 enum Phase {Bound,RecurrentZeroing,RecurrentClosing,HistoryZeroing,HistoryClosing,Prefix,LayerPreparing,LayerRunning,LayerComplete,LayerClosing,LayerClosed};
 PioneerStoreContext* const context;
 PioneerQ38EmbeddingNativeState* const prefix;
 const void* const cookie;
 const void* const hashes;
 const PioneerQ38GdnIdentity identity;
 const uint8_t* const ledger;
 const unsigned token,bank;
 const int64_t team;
 Phase phase=Bound;
 bool unsafe=false;
 PioneerQ38StateZeroLease recurrentZero;
 PioneerQ38ConvHistoryZeroLease historyZero;
 PioneerQ38ResidentLayerOwner layer;
 PioneerQ38ResidentLayerMemory* memory=nullptr; //44MB journal must be HEAP, never a kernel frame
 PioneerQ38ResidentLayerNativeState(PioneerStoreContext& c,PioneerQ38EmbeddingNativeState& m,
  const void* fd,const void* h,int64_t ownerTeam)
  :context(&c),prefix(&m),cookie(fd),hashes(h),identity(m.identity),ledger(m.ledger),token(m.token),bank(m.bank),team(ownerTeam){}
 ~PioneerQ38ResidentLayerNativeState(){delete memory;} // ONLY known-clean native context destruction
 bool RecurrentInitialized(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof)const{
  if(!prefix)return false;
  const auto* g=prefix->graph.Recurrent(identity);
  return g && prefix->plan.RecurrentCount()==672 && recurrentZero.saved && recurrentZero.closed
   && recurrentZero.nonce==672 && !recurrentZero.dirty && !recurrentZero.ready && !recurrentZero.running && !recurrentZero.unsafe
   && recurrentZero.identity==identity && g->InitializationPublished(identity)
   && g->MatchesInitializationOwner(banks,proof,identity);
 }
 bool Initialized(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof)const{
  if(!prefix || !RecurrentInitialized(banks,proof))return false;
  const auto* h=prefix->graph.ConvHistory(identity);
  return h
   && historyZero.saved && historyZero.closed && historyZero.nonce==336 && !historyZero.dirty && !historyZero.ready
   && !historyZero.running && !historyZero.unsafe && historyZero.identity==identity && historyZero.owner==h
   && historyZero.banks==&banks && historyZero.proof==&proof && h->InitializationPublished(identity)
   && h->MatchesInitializationOwner(banks,proof,identity) && !h->OwnsInitializationLease(&historyZero);
 }
public:
 PioneerQ38ResidentLayerNativeState(const PioneerQ38ResidentLayerNativeState&)=delete;
 PioneerQ38ResidentLayerNativeState& operator=(const PioneerQ38ResidentLayerNativeState&)=delete;
 bool MatchesBinding(const PioneerStoreContext& c,const PioneerQ38EmbeddingNativeState& m,const void* fd,
  const void* h,int64_t ownerTeam,const uint8_t* learned,uint64_t generation)const{
  return context==&c && prefix==&m && cookie==fd && fd && hashes==h && h && team==ownerTeam && team>=0
   && ledger==learned && learned && m.ledger==ledger && m.identity==identity && identity.Valid()
   && identity.driverGeneration==generation && generation && m.token==token && token<248320 && !m.position
   && m.bank==bank && bank<7 && !Unsafe();
 } // necessary binding ONLY; actual native source/clock/host and physical guards remain mandatory
 bool Unsafe()const{return unsafe || !prefix || prefix->Unsafe() || recurrentZero.unsafe || historyZero.unsafe
  || layer.Unsafe() || (memory && memory->Unsafe());}
 bool InitializationsClosed(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof)const{
  return !Unsafe() && Initialized(banks,proof);
 } // necessary private controller seals,not source/IO/compute authority
 bool ControlsLayerCohort(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof,const PioneerQ38DispatchLease* cohort,
  const uint8_t* learned,unsigned pages)const{
  if(!prefix || !cohort || Unsafe() || !Initialized(banks,proof) || learned!=ledger || !learned || pages!=PioneerResidentBanks::Pages
   || prefix->projectionCohort!=cohort || prefix->phase!=PioneerQ38EmbeddingNativeState::ProjectionComplete || !prefix->Closed()
   || cohort->unsafe || cohort->closed || cohort->prepared!=120 || cohort->booted || cohort->bootAttempted
   || !prefix->graph.MatchesInitializationOwner(banks,proof,identity))return false;
  if(phase==Bound || (phase==LayerPreparing && !layer.Claimed())){
   return (phase==Bound?!memory:memory && !memory->Unsafe() && !memory->Closed()) && !layer.Claimed()
    && prefix->projection.ResidentInput() && prefix->projection.ResidentInput()->Current()
    && prefix->projection.MatchesClosedNativeOwner(proof,banks,*cohort,prefix->projectionMemory,prefix->graph,prefix->plan,
     prefix->rms,identity,learned,pages);
  }
  return memory && (phase==LayerPreparing || phase==LayerRunning || phase==LayerComplete || phase==LayerClosing)
   && layer.MatchesNativeOwner(prefix->projection,*memory);
 } // necessary private CPU binding; NOT native source/mapping/reset/compute authority
 bool HasLayerMemory()const{return memory && !Unsafe() && (phase==LayerPreparing || phase==LayerRunning
  || phase==LayerComplete || phase==LayerClosing || phase==LayerClosed);}
 bool ClosedLayerBinding()const{return phase==LayerClosed && !Unsafe() && memory
  && layer.MatchesClosedNativeOwner(prefix->projection,*memory);}
 bool Closed(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof)const{
  return phase==LayerClosed && !Unsafe() && prefix->Closed() && Initialized(banks,proof)
   && memory && layer.Closed() && memory->Closed();
 } // a CLOSED old prefix alone can NEVER authorize END of this new owner
 void Fault(){unsafe=true;recurrentZero.unsafe=historyZero.unsafe=true;if(memory)memory->Fault();}
 // MEMORY ONLY. Native retention separately quarantines the borrowed
 // graph/banks/clock/hosts and keeps this object, prefix and mappings alive.
};
#endif
