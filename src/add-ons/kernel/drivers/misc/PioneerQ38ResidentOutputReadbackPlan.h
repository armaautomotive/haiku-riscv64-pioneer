/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_OUTPUT_READBACK_PLAN_H
#define PIONEER_Q38_RESIDENT_OUTPUT_READBACK_PLAN_H
#include "PioneerQ38ResidentOutputPlan.h"
// Fixed DEBUG observer DATA,not a read/IO capability or a public request.
// The combined matrix overwrites peer-local a0000 and writes its final
// result at100000,NOT the older DEBUG55 result at156000. Observe the
// canonical GDDR destinations instead. A future SAME-owner observer must
// separately prove actual execution receipts,ALL120 STOP/holds,physical
// source/code/counters,initialized mutable ownership and its own bounded
// transfer ledger BEFORE restore. Neither Build nor Page does any IO,
// validates numerical results,publishes a slot or authorizes cleanup.
struct PioneerQ38ResidentOutputReadbackPage {
 uint32_t kind,index,bank,address,route,firstWord,words;
};
class PioneerQ38ResidentOutputReadbackPlan {
 struct Region {uint32_t kind,index,bank,address,route,firstWord,words,firstPage,pages;};
 Region regions[199]={};
 unsigned count=0,words=0,pages=0;bool attempted=false,ready=false;
 bool Append(unsigned kind,unsigned index,unsigned bank,uint32_t address,unsigned length){
  const unsigned n=(length+1023)/1024;
  if(count>=199 || bank>=7 || !length || length>30720 || (address&4095)
   || address<PioneerQ38CanonicalModelPlan::Base
   || uint64_t(address)-PioneerQ38CanonicalModelPlan::Base+n*UINT64_C(4096)>PioneerQ38CanonicalModelPlan::Capacity
   || words>Words || length>Words-words || pages>Pages || n>Pages-pages)return false;
  for(unsigned i=0;i<count;i++){
   const auto& r=regions[i];
   if(r.bank==bank && uint64_t(address)<uint64_t(r.address)+r.pages*UINT64_C(4096)
    && uint64_t(r.address)<uint64_t(address)+n*UINT64_C(4096))return false;
  }
  regions[count++]={kind,index,bank,address,PioneerQ38ProjectionRoute(bank),words,length,pages,n};
  words+=length;pages+=n;return true;
 }
public:
 enum Kind {Convolution=0,ConvolutionNext=1,Gated=2,Operands=3,Lines=4,Scales=5,Output=6,CurrentState=7,NextState=8};
 enum {WorkspaceWords=56256,StateWords=96*8192,Words=WorkspaceWords+2*StateWords,
  WorkspacePages=56,StatePages=96*8,Pages=WorkspacePages+2*StatePages,Regions=199};
 PioneerQ38ResidentOutputReadbackPlan()=default;
 PioneerQ38ResidentOutputReadbackPlan(const PioneerQ38ResidentOutputReadbackPlan&)=delete;
 PioneerQ38ResidentOutputReadbackPlan& operator=(const PioneerQ38ResidentOutputReadbackPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& canonical,const PioneerQ38ResidentOutputPlan& operation,
  unsigned layer,unsigned position,unsigned bank,unsigned slot,unsigned token,unsigned phase,
  const PioneerQ38GdnIdentity& id,const uint32_t* sources,const uint32_t* sequence){
  if(attempted)return false;
  attempted=true;
  if(!operation.Matches(canonical,layer,position,bank,slot,token,phase,id,sources,sequence))return false;
  const auto* m=operation.Manifest();const auto* workspace=canonical.Workspace(bank);
  if(!m || !workspace || workspace->kind!=canonical.WorkspaceKind || workspace->bytes!=67108864
   || workspace->allocated!=67108864 || m[8]!=canonical.Base+workspace->offset)return false;
  const unsigned offsets[]={TT_Q38_CONV_OUTPUT,TT_Q38_CONV_NEXT,TT_Q38_GDN_GATED_OUTPUT,
   TT_Q38_PROJECTION_NATIVE_OFFSET,TT_Q38_PROJECTION_LINES_OFFSET,TT_Q38_PROJECTION_COMPACT_OFFSET,
   TT_Q38_PROJECTION_OUTPUT_OFFSET};
  const unsigned lengths[]={10240,30720,6144,3072,768,192,5120};
  for(unsigned i=0;i<7;i++){
   const unsigned allocated=((lengths[i]+1023)/1024)*4096;
   if(offsets[i]>=workspace->bytes || allocated>workspace->bytes-offsets[i]
    || !Append(i,0,bank,m[8]+offsets[i],lengths[i]))return false;
  }
  if(words!=WorkspaceWords || pages!=WorkspacePages)return false;
  for(unsigned side=0;side<2;side++)for(unsigned half=0;half<96;half++){
   const auto* state=PioneerQ38CohortGdn(m,half+1)+64;
   unsigned found=0;const unsigned head=half/2;
   for(unsigned i=0;i<canonical.RecurrentCount();i++){
    const auto& r=canonical.RecurrentRegions()[i];
    if(r.layer!=layer || r.slot!=(slot^side) || head<r.firstHead || head-r.firstHead>=r.heads)continue;
    const uint64_t offset=uint64_t(r.offset)+(head-r.firstHead)*UINT64_C(65536)+(half&1)*UINT64_C(32768);
    if(++found!=1 || offset>canonical.Capacity || 32768>canonical.Capacity-offset
     || state[4+side*2]!=r.bank || state[5+side*2]!=canonical.Base+offset
     || state[12+side]!=PioneerQ38ProjectionRoute(r.bank)
     || !Append(side?NextState:CurrentState,half,r.bank,canonical.Base+uint32_t(offset),8192))return false;
   }
   if(found!=1)return false;
  }
  ready=count==Regions && words==Words && pages==Pages;return ready;
 }
 unsigned WordCount()const{return ready?words:0;}
 unsigned PageCount()const{return ready?pages:0;}
 bool Page(unsigned ordinal,PioneerQ38ResidentOutputReadbackPage& output)const{
  if(!ready || ordinal>=pages)return false;
  for(unsigned i=0;i<count;i++){
   const auto& r=regions[i];if(ordinal<r.firstPage || ordinal-r.firstPage>=r.pages)continue;
   const unsigned within=ordinal-r.firstPage,first=within*1024,remaining=r.words-first;
   output={r.kind,r.index,r.bank,r.address+within*4096,r.route,r.firstWord+first,remaining<1024?remaining:1024};
   return true;
  }
  return false;
 }
};
static_assert(PioneerQ38ResidentOutputReadbackPlan::Words==1629120
 && PioneerQ38ResidentOutputReadbackPlan::Pages==1592,"fixed BOTH-slot and complete workspace DEBUG geometry");
static_assert(sizeof(PioneerQ38ResidentOutputReadbackPlan)>4096
 && sizeof(PioneerQ38ResidentOutputReadbackPlan)<8192,"fixed observer recipe HEAP ONLY,not a kernel frame");
#endif
