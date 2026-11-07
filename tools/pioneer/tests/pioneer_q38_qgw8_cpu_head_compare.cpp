/* SPDX-License-Identifier: MIT */
// One fixed token, complete CPU-only QGW8 FFN+head numerical observation.
#if !defined(PIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC) || PIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error QGW8 comparison requires the isolated CPU-only quality diagnostic
#endif
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_reference_math.h"
#include "pioneer_q38_whole_original_bytes.h"
#include "pioneer_q38_bfp4_full_cpu_backend.h"
#include "pioneer_q38_head_bfp8_cpu_backend.h"
#include "pioneer_q38_distribution_compare.h"
#include "../PioneerModelWeightCache.h"
#include <cstdio>
#include <cstring>
#include <memory>

static bool Pin(const Q38CpuTokenTrace& t){
 if(t.position || t.input!=248319 || t.hidden.size()!=65*5120 || t.logits.size()!=248320)return false;
 std::vector<uint32_t> words={0x31524651,1,t.position,t.input,t.selected,t.runnerUp,t.best,t.second,65,5120,248320,0,0,0,0,0};
 words.insert(words.end(),t.hidden.begin(),t.hidden.end());words.insert(words.end(),t.logits.begin(),t.logits.end());words.push_back(0x454e4451);
 return PioneerModelWeightCache::Hex(PioneerModelWeightCache::Hash(words.data(),words.size()*sizeof(uint32_t)))==
  "9323e2451c41e241c9a0391d3905890b662fd73df8542103da229f9b910d6937";
}
static bool Normalized(WholeModelOriginalBytes& source,const Q38CpuTokenTrace& trace,std::vector<uint32_t>& output){
 if(trace.hidden.size()!=65*5120 || !source.Ready())return false;
 unsigned index=UINT32_MAX;
 for(unsigned i=0;i<851;i++){
  Q38OriginalTensor t;if(!source.Describe(i,t))return false;
  if(t.name=="output_norm.weight"){
   if(index!=UINT32_MAX || t.type!=0 || t.rank!=1 || t.inputs!=5120 || t.outputs!=1 || t.bytes!=5120*4)return false;
   index=i;
  }
 }
 if(index==UINT32_MAX)return false;
 std::vector<uint32_t> weights(5120);if(!source.ReadOriginal(index,0,5120*4,weights.data(),5120*4))return false;
 std::vector<float> hidden(5120),norm(5120),normalized;
 for(unsigned i=0;i<5120;i++){
  hidden[i]=Q38CpuReferenceMath::Float(trace.hidden[64*5120+i]);norm[i]=Q38CpuReferenceMath::Float(weights[i]);
 }
 if(!Q38CpuReferenceMath::Rms(hidden,norm,normalized) || !source.Ready())return false;
 std::vector<uint32_t> result;result.reserve(5120);for(float x:normalized)result.push_back(Q38CpuReferenceMath::Bits(x));
 output=std::move(result);return true;
}
static bool Report(const char* label,const std::vector<uint32_t>& original,const std::vector<uint32_t>& candidate){
 Q38DistributionCompare::Result r={};if(!Q38DistributionCompare::Compute(original,candidate,r))return false;
 printf("QGW8_CPU_RESULT %s original_top%u candidate_top%u same%u kl_original_to_candidate%.9g top5_overlap%u original_top_probability%.9g candidate_top_probability%.9g\n",
  label,r.originalTop,r.candidateTop,unsigned(r.originalTop==r.candidateTop),r.kl,r.topFiveOverlap,
  r.originalTopProbability,r.candidateTopProbability);return !fflush(stdout);
}
int main(int argc,char** argv){
 static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"pinned CPU trace byte order");
 if(argc!=3 || strcmp(argv[1],"--fixed-one-qgw8-head") || argv[2][0]!='/')return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("QGW8_CPU authenticating original; fixed token 248319; complete logits; no device/Work/TPS");
 WholeModelOriginalBytes source;if(!source.Open(argv[2]) || !source.Ready())return 1;
 Q38CpuModelReference original,candidate;if(!original.Bind(source) || !candidate.Bind(source))return 1;
 Q38CpuTokenTrace baseline,ffn;
 if(!original.Step(248319,baseline) || !Pin(baseline) || !source.Ready())return 1;
 puts("QGW8_CPU original trace matches independently pinned fixed-token reference");
 constexpr uint64_t layers=~uint64_t(0),eight=UINT64_C(0x1ffc000000000000);
 std::unique_ptr<void,decltype(&Q38Bfp4CpuClose)> backend(Q38Bfp4CpuOpen(argv[2],layers,eight),Q38Bfp4CpuClose);
 if(!backend || !candidate.StepWithSelectedFfnDiagnostic(248319,layers,Q38Bfp4CpuProject,backend.get(),ffn)
  || !Q38Bfp4CpuComplete(backend.get()) || ffn.logits.size()!=248320 || !source.Ready())return 1;
 if(!Report("ffn_only",baseline.logits,ffn.logits))return 1;
 std::vector<uint32_t> input,converted;
 if(!Normalized(source,ffn,input) || !Q38HeadBfp8CpuObserve(argv[2],input,converted)
  || converted.size()!=248320 || !source.Ready())return 1;
 if(!Report("head_only_same_hidden",ffn.logits,converted) || !Report("combined_ffn_and_head",baseline.logits,converted))return 1;
 puts("PASS_QGW8_ONE_TOKEN_CPU_COMPARISON; ONE FIXED TOKEN, SOFTWARE MODEL ONLY; NOT DEVICE, WORK, ANSWER QUALITY OR SPEED");
 return fflush(stdout)?1:0;
}
