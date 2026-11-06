/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BF16_MATRIX_RECEIPT_H
#define PIONEER_Q38_BF16_MATRIX_RECEIPT_H
#include "PioneerQ38Bf16WorkerLayout.h"
// Receipt DATA validation only. The caller must independently authenticate
// the actual closed outer job,boot and physical counter frontier. A matching
// byte array alone is not completion,exclusive ownership or a launch grant.
struct PioneerQ38Bf16MatrixReceipt {
#ifdef PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU
 static __attribute__((noinline,section(".bf16.receipt")))
#else
 static __attribute__((noinline))
#endif
 bool Matrix(const uint32_t* p,const uint32_t* j,const uint32_t* b,
             const uint32_t* before,const uint32_t* after){
  namespace M=PioneerQ38Bf16WorkerLayout;
  using C=PioneerQ38Q4FpuCachedLayout;
  if(!p || !j || !b || !before || !after || !j[1] || !M::FpuJob(j)
   || !M::JobShape(j,b,j[1]-1,0) || before[2]!=before[6] || after[2]!=after[6])return false;
  C::Job leaf;for(unsigned i=0;i<16;i++)leaf.words[i]=j[16+i];
  const unsigned groups=leaf.words[4],blocks=leaf.words[2]/256;
  if(p[16]!=groups*(leaf.words[2]/16) || p[17]!=j[1] || p[18]!=b[3] || p[19]!=C::Magic
   || p[30]!=b[16] || p[31]!=b[4] || p[48]!=M::Producer::Format || p[49]!=leaf.words[2]
   || p[50]!=groups || p[51]!=j[7] || p[52]!=j[5] || p[53]!=j[6]
   || p[54]!=j[32] || p[55]!=j[33] || p[56]!=j[34])return false;
  for(unsigned i=0;i<16;i++)if(p[32+i]!=b[16+i])return false;
  for(unsigned i=57;i<64;i++)if(p[i])return false;
  C::Read last;if(!C::Packet(leaf,groups/2-1,blocks-1,(blocks-1)&1,25,last))return false;
  const unsigned command[]={last.source,0,last.route,last.destination,0,b[4],0,0,last.bytes,0};
  for(unsigned i=0;i<10;i++)if(p[20+i]!=command[i])return false;
  for(unsigned i=0;i<8;i++){
   const unsigned delta=i==1?(groups/2)*blocks*80:i==0 || i==2 || i==3 || i==6?(groups/2)*blocks*26:0U;
   if(p[i]!=before[i] || p[8+i]!=after[i] || uint32_t(after[i]-before[i])!=delta)return false;
  }
  return true;
 }
};
#endif
