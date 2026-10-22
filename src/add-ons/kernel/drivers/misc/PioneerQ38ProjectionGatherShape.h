/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_GATHER_SHAPE_H
#define PIONEER_Q38_PROJECTION_GATHER_SHAPE_H
#include "PioneerQ38ProjectionInputShape.h"
// OFFLINE ROOT data ONLY. Actual producer completions/physical counters,
// canonical source/initialized pages and exclusive owners MUST be proved by
// a new native coordinator BEFORE entry. Old QFS2/root views grant NONE.
enum {TT_Q38_PROJECTION_GATHER_MARKER=0x31475051,TT_Q38_PROJECTION_GATHER_FAULT=0x46475051,
 TT_Q38_PROJECTION_COMPACT_OFFSET=212992};
static inline bool PioneerQ38ProjectionGatherShape(const uint32_t* d){
 if(!d)return false;
 unsigned t=0,k=0,n=0;
 if(!PioneerQ38ProjectionGeometry(d[42],d[43],t,k,n)
  || d[0]!=TT_Q38_PROJECTION_GATHER_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || d[5]!=k/256
  || !d[6] || d[6]>=0x7fffffff || d[7] || d[8]!=PioneerQ38ProjectionRoute(d[3]) || d[9]!=129
  || d[10]!=TT_Q38_PROJECTION_NATIVE_OFFSET || d[11]!=TT_Q38_PROJECTION_LINES_OFFSET
  || d[12]!=TT_Q38_PROJECTION_COMPACT_OFFSET || d[13]!=k*2 || d[15]!=t
  || d[14]!=((((t==8?k/32:k/256)*4)+63)&~63U) || !d[16])return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=44;i<64;i++)if(d[i])return false;
 return true;
}
static inline bool PioneerQ38ProjectionGatherLine(const uint32_t* line,const uint32_t* d,unsigned chunk){
 if(!PioneerQ38ProjectionGatherShape(d) || chunk>=d[5])return false;
 uint32_t p[64];for(unsigned i=0;i<64;i++)p[i]=d[i];
 p[0]=TT_Q38_PROJECTION_INPUT_MARKER;p[5]=chunk;p[7]=chunk+1;p[9]=PioneerQ38ProjectionPeer(chunk);
 p[10]=chunk*1024;p[11]=d[13]/2;p[12]=TT_Q38_PROJECTION_NATIVE_OFFSET+chunk*512;
 p[13]=TT_Q38_PROJECTION_LINES_OFFSET+chunk*128;p[14]=d[15];p[15]=d[5];
 return PioneerQ38ProjectionInputLine(line,p);
}
static inline bool PioneerQ38ProjectionGatherSummary(const uint32_t* s,const uint32_t* d,const uint32_t* before,const uint32_t* after){
 if(!s || !before || !after || !PioneerQ38ProjectionGatherShape(d) || before[2]!=before[6] || after[2]!=after[6])return false;
 unsigned reads=d[5]+(d[13]+16383)/16384,readFlits=d[5]*2+d[13]/64,writeFlits=d[14]/64;
 if(s[0]!=reads || s[1]!=readFlits || s[2]!=1 || s[3]!=writeFlits || s[4]!=d[5] || s[5]!=d[13]
  || s[6]!=d[6] || s[7]!=TT_Q38_PROJECTION_GATHER_MARKER || s[24]!=d[3] || s[25]!=d[4]
  || s[26]!=d[42] || s[27]!=d[43] || s[28]!=d[15] || s[29]!=d[14] || s[30]!=d[13]/2 || s[31]!=129)return false;
 const unsigned delta[]={reads,readFlits,reads+1,reads,1,writeFlits,reads+1,1};
 for(unsigned i=0;i<8;i++)if(s[8+i]!=before[i] || s[16+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i])return false;
 return true; // DATA ONLY,never source/producer/native lifetime authority
}
#endif
