/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DENSE_PROJECTION_PLAN_H
#define PIONEER_Q38_DENSE_PROJECTION_PLAN_H
#include "PioneerQ38ProjectionJobs.h"
#include "PioneerQ38DenseProjectionShape.h"
// HEAP immutable canonical DATA for QG/K/V/O. ZERO sequence placeholders
// are NEVER caller readiness:future on-card scheduler binds actual proven
// next sequences into a separate runtime mailbox. Native source/upload/
// initialization/code/clock/host/cleanup lifetime is explicitly OWED.
class PioneerQ38DenseProjectionPlan {
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 PioneerQ38ProjectionJobs jobs[4];uint32_t frame[4][64]={},header[4][16]={},parts[4][119][8]={},first[4][119]={},banks[8]={},sources[1920]={};
 unsigned layer=64,position=8192,token=248320,bank=7,phase=0;PioneerQ38GdnIdentity identity={};bool attempted=false,ready=false;
public:
 PioneerQ38DenseProjectionPlan()=default;
 PioneerQ38DenseProjectionPlan(const PioneerQ38DenseProjectionPlan&)=delete;
 PioneerQ38DenseProjectionPlan& operator=(const PioneerQ38DenseProjectionPlan&)=delete;
 static unsigned Role(unsigned slot){return slot<3?5+slot:slot==3?10:~0U;}
 bool Build(const PioneerQ38CanonicalModelPlan& c,unsigned selected,unsigned pos,unsigned inputToken,unsigned workspaceBank,
  unsigned firstPhase,const PioneerQ38GdnIdentity& id,const uint32_t* source){
  if(attempted){return false;}attempted=true;
  if(!c.Ready() || selected>=64 || selected%4!=3 || pos>=8192 || inputToken>=248320 || workspaceBank>=7
   || !firstPhase || firstPhase>=0x7ffffffb || !id.Valid() || !source)return false;
  for(unsigned w=0;w<120;w++)if(!source[w*16])return false;
  canonical=&c;layer=selected;position=pos;token=inputToken;bank=workspaceBank;phase=firstPhase;identity=id;
  for(unsigned i=0;i<1920;i++)sources[i]=source[i];
  for(unsigned b=0;b<7;b++)banks[b]=PioneerQ38ProjectionRoute(b);
  for(unsigned slot=0;slot<4;slot++){
   auto& j=jobs[slot];unsigned role=Role(slot);if(!j.Build(c,layer,role,position) || j.Waves()!=1)return false;
   auto* d=frame[slot];auto* h=header[slot];
   d[0]=TT_Q38_DENSE_MATRIX_MARKER;d[1]=1;d[2]=67108864;d[3]=bank;d[4]=c.Base+c.Workspace(bank)->offset;
   d[6]=phase+slot;d[7]=j.Count();d[8]=banks[bank];d[9]=129;d[10]=PioneerQ38DenseProjectionOffset(layer,role);
   d[11]=j.Outputs();d[12]=j.Outputs()*4;d[13]=TT_Q38_DENSE_MATRIX_ROWS;d[14]=j.Inputs();d[15]=j.Type();
   for(unsigned i=0;i<16;i++)d[16+i]=source[i];
   const uint64_t ids[]={id.session,id.sequence,id.modelEpoch,id.driverGeneration};
   for(unsigned i=0;i<4;i++){d[32+i*2]=uint32_t(ids[i]);d[33+i*2]=uint32_t(ids[i]>>32);}
   d[40]=position;d[41]=token;d[42]=layer;d[43]=role;d[44]=j.Count();d[45]=1;
   const unsigned fields[]={TT_Q38_DENSE_MATRIX_MARKER,1,1,j.Type(),j.Inputs(),j.Count(),phase+slot,TT_Q38_DENSE_MATRIX_INPUT,1};
   for(unsigned i=0;i<9;i++)h[i]=fields[i];
   for(unsigned w=0;w<j.Count();w++){
    const auto* p=j.Part(w);auto* out=parts[slot][w];if(!p || p->offset>UINT32_MAX-c.Base)return false;
    out[0]=c.Base+uint32_t(p->offset);out[1]=p->bank;out[2]=p->groups;out[4]=banks[p->bank];out[5]=source[(w+1)*16];first[slot][w]=p->firstRow;
   }
   if(!PioneerQ38DenseProjectionRecipe(d,h,banks,parts[slot],first[slot],false))return false;
  }
  ready=true;return Matches(c);
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c)const{
  if(!ready || canonical!=&c)return false;
  const uint64_t ids[]={identity.session,identity.sequence,identity.modelEpoch,identity.driverGeneration};
  for(unsigned slot=0;slot<4;slot++){
   const auto& j=jobs[slot];const auto* d=frame[slot];
   if(!j.Matches(c,layer,Role(slot),position) || !PioneerQ38DenseProjectionRecipe(d,header[slot],banks,parts[slot],first[slot],false)
    || d[3]!=bank || d[4]!=c.Base+c.Workspace(bank)->offset || d[6]!=phase+slot || d[40]!=position || d[41]!=token
    || d[42]!=layer || d[43]!=Role(slot))return false;
   for(unsigned i=0;i<16;i++)if(d[16+i]!=sources[i])return false;
   for(unsigned i=0;i<4;i++)if(d[32+i*2]!=uint32_t(ids[i]) || d[33+i*2]!=uint32_t(ids[i]>>32))return false;
   for(unsigned w=0;w<j.Count();w++){
    const auto* p=j.Part(w);const auto* q=parts[slot][w];
    if(q[0]!=c.Base+p->offset || q[1]!=p->bank || q[2]!=p->groups || q[5]!=sources[(w+1)*16] || first[slot][w]!=p->firstRow)return false;
   }
  }
  return true;
 }
 bool LearnedPagesPresent(const uint8_t* pages,unsigned count)const{
  if(!canonical || !Matches(*canonical))return false;
  for(unsigned slot=0;slot<4;slot++)if(!jobs[slot].LearnedPagesPresent(pages,count))return false;
  return true; // MATRIX learned pages only,not norm/table/KV/input/native grant
 }
 const uint32_t* Frame(unsigned slot)const{return ready && slot<4?frame[slot]:nullptr;}
 const uint32_t* Header(unsigned slot)const{return ready && slot<4?header[slot]:nullptr;}
 const uint32_t (*Parts(unsigned slot)const)[8]{return ready && slot<4?parts[slot]:nullptr;}
 const uint32_t* Rows(unsigned slot)const{return ready && slot<4?first[slot]:nullptr;}
 const uint32_t* Banks()const{return ready?banks:nullptr;}
 const uint32_t* Source(unsigned worker)const{return ready && worker<120?sources+worker*16:nullptr;}
};
static_assert(sizeof(PioneerQ38DenseProjectionPlan)>114688 && sizeof(PioneerQ38DenseProjectionPlan)<163840,"HEAP ONLY,not kernel stack");
#endif
