/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT
#include "PioneerQ38RecurrentPrefixNativeRole.h"
#ifndef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_STATE_H
#define PIONEER_Q38_RECURRENT_PREFIX_NATIVE_STATE_H
#include "PioneerQ38EmbeddingNativeState.h"
#include "PioneerQ38RecurrentPrefixMemory.h"
#include "PioneerQ38RecurrentPrefixOwner.h"
#include "PioneerQ38RecurrentPrefixLearned.h"
#include "PioneerQ38StateZeroController.h"
#include "PioneerQ38ConvHistoryZeroController.h"
#include "../../../../../tools/pioneer/PioneerQ38RecurrentPrefixPlan.h"
struct PioneerStoreContext;struct PioneerSessionIO;
[[maybe_unused]] static bool BindQ38RecurrentPrefixNative(PioneerStoreContext&);
[[maybe_unused]] static bool InitializeQ38RecurrentPrefixState(PioneerStoreContext&,PioneerQ38StateZeroResult&);
[[maybe_unused]] static bool InitializeQ38RecurrentPrefixHistory(PioneerStoreContext&,PioneerQ38ConvHistoryZeroResult&);
[[maybe_unused]] static bool PrepareQ38RecurrentPrefixNativeHeld(PioneerStoreContext&,PioneerQ38RecurrentPrefixMemoryResult&);
[[maybe_unused]] static bool CloseQ38RecurrentPrefixNativeHeld(PioneerStoreContext&,PioneerQ38RecurrentPrefixMemoryResult&);
[[maybe_unused]] static bool PrepareQ38RecurrentPrefixNativeExecution(PioneerStoreContext&,PioneerQ38RecurrentPrefixResult&);
[[maybe_unused]] static bool RunQ38RecurrentPrefixNativeExecution(PioneerStoreContext&,PioneerQ38RecurrentPrefixResult&);
[[maybe_unused]] static bool CloseQ38RecurrentPrefixNativeExecution(PioneerStoreContext&,PioneerQ38RecurrentPrefixResult&,PioneerQ38RecurrentPrefixMemoryResult&);
#ifdef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK
[[maybe_unused]] static bool CloseQ38RecurrentPrefixNativeValidation(PioneerStoreContext&,PioneerQ38RecurrentPrefixResult&,PioneerQ38RecurrentPrefixMemoryResult&,
 uint32_t*,unsigned,PioneerQ38RecurrentPrefixReadbackResult&);
#endif
// Genuine retained context,NOT a caller-ready bit or old dense scope alias.
// State/history init use private leases on this SAME real completed
// embedding context. A DISTINCT execution owner proves QPX1 RUN/counters/
// STOP/ALL120holds/restoration. Runtime qualification/final END/full-model
// integration remain separate;compilation is NEVER hardware permission.
class PioneerQ38RecurrentPrefixNativeState {
 friend struct PioneerStoreContext;friend struct PioneerSessionIO;
 friend bool BindQ38RecurrentPrefixNative(PioneerStoreContext&);
 friend bool InitializeQ38RecurrentPrefixState(PioneerStoreContext&,PioneerQ38StateZeroResult&);
 friend bool InitializeQ38RecurrentPrefixHistory(PioneerStoreContext&,PioneerQ38ConvHistoryZeroResult&);
 friend bool PrepareQ38RecurrentPrefixNativeHeld(PioneerStoreContext&,PioneerQ38RecurrentPrefixMemoryResult&);
 friend bool CloseQ38RecurrentPrefixNativeHeld(PioneerStoreContext&,PioneerQ38RecurrentPrefixMemoryResult&);
 friend bool PrepareQ38RecurrentPrefixNativeExecution(PioneerStoreContext&,PioneerQ38RecurrentPrefixResult&);
 friend bool RunQ38RecurrentPrefixNativeExecution(PioneerStoreContext&,PioneerQ38RecurrentPrefixResult&);
 friend bool CloseQ38RecurrentPrefixNativeExecution(PioneerStoreContext&,PioneerQ38RecurrentPrefixResult&,PioneerQ38RecurrentPrefixMemoryResult&);
#ifdef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK
 friend bool CloseQ38RecurrentPrefixNativeValidation(PioneerStoreContext&,PioneerQ38RecurrentPrefixResult&,PioneerQ38RecurrentPrefixMemoryResult&,
  uint32_t*,unsigned,PioneerQ38RecurrentPrefixReadbackResult&);
#endif
 enum Phase {Bound,RecurrentZeroing,RecurrentClosing,HistoryZeroing,HistoryClosing,DispatchPreparing,MemoryPreparing,MemoryInstalled,
  ExecutionPreparing,ExecutionReady,ExecutionComplete,ExecutionClosing,MemoryClosing,MemoryClosed};
 PioneerStoreContext* const context;PioneerQ38EmbeddingNativeState* const prefix;
 const void* const cookie;const void* const hashes;const uint8_t* const ledger;
 const PioneerQ38GdnIdentity identity;const unsigned token,bank;const int64_t team;
 Phase phase=Bound;bool unsafe=false;
 PioneerQ38StateZeroLease recurrentZero;
 PioneerQ38ConvHistoryZeroLease historyZero;
 PioneerQ38RecurrentPrefixMemory* memory=nullptr;PioneerQ38RecurrentPrefixPlan* plan=nullptr;
 PioneerQ38RecurrentPrefixOwner* execution=nullptr;
 PioneerQ38RecurrentPrefixNativeState(PioneerStoreContext& c,PioneerQ38EmbeddingNativeState& m,const void* fd,const void* h,int64_t owner)
  :context(&c),prefix(&m),cookie(fd),hashes(h),ledger(m.ledger),identity(m.identity),token(m.token),bank(m.bank),team(owner){}
 ~PioneerQ38RecurrentPrefixNativeState(){delete execution;delete memory;delete plan;} // known-clean native destruction only
 static bool CompletedInput(const PioneerQ38EmbeddingNativeState& m){return !m.token && !m.position
  && m.embedding.MatchesCompleted(m.graph,m.plan,m.identity,0,0,m.bank);}
 bool RecurrentInitialized(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof)const{
  if(!prefix)return false;
  const auto* g=prefix->graph.Recurrent(identity);
  return g && prefix->plan.RecurrentCount()==672 && recurrentZero.saved && recurrentZero.closed && recurrentZero.nonce==672
   && !recurrentZero.dirty && !recurrentZero.ready && !recurrentZero.running && !recurrentZero.unsafe
   && recurrentZero.identity==identity && g->InitializationPublished(identity) && g->MatchesInitializationOwner(banks,proof,identity);
 }
public:
 PioneerQ38RecurrentPrefixNativeState(const PioneerQ38RecurrentPrefixNativeState&)=delete;
 PioneerQ38RecurrentPrefixNativeState& operator=(const PioneerQ38RecurrentPrefixNativeState&)=delete;
 bool Unsafe()const{return unsafe || !prefix || prefix->Unsafe() || recurrentZero.unsafe || historyZero.unsafe
  || (execution && execution->Unsafe()) || (memory && memory->Unsafe());}
 bool InitializationsClosed(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof)const{
  if(Unsafe() || !RecurrentInitialized(banks,proof))return false;
  const auto* h=prefix->graph.ConvHistory(identity);
  return h && historyZero.saved && historyZero.closed && historyZero.nonce==336 && !historyZero.dirty && !historyZero.ready
   && !historyZero.running && !historyZero.unsafe && historyZero.identity==identity && historyZero.owner==h
   && historyZero.banks==&banks && historyZero.proof==&proof && h->InitializationPublished(identity)
   && h->MatchesInitializationOwner(banks,proof,identity) && !h->OwnsInitializationLease(&historyZero);
 } // necessary actual controller seals ONLY;not a source/RUN/cleanup grant
 bool Closed()const{return phase==MemoryClosed && !Unsafe() && memory && memory->Closed() && plan
  && (!execution || execution->MatchesClosedNativeOwner(*memory,*plan));}
 bool ControlsCohort(const PioneerQ38DispatchLease* c)const{
  return execution && !Unsafe() && memory && plan && execution->MatchesCohort(c);
 } // necessary private ownership,not a physical/source/cleanup grant
 bool MatchesBinding(const PioneerStoreContext& c,const PioneerQ38EmbeddingNativeState& m,const void* fd,const void* h,
  int64_t owner,const uint8_t* initialized,uint64_t generation)const{
  return context==&c && prefix==&m && cookie==fd && fd && hashes==h && h && team==owner && owner>=0
   && ledger==initialized && initialized && m.ledger==ledger && m.identity==identity && identity.Valid()
   && identity.driverGeneration==generation && generation && m.token==token && !token && !m.position && m.bank==bank && bank<7
   && m.phase==PioneerQ38EmbeddingNativeState::Complete && m.Closed() && CompletedInput(m) && !Unsafe();
 }
 void Fault(){unsafe=true;recurrentZero.unsafe=historyZero.unsafe=true;if(execution)execution->Fault();if(memory)memory->Fault();} // MEMORY ONLY;retain source/clock/hosts/maps
};
#endif
#endif
