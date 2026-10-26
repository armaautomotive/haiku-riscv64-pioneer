/* SPDX-License-Identifier: MIT */
// SAME authenticated ORIGINAL GGUF only. References finish BEFORE any
// accelerator FD; no native code, card transport, product upload or Work.
#include "pioneer_q38_decoder_original_source.h"
#include "../PioneerQ38ResidentLayerComparison.h"
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 auto source=OpenResidentLayerOriginalSource(argv[1]);assert(source && !source->ResidentLayerReferenceReady()
  && !source->ResidentLayerReferenceFingerprint() && !source->ResidentLayerInstructionReferenceFingerprint()
  && source->Prepare(0) && source->Ready() && source->ReferenceReads()==4810
  && !source->ResidentLayerReferenceReady() && !source->ResidentLayerReferenceFingerprint()
  && !source->ProjectionReads() && !source->ConvolutionReads() && !source->FfnReads() && !source->GdnOutputReads()
  && source->SealBeforeNativeContext() && source->ResidentLayerReferenceReady());
 auto fingerprint=source->ResidentLayerReferenceFingerprint();assert(fingerprint && !source->Prepare(0)
  && !source->SealBeforeNativeContext() && source->ResidentLayerReferenceReady() && source->ResidentLayerReferenceFingerprint()==fingerprint
  && source->ReferenceReads()==4810 && !source->FfnReads() && !source->GdnOutputReads());
 auto instructionFingerprint=source->ResidentLayerInstructionReferenceFingerprint();assert(instructionFingerprint && instructionFingerprint!=fingerprint);
 std::vector<uint32_t> observed(PioneerQ38ResidentLayerComparison::Words);PioneerQ38ResidentLayerComparison comparison;
 assert(!source->CompareResidentLayer(nullptr,unsigned(observed.size()),&comparison)
  && !source->CompareResidentLayer(observed.data(),1629120,&comparison)
  && !source->CompareResidentLayer(observed.data(),unsigned(observed.size()),nullptr)
  && !source->CompareResidentLayer(observed.data(),unsigned(observed.size()),reinterpret_cast<PioneerQ38ResidentLayerComparison*>(observed.data()))
  && source->CompareResidentLayer(observed.data(),unsigned(observed.size()),&comparison) && !comparison.passed
  && comparison.checked==observed.size() && source->ReferenceReads()==4810 && source->ResidentLayerReferenceFingerprint()==fingerprint);
 assert(!source->CompareResidentLayerInstruction(nullptr,unsigned(observed.size()),&comparison)
  && !source->CompareResidentLayerInstruction(observed.data(),1629120,&comparison)
  && !source->CompareResidentLayerInstruction(observed.data(),unsigned(observed.size()),nullptr)
  && !source->CompareResidentLayerInstruction(observed.data(),unsigned(observed.size()),reinterpret_cast<PioneerQ38ResidentLayerComparison*>(observed.data()))
  && source->CompareResidentLayerInstruction(observed.data(),unsigned(observed.size()),&comparison) && !comparison.passed
  && source->ResidentLayerInstructionReferenceFingerprint()==instructionFingerprint && source->ReferenceReads()==4810);
 uint64_t hash=0xdeadbeef;assert(!source->ResidentLayerInstructionStageFingerprint(5,hash) && hash==0xdeadbeef);
 for(unsigned i=0;i<5;i++)assert(source->ResidentLayerInstructionStageFingerprint(i,hash) && hash);
 printf("PASS ORIGINAL fullSAME c600de03 SHA/all851tensor schema/ALLfirstlayer learned cache/independent CPU embedding+RMS+4projections+Conv+GDN+output+residualNorm+3FFNmatrices+libmSiLU+finalHidden BEFOREcontext:4810referenceReads idealFingerprint%016llx instructionFingerprint%016llx;TWOdistinct private numeric references NEVER exposed as native input,NO device/originalGPUcomparison/Work/fullmodel/TPS qualification\n",(unsigned long long)fingerprint,(unsigned long long)instructionFingerprint);
}
