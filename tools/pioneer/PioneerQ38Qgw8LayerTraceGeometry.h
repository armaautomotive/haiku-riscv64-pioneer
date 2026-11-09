/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_QGW8_LAYER_TRACE_GEOMETRY_H
#define PIONEER_Q38_QGW8_LAYER_TRACE_GEOMETRY_H
#ifdef PIONEER_ATTENTION_FREESTANDING
typedef __UINT32_TYPE__ uint32_t;
#else
#include <stdint.h>
#endif
// Shared address geometry ONLY. It allocates nothing, reads nothing, and
// grants neither GPU ownership nor permission to publish a snapshot.
namespace PioneerQ38Qgw8LayerTraceGeometry {
enum : uint32_t {Offset=0x3500000,Layers=64,Words=5120,Bytes=Words*4,
 Parts=2,ChunkBytes=16384,Chunks=Layers*Parts,WorkspaceBytes=0x4000000};
struct Chunk {uint32_t layer,part,firstWord,words,address;};
static inline bool Describe(uint32_t workspace,uint32_t ordinal,Chunk& output){
 if(workspace<0x04000000 || workspace>0xc0000000 || (workspace&4095) || ordinal>=Chunks)return false;
 const uint32_t layer=ordinal/Parts,part=ordinal%Parts,first=part*ChunkBytes;
 Chunk next={layer,part,first/4,(part?Bytes-ChunkBytes:ChunkBytes)/4,
  workspace+Offset+layer*Bytes+first};
 if((next.address&63) || next.address<workspace || next.words*4>WorkspaceBytes
  || next.address-workspace>WorkspaceBytes-next.words*4)return false;
 output=next;return true;
}
static_assert(Offset+Layers*Bytes<=WorkspaceBytes && (Offset&4095)==0 && (Bytes&4095)==0,
 "trace chunks fit the proposed workspace geometry");
}
#endif
