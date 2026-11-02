/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BATCHED_GLOBAL_OUTPUT_CLOSURE_H
#define PIONEER_Q38_BATCHED_GLOBAL_OUTPUT_CLOSURE_H
#include "PioneerQ38GlobalOutputClosure.h"
// Separate DATA predicate for Q6 two-tile batches. Only the explicitly
// selected batched-output-closure ROOT build binds it at the call site.
// The predicate alone grants no owner, execution, token or cleanup capability.
// The restored final single-tile COMMAND is unchanged;only actual read
// packet deltas halve. Payload flits and all other receipt checks stay exact.
namespace PioneerQ38BatchedGlobalOutputClosure {
namespace G=PioneerQ38SelectedGlobalWorker;
static inline bool Validate(const uint32_t* h,const uint32_t* rootBoot,const uint32_t* boot,const uint32_t* d,
 unsigned nonce,const uint32_t* job,const uint32_t* partial,const uint32_t* proof,const uint32_t* status,const uint32_t* physical){
 if(!partial || !proof || !status || !physical
  || !PioneerQ38GlobalOutputClosure::Job(h,rootBoot,boot,d,nonce,job))return false;
 const unsigned s[]={nonce,nonce,0,G::Magic,0,0,64,d[40]-1};
 for(unsigned i=0;i<16;i++)if(status[i]!=(i<8?s[i]:0U))return false;
 const unsigned groups=job[23],reads=groups*10+2,flits=groups*1060+162;
 const unsigned p[]={G::PartialMagic,nonce,64,job[6],boot[3],job[27],job[28],groups*16,partial[8],partial[9],1,job[7],248320,5120,14,1};
 for(unsigned i=0;i<16;i++)if(partial[i]!=p[i])return false;
 if(partial[8]<job[28] || partial[8]>=job[28]+groups*16 || (partial[9]&0x7f800000)==0x7f800000)return false;
 if(proof[16]!=groups*320 || proof[17]!=nonce || proof[18]!=boot[3] || proof[19]!=0x32504651
  || proof[30]!=boot[16] || proof[31]!=boot[4])return false;
 const unsigned command[]={job[22]+(groups*20-1)*3392,0,job[24],0x90000,0,boot[4],0,0,3392,0};
 for(unsigned i=0;i<10;i++)if(partial[24+i]!=command[i] || proof[20+i]!=command[i])return false;
 for(unsigned i=0;i<16;i++)if(partial[34+i]!=boot[16+i] || proof[32+i]!=boot[16+i])return false;
 for(unsigned i=0;i<8;i++){
  const unsigned delta=i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0;
  if(partial[16+i]!=physical[i] || proof[8+i]!=physical[i] || uint32_t(proof[8+i]-proof[i])!=delta
   || partial[50+i]!=boot[8+i])return false;
 }
 if(physical[2]!=physical[6])return false;
 for(unsigned i=48;i<64;i++)if(proof[i])return false;
 for(unsigned i=58;i<64;i++)if(partial[i])return false;
 return true; // DATA consistency only,never accept both old/new packet counts
}
}
#endif
