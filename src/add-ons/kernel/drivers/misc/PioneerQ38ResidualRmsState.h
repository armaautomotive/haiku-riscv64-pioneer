/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDUAL_RMS_STATE_H
#define PIONEER_Q38_RESIDUAL_RMS_STATE_H
#include "PioneerQ38PostAttentionProducer.h"
#include "PioneerQ38ResidualRmsPlan.h"
#include "PioneerQ38ResidualRmsCode.h"
class PioneerQ38GdnOutputExecutionState;
// Private HEAP state,retained with SAME real decoder on UNKNOWN.
class PioneerQ38ResidualRmsState {
 friend class PioneerQ38ResidentDecoderOwner;
 PioneerQ38ResidualRmsPlan plan;
 const PioneerQ38GdnOutputExecutionState* parent=nullptr;
 const PioneerQ38LearnedRmsController* raw=nullptr;
 uint32_t accepted[32]={},before[8]={};
 unsigned epoch=0;
 bool image=false,staged=false,receipt=false,done=false;
 PioneerQ38ResidualRmsState()=default;
public:
 PioneerQ38ResidualRmsState(const PioneerQ38ResidualRmsState&)=delete;
 PioneerQ38ResidualRmsState& operator=(const PioneerQ38ResidualRmsState&)=delete;
};
static_assert(sizeof(PioneerQ38ResidualRmsState)<1024,"private retained heap DATA,not stack numeric staging");
#endif
