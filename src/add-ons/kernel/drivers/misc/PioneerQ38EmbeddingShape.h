/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EMBEDDING_SHAPE_H
#define PIONEER_Q38_EMBEDDING_SHAPE_H
#include "PioneerQ38ProjectionInputShape.h"
// New OFFLINE embedding root DATA,not a native producer/launch grant.
// Global embedding is FIRST in the fixed canonical layout:seven original
// Q4_K byte-interleaved stripes at Base,554/555 groups of64 rows each.
// Position is context position;token ID is independently vocabulary-bounded.
enum {TT_Q38_EMBEDDING_MARKER=0x31455051,TT_Q38_EMBEDDING_FAULT=0x46455051,
 TT_Q38_EMBEDDING_VALUES=5120,TT_Q38_EMBEDDING_BYTES=20480,
 TT_Q38_EMBEDDING_TILE=2304,TT_Q38_EMBEDDING_GROUP=46080};
static inline unsigned PioneerQ38EmbeddingFirst(unsigned bank){return (bank*554+(bank<2?bank:2))*64;}
static inline unsigned PioneerQ38EmbeddingRows(unsigned bank){return (554+unsigned(bank<2))*64;}
static inline bool PioneerQ38EmbeddingShape(const uint32_t* d){
 if(!d || d[0]!=TT_Q38_EMBEDDING_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || d[5]>=248320
  || !d[6] || d[6]>=0x7fffffff || d[7]!=248320 || d[8]!=PioneerQ38ProjectionRoute(d[3]) || d[9]!=129
  || d[10]>=7 || d[12]!=PioneerQ38ProjectionRoute(d[10]) || d[13]>=16 || d[14]!=5120 || d[15]!=12
  || !d[16] || d[40]>=8192 || d[41] || d[42]!=PioneerQ38EmbeddingFirst(d[10])
  || d[43]!=PioneerQ38EmbeddingRows(d[10]) || d[44]!=0x04000000 || d[45]!=2880
  || d[46]!=2304 || d[47]!=20 || d[48]!=46080 || d[49]!=20480)return false;
 if(d[5]<d[42] || d[5]-d[42]>=d[43] || d[13]!=(d[5]-d[42])%16
  || d[11]!=d[44]+((d[5]-d[42])/16)*46080
  || (d[3]==d[10] && d[4]<d[44]+d[43]*2880))return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=50;i<64;i++)if(d[i])return false;
 return true;
}
static inline bool PioneerQ38EmbeddingSummary(const uint32_t* s,const uint32_t* d,const uint32_t* before,const uint32_t* after){
 if(!s || !before || !after || !PioneerQ38EmbeddingShape(d) || before[2]!=before[6] || after[2]!=after[6])return false;
 if(s[0]!=20 || s[1]!=720 || s[2]!=2 || s[3]!=320 || s[4]!=5120 || s[5]!=20480 || s[6]!=d[6]
  || s[7]!=TT_Q38_EMBEDDING_MARKER || s[24]!=d[3] || s[25]!=d[4] || s[26]!=d[5] || s[27]!=d[10]
  || s[28]!=d[11] || s[29]!=d[13] || s[30]!=d[40] || s[31]!=129)return false;
 const unsigned delta[]={20,720,22,20,2,320,22,2};
 for(unsigned i=0;i<8;i++)if(s[8+i]!=before[i] || s[16+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i])return false;
 return true; // Data ONLY;completion/source/holds must be proved by native owner.
}
#endif
