/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GLOBAL_HEAD_PLAN_H
#define PIONEER_Q38_GLOBAL_HEAD_PLAN_H
#include "PioneerQ38GlobalHeadLayout.h"
#include "PioneerQ38GlobalOutputPlan.h"
// Heap CPU serialization ONLY; never an original source/init/producer grant.
class PioneerQ38GlobalHeadPlan {
 uint32_t data[128]={},boot[64]={};const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 bool attempted=false,ready=false;
public:
 bool Build(const PioneerQ38CanonicalModelPlan& c,unsigned bank,const uint32_t* rootBoot){
  namespace H=PioneerQ38GlobalHeadLayout;namespace C=H::C;
  if(attempted)return false;
  attempted=true;
  if(!c.Ready() || bank>=7 || !C::BootShape(rootBoot))return false;
  const auto* workspace=c.Workspace(bank);if(!workspace || workspace->bytes!=C::WorkspaceBytes || workspace->allocated!=C::WorkspaceBytes)return false;
  const unsigned base=c.Base+workspace->offset;
  const unsigned header[]={C::HeadMagic,1,128,bank,base,base+C::HeadOffset,7,0,5120,248320,973,9,119,14,4240,20480};
  for(unsigned i=0;i<16;i++)data[i]=header[i];
  for(unsigned i=0;i<16;i++)data[16+i]=rootBoot[16+i];
  for(unsigned i=0;i<8;i++)data[32+i]=rootBoot[8+i];
  const unsigned witness[]={H::G::Magic,C::Magic,64,0,16,16,0,1};for(unsigned i=0;i<8;i++)data[40+i]=witness[i];
  bool found=false;unsigned stripes=0;
  for(unsigned i=0;i<c.Count();i++){
   const auto* p=c.Part(i);if(p->kind!=c.WeightKind || p->layer!=64)continue;
   if(p->role==2){
    if(found || p->type || p->firstRow || p->rows!=1 || p->rowBytes!=20480 || p->bytes!=20480 || p->allocated!=20480)return false;
    found=true;data[6]=p->bank;data[7]=c.Base+p->offset;
   }else if(p->role==1){
    if(p->bank>=7 || p->type!=14 || p->rowBytes!=H::RowBytes)return false;
    auto* out=data+H::Stripe+p->bank*8;if(out[7])return false;
    const unsigned words[]={p->bank,c.Base+p->offset,p->firstRow,p->rows,p->rowBytes,p->bytes,p->allocated,139};
    for(unsigned n=0;n<8;n++)out[n]=words[n];
    stripes++;
   }
  }
  if(!found || stripes!=7 || !H::Shape(data,rootBoot,bank,base))return false;
  for(unsigned i=0;i<64;i++)boot[i]=rootBoot[i];
  canonical=&c;ready=true;return Matches(c);
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c)const{
  namespace H=PioneerQ38GlobalHeadLayout;
  if(!ready || canonical!=&c || !c.Ready() || data[3]>=7 || !H::Shape(data,boot,data[3],c.Base+c.Workspace(data[3])->offset))return false;
  unsigned norms=0,stripes=0;
  for(unsigned i=0;i<c.Count();i++){
   const auto* p=c.Part(i);if(p->kind!=c.WeightKind || p->layer!=64)continue;
   if(p->role==2){if(data[6]!=p->bank || data[7]!=c.Base+p->offset || p->type || p->rows!=1 || p->firstRow || p->rowBytes!=20480 || p->bytes!=20480 || p->allocated!=20480)return false;norms++;}
   else if(p->role==1){
    if(p->bank>=7 || p->type!=14)return false;
    const unsigned words[]={p->bank,c.Base+p->offset,p->firstRow,p->rows,p->rowBytes,p->bytes,p->allocated,139};
    for(unsigned n=0;n<8;n++)if(data[64+p->bank*8+n]!=words[n])return false;
    stripes++;
   }
  }
  return norms==1 && stripes==7;
 }
 const uint32_t* Data()const{return ready?data:nullptr;}
};
#endif
