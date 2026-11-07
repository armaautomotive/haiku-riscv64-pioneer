/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_HEAD_LAYOUT_H
#define PIONEER_Q38_HYBRID_HEAD_LAYOUT_H
#if !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1
#error QGW8 head metadata is isolated offline plan DATA only
#endif
#include "PioneerQ38HybridHeadWorkerLayout.h"
#include "PioneerQ38GlobalChainLayout.h"

// Proposed fixed QGW8 head metadata and job construction. Caller must check
// and seal Shape once before calling Job, and separately bind authenticated
// source, residency, completed layers and ownership. No launch authority.
namespace PioneerQ38HybridHeadLayout {
namespace C=PioneerQ38GlobalChainLayout;namespace G=PioneerQ38HybridHeadWorkerLayout;
enum : unsigned {Magic=0x38485651,Words=128,Stripe=64,StripeWords=8,Jobs=973,Waves=9,RowBytes=5440};
static inline bool Shape(const uint32_t* h,const uint32_t* boot,unsigned bank,unsigned workspace){
 if(!h || !C::BootShape(boot) || bank>=7 || workspace<0x04000000 || workspace>0xc0000000 || (workspace&4095))return false;
 const unsigned fixed[]={Magic,1,128,bank,workspace,workspace+C::HeadOffset,0,0,5120,248320,973,9,119,8,RowBytes,20480};
 for(unsigned i=0;i<16;i++)if(i!=6 && i!=7 && h[i]!=fixed[i])return false;
 if(h[6]>=7 || h[7]<0x04000000 || h[7]>0xc3ffb000 || (h[7]&63)
  || (h[6]==bank && h[7]<workspace+C::WorkspaceBytes && workspace<h[7]+20480))return false;
 for(unsigned i=0;i<16;i++)if(h[16+i]!=boot[16+i])return false;
 for(unsigned i=0;i<8;i++)if(h[32+i]!=boot[8+i])return false;
 if(h[40]!=G::Magic || h[41]!=C::Magic || h[42]!=64 || h[43] || h[44]!=32 || h[45]!=8 || h[46] || h[47]!=1)return false;
 for(unsigned i=48;i<64;i++)if(h[i])return false;
 unsigned rows=0;
 for(unsigned s=0;s<7;s++){
  const auto* p=h+Stripe+s*StripeWords;
  if(p[0]!=s || p[1]<0x04000000 || (p[1]&4095) || (p[2]&63) || p[2]>=248320
   || (p[3]!=35456 && p[3]!=35520) || p[3]>248320-p[2] || p[4]!=RowBytes || p[5]!=p[3]*RowBytes
   || p[6]!=((p[5]+4095)&~4095U) || p[1]>0xc4000000-p[6] || p[7]!=139
   || (s==bank && p[1]<workspace+C::WorkspaceBytes && workspace<p[1]+p[6])
   || (s==h[6] && p[1]<h[7]+20480 && h[7]<p[1]+p[6]))return false;
  for(unsigned t=0;t<s;t++){
   const auto* q=h+Stripe+t*StripeWords;if(p[2]<q[2]+q[3] && q[2]<p[2]+p[3])return false;
  }
  rows+=p[3];
 }
 for(unsigned i=120;i<128;i++)if(h[i])return false;
 return rows==248320;
}
static inline bool Job(const uint32_t* h,unsigned ordinal,unsigned previous,unsigned position,const uint32_t* boot,uint32_t* j){
 if(!h || !boot || !j || ordinal>=Jobs || previous>=0x7ffffffe || position>=8192 || !G::BootShape(boot))return false;
 using Address=__UINTPTR_TYPE__;Address o=reinterpret_cast<Address>(j),a=reinterpret_cast<Address>(h),b=reinterpret_cast<Address>(boot),max=~Address(0);
 if(((o|a|b)&3) || o>max-256 || a>max-512 || b>max-256 || (o<a+512 && a<o+256) || (o<b+256 && b<o+256))return false;
 unsigned worker=ordinal,wave=0;while(worker>=119){worker-=119;wave++;}worker++;
 if(boot[3]!=worker)return false;
 for(unsigned i=0;i<8;i++)if(boot[8+i]!=h[32+i])return false;
 unsigned stripe=ordinal,index=0;while(stripe>=7){stripe-=7;index++;}
 const auto* p=h+Stripe+stripe*StripeWords;
 if(p[0]!=stripe || p[7]!=139 || (p[3]!=35456 && p[3]!=35520) || p[4]!=RowBytes || index>=139)return false;
 const unsigned remainder=p[3]/32-7*139,done=index*7+(index<remainder?index:remainder),bands=7+unsigned(index<remainder);
 uint32_t stage[64];for(unsigned i=0;i<64;i++)stage[i]=0;
 const unsigned head[]={G::Magic,previous+1,G::GlobalMatrix,worker,G::Peer(worker),64,position,2025+position*4096+wave};
 for(unsigned i=0;i<8;i++){stage[i]=head[i];stage[8+i]=boot[8+i];}
 const unsigned leaf[]={PioneerQ38Bfp8WorkerJob::Magic,1,5120,p[1]+done*174080,bands,PioneerQ38ProjectionRoute(p[0]),worker,G::Peer(worker),
  PioneerQ38Bf16Wire::Magic,PioneerQ38Bfp8WorkerJob::RootInput,PioneerQ38Bfp8WorkerJob::RootRoute,previous+1,
  PioneerQ38Bfp8WorkerJob::Output,0,0,0};
 for(unsigned i=0;i<16;i++)stage[16+i]=leaf[i];
 stage[32]=p[2]+done*32;stage[33]=248320;stage[34]=wave;stage[35]=G::JobWitness;
 if(!G::HeadShape(stage,boot,previous,0))return false;
 for(unsigned i=0;i<64;i++)j[i]=stage[i];
 return true;
}
}
#endif
