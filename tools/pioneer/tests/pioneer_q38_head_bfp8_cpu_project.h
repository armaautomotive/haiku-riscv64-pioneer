/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HEAD_BFP8_CPU_PROJECT_H
#define PIONEER_Q38_HEAD_BFP8_CPU_PROJECT_H
#if !defined(PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC) || PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Head projection is a separately compiled CPU-only diagnostic
#endif
#include "../PioneerQ38HeadBlockFloatDiagnostic.h"
#include "../PioneerBfp4PromptQuality.h"
#include <functional>

// The input is a CPU-private normalized hidden state. This software model is
// neither a card observation nor data for Work. Publish only all-or-nothing.
template<class Source>bool Q38HeadBfp8CpuProject(Source& source,unsigned tensor,
 const std::vector<uint32_t>& input,unsigned first,unsigned bands,std::vector<uint32_t>& output,
 const std::function<void(unsigned,unsigned)>& progress={}){
 if(input.size()!=5120 || !bands || first%32 || first>=248320 || bands>(248320-first)/32
  || !source.Ready() || !source.Stable() || !source.Epoch())return false;
 for(uint32_t bits:input)if((bits&0x7f800000)==0x7f800000)return false;
 const uint64_t epoch=source.Epoch();std::vector<uint32_t> staged;staged.reserve(size_t(bands)*32);
 for(unsigned band=0;band<bands;band++){
  std::vector<uint8_t> weights;std::array<uint32_t,32> logits={};
  if(!PioneerQ38HeadBlockFloatDiagnostic::PackBand(source,tensor,first+band*32,
    PioneerBlackholeBlockFloat::Format::Bfp8B,weights)
   || !PioneerBfp4PromptQuality::PredictFormat(PioneerBlackholeBlockFloat::Format::Bfp8B,
    weights.data(),weights.size(),input.data(),unsigned(input.size()),logits)
   || !source.Ready() || !source.Stable() || source.Epoch()!=epoch)return false;
  staged.insert(staged.end(),logits.begin(),logits.end());
  if(progress)progress(band+1,bands);
 }
 if(!source.Ready() || !source.Stable() || source.Epoch()!=epoch)return false;
 output=std::move(staged);return true;
}
#endif
