/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_CONV_STATE_H
#define PIONEER_Q38_NEXT_CONV_STATE_H
#include "PioneerQ38NextConvProducer.h"
#include "PioneerQ38ConvBatchPlan.h"
class PioneerQ38NextProjectionState;
// HEAP metadata only;SAME decoder retains original snapshot/ALL leases.
class PioneerQ38NextConvState {
 friend class PioneerQ38ResidentDecoderOwner;
 PioneerQ38ConvBatchPlan jobs[120];
 const PioneerQ38NextProjectionState* parent=nullptr;
 const PioneerQ38NextProjectionProducer* input=nullptr;
 uint32_t before[120][8]={},accepted[120][32]={};
 unsigned layer=64,epoch=0;
 bool image[120]={},staged[120]={},receipt[120]={},built=false,done=false;
 PioneerQ38NextConvState()=default;
public:
 PioneerQ38NextConvState(const PioneerQ38NextConvState&)=delete;
 PioneerQ38NextConvState& operator=(const PioneerQ38NextConvState&)=delete;
};
static_assert(sizeof(PioneerQ38NextConvState)>393216 && sizeof(PioneerQ38NextConvState)<524288,"ALL120 recipes/private receipts on HEAP,not copied snapshots/CPU products");
#endif
