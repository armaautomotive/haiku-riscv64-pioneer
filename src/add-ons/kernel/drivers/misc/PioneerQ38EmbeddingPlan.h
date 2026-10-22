/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EMBEDDING_PLAN_H
#define PIONEER_Q38_EMBEDDING_PLAN_H
#include "PioneerQ38CanonicalModelPlan.h"
#include "PioneerQ38EmbeddingShape.h"
#include "PioneerQ38GdnIdentity.h"
// Immutable pointer-bound canonical DATA,NO IO/initialization/source or
// producer authority. A future exclusive native owner must bind the actual
// retained learned-page ledger and new initialized projection workspace.
class PioneerQ38EmbeddingPlan {
 const PioneerQ38CanonicalModelPlan* plan=nullptr;
 uint32_t descriptor[64]={};
 bool attempted=false,ready=false;
 static bool Describe(const PioneerQ38CanonicalModelPlan& canonical,unsigned token,unsigned position,unsigned bank,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase,uint32_t* d){
  if(!canonical.Ready() || token>=248320 || position>=8192 || bank>=7 || !id.Valid() || !source || !source[0]
   || !phase || phase>=0x7fffffff)return false;
  const PioneerQ38CanonicalPart* selected=nullptr;
  for(unsigned stripe=0;stripe<7;stripe++){
   const auto* p=canonical.ProjectionStripe(64,0,stripe);
   if(!p || p->bank!=stripe || p->offset || p->firstRow!=PioneerQ38EmbeddingFirst(stripe)
    || p->rows!=PioneerQ38EmbeddingRows(stripe) || p->type!=12 || p->rowBytes!=2880)return false;
   if(token>=p->firstRow && token-p->firstRow<p->rows)selected=p;
  }
  const auto* workspace=canonical.Workspace(bank);
  if(!selected || !workspace || workspace->bytes!=67108864)return false;
  for(unsigned i=0;i<64;i++)d[i]=0;
  d[0]=TT_Q38_EMBEDDING_MARKER;d[1]=1;d[2]=workspace->bytes;d[3]=bank;d[4]=canonical.Base+workspace->offset;
  d[5]=token;d[6]=phase;d[7]=248320;d[8]=PioneerQ38ProjectionRoute(bank);d[9]=129;
  d[10]=selected->bank;d[11]=canonical.Base+selected->offset+((token-selected->firstRow)/16)*46080;
  d[12]=PioneerQ38ProjectionRoute(selected->bank);d[13]=(token-selected->firstRow)%16;d[14]=5120;d[15]=12;
  for(unsigned i=0;i<16;i++)d[16+i]=source[i];
  const uint64_t identity[]={id.session,id.sequence,id.modelEpoch,id.driverGeneration};
  for(unsigned i=0;i<4;i++){d[32+i*2]=uint32_t(identity[i]);d[33+i*2]=uint32_t(identity[i]>>32);}
  d[40]=position;d[42]=selected->firstRow;d[43]=selected->rows;d[44]=canonical.Base+selected->offset;
  d[45]=selected->rowBytes;d[46]=2304;d[47]=20;d[48]=46080;d[49]=20480;
  return PioneerQ38EmbeddingShape(d);
 }
public:
 PioneerQ38EmbeddingPlan()=default;
 PioneerQ38EmbeddingPlan(const PioneerQ38EmbeddingPlan&)=delete;
 PioneerQ38EmbeddingPlan& operator=(const PioneerQ38EmbeddingPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& canonical,unsigned token,unsigned position,unsigned bank,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase){
  if(attempted)return false;
  attempted=true;plan=&canonical;ready=Describe(canonical,token,position,bank,id,source,phase,descriptor);return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& canonical,unsigned token,unsigned position,unsigned bank,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase)const{
  if(!ready || plan!=&canonical)return false;
  uint32_t current[64];if(!Describe(canonical,token,position,bank,id,source,phase,current))return false;
  for(unsigned i=0;i<64;i++)if(current[i]!=descriptor[i])return false;
  return true;
 }
 bool LearnedPagesPresent(const uint8_t* initialized,unsigned pages)const{
  if(!ready || !initialized || pages!=PioneerResidentBanks::Pages || !PioneerQ38EmbeddingShape(descriptor))return false;
  // Read the selected16-row group ONLY,including both boundary pages.
  unsigned offset=descriptor[11]-plan->Base,first=offset/4096,last=(offset+46080-1)/4096;
  unsigned base=descriptor[10]*unsigned(plan->Capacity/4096);
  for(unsigned page=first;page<=last;page++)if(initialized[base+page]!=1)return false;
  return true;
 }
 const uint32_t* Descriptor()const{return ready?descriptor:nullptr;}
};
#endif
