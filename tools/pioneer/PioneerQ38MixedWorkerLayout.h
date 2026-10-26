/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_WORKER_LAYOUT_H
#define PIONEER_Q38_MIXED_WORKER_LAYOUT_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// OFFLINE QMW1 DATA only. Not QFP2/QDW1 compatibility or native authority.
namespace PioneerQ38MixedWorkerLayout {
enum {Magic=0x31574d51,Fault=0x46574d51,Boot=0x75000,Job=0x75100,
 Doorbell=0x75200,Status=0x75240,Code=0x20000,DenseCode=0x40000,
 DenseScores=9,DenseExp=10,DenseValues=11,Stop=2};
static inline unsigned Peer(unsigned w){unsigned x=1;while(w>=10){w-=10;x++;}if(x>=8)x+=2;return x|((w+2)<<6);}
static inline bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic || b[1]!=1 || b[2]!=64 || !b[3] || b[3]>=120 || b[4]!=Peer(b[3])
  || !b[5] || b[5]!=b[16] || b[6]!=8192 || b[7]!=64)return false;
 for(unsigned i=8;i<16;i+=2)if(!(b[i]|b[i+1]))return false;
 for(unsigned i=32;i<64;i++)if(b[i])return false;
 return true;
}
static inline bool JobShape(const uint32_t* j,const uint32_t* b,unsigned previous,unsigned densePhase){
 if(!j || !b || j[0]!=Magic || !j[1] || j[1]>=0x7fffffff || j[1]!=previous+1
  || !j[2] || j[2]>DenseValues || j[3]!=b[3] || j[4]!=b[4]
  || j[5]>=64 || j[6]>=8192 || !j[7] || j[7]>=0x80000000)return false;
 for(unsigned i=8;i<16;i++)if(j[i]!=b[i])return false;
 for(unsigned i=27;i<64;i++)if(j[i])return false;
 if(densePhase && j[2]!=DenseScores+densePhase)return false;
 if(j[2]>=DenseScores){
  if(b[3]>96 || j[5]%4!=3 || j[2]!=DenseScores+densePhase)return false;
  for(unsigned i=16;i<27;i++)if(j[i])return false;
 }else{
  if(j[26]>1 || (j[26] && j[2]!=1))return false;
  if(j[2]==Stop)for(unsigned i=16;i<27;i++)if(j[i])return false;
  if((j[2]==6 || j[2]==7 || j[2]==8) && j[5]%4==3)return false;
 }
 return true; // exact capsule arithmetic descriptors are checked separately
}
struct PrivateSeal {uint32_t boot[64],job[64];};
}
#endif
