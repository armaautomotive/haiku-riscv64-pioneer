/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_RMS_STATE_H
#define PIONEER_Q38_NEXT_RMS_STATE_H
#include "PioneerQ38NextRmsProducer.h"
#include "PioneerQ38LearnedRmsPlan.h"
#include "PioneerQ38LearnedRmsCode.h"
class PioneerQ38FfnResidualState;
// Private heap metadata only. SAME decoder retains its original full snapshot,
// all120 host/source/clock leases;no parallel RMS owner or host numeric stage.
class PioneerQ38NextRmsState {
 friend class PioneerQ38ResidentDecoderOwner;
 PioneerQ38LearnedRmsPlan plan;
 const PioneerQ38FfnHiddenProducer* input=nullptr;
 const PioneerQ38FfnResidualState* parent=nullptr;
 uint32_t before[8]={},accepted[32]={};
 unsigned epoch=0,layer=64;
 bool image=false,staged=false,receipt=false,done=false;
 PioneerQ38NextRmsState()=default;
public:
 PioneerQ38NextRmsState(const PioneerQ38NextRmsState&)=delete;
 PioneerQ38NextRmsState& operator=(const PioneerQ38NextRmsState&)=delete;
};
static_assert(sizeof(PioneerQ38NextRmsState)<1024,"next norm metadata only,not copied snapshot or CPU products");
#endif
