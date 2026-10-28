/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_STATE_PLAN_H
#define PIONEER_Q38_TOKEN_STATE_PLAN_H
#include "PioneerQ38GlobalChainPackage.h"
#include "PioneerQ38TokenStateLayout.h"
// Immutable canonical Conv-current destinations. These words neither reserve
// memory nor prove that any prior/next state or full token has completed.
class PioneerQ38TokenStatePlan {
 uint32_t data[64]={};const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 unsigned layer=64,bank=7;bool attempted=false,ready=false;
public:
 bool Build(const PioneerQ38CanonicalModelPlan& c,unsigned selected,unsigned selectedBank){
  namespace S=PioneerQ38TokenStateLayout;
  if(attempted)return false;
  attempted=true;
  if(!c.Ready() || selected>=64 || S::C::Dense(selected) || selectedBank>=7)return false;
  layer=selected;bank=selectedBank;canonical=&c;unsigned workspace=c.Base+c.Workspace(bank)->offset;uint32_t history=0;
  if(!S::H::Address(workspace,layer,history))return false;
  const unsigned head[]={S::Magic,1,layer,bank,workspace,history,S::H::Bytes,7};for(unsigned i=0;i<8;i++)data[i]=head[i];
  unsigned n=0;
  for(unsigned i=0;i<c.Count();i++){
   const auto* p=c.Part(i);if(p->kind!=c.ConvKind || p->layer!=layer)continue;
   if(n>=7 || p->role || p->type || p->rowBytes!=12)return false;
   const unsigned span[]={p->bank,c.Base+p->offset,p->firstRow,p->rows,p->rowBytes,p->bytes,p->allocated,PioneerQ38ProjectionRoute(p->bank)};
   for(unsigned w=0;w<8;w++)data[8+n*8+w]=span[w];
   n++;
  }
  ready=n==7 && S::Shape(data,layer,bank,workspace);return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c)const{
  if(!ready || canonical!=&c || !c.Ready() || !PioneerQ38TokenStateLayout::Shape(data,layer,bank,c.Base+c.Workspace(bank)->offset))return false;
  unsigned n=0;
  for(unsigned i=0;i<c.Count();i++){
   const auto* p=c.Part(i);if(p->kind!=c.ConvKind || p->layer!=layer)continue;
   if(n>=7)return false;
   const unsigned span[]={p->bank,c.Base+p->offset,p->firstRow,p->rows,p->rowBytes,p->bytes,p->allocated,PioneerQ38ProjectionRoute(p->bank)};
   for(unsigned w=0;w<8;w++)if(data[8+n*8+w]!=span[w])return false;
   n++;
  }
  return n==7;
 }
 const uint32_t* Data()const{return ready?data:nullptr;}
};
#endif
