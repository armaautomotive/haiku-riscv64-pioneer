/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_JOB_BINDING_H
#define PIONEER_Q38_HYBRID_JOB_BINDING_H
#if !defined(PIONEER_Q38_HYBRID_JOB_DIAGNOSTIC) || PIONEER_Q38_HYBRID_JOB_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Hybrid job binding is offline diagnostic data, not Work or device authority
#endif
#include "PioneerQ38HybridModelPlan.h"
#include "PioneerQ38PairedProjectionJobs.h"
#include "PioneerQ38Bfp4WorkerJob.h"
#include "PioneerQ38Bfp8WorkerJob.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
// Bind distinct BFP4 and BFP8 leaf jobs to the RELOCATED per-bank proposal.
// Original Q4 bytes are never valid BFP4 bytes despite identical lengths.
// This is private DATA only: no authenticated source, converted pages,
// producer lifetime, GPU ownership, launch or Work inference is established.
class PioneerQ38HybridJobBinding {
 using Canonical=PioneerQ38CanonicalModelPlan;
 const Canonical* source=nullptr;PioneerQ38HybridModelPlan::Proposal proposal;uint64_t mask=0;
 unsigned stripeIndex[64][3][7]={};
 bool attempted=false,ready=false;
 template<class W>bool Build(const PioneerQ38PairedProjectionJobs& paired,unsigned layer,unsigned role,unsigned position,
  unsigned ordinal,unsigned sequence,bool eight,typename W::Job& output)const{
  using Layout=typename W::L;
  if(!ready || !source || layer>=64 || bool(mask & (uint64_t(1)<<layer))!=eight
   || !paired.Matches(*source,layer,role,position) || ordinal>=paired.Count() || ordinal>=119
   || !sequence || sequence>=0x7fffffff)return false;
  const auto* part=paired.Part(ordinal);if(!part || part->stripe>=7 || !part->groups || (part->groups&1) || part->groups>16)return false;
  const auto* stripe=source->ProjectionStripe(layer,role,part->stripe);
  if(!stripe || stripe->bank!=part->bank || part->firstRow<stripe->firstRow || ((part->firstRow-stripe->firstRow)&31))return false;
  const unsigned index=stripeIndex[layer][role-2][part->stripe];
  if(index>=proposal.parts.size() || source->Part(index)!=stripe)return false;
  const auto& relocated=proposal.parts[index];
  if(relocated.original!=index || relocated.bank!=stripe->bank || relocated.encoding!=(eight?8U:0U))return false;
  const uint64_t bandBytes=uint64_t(paired.Inputs()/32)*Layout::WeightPayload;
  const uint64_t within=uint64_t((part->firstRow-stripe->firstRow)/32)*bandBytes,bytes=uint64_t(part->groups/2)*bandBytes;
  if(within>relocated.bytes || bytes>uint64_t(relocated.bytes)-within || uint64_t(relocated.offset)+within>UINT32_MAX-Canonical::Base)return false;
  const unsigned worker=ordinal+1;
  typename W::Job staged={{W::Magic,W::Version,paired.Inputs(),uint32_t(Canonical::Base+relocated.offset+within),part->groups/2,
   PioneerQ38ProjectionRoute(part->bank),worker,W::Existing::Peer(worker),PioneerQ38Bf16Wire::Magic,
   W::RootInput,W::RootRoute,sequence,W::Output,0,0,0}};
  if(!W::Shape(staged))return false;
  output=staged;return true;
 }
public:
 enum {RootManifestWords=64*3*7};
 bool Bind(const Canonical& canonical,uint64_t bfp8Layers){
  if(attempted)return false;
  attempted=true;PioneerQ38HybridModelPlan::Proposal next;
  if(!PioneerQ38HybridModelPlan::Build(canonical,bfp8Layers,next))return false;
  for(unsigned layer=0;layer<64;layer++)for(unsigned role=2;role<=4;role++)for(unsigned stripe=0;stripe<7;stripe++){
   const auto* part=canonical.ProjectionStripe(layer,role,stripe);if(!part)return false;
   unsigned index=canonical.Count();for(unsigned i=0;i<canonical.Count();i++)if(canonical.Part(i)==part){index=i;break;}
   if(index>=next.parts.size() || next.parts[index].original!=index)return false;
   stripeIndex[layer][role-2][stripe]=index;
  }
  source=&canonical;proposal=std::move(next);mask=bfp8Layers;ready=true;return true;
 }
 bool Build4(const PioneerQ38PairedProjectionJobs& plan,unsigned layer,unsigned role,unsigned position,unsigned ordinal,unsigned sequence,PioneerQ38Bfp4WorkerJob::Job& out)const{
  return Build<PioneerQ38Bfp4WorkerJob>(plan,layer,role,position,ordinal,sequence,false,out);
 }
 bool Build8(const PioneerQ38PairedProjectionJobs& plan,unsigned layer,unsigned role,unsigned position,unsigned ordinal,unsigned sequence,PioneerQ38Bfp8WorkerJob::Job& out)const{
  return Build<PioneerQ38Bfp8WorkerJob>(plan,layer,role,position,ordinal,sequence,true,out);
 }
 bool Matches4(const PioneerQ38PairedProjectionJobs& plan,unsigned layer,unsigned role,unsigned position,unsigned ordinal,unsigned sequence,const PioneerQ38Bfp4WorkerJob::Job& candidate)const{
  PioneerQ38Bfp4WorkerJob::Job expected={};if(!Build4(plan,layer,role,position,ordinal,sequence,expected))return false;
  for(unsigned i=0;i<PioneerQ38Bfp4WorkerJob::Words;i++)if(candidate.words[i]!=expected.words[i])return false;
  return true;
 }
 bool Matches8(const PioneerQ38PairedProjectionJobs& plan,unsigned layer,unsigned role,unsigned position,unsigned ordinal,unsigned sequence,const PioneerQ38Bfp8WorkerJob::Job& candidate)const{
  PioneerQ38Bfp8WorkerJob::Job expected={};if(!Build8(plan,layer,role,position,ordinal,sequence,expected))return false;
  for(unsigned i=0;i<PioneerQ38Bfp8WorkerJob::Words;i++)if(candidate.words[i]!=expected.words[i])return false;
  return true;
 }
 // Model-wide table for the experimental ROOT. This only creates host-side
 // metadata: the eventual owner must authenticate, upload and hold it fixed.
 bool RootManifest(uint32_t* output,unsigned words)const{
  if(!ready || !source || !output || words!=RootManifestWords || mask!=UINT64_C(0x1fff000000000000))return false;
  uint32_t staged[RootManifestWords];
  for(unsigned layer=0;layer<64;layer++)for(unsigned role=2;role<=4;role++){
   unsigned seen=0;
   for(unsigned stripe=0;stripe<7;stripe++){
    const auto* part=source->ProjectionStripe(layer,role,stripe);
    const unsigned index=stripeIndex[layer][role-2][stripe];
    if(!part || part->bank>=7 || (seen&(1U<<part->bank)) || index>=proposal.parts.size() || source->Part(index)!=part)return false;
    const auto& dest=proposal.parts[index];const bool eight=(mask&(UINT64_C(1)<<layer))!=0;
    if(dest.original!=index || dest.bank!=part->bank || dest.encoding!=(eight?8U:0U)
     || dest.offset>UINT32_MAX-Canonical::Base || (dest.offset&4095))return false;
    staged[(layer*3+role-2)*7+part->bank]=Canonical::Base+dest.offset;seen|=1U<<part->bank;
   }
   if(seen!=127)return false;
  }
  for(unsigned i=0;i<RootManifestWords;i++)output[i]=staged[i];
  return true;
 }
 const PioneerQ38HybridModelPlan::Proposal* Plan()const{return ready?&proposal:nullptr;}
};
#endif
