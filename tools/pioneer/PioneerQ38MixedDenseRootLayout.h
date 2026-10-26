/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_DENSE_ROOT_LAYOUT_H
#define PIONEER_Q38_MIXED_DENSE_ROOT_LAYOUT_H
#include "PioneerQ38DensePartitionRootLayout.h"
#include "PioneerQ38MixedWorkerLayout.h"
namespace PioneerQ38MixedDenseRootLayout {
// Common arithmetic scratch DATA, not old image/source/native authority.
using namespace PioneerQ38DensePartitionRootLayout;
enum {Magic=0x31524d51,Fault=0x46524d51,Boot=0x75300,Doorbell=0x75400,Status=0x75440,
 Nonces=0x123000,PeerBoots=0x170000,PeerJobs=0x178000,End=0x17f700,Peers=119};
static inline bool Shape(const uint32_t* d){
 if(!d || d[0]!=Magic || !d[48] || d[48]>=0x7fffffff || d[49]!=Peers || d[50]!=PeerBoots || d[51]!=PeerJobs || d[52]!=Nonces)return false;
 for(unsigned i=53;i<64;i++)if(d[i])return false;
 uint32_t old[64];for(unsigned i=0;i<64;i++)old[i]=i>=48?0:d[i];
 old[0]=PioneerQ38DensePartitionRootLayout::Magic;
 return PioneerQ38DensePartitionRootLayout::Shape(old);
}
static inline bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic || b[1]!=1 || b[2]!=64 || b[3]!=129 || b[4]!=b[16] || !b[4] || b[5]!=Peers || b[6]!=8192 || b[7]!=64)return false;
 for(unsigned i=8;i<16;i+=2)if(!(b[i]|b[i+1]))return false;
 for(unsigned i=32;i<64;i++)if(b[i])return false;
 return true;
}
struct PrivateSeal {uint32_t boot[64],frame[64];};
}
#endif
