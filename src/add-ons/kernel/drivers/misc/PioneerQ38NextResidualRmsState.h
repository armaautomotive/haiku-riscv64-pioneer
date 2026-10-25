/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_RESIDUAL_RMS_STATE_H
#define PIONEER_Q38_NEXT_RESIDUAL_RMS_STATE_H
#include "PioneerQ38NextFfnProducer.h"
#include "PioneerQ38NextResidualRmsPlan.h"
#include "PioneerQ38NextResidualRmsCode.h"
#include "PioneerQ38HostDeadline.h"
class PioneerQ38NextGdnOutputState;
// Retained private HEAP recipes/receipts only;no extra snapshot/products.
class PioneerQ38NextResidualRmsState {
 friend class PioneerQ38ResidentDecoderOwner;
 PioneerQ38NextResidualRmsPlan plan;
 const PioneerQ38NextGdnOutputState* parent=nullptr;const PioneerQ38NextGdnOutputProducer* input=nullptr;
 uint32_t accepted[32]={},before[8]={};unsigned epoch=0;
 bool image=false,staged=false,receipt=false,held=false,done=false,timing=false;
 PioneerQ38HostDeadline deadline;
 PioneerQ38NextResidualRmsState()=default;
public:
 PioneerQ38NextResidualRmsState(const PioneerQ38NextResidualRmsState&)=delete;
 PioneerQ38NextResidualRmsState& operator=(const PioneerQ38NextResidualRmsState&)=delete;
};
static_assert(sizeof(PioneerQ38NextResidualRmsState)<1024,"private retained heap DATA,not stack numerical staging");
#endif
