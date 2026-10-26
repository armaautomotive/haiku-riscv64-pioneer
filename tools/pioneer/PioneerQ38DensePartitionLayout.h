/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DENSE_PARTITION_LAYOUT_H
#define PIONEER_Q38_DENSE_PARTITION_LAYOUT_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// NEW96-worker attention DATA. Not an old QDA/QFP/native ownership alias.
namespace PioneerQ38DensePartitionLayout {
enum {Magic=0x31574451,Fault=0x46574451,Header=0x74600,Summary=0x74800,
 Job=0x74a00,Doorbell=0x74a80,Status=0x74b00,
 HalfQuery=0x80000,NewK=0x81800,NewV=0x81a00,Parameters=0x82000,
 PrivateQuery=0x85000,PrivateK=0x86800,PrivateV=0x86a00,
 Scores=0x90000,Local=0x93000,Accumulate=0x94000,Partial=0x98000,Chunk=0xa0000,
 Stride=384,Heads=6,WorkersPerGroup=24,Workers=96,Code=0x40000};
static inline unsigned Route(unsigned bank){const unsigned r[]={786,978,1170,785,977,1169,1361};return bank<7?r[bank]:~0U;}
static inline unsigned Peer(unsigned worker){
 unsigned x=1;while(worker>=10){worker-=10;x++;}if(x>=8)x+=2;
 return x|((worker+2)<<6);
}
static inline unsigned Per(unsigned tokens){unsigned n=0;while(tokens>=24){tokens-=24;n++;}return n+unsigned(tokens!=0);}
static inline unsigned First(unsigned tokens,unsigned ordinal){return Per(tokens)*ordinal;}
static inline unsigned Count(unsigned tokens,unsigned ordinal){unsigned first=First(tokens,ordinal),per=Per(tokens);return first>=tokens?0:tokens-first<per?tokens-first:per;}
static inline bool Shape(const uint32_t* d){
 if(!d || d[0]!=Magic || d[1]!=1 || d[2]!=64 || d[3]>=64 || d[3]%4!=3 || d[4]>=8192 || d[5]>=4 || d[6]>=24
  || d[7]!=d[4]+1 || (d[8]&4095) || d[8]<0x04000000 || d[8]>0xc3c00000
  || (d[9]&4095) || d[9]<0x04000000 || d[9]>0xc3c00000 || d[10]>=7 || d[11]>=7
  || d[12]!=Route(d[10]) || d[13]!=Route(d[11]) || d[15]!=1+d[5]*24+d[6] || d[14]!=Peer(d[15]) || !d[16]
  || !d[40] || d[40]>=0x80000000 || d[41]!=256 || d[42]!=0x3d800000 || d[43]!=32 || d[44]!=8192 || d[45]!=16384
  || d[48]!=First(d[7],d[6]) || d[49]!=Count(d[7],d[6]) || d[49]>Stride
  || d[50]!=3 || d[51]!=Heads || d[52]!=WorkersPerGroup || d[53]!=Stride || d[54]!=1536 || d[55]!=Magic)return false;
 unsigned prior=d[48]>=d[4]?0:d[4]-d[48];if(prior>d[49])prior=d[49];
 if(d[46]!=2*((prior+31)>>5) || d[47]!=prior*16)return false;
 if(d[10]==d[11] && d[8]<d[9]+4194304 && d[9]<d[8]+4194304)return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=56;i<64;i++)if(d[i])return false;
 return true; // geometry ONLY,not authenticated source/producer/prefix/native
}
static inline bool JobShape(const uint32_t* j,const uint32_t* d,unsigned sequence,unsigned phase){
 if(!j || !d || j[0]!=Magic || j[1]!=phase || j[2]!=sequence || !sequence || j[3]!=d[40] || j[4]!=d[15]
  || j[5]!=d[3] || j[6]!=d[4] || j[7]!=d[5] || j[8]!=d[48] || j[9]!=d[49])return false;
 for(unsigned i=10;i<16;i++)if(j[i])return false;
 return true;
}
}
#endif
