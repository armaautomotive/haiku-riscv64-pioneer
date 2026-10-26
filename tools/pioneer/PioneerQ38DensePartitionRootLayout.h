/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DENSE_PARTITION_ROOT_LAYOUT_H
#define PIONEER_Q38_DENSE_PARTITION_ROOT_LAYOUT_H
#include "PioneerQ38DensePartitionLayout.h"
// NEW standalone ROOT DATA,not reserved/initialized by old native owners.
namespace PioneerQ38DensePartitionRootLayout {
enum {Magic=0x31524451,Fault=0x46524451,Header=0x74600,Summary=0x74800,Code=0x50000,
 Scratch=0x78000,Frames=0x100000,Shadow=0x108000,Reports=0x110000,Proofs=0x118000,
 Maxima=0x120000,Parameters=0x121000,Counters=0x122000,
 Gate=0x124000,Output=0x12a000,Accumulate=0x130000,
 Input=0x140000,Query=0x140000,NewK=0x146000,NewV=0x146800,
 InputShadow=0x148000,GateShadow=0x150000,End=0x156000};
static inline bool Shape(const uint32_t* d){
 if(!d || d[0]!=Magic || d[1]!=1 || d[2]!=64 || d[3]>=64 || d[3]%4!=3 || d[4]>=8192 || d[5]!=129
  || !d[6] || d[6]>=0x80000000 || d[7]!=96 || d[8]!=4 || d[9]!=24 || d[10]!=256 || d[11]!=3
  || d[12]!=Frames || d[13]!=Shadow || d[14]!=Reports || d[15]!=Proofs || !d[16]
  || d[40]!=Input || d[41]!=Gate || d[42]!=Output || d[43]!=Accumulate
  || d[44]!=Scratch || d[45]!=12288 || d[46]!=6144 || d[47]!=7168)return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=48;i<64;i++)if(d[i])return false;
 return true;
}
static inline bool Child(const uint32_t* d,const uint32_t* f,unsigned group,unsigned ordinal){
 if(!d || !f || group>=4 || ordinal>=24 || !PioneerQ38DensePartitionLayout::Shape(f)
  || f[3]!=d[3] || f[4]!=d[4] || f[5]!=group || f[6]!=ordinal || f[40]!=d[6])return false;
 for(unsigned i=32;i<40;i++)if(f[i]!=d[i])return false;
 return true; // geometry DATA,not a canonical producer/native capability
}
}
#endif
