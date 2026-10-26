/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DENSE_PREPARE_LAYOUT_H
#define PIONEER_Q38_DENSE_PREPARE_LAYOUT_H
// NEW OFFLINE local geometry;NOT reserved/initialized by any native owner.
namespace PioneerQ38DensePrepareLayout {
enum {Magic=0x31504451,Fault=0x46504451,Header=0x74600,HeaderWords=64,
 Qg=0x80000,Key=0x83000,Value=0x83400,QWeight=0x83800,KWeight=0x83c00,Rope=0x84000,
 Query=0x90000,Gate=0x91800,PackedK=0x93000,PackedV=0x93200,End=0x93400};
static inline bool Shape(const uint32_t* d){
 if(!d || d[0]!=Magic || d[1]!=1 || d[2]!=HeaderWords || d[3]>=64 || d[3]%4!=3 || d[4]>=8192 || d[5]>=4
  || !d[6] || d[6]>=0x80000000 || d[7]!=5120 || d[8]!=24 || d[9]!=4 || d[10]!=256 || d[11]!=64
  || d[12]!=6 || d[13]!=3072 || d[14]!=1536 || d[15]!=1536)return false;
 for(unsigned i=16;i<HeaderWords;i++)if(d[i])return false;
 return true; // local arithmetic DATA,not provenance/state/producer proof
}
}
#endif
