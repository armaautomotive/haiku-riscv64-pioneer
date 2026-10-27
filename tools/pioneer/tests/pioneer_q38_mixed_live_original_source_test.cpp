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
 printf("PASS separate ORIGINAL dense CPU source/FULL c600de03 SHA/3717referenceReads/private fingerprint%016llx/prepare->seal BEFOREcontext/ONLY original weight pages+postreturn comparison exposed/no numeric products or caller plan/NO accelerator,Work,fullmodel or TPS\n",(unsigned long long)source->ReferenceFingerprint());
}
