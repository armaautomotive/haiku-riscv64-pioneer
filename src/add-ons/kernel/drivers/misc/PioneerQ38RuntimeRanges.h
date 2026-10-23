/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RUNTIME_RANGES_H
#define PIONEER_Q38_RUNTIME_RANGES_H
#include "PioneerQ38CanonicalModelPlan.h"
// DATA ONLY. The canonical loader places every learned part before runtime
// state, without gaps in each bank. Derive ONLY those seven exact suffixes;
// the retained joint owner must still call the real atomic ReserveMutable.
// Neither this recipe nor reservation proves initialization or execution.
static inline bool PioneerQ38RuntimeRanges(const PioneerQ38CanonicalModelPlan& plan,
 PioneerResidentMutableRange (&output)[7]) {
 if(!plan.Ready())return false;
 uint64_t end[7]={},first[7]={};unsigned recurrent=0,conv=0,kv=0,workspace=0;
 unsigned kvHeads[16]={},kvBanks[16]={};
 bool runtime=false;unsigned seen=0;
 for(unsigned i=0;i<plan.Count();i++){
  const auto* p=plan.Part(i);
  if(!p || p->bank>=7 || !p->rows || !p->rowBytes || !p->bytes || !p->allocated
   || p->offset!=end[p->bank] || (p->offset&4095) || (p->allocated&4095)
   || uint64_t(p->rows)*p->rowBytes!=p->bytes
   || ((uint64_t(p->bytes)+4095)&~UINT64_C(4095))!=p->allocated
   || end[p->bank]>plan.Capacity || p->allocated>plan.Capacity-end[p->bank])return false;
  if(p->kind==plan.WeightKind){if(runtime || workspace)return false;}
  else {
   if(!runtime){for(unsigned b=0;b<7;b++){if(!end[b])return false;first[b]=end[b];}runtime=true;}
   if(p->kind==plan.RecurrentKind){
    if(workspace || p->layer>=64 || p->layer%4==3 || p->role>=2 || p->type
     || p->firstRow>=48 || p->rows>48-p->firstRow || p->rowBytes!=65536)return false;
    if(recurrent>=plan.RecurrentCount())return false;
    const auto& r=plan.RecurrentRegions()[recurrent];
    if(r.layer!=p->layer || r.slot!=p->role || r.firstHead!=p->firstRow || r.heads!=p->rows || r.bank!=p->bank || r.offset!=p->offset)return false;
    recurrent++;
   }else if(p->kind==plan.ConvKind){
    if(workspace || p->layer>=64 || p->layer%4==3 || p->role || p->type
     || p->firstRow>=10240 || p->rows>10240-p->firstRow || ((p->firstRow|p->rows)&63)
     || p->rowBytes!=12 || p->allocated!=20480)return false;
    conv++;
   }else if(p->kind==plan.KvKind){
    if(workspace || p->layer>=64 || p->layer%4!=3 || p->role || p->type
     || p->firstRow>=8 || p->rows>8-p->firstRow || p->rowBytes!=4194304)return false;
    unsigned layer=p->layer/4;
    if(p->firstRow!=kvHeads[layer] || (kvBanks[layer]&(1U<<p->bank)))return false;
    kvHeads[layer]+=p->rows;kvBanks[layer]|=1U<<p->bank;
    kv++;
   }else if(p->kind==plan.WorkspaceKind){
    if(p->layer!=64 || p->role || p->type || p->firstRow || p->rows!=1
     || p->bytes!=67108864 || p->rowBytes!=p->bytes || plan.Workspace(p->bank)!=p
     || (seen&(1U<<p->bank)))return false;
    seen|=1U<<p->bank;workspace++;
   }else return false;
  }
  end[p->bank]+=p->allocated;
 }
 if(!runtime || recurrent!=672 || conv!=336 || kv!=112 || workspace!=7 || seen!=127)return false;
 for(unsigned layer=0;layer<16;layer++)if(kvHeads[layer]!=8 || kvBanks[layer]!=127)return false;
 PioneerResidentMutableRange ranges[7];
 for(unsigned b=0;b<7;b++){
  if(end[b]!=plan.Used(b) || first[b]>=end[b] || end[b]>plan.Capacity)return false;
  ranges[b]={b,uint32_t(first[b]/4096),uint32_t((end[b]-first[b])/4096)};
 }
 for(unsigned b=0;b<7;b++)output[b]=ranges[b]; // refusal leaves ALL output bytes unchanged
 return true;
}
#endif
