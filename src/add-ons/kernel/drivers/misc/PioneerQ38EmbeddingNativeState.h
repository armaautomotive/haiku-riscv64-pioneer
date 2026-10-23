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
 PioneerQ38ProjectionIdentity identity={};
 const uint8_t* ledger=nullptr;
 unsigned token=UINT32_MAX,position=UINT32_MAX,bank=7;
 Phase phase=Bound;
 PioneerQ38EmbeddingNativeState()=default;
 PioneerQ38EmbeddingNativeState(const PioneerQ38EmbeddingNativeState&)=delete;
 PioneerQ38EmbeddingNativeState& operator=(const PioneerQ38EmbeddingNativeState&)=delete;
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
 bool Unsafe()const{return graph.Unsafe() || zero.unsafe || embedding.Unsafe() || rms.Unsafe() || projection.Unsafe() || projectionMemory.Unsafe()
  || (projectionCohort && projectionCohort->unsafe);}
 bool Closed()const{return !Unsafe() && zero.closed && embedding.Completed()
  && ((phase==Complete && !rms.Claimed() && !projectionCohort && !projection.Claimed())
   || (phase==RmsComplete && rms.Completed() && !projectionCohort && !projection.Claimed())
   || (phase==ProjectionComplete && rms.Completed() && projectionCohort && projection.Closed() && projectionMemory.Closed()));}
 void Fault(){graph.Fault();zero.unsafe=true;embedding.Fault();rms.Fault();projection.Fault();projectionMemory.Fault();} // MEMORY ONLY
 bool ControlsProjectionCohort(const tt_dram_samples_result& dram,const PioneerResidentBanks& banks,const PioneerQ38DispatchLease* cohort,
  const uint8_t* initialized,unsigned count)const{
  if(!cohort || cohort!=projectionCohort || Unsafe() || ledger!=initialized || !initialized || count!=PioneerResidentBanks::Pages
   || (phase!=ProjectionPreparing && phase!=Projection) || cohort->prepared>120 || cohort->unsafe || cohort->closed
   || cohort->bootAttempted || cohort->booted)return false;
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
