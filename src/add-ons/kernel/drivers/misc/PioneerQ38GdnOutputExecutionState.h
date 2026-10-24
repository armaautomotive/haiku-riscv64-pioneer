/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_OUTPUT_EXECUTION_STATE_H
#define PIONEER_Q38_GDN_OUTPUT_EXECUTION_STATE_H
#include "PioneerQ38GdnOutputBootstrapPlan.h"
#include "PioneerQ38GdnOutputGraphProof.h"
#include "PioneerQ38GdnOutputImages.h"
// Private HEAP block retained by SAME real decoder on UNKNOWN. No caller
// can set a producer/ready/phase,replace a manifest or admit a live epoch.
class PioneerQ38GdnOutputExecutionState {
 friend class PioneerQ38ResidentDecoderOwner;
 PioneerQ38GdnOutputProjectionPlan jobs;
 PioneerQ38GdnOutputBootstrapPlan bootstrap;
 uint32_t sequence[119]={},before[8]={},accepted[64]={};
 bool image[120]={},bootPrepared=false,rootProof=false,stopping=false,done=false,stopped=false;
 PioneerQ38GdnOutputExecutionState()=default;
public:
 PioneerQ38GdnOutputExecutionState(const PioneerQ38GdnOutputExecutionState&)=delete;
 PioneerQ38GdnOutputExecutionState& operator=(const PioneerQ38GdnOutputExecutionState&)=delete;
};
static_assert(sizeof(PioneerQ38GdnOutputExecutionState)>65536 && sizeof(PioneerQ38GdnOutputExecutionState)<81920,"canonical state HEAP ONLY");
#endif
