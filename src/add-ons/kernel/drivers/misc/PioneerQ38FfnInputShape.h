/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_INPUT_SHAPE_H
#define PIONEER_Q38_FFN_INPUT_SHAPE_H
#include "PioneerQ38FfnResidentShape.h"
// NEW OFFLINE initial5120-lane quantizer DATA. Shape/tag is NOT producer,
// initialized-workspace,model or physical/lifetime ownership authority.
enum {TT_Q38_FFN_INPUT_MARKER=0x31494651,TT_Q38_FFN_INPUT_NATIVE=20480,
 TT_Q38_FFN_INPUT_SCALES=30720,TT_Q38_FFN_INPUT_BLOCKS=20};
static inline bool PioneerQ38FfnInputShape(const uint32_t* d)
{
 if(!d || d[0]!=TT_Q38_FFN_INPUT_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || d[5]>=20
  || !d[6] || d[6]>=0x7fffffff || d[7]!=d[5]+1 || d[8]!=PioneerQ38SpanRoute(d[3])
  || d[9]!=PioneerQ38FfnResidentPeer(d[5]) || d[10]!=d[5]*1024 || d[11]
  || d[12]!=TT_Q38_FFN_INPUT_NATIVE+d[5]*512 || d[13]!=TT_Q38_FFN_SCALE_LINES_OFFSET+d[5]*64
  || d[14]!=256 || d[15] || !d[16] || d[42]>=64)return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=43;i<64;i++)if(d[i])return false;
 return true;
}
static inline bool PioneerQ38FfnInputScale(const uint32_t* line,const uint32_t* d,unsigned block)
{
 // Producer OR gather context:caller separately validates its own frame.
 // The root's physical-source fields are not the producer's identity.
 if(!line || !d || block>=20)return false;
 uint32_t magnitude=line[0]&0x7fffffff;
 if((magnitude && (magnitude<0x2f800000 || magnitude>0x47800000)) || line[0]==0x80000000
  || line[1]!=TT_Q38_FFN_INPUT_MARKER || line[2]!=d[6] || line[3]!=block || line[4]!=d[42]
  || line[5]!=d[40] || line[6]!=d[41] || line[15])return false;
 for(unsigned i=0;i<8;i++)if(line[7+i]!=d[32+i])return false;
 return true; // DATA freshness ONLY,not a producer capability
}
#endif
