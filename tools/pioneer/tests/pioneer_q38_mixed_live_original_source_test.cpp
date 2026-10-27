/* SPDX-License-Identifier: MIT */
// Interface consumer: no CPU-reference/native/device/old DEBUG/Work flags.
#include "../PioneerQ38MixedLiveOriginalSource.h"
#include <array>
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);auto source=OpenPioneerQ38MixedLiveOriginalSource(argv[1]);
 std::array<uint32_t,5120> values={};PioneerQ38MixedLiveReferenceComparison report;std::array<uint8_t,4096> bytes;
 bytes.fill(0xa5);report.checked=0xdeadbeef;
 assert(source && !source->Ready() && !source->SealedModel() && !source->ReferenceReads() && !source->ReferenceFingerprint()
  && !source->ReadWeightPage(0,0,bytes.data(),4096) && !source->CompareFinal(values.data(),5120,&report) && !source->SealBeforeNativeContext());
 assert(source->Prepare() && source->ReferenceReads()==3717 && !source->Ready() && !source->SealedModel() && !source->ReferenceFingerprint()
  && !source->Prepare() && !source->ReadWeightPage(0,0,bytes.data(),4096) && !source->CompareFinal(values.data(),5120,&report));
 for(auto b:bytes)assert(b==0xa5);assert(report.checked==0xdeadbeef);
 assert(source->SealBeforeNativeContext() && source->Ready() && source->SealedModel() && source->ReferenceFingerprint()==UINT64_C(0xec4e50bb1a601765)
  && !source->Prepare() && !source->SealBeforeNativeContext() && source->ReferenceReads()==3717);
 assert(source->ReadWeightPage(0,0,bytes.data(),4096) && source->CompareFinal(values.data(),5120,&report) && !report.passed
  && report.checked==5120 && report.differences && source->Ready() && source->ReferenceReads()==3717);
 assert(!source->CompareFinal(nullptr,5120,&report) && !source->CompareFinal(values.data(),5119,&report)
  && !source->CompareFinal(values.data(),5120,nullptr)
  && !source->CompareFinal(values.data(),5120,reinterpret_cast<PioneerQ38MixedLiveReferenceComparison*>(values.data())));
 const unsigned stageWords[]={5120,5120,12288,1024,1024,6144,6144,1024,1024,6144,5120,5120,5120,17408,17408,17408,5120,5120};
 std::array<uint32_t,17408> zeros={};
 for(unsigned i=0;i<PioneerQ38MixedStageCount;i++){
  auto stage=PioneerQ38MixedLiveReferenceStage(i);unsigned words=stageWords[i];
  assert(source->CompareStage(stage,zeros.data(),words,&report) && report.checked==words && !report.passed && report.differences && !report.nonfinite);
  report.checked=0xdeadbeef;
  assert(!source->CompareStage(stage,zeros.data(),words-1,&report) && !source->CompareStage(stage,nullptr,words,&report)
   && !source->CompareStage(stage,zeros.data(),words,nullptr)
   && !source->CompareStage(stage,zeros.data(),words,reinterpret_cast<PioneerQ38MixedLiveReferenceComparison*>(zeros.data())) && report.checked==0xdeadbeef);
 }
 assert(!source->CompareStage(PioneerQ38MixedStageCount,zeros.data(),5120,&report) && report.checked==0xdeadbeef && source->ReferenceReads()==3717);
 printf("PASS separate ORIGINAL dense CPU source/FULL c600de03 SHA/3717referenceReads/private fingerprint%016llx/prepare->seal BEFOREcontext/ONLY original weight pages+postreturn comparison exposed/no numeric products or caller plan/NO accelerator,Work,fullmodel or TPS\n",(unsigned long long)source->ReferenceFingerprint());
}
