/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
#ifndef PIONEER_Q38_TOKEN_NATIVE_STATE_H
#define PIONEER_Q38_TOKEN_NATIVE_STATE_H
#include "PioneerQ38ProjectionMutableBanks.h"
#include "PioneerQ38TokenMemory.h"
struct PioneerStoreContext;struct PioneerSessionIO;
[[maybe_unused]] static bool BindQ38TokenNative(PioneerStoreContext&,unsigned);
// Genuine retained full851 model/reservation object. It borrows NO old
// completed embedding/prefix owner and cannot make those old lifetimes safe.
// Private factory publication occurs under the real global FD call lock.
class PioneerQ38TokenNativeState {
 friend struct PioneerStoreContext;friend struct PioneerSessionIO;
 friend bool BindQ38TokenNative(PioneerStoreContext&,unsigned);
 enum Phase {Claiming,Bound,Unknown};
 PioneerStoreContext* const context;const void* const cookie;const void* const hashes;
 const uint8_t* const ledger;const int64_t team;const unsigned bank;
 const PioneerQ38GdnIdentity identity;
 PioneerQ38CanonicalModelPlan canonical;
 PioneerQ38ProjectionMutableBanks graph;
 PioneerQ38TokenStartupPlan* startup=nullptr;
 PioneerQ38TokenMemory* memory=nullptr;
 Phase phase=Claiming;bool unsafe=false;
 PioneerQ38TokenNativeState(PioneerStoreContext& c,const void* fd,const void* h,const uint8_t* uploaded,int64_t owner,
  unsigned selected,const PioneerQ38GdnIdentity& id):context(&c),cookie(fd),hashes(h),ledger(uploaded),team(owner),bank(selected),identity(id){}
 ~PioneerQ38TokenNativeState(){delete memory;delete startup;} // known-clean native destruction ONLY
public:
 PioneerQ38TokenNativeState(const PioneerQ38TokenNativeState&)=delete;
 PioneerQ38TokenNativeState& operator=(const PioneerQ38TokenNativeState&)=delete;
 bool Unsafe()const{return unsafe || phase==Unknown || graph.Unsafe() || (memory && memory->Unsafe());}
 bool Closed()const{return false;} // NO full-token native END capability has been implemented yet
 bool MatchesBinding(const PioneerStoreContext& c,const void* fd,const void* h,const uint8_t* uploaded,int64_t owner,uint64_t generation)const{
  return context==&c && cookie==fd && fd && hashes==h && h && ledger==uploaded && uploaded && team==owner && owner>=0
   && identity.Valid() && generation && identity.driverGeneration==generation && bank<7 && canonical.Ready() && !Unsafe();
 }
 bool MatchesReservation(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof)const{
  return !Unsafe() && graph.CanonicalModel(identity)==&canonical && graph.MatchesInitializationOwner(banks,proof,identity);
 } // reservation binding ONLY,not initialized state,source authentication or RUN
 void Fault(){unsafe=true;phase=Unknown;graph.Fault();if(memory)memory->Fault();} // MEMORY ONLY;retain every borrowed resource
};
static_assert(sizeof(PioneerQ38TokenNativeState)>262144 && sizeof(PioneerQ38TokenNativeState)<1048576,
 "complete canonical+runtime reservation model is HEAP ONLY");
#endif
#endif
