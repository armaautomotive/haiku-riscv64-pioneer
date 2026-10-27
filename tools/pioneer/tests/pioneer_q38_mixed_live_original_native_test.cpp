/* SPDX-License-Identifier: MIT */
// CPU coordinator ONLY. The original reference remains in its separate TU.
#include "pioneer_q38_mixed_live_original_native.h"
#include "pioneer_q38_mixed_live_original_bytes.h"
#include <array>
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==8);setvbuf(stdout,nullptr,_IOLBF,0);
 puts("START ORIGINAL GGUF authentication/reference+raw-byte staging BEFORE native test context;NO device/Work entry");
 auto source=OpenPioneerQ38MixedLiveOriginalSource(argv[1]);assert(source);
 assert(source->Prepare() && source->SealBeforeNativeContext());MixedLiveOriginalBytes bytes(*source);
 assert(bytes.Prepare() && bytes.Ready());
 printf("READY original cached bytes%llu/pages%u/referenceReads%llu/private fingerprint%016llx;NO expected products exposed to native TU\n",
  (unsigned long long)bytes.CachedBytes(),bytes.PageReads(),(unsigned long long)source->ReferenceReads(),(unsigned long long)source->ReferenceFingerprint());
 const char* images[]={argv[2],argv[3],argv[4],argv[5],argv[6],argv[7]};
 std::array<uint32_t,5120> actual;actual.fill(0xdeadbeef);
 assert(ExecuteMixedOriginalNativeScenario(nullptr,bytes,actual.data(),unsigned(actual.size()))!=0
  && ExecuteMixedOriginalNativeScenario(images,bytes,actual.data(),5119)!=0
  && ExecuteMixedOriginalNativeScenario(images,bytes,nullptr,unsigned(actual.size()))!=0);
 for(auto word:actual)assert(word==0xdeadbeef);
 if(ExecuteMixedOriginalNativeScenario(images,bytes,actual.data(),unsigned(actual.size()))!=0)return 1;
 assert(bytes.Ready());PioneerQ38MixedLiveReferenceComparison report;
 assert(source->CompareFinal(actual.data(),unsigned(actual.size()),&report));
 printf("ORIGINAL_NATIVE_SCENARIO_COMPARISON checked%u/differences%u/nonfinite%u/maxAbsolute%.12g/relativeL2%.12g/exactPassed%u;AFTER genuine capture/restore/fresh END,NOT physical hardware/full64/model TPS\n",
  report.checked,report.differences,report.nonfinite,report.maxAbsolute,report.relativeL2,report.passed);
 return report.passed?0:1; // Numerical mismatch is a real failure,not a fitted tolerance.
}
