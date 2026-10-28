/* SPDX-License-Identifier: MIT */
// Unflagged coordinator. CPU oracle is in a separate compiled source TU.
#include "../PioneerQ38RecurrentPrefixOriginalSource.h"
#include "pioneer_q38_recurrent_prefix_original_instructions.h"
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);
 puts("START ORIGINAL prefix full c600 authentication/RAW3layer cache/independent chained CPU reference BEFORE instruction contexts;NO expected products enter instruction TUs");
 auto source=OpenPioneerQ38RecurrentPrefixOriginalSource(argv[1]);
 assert(source && source->Prepare() && source->SealBeforeNativeContext() && source->Ready() && source->Spans()==169 && source->ReferenceReads()==11557);
 std::array<uint32_t,5120> embedding;ExecutePrefixOriginalEmbedding(argv[2],*source,embedding.data(),unsigned(embedding.size()));
 PrefixOriginalInstructionTrace trace;ExecutePrefixOriginalInstructions(argv[3],argv[4],*source,embedding.data(),unsigned(embedding.size()),trace);
 unsigned exact=0;bool final=true;
 for(unsigned layer=0;layer<3;layer++)for(unsigned which=0;which<PioneerRecurrentStageCount;which++){
  const auto& actual=trace.actual[layer][which];PioneerRecurrentOriginalComparison report;
  assert(source->CompareStage(layer,PioneerRecurrentOriginalStage(which),actual.data(),unsigned(actual.size()),&report));
  exact+=report.passed;if(which==PioneerRecurrentFinal)final&=report.passed;
  printf("ORIGINAL_PREFIX_STAGE layer%u/stage%u/checked%u/differences%u/nonfinite%u/maxAbsolute%.12g/relativeL2%.12g/exactPassed%u;AFTER actual3layers+realSTOP\n",
   layer,which,report.checked,report.differences,report.nonfinite,report.maxAbsolute,report.relativeL2,report.passed);
 }
 printf("ORIGINAL_PREFIX_FINAL all3finalExact%u/stagesExact%u/54/no reference products fed into instructions;NOT native,physical,full64,persistent state,Work or TPS\n",unsigned(final),exact);
 return final?0:1; // never fit an error bound to observed device output
}
