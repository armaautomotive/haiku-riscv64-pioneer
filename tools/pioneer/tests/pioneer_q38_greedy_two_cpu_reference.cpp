/* SPDX-License-Identifier: MIT */
// Two-position original-weight CPU reference for a future QGWD diagnostic.
// This is not a device backend, Work implementation, or speed benchmark.
#if !defined(PIONEER_Q38_GREEDY_TWO_CPU_DIAGNOSTIC) || PIONEER_Q38_GREEDY_TWO_CPU_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error This reference is isolated CPU-only diagnostic code
#endif
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_model_reference.h"
#include "pioneer_q38_whole_original_bytes.h"
#include "../PioneerModelWeightCache.h"
#include <cstdio>
#include <vector>

static bool PinnedFirst(const Q38CpuTokenTrace& t){
 if(t.position || t.input!=248319 || t.hidden.size()!=65*5120 || t.logits.size()!=248320)return false;
 std::vector<uint32_t> words={0x31524651,1,t.position,t.input,t.selected,t.runnerUp,t.best,t.second,65,5120,248320,0,0,0,0,0};
 words.insert(words.end(),t.hidden.begin(),t.hidden.end());words.insert(words.end(),t.logits.begin(),t.logits.end());words.push_back(0x454e4451);
 return PioneerModelWeightCache::Hex(PioneerModelWeightCache::Hash(words.data(),words.size()*sizeof(uint32_t)))==
  "9323e2451c41e241c9a0391d3905890b662fd73df8542103da229f9b910d6937";
}
int main(int argc,char** argv){
 static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"pinned reference byte order");
 if(argc!=2 || argv[1][0]!='/')return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("QGWD_GREEDY_TWO_CPU_REFERENCE original weights only; no device or Work");
 WholeModelOriginalBytes source;Q38CpuModelReference model;
 if(!source.Open(argv[1]) || !source.Ready() || !model.Bind(source))return 1;
 unsigned input=248319;
 for(unsigned pos=0;pos<2;pos++){
  Q38CpuTokenTrace trace;
  if(!model.Step(input,trace) || trace.position!=pos || trace.input!=input
   || (!pos && (!PinnedFirst(trace) || trace.selected!=70253)) || !source.Ready())return 1;
  printf("QGWD_GREEDY_TWO_CPU_RESULT position=%u input=%u selected=%u; CPU_ONLY\n",pos,input,trace.selected);
  input=trace.selected;
 }
 if(!source.Ready() || !model.Ready())return 1;
 puts("PASS_QGWD_GREEDY_TWO_CPU_REFERENCE; NOT_CARD_OR_SPEED_OR_GENERAL_QUALITY");
 return fflush(stdout)?1:0;
}
