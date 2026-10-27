/* SPDX-License-Identifier: MIT */
// Coordinator only:private CPU products stay in a separately compiled TU.
#include "pioneer_q38_recurrent_block_original_native.h"
#include "../PioneerQ38RecurrentBlockOriginalSource.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cmath>
struct RecurrentComparisonSink:RecurrentOriginalStageSink {
 PioneerQ38RecurrentBlockOriginalSource& source;unsigned published=0,exact=0;bool finalPassed=false;
 explicit RecurrentComparisonSink(PioneerQ38RecurrentBlockOriginalSource& s):source(s){}
 bool Publish(PioneerRecurrentOriginalStage which,const uint32_t* values,unsigned words)override{
  if(unsigned(which)!=published || !source.Ready())return false;PioneerRecurrentOriginalComparison report;
  if(!source.CompareStage(which,values,words,&report))return false;published++;exact+=report.passed;
  if(which==PioneerRecurrentFinal)finalPassed=report.passed;
  printf("ORIGINAL_RECURRENT_STAGE stage%u/checked%u/differences%u/nonfinite%u/maxAbsolute%.12g/relativeL2%.12g/exactPassed%u;AFTER complete native END\n",
   unsigned(which),report.checked,report.differences,report.nonfinite,report.maxAbsolute,report.relativeL2,report.passed);
  return source.Ready(); // diagnostic comparison NEVER a pre-END ownership or execution grant
 }
};
int main(int argc,char** argv){
 assert(argc==6);setvbuf(stdout,nullptr,_IOLBF,0);
 puts("START full c600 ORIGINAL SHA/cache+INDEPENDENT row-major/libm recurrent CPU reference BEFORE native test context;NO expected products cross native TU");
 auto source=OpenPioneerQ38RecurrentBlockOriginalSource(argv[1]);
 assert(source && source->Prepare() && source->SealBeforeNativeContext() && source->Ready()
  && source->Spans()==57 && source->Bytes()==280971136 && source->ReferenceReads()==3853);
 printf("READY ORIGINAL raw recurrent+embedding bytes%llu/pages%u/spans%u/private referenceReads%llu/fingerprint%016llx;SAME authenticated open file\n",
  (unsigned long long)source->Bytes(),source->Pages(),source->Spans(),(unsigned long long)source->ReferenceReads(),(unsigned long long)source->ReferenceFingerprint());
 const char* images[]={argv[2],argv[3],argv[4],argv[5]};
 std::array<uint32_t,5120> actual;actual.fill(0xdeadbeef);RecurrentComparisonSink sink(*source);
 assert(ExecuteRecurrentOriginalNativeScenario(nullptr,*source,actual.data(),unsigned(actual.size()),&sink)!=0
  && ExecuteRecurrentOriginalNativeScenario(images,*source,actual.data(),5119,&sink)!=0
  && ExecuteRecurrentOriginalNativeScenario(images,*source,nullptr,unsigned(actual.size()),&sink)!=0);
 for(auto v:actual)assert(v==0xdeadbeef);assert(!sink.published);
 if(ExecuteRecurrentOriginalNativeScenario(images,*source,actual.data(),unsigned(actual.size()),&sink)!=0)return 1;
 assert(source->Ready() && source->ReferenceReads()==3853 && sink.published==PioneerRecurrentStageCount);PioneerRecurrentOriginalComparison report;
 assert(source->CompareStage(PioneerRecurrentFinal,actual.data(),unsigned(actual.size()),&report));
 printf("ORIGINAL_RECURRENT_NATIVE_FINAL checked%u/differences%u/nonfinite%u/maxAbsolute%.12g/relativeL2%.12g/exactPassed%u/stagesExact%u/%u;actual QPE1+672/336init+whole layer0/realSTOP+120holds+restore+fresh END,raw OS/registers SIMULATED,NOT physical readback,full64/token/Work or TPS\n",
  report.checked,report.differences,report.nonfinite,report.maxAbsolute,report.relativeL2,report.passed,sink.exact,sink.published);
 return report.passed && sink.finalPassed?0:1; // strict final failure is NEVER fitted to observed accelerator output
}
