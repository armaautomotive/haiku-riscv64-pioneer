/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_OUTPUT_PLAN_H
#define PIONEER_Q38_RESIDENT_OUTPUT_PLAN_H
#include "PioneerQ38ResidentOutputCohortShape.h"
#include "PioneerQ38ConvBatchPlan.h"
#include "PioneerQ38GdnRendezvousPlan.h"
#include "PioneerQ38ProjectionJobs.h"
// HEAP-ONLY immutable canonical DATA for the complete resident recurrent
// layer. No caller routes, weight offsets, descriptors or job geometry.
// Source/sequence/slot parameters are DATA, NOT proof of physical/current
// state. A genuine native joint owner MUST supply its private observed
// values and retain authenticated model/upload/initialized history+GDN,
// completed input producers, code/clock/hosts/ALL120 ownership. This class
// grants NO mapping, IO, boot, current-slot publication or token commit.
class PioneerQ38ResidentOutputPlan {
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 PioneerQ38ConvBatchPlan convolution[120];
 PioneerQ38GdnInputPlan input[96];
 PioneerQ38GdnRendezvousPlan gated[96];
 PioneerQ38ProjectionJobs output;
 uint32_t manifest[TT_Q38_COHORT_WORDS]={},post[TT_Q38_OUTPUT_COHORT_WORDS]={};
 bool attempted=false,ready=false;
 static bool State(const PioneerQ38CanonicalModelPlan& plan,const uint32_t* in,unsigned slot,uint32_t* d){
  if(!in || slot>=2 || !PioneerQ38GdnInputShape(in))return false;
  for(unsigned i=0;i<64;i++)d[i]=0;
  d[0]=TT_Q38_RGDN_MARKER;d[1]=1;d[2]=128;d[3]=64;d[8]=in[6];d[9]=in[7];d[10]=in[5];d[11]=slot;
  d[14]=32768;d[15]=in[9];d[40]=in[40];for(unsigned i=16;i<40;i++)d[i]=in[i];
  for(unsigned side=0;side<2;side++){
   unsigned found=0;const unsigned head=in[7]/2;
   for(unsigned i=0;i<plan.RecurrentCount();i++){
    const auto& r=plan.RecurrentRegions()[i];
    if(r.layer!=in[5] || r.slot!=(slot^side) || head<r.firstHead || head-r.firstHead>=r.heads)continue;
    if(++found!=1 || r.bank>=7 || !r.heads)return false;
    const uint64_t offset=uint64_t(r.offset)+(head-r.firstHead)*UINT64_C(65536)+(in[7]&1)*32768;
    if(offset>plan.Capacity || 32768>plan.Capacity-offset)return false;
    d[4+side*2]=r.bank;d[5+side*2]=plan.Base+uint32_t(offset);d[12+side]=PioneerQ38ProjectionRoute(r.bank);
   }
   if(found!=1)return false;
  }
  return PioneerQ38GdnRecurrentShape(in,d);
 }
 bool Header(const PioneerQ38CanonicalModelPlan& plan,unsigned layer,unsigned position,unsigned bank,unsigned slot,
  unsigned token,unsigned phase,const PioneerQ38GdnIdentity& id,const uint32_t* sources,const uint32_t* sequence)const{
  if(!plan.Ready() || layer>=64 || layer%4==3 || position>=8192 || bank>=7 || slot>=2 || token>=248320
   || !phase || phase>=0x7ffffffa || !id.Valid() || !sources || !sequence)return false;
  for(unsigned w=0;w<120;w++)if(!sources[w*16])return false;
  for(unsigned w=0;w<119;w++)if(sequence[w]>=phase || sequence[w]>=0x7ffffffc)return false;
  return true;
 }
public:
 PioneerQ38ResidentOutputPlan()=default;
 PioneerQ38ResidentOutputPlan(const PioneerQ38ResidentOutputPlan&)=delete;
 PioneerQ38ResidentOutputPlan& operator=(const PioneerQ38ResidentOutputPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& plan,unsigned layer,unsigned position,unsigned bank,unsigned slot,
  unsigned token,unsigned phase,const PioneerQ38GdnIdentity& id,const uint32_t* sources,const uint32_t* sequence){
  if(attempted)return false;
  attempted=true;if(!Header(plan,layer,position,bank,slot,token,phase,id,sources,sequence))return false;
  canonical=&plan;auto* m=manifest;auto* q=post;const auto* workspace=plan.Workspace(bank);
  if(!workspace || workspace->bytes!=67108864 || workspace->allocated!=67108864)return false;
  m[0]=TT_Q38_RESIDENT_COHORT_MARKER;m[1]=1;m[2]=TT_Q38_COHORT_WORDS;m[3]=layer;m[4]=position;m[5]=phase;m[6]=phase+1;
  m[7]=bank;m[8]=plan.Base+workspace->offset;m[9]=PioneerQ38ProjectionRoute(bank);m[10]=129;m[11]=119;m[12]=96;m[13]=640;m[14]=slot;
  m[40]=120;m[41]=384;m[42]=192;for(unsigned i=0;i<16;i++)m[16+i]=sources[i];
  const uint64_t identity[]={id.session,id.sequence,id.modelEpoch,id.driverGeneration};
  for(unsigned i=0;i<4;i++){m[32+i*2]=uint32_t(identity[i]);m[33+i*2]=uint32_t(identity[i]>>32);}
  for(unsigned w=0;w<119;w++)m[64+w]=sequence[w];
  for(unsigned w=0;w<120;w++){
   const auto* src=sources+w*16;for(unsigned i=0;i<16;i++)m[TT_Q38_COHORT_SOURCE_WORD+w*16+i]=src[i];
   if(!convolution[w].Build(plan,layer,position,bank,w,id,src,phase))return false;
   for(unsigned i=0;i<384;i++)m[TT_Q38_COHORT_CONV_WORD+w*384+i]=convolution[w].Records()[i];
   if(!w || w>96)continue;
   auto* record=m+TT_Q38_COHORT_GDN_WORD+(w-1)*192;
   if(!input[w-1].Build(plan,layer,w-1,position,bank,id,src,phase+1))return false;
   for(unsigned i=0;i<64;i++)record[i]=input[w-1].Descriptor()[i];
   if(!State(plan,record,slot,record+64) || !gated[w-1].Build(plan,record,record+64))return false;
   for(unsigned i=0;i<64;i++)record[128+i]=gated[w-1].Descriptor()[i];
  }
  if(!output.Build(plan,layer,13,position) || output.Type()!=8 || output.Inputs()!=6144
   || output.Outputs()!=5120 || output.Count()!=119 || output.Waves()!=1)return false;
  q[0]=TT_Q38_RESIDENT_OUTPUT_COHORT_MARKER;q[1]=1;q[2]=TT_Q38_OUTPUT_COHORT_WORDS;q[3]=layer;q[4]=position;
  q[5]=phase+2;q[6]=phase+3;q[7]=phase+4;q[8]=bank;q[9]=m[8];q[10]=m[9];q[11]=token;q[12]=6144;q[13]=5120;q[14]=119;
  for(unsigned i=16;i<40;i++)q[i]=m[i];
  for(unsigned w=0;w<119;w++){
   const auto& p=*output.Part(w);auto* r=q+TT_Q38_OUTPUT_COHORT_PART+w*8;
   if(p.offset>UINT32_MAX-plan.Base)return false;
   r[0]=plan.Base+uint32_t(p.offset);r[1]=p.bank;r[2]=p.groups;r[4]=PioneerQ38ProjectionRoute(p.bank);r[5]=sources[(w+1)*16];
   q[TT_Q38_OUTPUT_COHORT_ROW+w]=p.firstRow;
  }
  ready=PioneerQ38ResidentCohortShape(m) && PioneerQ38ResidentOutputCohortShape(q,m);
  if(ready && Matches(plan,layer,position,bank,slot,token,phase,id,sources,sequence))return true;
  ready=false;return false;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& plan,unsigned layer,unsigned position,unsigned bank,unsigned slot,
  unsigned token,unsigned phase,const PioneerQ38GdnIdentity& id,const uint32_t* sources,const uint32_t* sequence)const{
  if(!ready || canonical!=&plan || !Header(plan,layer,position,bank,slot,token,phase,id,sources,sequence)
   || !PioneerQ38ResidentCohortShape(manifest) || !PioneerQ38ResidentOutputCohortShape(post,manifest)
   || manifest[3]!=layer || manifest[4]!=position || manifest[5]!=phase || manifest[7]!=bank
   || manifest[14]!=slot || post[11]!=token || !output.Matches(plan,layer,13,position))return false;
  const uint64_t identity[]={id.session,id.sequence,id.modelEpoch,id.driverGeneration};
  for(unsigned i=0;i<4;i++)if(manifest[32+i*2]!=uint32_t(identity[i]) || manifest[33+i*2]!=uint32_t(identity[i]>>32))return false;
  for(unsigned w=0;w<119;w++)if(manifest[64+w]!=sequence[w])return false;
  for(unsigned w=0;w<120;w++){
   const auto* src=sources+w*16;const auto* record=PioneerQ38CohortConv(manifest,w);
   if(!convolution[w].Matches(plan,layer,position,bank,w,id,src,phase))return false;
   for(unsigned i=0;i<16;i++)if(PioneerQ38CohortSource(manifest,w)[i]!=src[i])return false;
   for(unsigned i=0;i<384;i++)if(record[i]!=convolution[w].Records()[i])return false;
   if(!w || w>96)continue;
   const auto* g=PioneerQ38CohortGdn(manifest,w);uint32_t state[64];
   if(!input[w-1].Matches(plan,layer,w-1,position,bank,id,src,phase+1) || !State(plan,g,slot,state)
    || !gated[w-1].Matches(plan,g,g+64))return false;
   for(unsigned i=0;i<64;i++)if(g[i]!=input[w-1].Descriptor()[i] || g[64+i]!=state[i]
    || g[128+i]!=gated[w-1].Descriptor()[i])return false;
  }
  for(unsigned w=0;w<119;w++){
   const auto& p=*output.Part(w);const auto* r=post+TT_Q38_OUTPUT_COHORT_PART+w*8;
   if(r[0]!=plan.Base+p.offset || r[1]!=p.bank || r[2]!=p.groups || r[4]!=PioneerQ38ProjectionRoute(p.bank)
    || r[5]!=sources[(w+1)*16] || post[TT_Q38_OUTPUT_COHORT_ROW+w]!=p.firstRow)return false;
  }
  return true;
 }
 bool LearnedPagesPresent(const uint8_t* pages,unsigned count)const{
  if(!ready || !canonical || !pages || count!=PioneerResidentBanks::Pages)return false;
  const PioneerQ38GdnIdentity id={uint64_t(manifest[32])|(uint64_t(manifest[33])<<32),
   uint64_t(manifest[34])|(uint64_t(manifest[35])<<32),uint64_t(manifest[36])|(uint64_t(manifest[37])<<32),
   uint64_t(manifest[38])|(uint64_t(manifest[39])<<32)};
  // Page presence must describe the SAME private canonical recipes, not
  // a structurally plausible mutated schedule. This still does NOT bind
  // the caller's current physical source/token/slot; Matches and actual
  // native ownership checks remain mandatory at that boundary.
  if(!Matches(*canonical,manifest[3],manifest[4],manifest[7],manifest[14],post[11],manifest[5],id,
    manifest+TT_Q38_COHORT_SOURCE_WORD,manifest+TT_Q38_COHORT_SEQUENCE_WORD)
   || !output.LearnedPagesPresent(pages,count))return false;
  for(unsigned w=0;w<120;w++)if(!convolution[w].LearnedPagesPresent(pages,count))return false;
  for(unsigned h=0;h<96;h++)if(!input[h].LearnedPagesPresent(pages,count) || !gated[h].LearnedPagePresent(pages,count))return false;
  return true; // actual upload ledger DATA,NOT authenticated/initialized-state or native authority
 }
 const uint32_t* Manifest()const{return ready?manifest:nullptr;}
 const uint32_t* PostManifest()const{return ready?post:nullptr;}
};
static_assert(sizeof(PioneerQ38ResidentOutputPlan)>524288 && sizeof(PioneerQ38ResidentOutputPlan)<1048576,"ALL120 immutable canonical recipes/schedules HEAP ONLY");
#endif
