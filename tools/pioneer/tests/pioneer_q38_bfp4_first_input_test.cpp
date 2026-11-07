/* SPDX-License-Identifier: MIT */
// CPU-only synthetic ownership/refusal tests for the isolated observer.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
#endif
#define main PioneerCpuReferenceExistingMain
#include "pioneer_q38_cpu_model_reference_test.cpp"
#undef main
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
int main(){
 {ReferenceRowsProbe source;Q38CpuModelReference observer;assert(observer.Bind(source));
  std::vector<uint32_t> input={123},final={456};assert(!observer.ObserveFirstFfnInput(17,input,final));
  assert(input==std::vector<uint32_t>{123} && final==std::vector<uint32_t>{456} && !observer.Ready() && !source.reads);}
 {ReferenceRowsProbe source;Q38CpuModelReference observer;assert(observer.Bind(source));
  std::vector<uint32_t> output={123};assert(!observer.ObserveFirstFfnInput(248319,output,output));
  assert(output==std::vector<uint32_t>{123} && !observer.Ready() && !source.reads);}
 {ReferenceRowsProbe source;Q38CpuModelReference observer;assert(observer.Bind(source));
  std::vector<uint32_t> input,final;assert(observer.ObserveFirstFfnInput(248319,input,final));
  assert(input.size()==5120 && final.size()==5120 && !observer.Ready());
  for(auto word:input)assert(!(word&0x7fffffff));for(auto word:final)assert(!(word&0x7fffffff));
  Q38CpuTokenTrace trace;assert(!observer.Step(248319,trace) && !observer.ObserveFirstFfnInput(248319,input,final));}
 puts("PASS first-FFN-input CPU-only observer: synthetic zero, exact prompt gate, alias/refusal, one-shot isolation; NO MODEL QUALITY OR DEVICE CLAIM");
}
