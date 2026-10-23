/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DECODER_CONV_BRIDGE_H
#define PIONEER_Q38_DECODER_CONV_BRIDGE_H
#include "pioneer_q38_projection_execution_machine.h"
class PioneerQ38CanonicalModelPlan;class ProjectionExecutionProducer;
// TEST emitted-instruction bridge ONLY. Reads SAME actual producer's GDDR
// bytes and the actual observed history-zero output;no numerical input or
// fabricated completion/producer capability accepted.
unsigned ExecuteDecoderConv(const char* image,const PioneerQ38CanonicalModelPlan&,unsigned worker,unsigned bank,
 ProjectionExecutionFabric&,ProjectionExecutionProducer&);
#endif
