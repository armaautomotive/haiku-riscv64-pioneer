/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
#include "PioneerQ38MixedLiveNativeRole.h"
#ifndef PIONEER_Q38_MIXED_LIVE_NATIVE_STATE_H
#define PIONEER_Q38_MIXED_LIVE_NATIVE_STATE_H
#include "PioneerQ38EmbeddingNativeState.h"
#include "PioneerQ38MixedLiveMemory.h"
#include "PioneerQ38MixedLiveOwner.h"
#include "../../../../../tools/pioneer/PioneerQ38MixedLiveBlockPlan.h"
struct PioneerStoreContext;
struct PioneerSessionIO;
[[maybe_unused]] static bool BindQ38MixedLiveNative(PioneerStoreContext&);
[[maybe_unused]] static bool PrepareQ38MixedLiveNativeHeld(PioneerStoreContext&,PioneerQ38MixedLiveMemoryResult&);
[[maybe_unused]] static bool CloseQ38MixedLiveNativeHeld(PioneerStoreContext&,PioneerQ38MixedLiveMemoryResult&);
[[maybe_unused]] static bool PrepareQ38MixedLiveNativeExecution(PioneerStoreContext&,PioneerQ38MixedLiveResult&);
[[maybe_unused]] static bool RunQ38MixedLiveNativeExecution(PioneerStoreContext&,PioneerQ38MixedLiveResult&);
[[maybe_unused]] static bool CloseQ38MixedLiveNativeExecution(PioneerStoreContext&,PioneerQ38MixedLiveResult&,PioneerQ38MixedLiveMemoryResult&);
#ifdef PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK
[[maybe_unused]] static bool CloseQ38MixedLiveNativeValidation(PioneerStoreContext&,PioneerQ38MixedLiveResult&,PioneerQ38MixedLiveMemoryResult&,
 uint32_t*,unsigned,PioneerQ38MixedLiveReadbackResult&);
#endif
// Real retained context/source/cookie/team/thread/clock/host perimeter.
// Private RUN/STOP/ALL120-holds/restore controller integration is compiled
// only in the refusing NODRV qualification unit. No public execution entry,
// completed model/token publication, native runtime or full-model claim.
// The executor retains this same object; no caller "ready" flag exists.
class PioneerQ38MixedLiveNativeState {
 friend struct PioneerStoreContext;
 friend struct PioneerSessionIO;
 friend bool BindQ38MixedLiveNative(PioneerStoreContext&);
 friend bool PrepareQ38MixedLiveNativeHeld(PioneerStoreContext&,PioneerQ38MixedLiveMemoryResult&);
 friend bool CloseQ38MixedLiveNativeHeld(PioneerStoreContext&,PioneerQ38MixedLiveMemoryResult&);
 friend bool PrepareQ38MixedLiveNativeExecution(PioneerStoreContext&,PioneerQ38MixedLiveResult&);
 friend bool RunQ38MixedLiveNativeExecution(PioneerStoreContext&,PioneerQ38MixedLiveResult&);
 friend bool CloseQ38MixedLiveNativeExecution(PioneerStoreContext&,PioneerQ38MixedLiveResult&,PioneerQ38MixedLiveMemoryResult&);
#ifdef PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK
 friend bool CloseQ38MixedLiveNativeValidation(PioneerStoreContext&,PioneerQ38MixedLiveResult&,PioneerQ38MixedLiveMemoryResult&,
  uint32_t*,unsigned,PioneerQ38MixedLiveReadbackResult&);
#endif
 static bool CompletedInputMatches(const PioneerQ38EmbeddingNativeState& m){
  return m.embedding.MatchesCompleted(m.graph,m.plan,m.identity,m.token,m.position,m.bank);
 } // actual opaque producer proof; selector DATA alone cannot retarget it
 enum Phase {Bound,DispatchPreparing,MemoryPreparing,MemoryInstalled,ExecutionPreparing,ExecutionReady,ExecutionComplete,ExecutionClosing,MemoryClosing,MemoryClosed};
 PioneerStoreContext* const context;
 PioneerQ38EmbeddingNativeState* const prefix;
 const void* const cookie;
 const void* const hashes;
 const uint8_t* const ledger;
 const PioneerQ38GdnIdentity identity;
 const unsigned token,bank;
 const int64_t team;
 Phase phase=Bound;
 bool unsafe=false;
 PioneerQ38MixedLiveMemory* memory=nullptr;
 PioneerQ38MixedLiveBlockPlan* plan=nullptr;
 PioneerQ38MixedLiveOwner* execution=nullptr;
 PioneerQ38MixedLiveNativeState(PioneerStoreContext& c,PioneerQ38EmbeddingNativeState& m,const void* fd,const void* h,int64_t owner)
  :context(&c),prefix(&m),cookie(fd),hashes(h),ledger(m.ledger),identity(m.identity),token(m.token),bank(m.bank),team(owner){}
 ~PioneerQ38MixedLiveNativeState(){delete execution;delete memory;delete plan;} // genuine clean END only
public:
 PioneerQ38MixedLiveNativeState(const PioneerQ38MixedLiveNativeState&)=delete;
 PioneerQ38MixedLiveNativeState& operator=(const PioneerQ38MixedLiveNativeState&)=delete;
 bool Unsafe()const{return unsafe || !prefix || prefix->Unsafe() || (memory && memory->Unsafe()) || (execution && execution->Unsafe());}
 bool Closed()const{return phase==MemoryClosed && !Unsafe() && memory && memory->Closed() && plan && (!execution || execution->Closed());}
 bool ControlsCohort(const PioneerQ38DispatchLease* c)const{
  return c && memory && execution && plan && !Unsafe() && execution->MatchesCohort(c) && execution->MatchesNativeOwner(*memory,*plan);
 } // metadata only; never substitutes for the genuine FD/source/clock/host or physical guard
 bool MatchesBinding(const PioneerStoreContext& c,const PioneerQ38EmbeddingNativeState& m,const void* fd,const void* h,
  int64_t owner,const uint8_t* learned,uint64_t generation)const{
  return context==&c && prefix==&m && cookie==fd && fd && hashes==h && h && team==owner && owner>=0
   && ledger==learned && learned && m.ledger==ledger && m.identity==identity && identity.Valid()
   && identity.driverGeneration==generation && generation && m.token==token && token<248320 && !m.position
   && m.bank==bank && bank<7 && m.phase==PioneerQ38EmbeddingNativeState::Complete && m.Closed()
   && CompletedInputMatches(m) && !Unsafe();
 } // necessary binding only; authentic native scope and physical guards ALSO mandatory
 void Fault(){unsafe=true;if(memory)memory->Fault();} // MEMORY ONLY;retain all maps,source,clock,hosts
};
#endif
#endif
