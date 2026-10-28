/* SPDX-License-Identifier: MIT */
// Coordinator ONLY. Full-file auth, raw bytes and independent CPU products
// live in a separate unflagged source TU; no expected vector is exported.
#include "pioneer_q38_recurrent_prefix_original_native.h"
#include "../PioneerQ38RecurrentPrefixOriginalSource.h"
#include <array>
#include <cassert>
#include <cstdio>
struct PrefixComparisonSink:PrefixOriginalStageSink {
 PioneerQ38RecurrentPrefixOriginalSource& source;unsigned published=0,exact=0,finals=0;
 explicit PrefixComparisonSink(PioneerQ38RecurrentPrefixOriginalSource& s):source(s){}
 bool Publish(unsigned layer,PioneerRecurrentOriginalStage stage,const uint32_t* values,unsigned words)override{
  if(layer>=3 || layer*PioneerRecurrentStageCount+unsigned(stage)!=published || !source.Ready())return false;
  PioneerRecurrentOriginalComparison report;
  if(!source.CompareStage(layer,stage,values,words,&report))return false;
  published++;exact+=report.passed;if(stage==PioneerRecurrentFinal)finals+=report.passed;
  printf("ORIGINAL_PREFIX_NATIVE_STAGE layer%u/stage%u/checked%u/differences%u/nonfinite%u/maxAbsolute%.12g/relativeL2%.12g/exactPassed%u;AFTER complete native3RUN+STOP+120holds+restore+fresh END\n",
   layer,unsigned(stage),report.checked,report.differences,report.nonfinite,report.maxAbsolute,report.relativeL2,report.passed);
  return source.Ready(); // no execution/cleanup/producer authority flows back
 }
};
int main(int argc,char** argv){
 assert(argc==6);setvbuf(stdout,nullptr,_IOLBF,0);
 puts("START ORIGINAL full c600 SHA/cache+independent chained CPU reference BEFORE native contexts;NO expected products cross native/operand TUs");
 auto source=OpenPioneerQ38RecurrentPrefixOriginalSource(argv[1]);
 assert(source && source->Prepare() && source->SealBeforeNativeContext() && source->Ready()
  && source->Spans()==169 && source->Bytes()==842821248 && source->ReferenceReads()==11557);
 printf("READY ORIGINAL native prefix/rawbytes%llu/pages%u/spans%u/private referenceReads%llu;SAME authenticated open file\n",
  (unsigned long long)source->Bytes(),source->Pages(),source->Spans(),(unsigned long long)source->ReferenceReads());
 const char* images[]={argv[2],argv[3],argv[4],argv[5]};
 std::array<uint32_t,5120> actual;actual.fill(0xdeadbeef);PrefixComparisonSink sink(*source);
 assert(ExecutePrefixOriginalNativeScenario(nullptr,*source,actual.data(),unsigned(actual.size()),&sink)!=0
  && ExecutePrefixOriginalNativeScenario(images,*source,actual.data(),5119,&sink)!=0
  && ExecutePrefixOriginalNativeScenario(images,*source,nullptr,unsigned(actual.size()),&sink)!=0);
 for(auto value:actual)assert(value==0xdeadbeef);assert(!sink.published);
 if(ExecutePrefixOriginalNativeScenario(images,*source,actual.data(),unsigned(actual.size()),&sink)!=0)return 1;
 assert(source->Ready() && source->ReferenceReads()==11557 && sink.published==3*PioneerRecurrentStageCount);
 PioneerRecurrentOriginalComparison report;assert(source->CompareStage(2,PioneerRecurrentFinal,actual.data(),unsigned(actual.size()),&report));
 printf("ORIGINAL_PREFIX_NATIVE_FINAL checked%u/differences%u/nonfinite%u/exactPassed%u/all3finalsExact%u/stagesExact%u/54;actual genuine native QPE1+672/336init+3layers+realSTOP+120holds+restore+fresh END,raw OS/registers SIMULATED,NOT physical readback,full64/token/Work or TPS\n",
  report.checked,report.differences,report.nonfinite,report.passed,sink.finals,sink.exact);
 return report.passed && sink.finals==3?0:1; // strict existing final bound; never fit to device output
}
