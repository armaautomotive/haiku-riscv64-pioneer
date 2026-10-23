/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_INPUT_PLAN_H
#define PIONEER_Q38_GDN_INPUT_PLAN_H
#include "PioneerQ38CanonicalModelPlan.h"
#include "PioneerQ38GdnIdentity.h"
#include "PioneerQ38GdnInputShape.h"
// Canonical DATA factory only. Future SAME native owner must bind actual
// retained QCB1/QPS1 producers,bank/source/upload/clock/initialization owners.
class PioneerQ38GdnInputPlan {
 const PioneerQ38CanonicalModelPlan* plan=nullptr;uint32_t descriptor[64]={};bool attempted=false,ready=false;
 static bool Describe(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned half,unsigned position,
  unsigned bank,const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase,uint32_t* d){
  if(!canonical.Ready() || layer>=64 || layer%4==3 || half>=96 || position>=8192 || bank>=7
   || !id.Valid() || !source || !source[0] || !phase || phase>=0x7fffffff)return false;
  const PioneerQ38CanonicalPart* learned[2]={};
  for(unsigned i=0;i<canonical.Count();i++){
   const auto* p=canonical.Part(i);
   if(p->kind!=canonical.WeightKind || p->layer!=layer || (p->role!=7 && p->role!=8))continue;
   unsigned index=p->role-7;
   if(learned[index] || p->type || p->firstRow || p->rows!=1 || p->rowBytes!=192 || p->bytes!=192 || p->allocated!=4096)return false;
   learned[index]=p;
  }
  const auto* workspace=canonical.Workspace(bank);
  if(!learned[0] || !learned[1] || !workspace || workspace->bytes!=67108864)return false;
  for(unsigned i=0;i<64;i++)d[i]=0;
  unsigned h=half>>1,k=h&15,block=(h>>4)*64;
  d[0]=TT_Q38_GDN_INPUT_MARKER;d[1]=1;d[2]=67108864;d[3]=bank;d[4]=canonical.Base+workspace->offset;
  d[5]=layer;d[6]=phase;d[7]=half;d[8]=PioneerQ38ProjectionRoute(bank);d[9]=PioneerQ38ConvPeer(half+1);d[10]=half+1;
  d[11]=learned[0]->bank;d[12]=canonical.Base+learned[0]->offset+block;d[13]=PioneerQ38ProjectionRoute(d[11]);
  d[14]=learned[1]->bank;d[15]=canonical.Base+learned[1]->offset+block;d[41]=PioneerQ38ProjectionRoute(d[14]);
  for(unsigned i=0;i<16;i++)d[16+i]=source[i];
  const uint64_t values[]={id.session,id.sequence,id.modelEpoch,id.driverGeneration};
  for(unsigned i=0;i<4;i++){d[32+i*2]=uint32_t(values[i]);d[33+i*2]=uint32_t(values[i]>>32);}
  d[40]=position;d[42]=TT_Q38_CONV_OUTPUT+k*512;d[43]=TT_Q38_CONV_OUTPUT+8192+k*512;
  d[44]=TT_Q38_CONV_OUTPUT+16384+h*512+(half&1)*256;
  d[45]=PioneerQ38ProjectionResultOffset(10)+block;d[46]=PioneerQ38ProjectionResultOffset(11)+block;d[47]=h&15;
  d[48]=512;d[49]=512;d[50]=256;d[51]=64;d[52]=7;d[53]=24;d[54]=1536;d[55]=322;
  return PioneerQ38GdnInputShape(d);
 }
public:
 PioneerQ38GdnInputPlan()=default;PioneerQ38GdnInputPlan(const PioneerQ38GdnInputPlan&)=delete;
 PioneerQ38GdnInputPlan& operator=(const PioneerQ38GdnInputPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned half,unsigned position,unsigned bank,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase){
  if(attempted)return false;
  attempted=true;plan=&canonical;
  ready=Describe(canonical,layer,half,position,bank,id,source,phase,descriptor);return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned half,unsigned position,unsigned bank,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase)const{
  if(!ready || plan!=&canonical)return false;
  uint32_t current[64];if(!Describe(canonical,layer,half,position,bank,id,source,phase,current))return false;
  for(unsigned i=0;i<64;i++)if(current[i]!=descriptor[i])return false;
  return true;
 }
 bool LearnedPagesPresent(const uint8_t* pages,unsigned count)const{
  if(!ready || !pages || count!=PioneerResidentBanks::Pages || !PioneerQ38GdnInputShape(descriptor))return false;
  for(unsigned i=0;i<2;i++){
   unsigned at=descriptor[i?14:11]*unsigned(plan->Capacity/4096)+(descriptor[i?15:12]-plan->Base)/4096;
   if(pages[at]!=1)return false;
  }
  return true; // uploaded original F32 pages,NOT resident input/producer proof
 }
 const uint32_t* Descriptor()const{return ready?descriptor:nullptr;}
};
#endif
