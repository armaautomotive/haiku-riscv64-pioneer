/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_LAYER_READBACK_PLAN_H
#define PIONEER_Q38_RESIDENT_LAYER_READBACK_PLAN_H
#include "PioneerQ38ResidentLayerPlan.h"
// Fixed NEW complete-layer DEBUG observation DATA,not native/IO authority.
// FFN reuses old operand/scale/RMS-backup workspace:NEVER reuse the old
// prefix payload just because its page COUNT happens to be the same.
// Native owner still owes genuine completed ROOT/119 proofs,ALL STOPs,
// ALL120 holds and SAME source/clock/maps/hosts before ANY transfer.
struct PioneerQ38ResidentLayerReadbackPage {
 uint32_t bank=7,address=0,route=0,firstWord=0,words=0,kind=0;
};
class PioneerQ38ResidentLayerReadbackPlan {
 const PioneerQ38ResidentLayerPlan* recipe=nullptr;
 bool attempted=false,ready=false;
public:
 enum {Marker=0x31564c51,Version=1,Pages=1592,Words=1630208,
  FinalHidden=0,Conv=1,History=2,Gated=3,Projection=4,CurrentState=5,NextState=6};
 bool Build(const PioneerQ38ResidentLayerPlan& layer){
  if(attempted)return false;
  attempted=true;
  const auto* m=layer.Manifest();const auto* q=layer.PostManifest();const auto* l=layer.LayerManifest();
  if(!m || !q || !l || !layer.CanonicalRecipe() || !PioneerQ38ResidentLayerShape(l,m,q) || m[3] || m[4] || m[14])return false;
  recipe=&layer;ready=true;return true;
 }
 unsigned PageCount()const{return ready?unsigned(Pages):0;}
 bool Page(unsigned index,PioneerQ38ResidentLayerReadbackPage& out)const{
  if(!ready || !recipe || index>=Pages)return false;
  const auto* m=recipe->Manifest();const auto* q=recipe->PostManifest();const auto* l=recipe->LayerManifest();
  if(!recipe->CanonicalRecipe() || !PioneerQ38ResidentLayerShape(l,m,q) || m[3] || m[4] || m[14])return false;
  PioneerQ38ResidentLayerReadbackPage value;
  value.firstWord=index*1024;value.words=1024;
  if(index<56){
   const unsigned first[]={0,5,15,45,51},count[]={5,10,30,6,5},offset[]={0,0x120000,0x130000,0x150000,0x100000};
   unsigned kind=0;while(kind<5 && index>=first[kind]+count[kind])kind++;
   if(kind>=5)return false;
   value.kind=kind;value.bank=m[7];value.route=m[9];value.address=m[8]+offset[kind]+(index-first[kind])*4096;
  }else{
   const unsigned slot=(index-56)/768,at=(index-56)%768,half=at/8,page=at%8;
   if(slot>=2 || half>=96)return false;
   const auto* d=PioneerQ38CohortGdn(m,half+1)+64;
   value.kind=slot?NextState:CurrentState;value.bank=d[4+slot*2];value.address=d[5+slot*2]+page*4096;
   value.route=d[12+slot];
  }
  if(value.bank>=7 || value.route!=PioneerQ38ProjectionRoute(value.bank) || (value.address&4095)
   || value.address<0x04000000 || value.address>0xc3fff000 || value.firstWord>Words-1024)return false;
  out=value;return true; // no caller-selected address,route,count or output mutation on refusal
 }
};
static_assert(PioneerQ38ResidentLayerReadbackPlan::Pages*1024==PioneerQ38ResidentLayerReadbackPlan::Words,"all NEW pages are complete;different from old partial-prefix payload");
#endif
