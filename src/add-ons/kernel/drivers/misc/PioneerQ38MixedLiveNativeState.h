/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
#include "PioneerQ38MixedLiveNativeRole.h"
#ifndef PIONEER_Q38_MIXED_LIVE_NATIVE_STATE_H
#define PIONEER_Q38_MIXED_LIVE_NATIVE_STATE_H
#include "PioneerQ38EmbeddingNativeState.h"
#include "PioneerQ38MixedLiveMemory.h"
#include "../../../../../tools/pioneer/PioneerQ38MixedLiveBlockPlan.h"
struct PioneerStoreContext;
struct PioneerSessionIO;
[[maybe_unused]] static bool BindQ38MixedLiveNative(PioneerStoreContext&);
[[maybe_unused]] static bool PrepareQ38MixedLiveNativeHeld(PioneerStoreContext&,PioneerQ38MixedLiveMemoryResult&);
[[maybe_unused]] static bool CloseQ38MixedLiveNativeHeld(PioneerStoreContext&,PioneerQ38MixedLiveMemoryResult&);
// Real retained context/source/cookie/team/thread/clock/host perimeter.
// This initial integration is HELD ONLY. It supplies no execution entry,
// completed-block/token publication, public opcode or full-model claim.
// The future executor must own the same object through genuine RUN/STOP,
// ALL120 holds and restore/END; it cannot set a caller "ready" flag here.
class PioneerQ38MixedLiveNativeState {
 friend struct PioneerStoreContext;
 friend struct PioneerSessionIO;
 friend bool BindQ38MixedLiveNative(PioneerStoreContext&);
 friend bool PrepareQ38MixedLiveNativeHeld(PioneerStoreContext&,PioneerQ38MixedLiveMemoryResult&);
 friend bool CloseQ38MixedLiveNativeHeld(PioneerStoreContext&,PioneerQ38MixedLiveMemoryResult&);
 enum Phase {Bound,DispatchPreparing,MemoryPreparing,MemoryInstalled,MemoryClosing,MemoryClosed};
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
 PioneerQ38MixedLiveNativeState(PioneerStoreContext& c,PioneerQ38EmbeddingNativeState& m,const void* fd,const void* h,int64_t owner)
  :context(&c),prefix(&m),cookie(fd),hashes(h),ledger(m.ledger),identity(m.identity),token(m.token),bank(m.bank),team(owner){}
 ~PioneerQ38MixedLiveNativeState(){delete memory;delete plan;} // genuine clean END only
public:
 PioneerQ38MixedLiveNativeState(const PioneerQ38MixedLiveNativeState&)=delete;
 PioneerQ38MixedLiveNativeState& operator=(const PioneerQ38MixedLiveNativeState&)=delete;
 bool Unsafe()const{return unsafe || !prefix || prefix->Unsafe() || (memory && memory->Unsafe());}
 bool Closed()const{return phase==MemoryClosed && !Unsafe() && memory && memory->Closed() && plan;}
 bool MatchesBinding(const PioneerStoreContext& c,const PioneerQ38EmbeddingNativeState& m,const void* fd,const void* h,
  int64_t owner,const uint8_t* learned,uint64_t generation)const{
  return context==&c && prefix==&m && cookie==fd && fd && hashes==h && h && team==owner && owner>=0
   && ledger==learned && learned && m.ledger==ledger && m.identity==identity && identity.Valid()
   && identity.driverGeneration==generation && generation && m.token==token && token<248320 && !m.position
   && m.bank==bank && bank<7 && m.phase==PioneerQ38EmbeddingNativeState::Complete && m.Closed() && !Unsafe();
 } // necessary binding only; authentic native scope and physical guards ALSO mandatory
 void Fault(){unsafe=true;if(memory)memory->Fault();} // MEMORY ONLY;retain all maps,source,clock,hosts
};
#endif
#endif
