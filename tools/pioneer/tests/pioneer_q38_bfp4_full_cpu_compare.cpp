/* SPDX-License-Identifier: MIT */
// One position, CPU-only: compare original Q4 with BFP4 FFN substitutions.
#if !defined(PIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC) || PIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error CPU-only BFP4 model comparison role required
#endif
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_model_reference.h"
#include "pioneer_q38_whole_original_bytes.h"
#include "pioneer_q38_bfp4_full_cpu_backend.h"
#include "pioneer_q38_distribution_compare.h"
#include "../PioneerModelWeightCache.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>

static bool Compare(const char* label,const uint32_t* reference,const uint32_t* candidate,size_t count){
 long double energy=0,error=0;double maximum=0;
 for(size_t i=0;i<count;i++){
  float a,b;memcpy(&a,reference+i,4);memcpy(&b,candidate+i,4);
  if(!std::isfinite(a) || !std::isfinite(b))return false;
  long double delta=static_cast<long double>(b)-a;energy+=static_cast<long double>(a)*a;error+=delta*delta;
  maximum=std::max(maximum,double(std::fabs(delta)));
 }
 if(!(energy>0))return false;
 printf("BFP4_CPU_COMPARE %s values%zu relative_l2%.9g max_abs%.9g; CPU_ONLY\n",label,count,double(std::sqrt(error/energy)),maximum);
 return !fflush(stdout);
}
static std::string Hash(const std::vector<uint32_t>& data){
 return PioneerModelWeightCache::Hex(PioneerModelWeightCache::Hash(data.data(),data.size()*4));
}
static bool FullPin(const Q38CpuTokenTrace& t){
 if(t.position || t.input!=248319 || t.hidden.size()!=65*5120 || t.logits.size()!=248320)return false;
 std::vector<uint32_t> bytes={0x31524651,1,t.position,t.input,t.selected,t.runnerUp,t.best,t.second,65,5120,248320,0,0,0,0,0};
 bytes.insert(bytes.end(),t.hidden.begin(),t.hidden.end());bytes.insert(bytes.end(),t.logits.begin(),t.logits.end());bytes.push_back(0x454e4451);
 return Hash(bytes)=="9323e2451c41e241c9a0391d3905890b662fd73df8542103da229f9b910d6937";
}
int main(int argc,char** argv){
 static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"pinned reference byte order");
 if(argc!=3 || (strcmp(argv[1],"--first") && strcmp(argv[1],"--full") && strcmp(argv[1],"--split48")
  && strcmp(argv[1],"--hybrid48") && strcmp(argv[1],"--hybrid51") && strcmp(argv[1],"--hybrid48to60") && strcmp(argv[1],"--bfp8")) || argv[2][0]!='/')return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);const bool first=!strcmp(argv[1],"--first"),split=!strcmp(argv[1],"--split48");
 const bool hybrid=!strcmp(argv[1],"--hybrid48"),hybrid51=!strcmp(argv[1],"--hybrid51");
 const bool window13=!strcmp(argv[1],"--hybrid48to60"),all8=!strcmp(argv[1],"--bfp8");
 puts("CPU_BFP4_COMPARE authenticating original model; NO accelerator or Work");
 WholeModelOriginalBytes source;if(!source.Open(argv[2]) || !source.Ready())return 1;
 Q38CpuModelReference original;if(!original.Bind(source))return 1;
 Q38CpuFirstFfnTrace originalFirst;Q38CpuTokenTrace originalFull;
 if(first){
  if(!original.ObserveFirstFfnTrace(248319,originalFirst)
   || Hash(originalFirst.final)!="0fc71cc5a79d621c65b45aceb6bf705b1989d2b2385af99c8e2a2fbb5e0c064c")return 1;
 }else if(!original.Step(248319,originalFull) || !FullPin(originalFull))return 1;
 puts("CPU_BFP4_COMPARE original trace matches independent pinned reference");
 for(unsigned experiment=0;experiment<(split?2U:1U);experiment++){
  const uint64_t layers=split?(experiment?UINT64_C(0xffff000000000000):UINT64_C(0x0000ffffffffffff)):~uint64_t(0);
  const uint64_t layers8=hybrid?UINT64_C(0xffff000000000000):hybrid51?UINT64_C(0xfff8000000000000):
   window13?UINT64_C(0x1fff000000000000):all8?~uint64_t(0):0;
  const char* name=split?(experiment?"late16":"early48"):hybrid?"hybrid48":hybrid51?"hybrid51":window13?"window13":all8?"all8":"all64";
  printf("CPU_BFP4_EXPERIMENT %s layer_mask%016llx bfp8_mask%016llx; ORIGINAL_Q4_ON_UNSELECTED_LAYERS\n",name,(unsigned long long)layers,(unsigned long long)layers8);
  std::unique_ptr<void,decltype(&Q38Bfp4CpuClose)> backend(Q38Bfp4CpuOpen(argv[2],layers,layers8),Q38Bfp4CpuClose);
  if(!backend || !source.Ready())return 1;
  Q38CpuModelReference candidate;if(!candidate.Bind(source))return 1;
  if(first){
   Q38CpuFirstFfnTrace actual;if(!candidate.ObserveFirstFfnWithDiagnostic(248319,Q38Bfp4CpuProject,backend.get(),actual))return 1;
   if(actual.final.size()!=originalFirst.final.size() || !Compare("first_layer_final",originalFirst.final.data(),actual.final.data(),actual.final.size()))return 1;
  }else{
   Q38CpuTokenTrace actual;if(!candidate.StepWithSelectedFfnDiagnostic(248319,layers,Q38Bfp4CpuProject,backend.get(),actual)
    || !Q38Bfp4CpuComplete(backend.get()) || actual.hidden.size()!=originalFull.hidden.size() || actual.logits.size()!=originalFull.logits.size())return 1;
   for(unsigned layer=0;layer<64;layer++){
    char label[64];snprintf(label,sizeof(label),"%s_layer%u_hidden",name,layer);
    if(!Compare(label,originalFull.hidden.data()+(layer+1)*5120,actual.hidden.data()+(layer+1)*5120,5120))return 1;
   }
   char label[64];snprintf(label,sizeof(label),"%s_logits",name);
   if(!Compare(label,originalFull.logits.data(),actual.logits.data(),actual.logits.size()))return 1;
   Q38DistributionCompare::Result distribution={};
   if(!Q38DistributionCompare::Compute(originalFull.logits,actual.logits,distribution)
    || distribution.originalTop!=originalFull.selected || distribution.candidateTop!=actual.selected)return 1;
   printf("CPU_BFP4_DISTRIBUTION %s kl_original_to_candidate%.9g top5_overlap%u original_top_probability%.9g candidate_top_probability%.9g candidate_original_top_probability%.9g; ONE_POSITION_CPU_ONLY\n",
    name,distribution.kl,distribution.topFiveOverlap,distribution.originalTopProbability,distribution.candidateTopProbability,distribution.candidateOriginalTopProbability);
   printf("CPU_BFP4_TOKEN %s original%u candidate%u same%u; ONE_POSITION_NOT_ANSWER_QUALITY_OR_CARD_SPEED\n",
    name,originalFull.selected,actual.selected,unsigned(originalFull.selected==actual.selected));
  }
 }
 if(!source.Ready())return 1;
 puts("PASS_BFP4_CPU_COMPARISON; NO DEVICE, NO INFERENCE RATE, NO PRODUCTION QUALITY CLAIM");return fflush(stdout)?1:0;
}
