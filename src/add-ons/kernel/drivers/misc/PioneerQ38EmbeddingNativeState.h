/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EMBEDDING_NATIVE_STATE_H
#define PIONEER_Q38_EMBEDDING_NATIVE_STATE_H
#include "PioneerQ38EmbeddingController.h"
#include "PioneerQ38ProjectionZeroController.h"
// Same native context owns these ACTUAL objects,never copied descriptors.
// No public opcode/selector exists. Only the separately gated native
// factory may publish this heap object under the real global call lock.
class PioneerQ38EmbeddingNativeState {
public:
 enum Phase {Bound=0,Zeroing=1,Embedding=2,Complete=3};
 PioneerQ38CanonicalModelPlan plan;
 PioneerQ38ProjectionMutableBanks graph;
 PioneerQ38ProjectionZeroLease zero;
 PioneerQ38EmbeddingController embedding;
 PioneerQ38ProjectionIdentity identity={};
 const uint8_t* ledger=nullptr;
 unsigned token=UINT32_MAX,position=UINT32_MAX,bank=7;
 Phase phase=Bound;
 PioneerQ38EmbeddingNativeState()=default;
 PioneerQ38EmbeddingNativeState(const PioneerQ38EmbeddingNativeState&)=delete;
 PioneerQ38EmbeddingNativeState& operator=(const PioneerQ38EmbeddingNativeState&)=delete;
 bool Unsafe()const{return graph.Unsafe() || zero.unsafe || embedding.Unsafe();}
 bool Closed()const{return phase==Complete && !Unsafe() && zero.closed && embedding.Completed();}
 void Fault(){graph.Fault();zero.unsafe=true;embedding.Fault();} // MEMORY ONLY
};
#endif
