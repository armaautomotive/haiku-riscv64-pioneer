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
// A distinct retained NATIVE context, borrowing the genuine prefix. No
// public construction/copy, phase setter, readiness bit or destructor.
// Binding alone is CPU metadata, NEVER initialization/compute permission.
// The actual initializer/output factories are still required; the driver
// continues to reject this experimental role before compilation.
class PioneerQ38ResidentOutputNativeState {
 friend struct PioneerStoreContext;
 friend struct PioneerSessionIO;
 friend bool BindQ38ResidentOutputNativeState(PioneerStoreContext&);
 enum Phase {Bound,RecurrentZeroing,HistoryZeroing,Prefix,OutputPreparing,OutputRunning,OutputComplete,OutputClosed};
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
 bool Initialized(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof)const{
  if(!prefix)return false;
  const auto* g=prefix->graph.Recurrent(identity);const auto* h=prefix->graph.ConvHistory(identity);
  return g && h && prefix->plan.RecurrentCount()==672 && recurrentZero.saved && recurrentZero.closed
   && recurrentZero.nonce==672 && !recurrentZero.dirty && !recurrentZero.ready && !recurrentZero.running && !recurrentZero.unsafe
   && recurrentZero.identity==identity && g->InitializationPublished(identity)
   && g->MatchesInitializationOwner(banks,proof,identity)
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
 bool Closed(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof)const{
  return phase==OutputClosed && !Unsafe() && prefix->Closed() && Initialized(banks,proof)
   && memory && output.Closed() && memory->Closed();
 } // a CLOSED old prefix alone can NEVER authorize END of this new owner
 void Fault(){unsafe=true;recurrentZero.unsafe=historyZero.unsafe=true;if(memory)memory->Fault();}
 // MEMORY ONLY. Native retention separately quarantines the borrowed
 // graph/banks/clock/hosts and keeps this object, prefix and mappings alive.
};
#endif
