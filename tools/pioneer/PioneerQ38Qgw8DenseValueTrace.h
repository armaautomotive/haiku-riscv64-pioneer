/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_QGW8_DENSE_VALUE_TRACE_H
#define PIONEER_Q38_QGW8_DENSE_VALUE_TRACE_H
#include "PioneerQ38Qgw8LayerTraceGeometry.h"
// Diagnostic layer-3/position-0 DATA only: 1024 raw F32 value projection
// words, followed by 512 words containing the four packed-F16 value groups.
// Slot 0 is replaced; slots 1-63 retain their ordinary layer trace data.
namespace PioneerQ38Qgw8DenseValueTrace {
enum : uint32_t {Layer=3,Position=0,RawBytes=4096,PackedBytes=2048,Bytes=RawBytes+PackedBytes};
static inline bool Describe(uint32_t workspace,uint32_t layer,uint32_t position,uint32_t& address){
 if(layer!=Layer || position!=Position)return false;
 PioneerQ38Qgw8LayerTraceGeometry::Chunk first={},next={};
 if(!PioneerQ38Qgw8LayerTraceGeometry::Describe(workspace,0,first)
  || !PioneerQ38Qgw8LayerTraceGeometry::Describe(workspace,2,next)
  || first.layer || first.part || next.layer!=1 || next.part
  || first.address+Bytes>next.address)return false;
 address=first.address;return true;
}
static_assert(Bytes==6144 && uint32_t(Bytes)<uint32_t(PioneerQ38Qgw8LayerTraceGeometry::Bytes),
 "value snapshot must fit inside replaced layer-0 slot");
}
#endif
