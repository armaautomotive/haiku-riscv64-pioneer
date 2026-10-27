/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_RECURRENT_BLOCK_PLAN_H
#define PIONEER_Q38_MIXED_RECURRENT_BLOCK_PLAN_H
#include "PioneerQ38MixedRecurrentBlockRootLayout.h"
#include "PioneerQ38MixedRecurrentProjectionPlan.h"
#include "PioneerQ38MixedRecurrentCohortPlan.h"
// Immutable HEAP DATA. No source authentication, initialized state, input
// producer, card submission or native lifetime is conferred by this plan.
class PioneerQ38MixedRecurrentBlockPlan {
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 PioneerQ38MixedRecurrentProjectionPlan matrices[8];PioneerQ38MixedRecurrentCohortPlan cohort;
 uint32_t outer[128]={},bound[128]={},boots[120][64]={},recipes[8][1280]={};
 const PioneerQ38CanonicalPart* norm[2]={};bool attempted=false,ready=false;
 static const PioneerQ38CanonicalPart* Norm(const PioneerQ38CanonicalModelPlan& c,unsigned layer,unsigned role){
  const PioneerQ38CanonicalPart* found=nullptr;
  for(unsigned i=0;i<c.Count();i++){
   const auto* p=c.Part(i);if(p->kind!=c.WeightKind || p->layer!=layer || p->role!=role)continue;
   if(found || p->type || p->firstRow || p->rows!=1 || p->rowBytes!=20480 || p->bytes!=20480 || p->allocated!=20480)return nullptr;
   found=p;
  }
  return found;
 }
 bool Recipes()const{
  for(unsigned slot=0;slot<8;slot++){
   const auto& p=matrices[slot];const auto* r=recipes[slot];
   for(unsigned i=0;i<1280;i++){
    unsigned expected=i<64?(i>=48?0:i==0?unsigned(TT_Q38_MIXED_RECURRENT_MATRIX):i==13?unsigned(TT_Q38_DENSE_MATRIX_ROWS):p.Frame()[i]):
     i<80?p.Header()[i-64]:i<1032?p.Parts()[(i-80)/8][(i-80)%8]:i<1151?p.Rows()[i-1032]:i<1159?p.Banks()[i-1151]:0;
    if(r[i]!=expected)return false;
   }
  }
  return true;
 }
public:
 PioneerQ38MixedRecurrentBlockPlan()=default;
 PioneerQ38MixedRecurrentBlockPlan(const PioneerQ38MixedRecurrentBlockPlan&)=delete;
 PioneerQ38MixedRecurrentBlockPlan& operator=(const PioneerQ38MixedRecurrentBlockPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& c,unsigned layer,unsigned position,unsigned token,unsigned bank,unsigned slot,
  unsigned phase,unsigned request,const PioneerQ38GdnIdentity& id,const uint32_t* sources,unsigned inputBank,unsigned inputAddress){
  namespace L=PioneerQ38MixedRecurrentBlockRootLayout;namespace M=PioneerQ38MixedWorkerLayout;
  if(attempted)return false;
  attempted=true;
  if(!c.Ready() || layer>=64 || layer%4==3 || position>=8192 || token>=248320 || bank>=7 || slot>1 || !phase || phase>=0x7ffffff0
   || !request || request>=0x7ffffffe || !id.Valid() || !sources || inputBank>=7 || inputAddress<c.Base
   || uint64_t(inputAddress)+20480>uint64_t(c.Base)+c.Capacity || (inputAddress&63))return false;
  for(unsigned w=0;w<120;w++)if(!sources[w*16])return false;
  for(unsigned i=0;i<8;i++)if(!matrices[i].Build(c,layer,L::Role(i),position,token,bank,phase+L::PhaseDelta(i),request,id,sources))return false;
  if(!cohort.Build(c,layer,position,bank,slot,token,phase+4,request,id,sources))return false;
  for(unsigned i=0;i<2;i++){norm[i]=Norm(c,layer,i);if(!norm[i])return false;}
  canonical=&c;
  const auto* ch=cohort.Header();auto* o=outer;
  const unsigned head[]={L::Magic,1,128,layer,position,129,phase,bank,ch[9],ch[8],5120,5120,slot,token};
  for(unsigned i=0;i<14;i++)o[i]=head[i];
  for(unsigned i=0;i<16;i++)o[16+i]=sources[i];
  for(unsigned i=0;i<8;i++)o[32+i]=ch[32+i];
  const unsigned tail[]={request,119,L::Recipes,L::RecipeCopies,L::RecipeStride,L::Original,L::OriginalCopy,L::Final,L::FinalOffset,
   o[9]+L::MetaOffset,o[9]+L::ConvMetaOffset,o[9]+L::GdnMetaOffset,
   PioneerQ38MixedRecurrentCohortLayout::ConvWords,PioneerQ38MixedRecurrentCohortLayout::GdnWords,8,1};
  for(unsigned i=0;i<16;i++)o[48+i]=tail[i];
  for(unsigned i=0;i<2;i++){o[64+i*2]=norm[i]->bank;o[65+i*2]=c.Base+norm[i]->offset;}
  o[68]=inputBank;o[69]=inputAddress;o[70]=5120;o[71]=1;
  for(unsigned w=0;w<120;w++){
   auto* b=boots[w];const unsigned h[]={w?unsigned(M::Magic):unsigned(L::Magic),1,64,w?w:129,w?M::Peer(w):sources[0],w?sources[w*16]:119,8192,64};
   for(unsigned i=0;i<8;i++){b[i]=h[i];b[8+i]=ch[32+i];}for(unsigned i=0;i<16;i++)b[16+i]=sources[w*16+i];
  }
  for(unsigned n=0;n<8;n++)for(unsigned i=0;i<1280;i++){
   const auto& p=matrices[n];recipes[n][i]=i<64?(i>=48?0:i==0?unsigned(TT_Q38_MIXED_RECURRENT_MATRIX):i==13?unsigned(TT_Q38_DENSE_MATRIX_ROWS):p.Frame()[i]):
    i<80?p.Header()[i-64]:i<1032?p.Parts()[(i-80)/8][(i-80)%8]:i<1151?p.Rows()[i-1032]:i<1159?p.Banks()[i-1151]:0;
  }
  for(unsigned i=0;i<128;i++)bound[i]=o[i];
  ready=true;
  if(Matches(c))return true;
  ready=false;return false;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c)const{
  namespace L=PioneerQ38MixedRecurrentBlockRootLayout;namespace M=PioneerQ38MixedWorkerLayout;
  if(!ready || canonical!=&c || !c.Ready() || !L::OuterShape(outer) || !L::BootShape(boots[0]) || !cohort.Matches(c))return false;
  for(unsigned i=0;i<128;i++)if(outer[i]!=bound[i])return false;
  for(unsigned n=0;n<8;n++)if(!matrices[n].Matches(c))return false;
  for(unsigned w=0;w<120;w++){
   const auto* b=boots[w];if(w && !M::BootShape(b))return false;
   const unsigned h[]={w?unsigned(M::Magic):unsigned(L::Magic),1,64,w?w:129,w?M::Peer(w):cohort.Header()[16],w?b[16]:119,8192,64};
   for(unsigned i=0;i<64;i++){
    unsigned expected=i<8?h[i]:i<16?outer[24+i]:i<32?cohortSource(w,i-16):0;
    if(b[i]!=expected)return false;
   }
  }
  return Recipes();
 }
 uint32_t cohortSource(unsigned worker,unsigned word)const{
  if(!worker)return cohort.Header()[16+word];
  return cohort.Conv()[worker*384+16+word];
 }
 bool LearnedPagesPresent(const uint8_t* pages,unsigned count)const{
  if(!canonical || !Matches(*canonical) || !cohort.LearnedPagesPresent(pages,count))return false;
  for(unsigned n=0;n<8;n++)if(!matrices[n].LearnedPagesPresent(pages,count))return false;
  for(unsigned n=0;n<2;n++){
   unsigned first=norm[n]->bank*unsigned(canonical->Capacity/4096)+norm[n]->offset/4096;
   if(first>=count || count-first<5)return false;
   for(unsigned i=0;i<5;i++)if(pages[first+i]!=1)return false;
  }
  return true; // neither current/next state nor input initialization/producer
 }
 const uint32_t* Outer()const{return ready?outer:nullptr;}
 const uint32_t* Boot(unsigned w)const{return ready && w<120?boots[w]:nullptr;}
 const uint32_t* Recipe(unsigned slot)const{return ready && slot<8?recipes[slot]:nullptr;}
 const uint32_t* Conv()const{return ready?cohort.Conv():nullptr;}
 const uint32_t* Gdn()const{return ready?cohort.Gdn():nullptr;}
};
static_assert(sizeof(PioneerQ38MixedRecurrentBlockPlan)>1048576 && sizeof(PioneerQ38MixedRecurrentBlockPlan)<2097152,"complete immutable layer HEAP ONLY");
#endif
