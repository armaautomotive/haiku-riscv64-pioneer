/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_QGW8_DENSE_STAGE_TRACE_H
#define PIONEER_Q38_QGW8_DENSE_STAGE_TRACE_H
#include "PioneerQ38Qgw8LayerTraceGeometry.h"

// Diagnostic data geometry only. These two slots replace the already-recorded
// layer-0 and layer-1 traces after layer 3 reaches its projection; all later
// layer traces must keep their existing destinations. Neither address nor
// stage number grants a launch, ownership, CPU validation, or Work mode.
namespace PioneerQ38Qgw8DenseStageTrace {
enum : uint32_t {Layer=3,Position=0,Projection=0,Residual=1,Stages=2};
static inline bool Describe(uint32_t workspace,uint32_t layer,uint32_t position,
 uint32_t stage,uint32_t& address){
 if(layer!=Layer || position!=Position || stage>=Stages)return false;
 PioneerQ38Qgw8LayerTraceGeometry::Chunk first={},last={};
 if(!PioneerQ38Qgw8LayerTraceGeometry::Describe(workspace,stage*2,first)
  || !PioneerQ38Qgw8LayerTraceGeometry::Describe(workspace,stage*2+1,last)
  || first.layer!=stage || first.part || last.layer!=stage || last.part!=1
  || first.address+first.words*4!=last.address)return false;
 address=first.address;return true;
}
static_assert(unsigned(Stages)<=unsigned(PioneerQ38Qgw8LayerTraceGeometry::Layers) && Layer>=Stages,
 "captured stages replace earlier layer slots, never their own or later slots");
}
#endif
