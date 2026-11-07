/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BFP4_JOB_BINDING_H
#define PIONEER_Q38_BFP4_JOB_BINDING_H
#if !defined(PIONEER_Q38_BFP4_JOB_DIAGNOSTIC) || PIONEER_Q38_BFP4_JOB_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error BFP4 job binding is offline diagnostic data, not Work or device authority
#endif
#include "PioneerQ38Bfp4WorkerJob.h"
#include "PioneerQ38PairedProjectionJobs.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
// Resolve only from the validated canonical paired-Q4 stripe geometry. The
// data footprint is equal but the bytes are NOT interchangeable. This shape
// binding proves neither that BFP4 pages were uploaded nor that ROOT supplied
// the BF16 input or that any native owner may launch/read/write the device.
struct PioneerQ38Bfp4JobBinding {
 using W=PioneerQ38Bfp4WorkerJob;
 static bool Build(const PioneerQ38CanonicalModelPlan& canonical,const PioneerQ38PairedProjectionJobs& plan,
  unsigned layer,unsigned role,unsigned position,unsigned ordinal,unsigned sequence,W::Job& output){
  if(!plan.Matches(canonical,layer,role,position) || ordinal>=plan.Count() || ordinal>=119 || !sequence || sequence>=0x7fffffff)return false;
  const auto* part=plan.Part(ordinal);if(!part || !part->groups || (part->groups&1) || part->groups>16 || part->bank>=7
   || part->offset>UINT32_MAX-PioneerQ38CanonicalModelPlan::Base)return false;
  const unsigned worker=ordinal+1;
  W::Job staged={{W::Magic,W::Version,plan.Inputs(),uint32_t(PioneerQ38CanonicalModelPlan::Base+part->offset),
   part->groups/2,PioneerQ38ProjectionRoute(part->bank),worker,W::Existing::Peer(worker),PioneerQ38Bf16Wire::Magic,
   W::RootInput,W::RootRoute,sequence,W::Output,0,0,0}};
  if(!W::Shape(staged))return false;
  output=staged;return true;
 }
 static bool Matches(const PioneerQ38CanonicalModelPlan& canonical,const PioneerQ38PairedProjectionJobs& plan,
  unsigned layer,unsigned role,unsigned position,unsigned ordinal,unsigned sequence,const W::Job& candidate){
  W::Job expected={};if(!Build(canonical,plan,layer,role,position,ordinal,sequence,expected))return false;
  for(unsigned i=0;i<W::Words;i++)if(expected.words[i]!=candidate.words[i])return false;
  return true;
 }
};
#endif
