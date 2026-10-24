/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_PROJECTION_STATE_H
#define PIONEER_Q38_NEXT_PROJECTION_STATE_H
#include "PioneerQ38NextProjectionProducer.h"
#include "PioneerQ38NextProjectionGraphPlan.h"
#include "PioneerQ38NextProjectionGraphProof.h"
#include "PioneerQ38NextProjectionGraphImages.h"
class PioneerQ38NextRmsState;
class PioneerQ38NextProjectionState {
 friend class PioneerQ38ResidentDecoderOwner;
 PioneerQ38ProjectionJobs jobs[4];
 PioneerQ38NextProjectionGraphPlan* graph=nullptr;
 const PioneerQ38NextRmsState* parent=nullptr;
 const PioneerQ38NextRmsProducer* input=nullptr;
 uint32_t sources[119][16]={},sequence[119]={},before[8]={},accepted[64]={};
 unsigned epoch=0,layer=64,slot=0,produced=0;
 bool image[120]={},built=false,bootPrepared=false,rootProof=false,stopping=false,stopped=false;
 PioneerQ38NextProjectionState()=default;
public:
 ~PioneerQ38NextProjectionState(){delete graph;}
 PioneerQ38NextProjectionState(const PioneerQ38NextProjectionState&)=delete;
 PioneerQ38NextProjectionState& operator=(const PioneerQ38NextProjectionState&)=delete;
};
static_assert(sizeof(PioneerQ38NextProjectionState)>98304 && sizeof(PioneerQ38NextProjectionState)<131072,"private HEAP metadata/jobs only;no copied snapshot or CPU products");
#endif
