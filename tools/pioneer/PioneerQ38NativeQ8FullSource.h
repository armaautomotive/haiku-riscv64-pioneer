/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_Q8_FULL_SOURCE_H
#define PIONEER_Q38_NATIVE_Q8_FULL_SOURCE_H
#include "PioneerQ38NativeQ8Data.h"
#include "PioneerQwen38PackedPlan.h"

// Applies to the exact authenticated public Qwen 27B plan only. The type
// AND semantic role must agree: no embedding, output head, state or Q4/Q6
// data may be converted by virtue of a coincidental tile size.
namespace PioneerQ38NativeQ8FullSource {
enum class Format { Invalid, Raw, SignedMagnitude };
static inline bool ProjectionRole(unsigned layer,unsigned role){
 if(layer>=64)return false;
 return layer%4==3?role>=5 && role<=7:role==5 || role==6 || role==10 || role==11 || role==13;
}
static inline Format Layout(const PioneerQ38PackedPart& p,const PioneerQ38Tensor& t){
 if(p.kind!=PIONEER_Q38_WEIGHT || p.layer>64 || !p.rows || !p.bytes || p.bytes>p.allocated
  || uint64_t(p.rows)*p.rowBytes!=p.bytes || (p.allocated&4095) || p.allocated-p.bytes>=4096
  || p.firstRow>=t.outputs || p.rows>t.outputs-p.firstRow)return Format::Invalid;
 bool projection=ProjectionRole(p.layer,p.role);
 if((t.type==8)!=projection)return Format::Invalid;
 if(!projection)return Format::Raw;
 if(t.rank!=2 || (t.inputs!=5120 && t.inputs!=6144) || ((p.rows|p.firstRow)&15)
  || p.rowBytes!=PioneerQ38TiledRowBytes(t.type,t.inputs) || p.bytes%PioneerQ38NativeQ8Data::TileBytes)return Format::Invalid;
 return Format::SignedMagnitude;
}
static inline bool Prepare(const PioneerQ38PackedPart& p,unsigned page,const uint8_t* raw,uint8_t* out,unsigned bytes){
 uint64_t offset=uint64_t(page)*PioneerQ38NativeQ8Data::PageBytes;
 if(offset>=p.bytes)return false;
 const unsigned used=unsigned(std::min<uint64_t>(p.bytes-offset,PioneerQ38NativeQ8Data::PageBytes));
 return PioneerQ38NativeQ8Data::PreparePage(raw,offset,used,out,bytes);
}
}
#endif
