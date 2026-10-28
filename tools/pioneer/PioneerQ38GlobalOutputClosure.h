/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GLOBAL_OUTPUT_CLOSURE_H
#define PIONEER_Q38_GLOBAL_OUTPUT_CLOSURE_H
#include "PioneerQ38GlobalHeadLayout.h"
// DATA checks for ONE worker's closed FINAL vocabulary job. These checks do
// NOT prove all973 jobs, the global candidate, state commit, a native owner
// or permission to launch the next token. The persistent parent owes those.
namespace PioneerQ38GlobalOutputClosure {
namespace G=PioneerQ38GlobalWorkerLayout;namespace H=PioneerQ38GlobalHeadLayout;
static inline bool Current(const uint32_t* d,unsigned worker){
 if(!d || !worker || worker>119 || !PioneerQ38MixedRecurrentBlockProjectionLayout::Shape(d)
  || d[42] || !d[40] || d[40]>=8192 || d[41]>=248320 || d[48]!=d[40]*64+1)return false;
 unsigned offset=0;
 if(d[43]==10){if(d[7]!=3)return false;}
 else if(d[43]==11){if(worker<=3 || d[7]!=3)return false;offset=1;}
 else if(d[43]==5){if(worker<=3 || d[7]!=119)return false;offset=2;}
 else return false;
 return d[6]==1001+d[40]*1024+offset;
}
static inline bool Job(const uint32_t* h,const uint32_t* rootBoot,const uint32_t* boot,const uint32_t* d,
 unsigned nonce,const uint32_t* job){
 if(!h || !rootBoot || !boot || !job || !nonce || nonce>=0x7fffffff || !Current(d,boot[3])
  || !H::Shape(h,rootBoot,d[3],d[4]) || !G::BootShape(boot))return false;
 for(unsigned i=0;i<16;i++)if(d[16+i]!=rootBoot[16+i])return false;
 for(unsigned i=0;i<8;i++)if(d[32+i]!=rootBoot[8+i] || boot[8+i]!=rootBoot[8+i])return false;
 const unsigned wave=boot[3]<=21?8:7,ordinal=wave*119+boot[3]-1;
 if(nonce<65+wave)return false; // necessary64 down frontiers +8/9 outputs,NOT a full-model proof
 uint32_t expected[64];if(!H::Job(h,ordinal,nonce-1,boot,expected))return false;
 expected[6]=d[40]-1;
 expected[7]+=(d[40]-1)*4096;
 if(!G::JobShape(expected,boot,nonce-1,0))return false;
 for(unsigned i=0;i<64;i++)if(job[i]!=expected[i])return false;
 return true;
}
static inline bool Validate(const uint32_t* h,const uint32_t* rootBoot,const uint32_t* boot,const uint32_t* d,
 unsigned nonce,const uint32_t* job,const uint32_t* partial,const uint32_t* proof,const uint32_t* status,const uint32_t* physical){
 if(!partial || !proof || !status || !physical || !Job(h,rootBoot,boot,d,nonce,job))return false;
 const unsigned s[]={nonce,nonce,0,G::Magic,0,0,64,d[40]-1};
 for(unsigned i=0;i<16;i++)if(status[i]!=(i<8?s[i]:0U))return false;
 unsigned groups=job[23],reads=groups*20+2,flits=groups*1060+162;
 const unsigned p[]={G::PartialMagic,nonce,64,job[6],boot[3],job[27],job[28],groups*16,partial[8],partial[9],1,job[7],248320,5120,14,1};
 for(unsigned i=0;i<16;i++)if(partial[i]!=p[i])return false;
 if(partial[8]<job[28] || partial[8]>=job[28]+groups*16 || (partial[9]&0x7f800000)==0x7f800000)return false;
 if(proof[16]!=groups*320 || proof[17]!=nonce || proof[18]!=boot[3] || proof[19]!=0x32504651
  || proof[30]!=boot[16] || proof[31]!=boot[4])return false;
 const unsigned command[]={job[22]+(groups*20-1)*3392,0,job[24],0x90000,0,boot[4],0,0,3392,0};
 for(unsigned i=0;i<10;i++)if(partial[24+i]!=command[i] || proof[20+i]!=command[i])return false;
 for(unsigned i=0;i<16;i++)if(partial[34+i]!=boot[16+i] || proof[32+i]!=boot[16+i])return false;
 for(unsigned i=0;i<8;i++){
  unsigned delta=i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0;
  if(partial[16+i]!=physical[i] || proof[8+i]!=physical[i] || uint32_t(proof[8+i]-proof[i])!=delta
   || partial[50+i]!=boot[8+i])return false;
 }
 if(physical[2]!=physical[6])return false;
 for(unsigned i=48;i<64;i++)if(proof[i])return false;
 for(unsigned i=58;i<64;i++)if(partial[i])return false;
 return true; // receipt consistency ONLY, not a numerical or token authority
}
}
#endif
