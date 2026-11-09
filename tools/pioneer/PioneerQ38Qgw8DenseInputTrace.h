/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_QGW8_DENSE_INPUT_TRACE_H
#define PIONEER_Q38_QGW8_DENSE_INPUT_TRACE_H
#include "PioneerQ38Qgw8LayerTraceGeometry.h"
// Diagnostic geometry only: a single 6144-float vector replaces slot 0
// and the first 4096 bytes of slot 1. Layer 2 onward is not overwritten.
namespace PioneerQ38Qgw8DenseInputTrace {
enum : uint32_t {Layer=3,Position=0,Words=6144,Bytes=Words*4};
static inline bool Describe(uint32_t workspace,uint32_t layer,uint32_t position,uint32_t& address){
 if(layer!=Layer || position!=Position)return false;
 PioneerQ38Qgw8LayerTraceGeometry::Chunk first={},next={},limit={};
 if(!PioneerQ38Qgw8LayerTraceGeometry::Describe(workspace,0,first)
  || !PioneerQ38Qgw8LayerTraceGeometry::Describe(workspace,1,next)
  || !PioneerQ38Qgw8LayerTraceGeometry::Describe(workspace,2,limit)
  || first.layer || first.part || next.layer || next.part!=1 || limit.layer!=1 || limit.part
  || first.address+first.words*4!=next.address || next.address+next.words*4!=limit.address
  || first.address+Bytes>limit.address+PioneerQ38Qgw8LayerTraceGeometry::Bytes)return false;
 address=first.address;return true;
}
static_assert(Bytes==24576 && Bytes<2*PioneerQ38Qgw8LayerTraceGeometry::Bytes,
 "input snapshot must fit in replaced layer-0 and layer-1 slots");
}
#endif
