/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_RESIDUAL_STATE_H
#define PIONEER_Q38_FFN_RESIDUAL_STATE_H
#include "PioneerQ38FfnHiddenProducer.h"
#include "PioneerQ38FfnCanonicalJobs.h"
#include "PioneerQ38FfnResidualGraphProof.h"
#include "PioneerQ38FfnResidualGraphImages.h"
class PioneerQ38ResidualRmsState;
// HEAP ONLY,private and retained on UNKNOWN. No parallel code/mutable/
// source owner or copied snapshot:the SAME decoder owns all original state.
class PioneerQ38FfnResidualState {
 friend class PioneerQ38ResidentDecoderOwner;
 PioneerQ38FfnCanonicalJobs jobs;
 const PioneerQ38PostAttentionProducer* input=nullptr;
 const PioneerQ38ResidualRmsState* parent=nullptr;
 uint32_t manifest[TT_Q38_FFN_RESIDUAL_GRAPH_WORDS]={},sources[119*16]={},sequence[119]={},before[8]={},accepted[64]={};
 bool image[120]={},built=false,bootPrepared=false,rootProof=false,stopping=false,done=false,stopped=false;
 unsigned epoch=0;
 PioneerQ38FfnResidualState()=default;
public:
 PioneerQ38FfnResidualState(const PioneerQ38FfnResidualState&)=delete;
 PioneerQ38FfnResidualState& operator=(const PioneerQ38FfnResidualState&)=delete;
};
static_assert(sizeof(PioneerQ38FfnResidualState)>24576 && sizeof(PioneerQ38FfnResidualState)<40960,"complete FFN state HEAP ONLY,not numeric host staging");
#endif
