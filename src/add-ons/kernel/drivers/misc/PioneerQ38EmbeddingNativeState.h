/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EMBEDDING_NATIVE_STATE_H
#define PIONEER_Q38_EMBEDDING_NATIVE_STATE_H
#include "PioneerQ38EmbeddingController.h"
#include "PioneerQ38ProjectionZeroController.h"
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
#include "PioneerQ38LearnedRmsController.h"
#endif
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
#include "PioneerQ38ProjectionGraphOwner.h"
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
#include "PioneerQ38ResidentDecoderOwner.h"
#include "PioneerQ38ConvHistoryZeroController.h"
#endif
// Same native context owns these ACTUAL objects,never copied descriptors.
// No public opcode/selector exists. Only the separately gated native
// factory may publish this heap object under the real global call lock.
class PioneerQ38EmbeddingNativeState {
public:
 enum Phase {Bound=0,Zeroing=1,Embedding=2,Complete=3
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
 ,Rms=4,RmsComplete=5
#endif
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
 ,ProjectionPreparing=6,Projection=7,ProjectionComplete=8
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
 ,HistoryZeroing=9,Decoder=10,DecoderHeld=11,DecoderComplete=12
#endif
 };
 PioneerQ38CanonicalModelPlan plan;
 PioneerQ38ProjectionMutableBanks graph;
 PioneerQ38ProjectionZeroLease zero;
 PioneerQ38EmbeddingController embedding;
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
 PioneerQ38LearnedRmsController rms;
#endif
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
 PioneerQ38ProjectionGraphMemory projectionMemory;
 PioneerQ38ProjectionGraphOwner projection;
 PioneerQ38DispatchLease* projectionCohort=nullptr; // borrowed from SAME actual native context;never deleted here
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
 PioneerQ38ConvHistoryZeroLease historyZero;
 PioneerQ38ResidentDecoderOwner decoder;
 bool MatchesInitializedHistory(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof)const{
  const auto* h=graph.ConvHistory(identity);
  return h && historyZero.saved && historyZero.closed && historyZero.nonce==336 && !historyZero.unsafe && !historyZero.running
   && !historyZero.dirty && !historyZero.ready && historyZero.owner==h && historyZero.banks==&banks && historyZero.proof==&proof
   && historyZero.identity==identity && h->MatchesInitializationOwner(banks,proof,identity) && h->InitializationPublished(identity)
   && !h->OwnsInitializationLease(&historyZero);
 } // necessary CPU binding/zero-CLOSE seal ONLY,not native current-state/input permission
#endif
 PioneerQ38ProjectionIdentity identity={};
 const uint8_t* ledger=nullptr;
 unsigned token=UINT32_MAX,position=UINT32_MAX,bank=7;
 Phase phase=Bound;
 PioneerQ38EmbeddingNativeState()=default;
 PioneerQ38EmbeddingNativeState(const PioneerQ38EmbeddingNativeState&)=delete;
 PioneerQ38EmbeddingNativeState& operator=(const PioneerQ38EmbeddingNativeState&)=delete;
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
 bool Unsafe()const{return graph.Unsafe() || zero.unsafe || embedding.Unsafe() || rms.Unsafe() || projection.Unsafe() || projectionMemory.Unsafe()
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
  || historyZero.unsafe || decoder.Unsafe()
#endif
  || (projectionCohort && projectionCohort->unsafe);}
 bool Closed()const{return !Unsafe() && zero.closed && embedding.Completed()
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
  && historyZero.closed && historyZero.saved && !historyZero.ready && !historyZero.running && !historyZero.dirty
#endif
  && ((phase==Complete && !rms.Claimed() && !projectionCohort && !projection.Claimed())
   || (phase==RmsComplete && rms.Completed() && !projectionCohort && !projection.Claimed())
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
   || (phase==DecoderComplete && rms.Completed() && projectionCohort && projection.Closed() && projectionMemory.Closed() && decoder.Closed()))
  && ((phase==DecoderComplete && decoder.Claimed()) || !decoder.Claimed())
#else
   || (phase==ProjectionComplete && rms.Completed() && projectionCohort && projection.Closed() && projectionMemory.Closed()))
#endif
  ;}
 void Fault(){graph.Fault();zero.unsafe=true;embedding.Fault();rms.Fault();projection.Fault();projectionMemory.Fault();
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
  historyZero.unsafe=true;
#endif
 } // MEMORY ONLY
 bool ControlsProjectionCohort(const tt_dram_samples_result& dram,const PioneerResidentBanks& banks,const PioneerQ38DispatchLease* cohort,
  const uint8_t* initialized,unsigned count)const{
  if(!cohort || cohort!=projectionCohort || Unsafe() || ledger!=initialized || !initialized || count!=PioneerResidentBanks::Pages
   || (phase!=ProjectionPreparing && phase!=Projection
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
    && phase!=Decoder && phase!=DecoderHeld
#endif
    ) || cohort->prepared>120 || cohort->unsafe || cohort->closed
   || cohort->bootAttempted || cohort->booted)return false;
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
  if(!MatchesInitializedHistory(banks,dram))return false;
  if(phase==Decoder || phase==DecoderHeld){
   const auto* h=graph.ConvHistory(identity);
   return h && decoder.MatchesNativeOwner(projection,*h)
    && projection.MatchesNativeOwner(dram,banks,*cohort,projectionMemory,graph,plan,rms,identity,initialized,count);
  }
  if(decoder.Claimed())return false;
#endif
  if(projection.Claimed())return projection.MatchesNativeOwner(dram,banks,*cohort,projectionMemory,graph,plan,rms,identity,initialized,count);
  return phase==ProjectionPreparing && !cohort->rootRunning && !PioneerQ38DispatchUnsafe(*cohort);
 } // CPU-only necessary binding,NOT a native scope/physical permission
#elif defined(PIONEER_Q38_LEARNED_RMS_NATIVE)
 bool Unsafe()const{return graph.Unsafe() || zero.unsafe || embedding.Unsafe() || rms.Unsafe();}
 bool Closed()const{return !Unsafe() && zero.closed && embedding.Completed()
  && ((phase==Complete && !rms.Claimed()) || (phase==RmsComplete && rms.Completed()));}
 void Fault(){graph.Fault();zero.unsafe=true;embedding.Fault();rms.Fault();} // MEMORY ONLY
#else
 bool Unsafe()const{return graph.Unsafe() || zero.unsafe || embedding.Unsafe();}
 bool Closed()const{return phase==Complete && !Unsafe() && zero.closed && embedding.Completed();}
 void Fault(){graph.Fault();zero.unsafe=true;embedding.Fault();} // MEMORY ONLY
#endif
};
#endif
