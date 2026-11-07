/* SPDX-License-Identifier: MIT */
// Short teacher-forced sequences with independent retained CPU state.
// Diagnostic only: not P100A execution, Work, or an answer-quality pass.
#if !defined(PIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC) || PIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error CPU-only BFP4 sequence comparison role required
#endif
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_model_reference.h"
#include "pioneer_q38_whole_original_bytes.h"
#include "pioneer_q38_bfp4_full_cpu_backend.h"
#include "pioneer_q38_distribution_compare.h"
#include "../PioneerModelWeightCache.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

static bool FirstPin(const Q38CpuTokenTrace& t){
 if(t.position || t.input!=248319 || t.hidden.size()!=65*5120 || t.logits.size()!=248320)return false;
 std::vector<uint32_t> words={0x31524651,1,t.position,t.input,t.selected,t.runnerUp,t.best,t.second,65,5120,248320,0,0,0,0,0};
 words.insert(words.end(),t.hidden.begin(),t.hidden.end());words.insert(words.end(),t.logits.begin(),t.logits.end());words.push_back(0x454e4451);
 return PioneerModelWeightCache::Hex(PioneerModelWeightCache::Hash(words.data(),words.size()*sizeof(uint32_t)))==
  "9323e2451c41e241c9a0391d3905890b662fd73df8542103da229f9b910d6937";
}
int main(int argc,char** argv){
 static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"pinned reference byte order");
 const bool window11=argc==4 && !strcmp(argv[1],"--prefix-three-window11");
 const bool prefix=argc==4 && (!strcmp(argv[1],"--prefix-three-window13") || window11);
 if((!prefix && (argc!=3 || (strcmp(argv[1],"--two-fixed-window13") && strcmp(argv[1],"--hello-three-window13")))) || argv[2][0]!='/')return 2;
 unsigned initial=9419;
 if(prefix){
  if(argv[3][0]<'0' || argv[3][0]>'9')return 2;
  char* end=nullptr;errno=0;const unsigned long value=strtoul(argv[3],&end,10);
  if(errno || !end || *end || value>=248320)return 2;initial=unsigned(value);
 }
 const bool hello=prefix || !strcmp(argv[1],"--hello-three-window13");
 setvbuf(stdout,nullptr,_IOLBF,0);
 if(prefix)printf("CPU_SEQUENCE authenticating model; raw initial token%u, then original greedy tokens; FFN_BFP8_layers=%s; teacher-forced; NO DEVICE OR WORK\n",initial,window11?"50..60":"48..60");
 else if(hello)puts("CPU_SEQUENCE authenticating model; raw Hello token9419, then original greedy tokens; teacher-forced; NO DEVICE OR WORK");
 else puts("CPU_SEQUENCE authenticating model; fixed inputs 248319,17; teacher-forced; NO DEVICE OR WORK");
 WholeModelOriginalBytes source;if(!source.Open(argv[2]) || !source.Ready())return 1;
 Q38CpuModelReference original,candidate;
 if(!original.Bind(source) || !candidate.Bind(source))return 1;
 constexpr uint64_t layers=~uint64_t(0);
 const uint64_t bfp8=window11?UINT64_C(0x1ffc000000000000):UINT64_C(0x1fff000000000000);
 unsigned previousOriginal=0;
 for(unsigned position=0;position<(hello?3U:2U);position++){
  const unsigned token=hello?(position?previousOriginal:initial):(position?17U:248319U);
  Q38CpuTokenTrace baseline,changed;
  if(!original.Step(token,baseline) || baseline.position!=position || baseline.input!=token
   || (!hello && !position && !FirstPin(baseline)) || (!hello && position==1 && baseline.selected!=17)
   || !source.Ready())return 1;
  previousOriginal=baseline.selected;
  // Re-authenticate and create a fresh ordered FFN sweep for each token. The
  // candidate's convolution/GDN/attention history remains in candidate.
  std::unique_ptr<void,decltype(&Q38Bfp4CpuClose)> backend(Q38Bfp4CpuOpen(argv[2],layers,bfp8),Q38Bfp4CpuClose);
  if(!backend || !candidate.StepWithSelectedFfnSequence(token,layers,Q38Bfp4CpuProject,backend.get(),changed)
   || !Q38Bfp4CpuComplete(backend.get()) || changed.position!=position || changed.input!=token
   || changed.hidden.size()!=65*5120 || changed.logits.size()!=248320 || !source.Ready())return 1;
  Q38DistributionCompare::Result result={};
  if(!Q38DistributionCompare::Compute(baseline.logits,changed.logits,result)
   || result.originalTop!=baseline.selected || result.candidateTop!=changed.selected)return 1;
  printf("CPU_SEQUENCE_RESULT position%u input%u original%u candidate%u same%u kl_original_to_candidate%.9g top5_overlap%u original_top_probability%.9g candidate_top_probability%.9g; TEACHER_FORCED_CPU_ONLY\n",
   position,token,baseline.selected,changed.selected,unsigned(baseline.selected==changed.selected),result.kl,
   result.topFiveOverlap,result.originalTopProbability,result.candidateTopProbability);
 }
 if(!source.Ready() || !original.Ready() || !candidate.Ready())return 1;
 puts(prefix?"PASS_THREE_TOKEN_PREFIX_CPU_SEQUENCE; BASELINE_NOT_INDEPENDENTLY_PINNED_NOT_ANSWER_QUALITY_CARD_OR_SPEED":hello?"PASS_THREE_TOKEN_HELLO_CPU_SEQUENCE; BASELINE_NOT_INDEPENDENTLY_PINNED_NOT_ANSWER_QUALITY_CARD_OR_SPEED":
  "PASS_TWO_TOKEN_CPU_SEQUENCE; NOT NATURAL_ANSWER_QUALITY, CARD_EXECUTION OR SPEED");
 return fflush(stdout)?1:0;
}
