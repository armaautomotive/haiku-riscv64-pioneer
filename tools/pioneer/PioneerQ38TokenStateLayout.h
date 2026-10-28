/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_STATE_LAYOUT_H
#define PIONEER_Q38_TOKEN_STATE_LAYOUT_H
#include "PioneerQ38GlobalChainLayout.h"
#include "PioneerQ38RecurrentHistoryLayout.h"
// Immutable canonical Conv-current destination DATA. Neither initialized
// memory, completed state nor a native launch/lifetime is conferred here.
namespace PioneerQ38TokenStateLayout {
namespace C=PioneerQ38GlobalChainLayout;namespace H=PioneerQ38RecurrentHistoryLayout;
enum {Magic=0x31435351,Offset=0x78000,Words=64,Bytes=256,Local=C::Head+1024};
static inline bool Shape(const uint32_t* d,unsigned layer,unsigned bank,unsigned workspace){
 uint32_t history=0;if(!d || bank>=7 || !H::Address(workspace,layer,history))return false;
 const unsigned head[]={Magic,1,layer,bank,workspace,history,H::Bytes,7};
 for(unsigned i=0;i<8;i++)if(d[i]!=head[i])return false;
 unsigned first=0,banks=0;
 for(unsigned s=0;s<7;s++){
  const auto* p=d+8+s*8;
  if(p[0]>=7 || (banks&(1U<<p[0])) || p[1]<0x04000000 || (p[1]&4095) || p[2]!=first
   || (p[3]!=1408 && p[3]!=1472) || p[4]!=12 || p[5]!=p[3]*12 || p[6]!=((p[5]+4095)&~4095U)
   || p[1]>0xc4000000-p[6] || p[7]!=PioneerQ38ProjectionRoute(p[0])
   || (p[0]==bank && p[1]<workspace+C::WorkspaceBytes && workspace<p[1]+p[6]))return false;
  first+=p[3];banks|=1U<<p[0];
 }
 return first==10240 && banks==127;
}
static_assert(unsigned(Offset)>=unsigned(C::P::Bytes) && Offset+Bytes<=C::Stride && Local+Bytes<=C::R::Projected,
 "state table fits package slack and its own L1 region");
}
#endif
