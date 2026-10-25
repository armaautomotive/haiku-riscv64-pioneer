/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_FFN_RESIDUAL_STATE_H
#define PIONEER_Q38_NEXT_FFN_RESIDUAL_STATE_H
#include "PioneerQ38NextFfnHiddenProducer.h"
#include "PioneerQ38FfnCanonicalJobs.h"
#include "PioneerQ38FfnResidualGraphProof.h"
#include "PioneerQ38FfnResidualGraphImages.h"
#include "PioneerQ38HostDeadline.h"
class PioneerQ38NextResidualRmsState;
// SAME original snapshot/host/clock owner. Only private HEAP recipes and
// receipts;no host activation staging or caller-provided numeric products.
class PioneerQ38NextFfnResidualState {
 friend class PioneerQ38ResidentDecoderOwner;
 PioneerQ38FfnCanonicalJobs jobs;
 const PioneerQ38NextFfnProducer* input=nullptr;
 const PioneerQ38NextResidualRmsState* parent=nullptr;
 uint32_t manifest[TT_Q38_FFN_RESIDUAL_GRAPH_WORDS]={},sources[119*16]={},sequence[119]={},before[8]={},accepted[64]={};
 bool image[120]={},built=false,bootPrepared=false,rootProof=false,stopping=false,done=false,stopped=false,timing=false,uncertain=false;
 bool rootHeld=false,held[120]={},stopReceipt[120]={};
 unsigned epoch=0;PioneerQ38HostDeadline deadline;
 PioneerQ38NextFfnResidualState()=default;
public:
 PioneerQ38NextFfnResidualState(const PioneerQ38NextFfnResidualState&)=delete;
 PioneerQ38NextFfnResidualState& operator=(const PioneerQ38NextFfnResidualState&)=delete;
};
static_assert(sizeof(PioneerQ38NextFfnResidualState)>24576 && sizeof(PioneerQ38NextFfnResidualState)<40960,"complete layer1 FFN recipes HEAP ONLY,not numeric host staging");
#endif
