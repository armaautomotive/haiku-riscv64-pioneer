/* SPDX-License-Identifier: MIT */
#include "PioneerQ38ResidentOutputOwner.h" // revalidate role/runtime even after inclusion
#ifndef PIONEER_Q38_RESIDENT_OUTPUT_NATIVE_STATE_H
#define PIONEER_Q38_RESIDENT_OUTPUT_NATIVE_STATE_H
#include "PioneerQ38EmbeddingNativeState.h"
#include "PioneerQ38ConvHistoryZeroController.h"
#include "PioneerQ38StateZeroController.h"
struct PioneerStoreContext;
struct PioneerSessionIO;
[[maybe_unused]] static bool BindQ38ResidentOutputNativeState(PioneerStoreContext&);
[[maybe_unused]] static bool InitializeQ38ResidentOutputRecurrent(PioneerStoreContext&,PioneerQ38StateZeroResult&);
[[maybe_unused]] static bool InitializeQ38ResidentOutputHistory(PioneerStoreContext&,PioneerQ38ConvHistoryZeroResult&);
[[maybe_unused]] static bool PrepareQ38ResidentOutputNative(PioneerStoreContext&,PioneerQ38ResidentOutputResult&);
[[maybe_unused]] static bool RunQ38ResidentOutputNative(PioneerStoreContext&,PioneerQ38ResidentOutputResult&);
[[maybe_unused]] static bool CloseQ38ResidentOutputNative(PioneerStoreContext&,PioneerQ38ResidentOutputResult&);
[[maybe_unused]] static bool CloseQ38ResidentOutputNativeForValidation(PioneerStoreContext&,PioneerQ38ResidentOutputResult&,
 uint32_t*,unsigned,PioneerQ38ResidentOutputReadbackResult&);
// A distinct retained NATIVE context, borrowing the genuine prefix. No
// public construction/copy, phase setter, readiness bit or destructor.
// Binding alone is CPU metadata, NEVER initialization/compute permission.
// Initializers use actual private leases; complete output IO/factory and
// native/physical qualification remain required. The driver still rejects
// this experimental role before compilation.
class PioneerQ38ResidentOutputNativeState {
 friend struct PioneerStoreContext;
 friend struct PioneerSessionIO;
 friend bool BindQ38ResidentOutputNativeState(PioneerStoreContext&);
 friend bool InitializeQ38ResidentOutputRecurrent(PioneerStoreContext&,PioneerQ38StateZeroResult&);
 friend bool InitializeQ38ResidentOutputHistory(PioneerStoreContext&,PioneerQ38ConvHistoryZeroResult&);
 friend bool PrepareQ38ResidentOutputNative(PioneerStoreContext&,PioneerQ38ResidentOutputResult&);
 friend bool RunQ38ResidentOutputNative(PioneerStoreContext&,PioneerQ38ResidentOutputResult&);
 friend bool CloseQ38ResidentOutputNative(PioneerStoreContext&,PioneerQ38ResidentOutputResult&);
 friend bool CloseQ38ResidentOutputNativeForValidation(PioneerStoreContext&,PioneerQ38ResidentOutputResult&,
  uint32_t*,unsigned,PioneerQ38ResidentOutputReadbackResult&);
 enum Phase {Bound,RecurrentZeroing,RecurrentClosing,HistoryZeroing,HistoryClosing,Prefix,OutputPreparing,OutputRunning,OutputComplete,OutputClosing,OutputClosed};
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
 PioneerQ38ResidentOutputOwner output;
 PioneerQ38ResidentOutputMemory* memory=nullptr; //43.8MB journal must be HEAP, never a kernel frame
 PioneerQ38ResidentOutputNativeState(PioneerStoreContext& c,PioneerQ38EmbeddingNativeState& m,
  const void* fd,const void* h,int64_t ownerTeam)
  :context(&c),prefix(&m),cookie(fd),hashes(h),identity(m.identity),ledger(m.ledger),token(m.token),bank(m.bank),team(ownerTeam){}
 ~PioneerQ38ResidentOutputNativeState(){delete memory;} // ONLY known-clean native context destruction
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
 PioneerQ38ResidentOutputNativeState(const PioneerQ38ResidentOutputNativeState&)=delete;
 PioneerQ38ResidentOutputNativeState& operator=(const PioneerQ38ResidentOutputNativeState&)=delete;
 bool MatchesBinding(const PioneerStoreContext& c,const PioneerQ38EmbeddingNativeState& m,const void* fd,
  const void* h,int64_t ownerTeam,const uint8_t* learned,uint64_t generation)const{
  return context==&c && prefix==&m && cookie==fd && fd && hashes==h && h && team==ownerTeam && team>=0
   && ledger==learned && learned && m.ledger==ledger && m.identity==identity && identity.Valid()
   && identity.driverGeneration==generation && generation && m.token==token && token<248320 && !m.position
   && m.bank==bank && bank<7 && !Unsafe();
 } // necessary binding ONLY; actual native source/clock/host and physical guards remain mandatory
 bool Unsafe()const{return unsafe || !prefix || prefix->Unsafe() || recurrentZero.unsafe || historyZero.unsafe
  || output.Unsafe() || (memory && memory->Unsafe());}
 bool InitializationsClosed(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof)const{
  return !Unsafe() && Initialized(banks,proof);
 } // necessary private controller seals,not source/IO/compute authority
 bool ControlsOutputCohort(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof,const PioneerQ38DispatchLease* cohort,
  const uint8_t* learned,unsigned pages)const{
  if(!prefix || !cohort || Unsafe() || !Initialized(banks,proof) || learned!=ledger || !learned || pages!=PioneerResidentBanks::Pages
   || prefix->projectionCohort!=cohort || prefix->phase!=PioneerQ38EmbeddingNativeState::ProjectionComplete || !prefix->Closed()
   || cohort->unsafe || cohort->closed || cohort->prepared!=120 || cohort->booted || cohort->bootAttempted
   || !prefix->graph.MatchesInitializationOwner(banks,proof,identity))return false;
  if(phase==Bound || (phase==OutputPreparing && !output.Claimed())){
   return (phase==Bound?!memory:memory && !memory->Unsafe() && !memory->Closed()) && !output.Claimed()
    && prefix->projection.ResidentInput() && prefix->projection.ResidentInput()->Current()
    && prefix->projection.MatchesClosedNativeOwner(proof,banks,*cohort,prefix->projectionMemory,prefix->graph,prefix->plan,
     prefix->rms,identity,learned,pages);
  }
  return memory && (phase==OutputPreparing || phase==OutputRunning || phase==OutputComplete || phase==OutputClosing)
   && output.MatchesNativeOwner(prefix->projection,*memory);
 } // necessary private CPU binding; NOT native source/mapping/reset/compute authority
 bool HasOutputMemory()const{return memory && !Unsafe() && (phase==OutputPreparing || phase==OutputRunning
  || phase==OutputComplete || phase==OutputClosing || phase==OutputClosed);}
 bool ClosedOutputBinding()const{return phase==OutputClosed && !Unsafe() && memory
  && output.MatchesClosedNativeOwner(prefix->projection,*memory);}
 bool Closed(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof)const{
  return phase==OutputClosed && !Unsafe() && prefix->Closed() && Initialized(banks,proof)
   && memory && output.Closed() && memory->Closed();
 } // a CLOSED old prefix alone can NEVER authorize END of this new owner
 void Fault(){unsafe=true;recurrentZero.unsafe=historyZero.unsafe=true;if(memory)memory->Fault();}
 // MEMORY ONLY. Native retention separately quarantines the borrowed
 // graph/banks/clock/hosts and keeps this object, prefix and mappings alive.
};
#endif
