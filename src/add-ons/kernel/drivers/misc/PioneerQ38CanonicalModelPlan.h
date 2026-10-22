/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_CANONICAL_MODEL_PLAN_H
#define PIONEER_Q38_CANONICAL_MODEL_PLAN_H
#include "PioneerQ38GdnBanks.h"
// KERNEL-PRIVATE,HEAP ONLY. Reconstruct the FIXED public27B original-TC
// tiled layout,8192 context,TWO recurrent slots. No GGUF descriptor order,
// caller coordinates,NoC route,host pointer or claimed digest is accepted.
// This is immutable GEOMETRY,NOT source authentication,bank ownership,
// initialization,an input producer or launch/terminal authority. A native
// caller must separately bind the SAME authenticated retained model and
// actual bank/upload ledger under its exclusive context lock.
struct PioneerQ38CanonicalPart {
 uint32_t kind,layer,role,type,bank,firstRow,rows,rowBytes,offset,bytes,allocated;
};
class PioneerQ38CanonicalModelPlan {
 struct Shape {uint32_t inputs,outputs,type;};
 PioneerQ38CanonicalPart parts[6144]={};
 PioneerQ38GdnRegion recurrent[672]={};
 uint32_t firstFfn[64][3]={},workspace[7]={};
 uint64_t used[7]={};
 unsigned count=0,recurrentCount=0,tensors=0;
 bool attempted=false,ready=false;
 static bool Tensor(unsigned layer,unsigned role,Shape& s) {
  static constexpr Shape common[]={{5120,1,0},{5120,1,0},{5120,17408,12},{5120,17408,12},{17408,5120,12}};
  static constexpr Shape delta[]={{5120,6144,8},{5120,10240,8},{48,1,0},{48,1,0},{128,1,0},
   {5120,48,8},{5120,48,8},{4,10240,0},{6144,5120,8}};
  static constexpr Shape attention[]={{5120,12288,8},{5120,1024,8},{5120,1024,8},{256,1,0},{256,1,0},{6144,5120,14}};
  static constexpr Shape global[]={{5120,248320,12},{5120,248320,14},{5120,1,0}};
  if(layer==64){if(role>=3)return false;s=global[role];return true;}
  if(layer>=64 || role>=(layer%4==3?11U:14U))return false;
  s=role<5?common[role]:layer%4==3?attention[role-5]:delta[role-5];return true;
 }
 static unsigned RowBytes(const Shape& s) {
  if(!s.type)return s.inputs*4;
  unsigned block=s.type==8?32:256,tile=s.type==8?576:s.type==12?2304:s.type==14?3392:0;
  return tile && !(s.inputs%block)?s.inputs/block*(tile/16):0;
 }
 bool Stripe(unsigned kind,unsigned layer,unsigned role,unsigned type,unsigned rows,unsigned rowBytes,unsigned unit) {
  if(!rows || !rowBytes || !unit)return false;
  unsigned chunks=rows/unit,n=chunks<7?1:7,counts[7]={},banks[7]={0,1,2,3,4,5,6};
  if(n==1)counts[0]=rows;
  else {for(unsigned b=0;b<7;b++)counts[b]=(chunks/7+unsigned(b<chunks%7))*unit;counts[6]+=rows%unit;}
  // Exact loader ordering:least-used bank first,bank index breaks ties.
  // Sort this snapshot ONCE;appending one stripe must not reorder later ones.
  for(unsigned a=0;a<7;a++)for(unsigned b=a+1;b<7;b++)
   if(used[banks[b]]<used[banks[a]] || (used[banks[b]]==used[banks[a]] && banks[b]<banks[a])){
    unsigned swap=banks[a];banks[a]=banks[b];banks[b]=swap;}
  unsigned first=0;
  for(unsigned p=0;p<n;p++) {
   unsigned bank=banks[p];uint64_t bytes=uint64_t(counts[p])*rowBytes,allocated=(bytes+4095)&~UINT64_C(4095);
   if(count>=6144 || !counts[p] || bytes>UINT32_MAX || used[bank]>Capacity || allocated>Capacity-used[bank])return false;
   parts[count++]={kind,layer,role,type,bank,first,counts[p],rowBytes,uint32_t(used[bank]),uint32_t(bytes),uint32_t(allocated)};
   if(kind==RecurrentKind){if(recurrentCount>=672)return false;
    recurrent[recurrentCount++]={layer,role,first,counts[p],bank,uint32_t(used[bank])};}
   used[bank]+=allocated;first+=counts[p];
  }
  return first==rows;
 }
public:
 static constexpr uint64_t Capacity=UINT64_C(3)*1024*1024*1024;
 static constexpr uint32_t Base=0x04000000,Context=8192;
 enum {WeightKind=0,RecurrentKind=1,ConvKind=2,KvKind=3,WorkspaceKind=4};
 PioneerQ38CanonicalModelPlan()=default;
 PioneerQ38CanonicalModelPlan(const PioneerQ38CanonicalModelPlan&)=delete;
 PioneerQ38CanonicalModelPlan& operator=(const PioneerQ38CanonicalModelPlan&)=delete;
 bool Build() {
  if(attempted)return false;
  attempted=true;
  // Same semantic order as the independently implemented authenticated
  // host loader:global tensors first,then exact layer/role order.
  for(unsigned ordinal=0;ordinal<=64;ordinal++) {
   unsigned layer=ordinal?ordinal-1:64;
   for(unsigned role=0;;role++) {
    Shape s={};if(!Tensor(layer,role,s))break;
    if(layer<64 && role>=2 && role<=4)firstFfn[layer][role-2]=count;
    if(!Stripe(WeightKind,layer,role,s.type,s.outputs,RowBytes(s),64))return false;
    tensors++;
   }
  }
  if(tensors!=851)return false;
  for(unsigned layer=0;layer<64;layer++) {
   if(layer%4==3){if(!Stripe(KvKind,layer,0,0,8,Context*256*2,1))return false;}
   else {
    for(unsigned slot=0;slot<2;slot++)if(!Stripe(RecurrentKind,layer,slot,0,48,65536,1))return false;
    if(!Stripe(ConvKind,layer,0,0,10240,12,64))return false;
   }
  }
  if(recurrentCount!=672)return false;
  for(unsigned bank=0;bank<7;bank++) {
   constexpr unsigned bytes=67108864;
   if(count>=6144 || used[bank]>Capacity || bytes>Capacity-used[bank])return false;
   workspace[bank]=count;parts[count++]={WorkspaceKind,64,0,0,bank,0,1,bytes,uint32_t(used[bank]),bytes,bytes};used[bank]+=bytes;
  }
  ready=true;return true;
 }
 bool Ready()const{return ready;}
 unsigned Count()const{return ready?count:0;}
 const PioneerQ38CanonicalPart* Part(unsigned i)const{return ready && i<count?&parts[i]:nullptr;}
 const PioneerQ38CanonicalPart* FfnStripe(unsigned layer,unsigned role,unsigned stripe)const {
  if(!ready || layer>=64 || role>=3 || stripe>=7)return nullptr;
  return &parts[firstFfn[layer][role]+stripe];
 }
 const PioneerQ38CanonicalPart* Workspace(unsigned bank)const{return ready && bank<7?&parts[workspace[bank]]:nullptr;}
 const PioneerQ38GdnRegion* RecurrentRegions()const{return ready?recurrent:nullptr;}
 unsigned RecurrentCount()const{return ready?recurrentCount:0;}
 uint64_t Used(unsigned bank)const{return ready && bank<7?used[bank]:0;}
 // General learned-matrix geometry for the SAME future persistent family,
 // including small recurrent projections and multi-wave vocabulary rows.
 // Never an input/source/initialized page or native execution grant.
 bool ProjectionGeometry(unsigned selected,unsigned role,unsigned& type,unsigned& inputs,unsigned& outputs)const {
  Shape s={};if(!ready || !Tensor(selected,role,s) || (s.type!=8 && s.type!=12 && s.type!=14))return false;
  type=s.type;inputs=s.inputs;outputs=s.outputs;return true;
 }
 const PioneerQ38CanonicalPart* ProjectionStripe(unsigned selected,unsigned role,unsigned stripe)const {
  unsigned type=0,inputs=0,outputs=0;
  if(!ProjectionGeometry(selected,role,type,inputs,outputs) || stripe>=(outputs/64<7?1U:7U))return nullptr;
  for(unsigned i=0;i<count;i++){
   const auto& p=parts[i];if(p.kind!=WeightKind || p.layer!=selected || p.role!=role)continue;
   if(!stripe)return &p;
   stripe--;
  }
  return nullptr;
 }
 bool ProjectionPagesPresent(unsigned selected,unsigned role,const uint8_t* initialized,unsigned pages)const {
  unsigned type=0,inputs=0,outputs=0;
  if(!ProjectionGeometry(selected,role,type,inputs,outputs) || !initialized || pages!=PioneerResidentBanks::Pages)return false;
  for(unsigned stripe=0;stripe<(outputs/64<7?1U:7U);stripe++){
   const auto* p=ProjectionStripe(selected,role,stripe);if(!p)return false;
   unsigned at=p->bank*unsigned(Capacity/4096)+p->offset/4096;
   for(unsigned i=0;i<p->allocated/4096;i++)if(initialized[at+i]!=1)return false;
  }
  return true;
 }
 // Exact canonical per-layer learned geometry,NOT a page/producer grant.
 // A same-shaped OTHER layer or a shifted learned range cannot pass.
 bool FfnSpan(unsigned layer,unsigned role,unsigned bank,uint64_t offset,unsigned first,unsigned rows)const {
  if(!ready || layer>=64 || role>=3 || bank>=7 || !rows || ((first|rows)&15))return false;
  for(unsigned stripe=0;stripe<7;stripe++) {
   const auto& p=*FfnStripe(layer,role,stripe);
   if(p.bank==bank && first>=p.firstRow && first-p.firstRow<=p.rows && rows<=p.rows-(first-p.firstRow)
    && offset==uint64_t(p.offset)+uint64_t(first-p.firstRow)*p.rowBytes)return true;
  }
  return false;
 }
 // Presence in the ACTUAL retained upload ledger is necessary,but still
 // not authentication of a model/source or proof of an input producer.
 // Refusal never writes the caller's bitmap or performs any device IO.
 bool FfnPagesPresent(unsigned layer,const uint8_t* initialized,unsigned pages)const {
  if(!ready || layer>=64 || !initialized || pages!=PioneerResidentBanks::Pages)return false;
  for(unsigned role=0;role<3;role++)for(unsigned stripe=0;stripe<7;stripe++) {
   const auto& p=*FfnStripe(layer,role,stripe);unsigned first=p.bank*unsigned(Capacity/4096)+p.offset/4096;
   for(unsigned i=0;i<p.allocated/4096;i++)if(initialized[first+i]!=1)return false;
  }
  return true;
 }
};
static_assert(sizeof(PioneerQ38CanonicalModelPlan)>262144 && sizeof(PioneerQ38CanonicalModelPlan)<327680,"complete canonical layout is HEAP ONLY");
#endif
