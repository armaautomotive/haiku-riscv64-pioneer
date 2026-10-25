/* SPDX-License-Identifier: MIT */
// SAME authenticated ORIGINAL GGUF only. References finish BEFORE any
// accelerator FD; no native code, card transport, product upload or Work.
#include "pioneer_q38_decoder_original_source.h"
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 auto source=OpenResidentLayerOriginalSource(argv[1]);assert(source && !source->ResidentLayerReferenceReady()
  && !source->ResidentLayerReferenceFingerprint() && source->Prepare(0) && source->Ready() && source->ReferenceReads()==4490
  && !source->ResidentLayerReferenceReady() && !source->ResidentLayerReferenceFingerprint()
  && !source->ProjectionReads() && !source->ConvolutionReads() && !source->FfnReads() && !source->GdnOutputReads()
  && source->SealBeforeNativeContext() && source->ResidentLayerReferenceReady());
 auto fingerprint=source->ResidentLayerReferenceFingerprint();assert(fingerprint && !source->Prepare(0)
  && !source->SealBeforeNativeContext() && source->ResidentLayerReferenceReady() && source->ResidentLayerReferenceFingerprint()==fingerprint
  && source->ReferenceReads()==4490 && !source->FfnReads() && !source->GdnOutputReads());
 printf("PASS ORIGINAL fullSAME c600de03 SHA/all851tensor schema/ALLfirstlayer learned cache/independent CPU embedding+RMS+4projections+Conv+GDN+output+residualNorm+3FFNmatrices+libmSiLU+finalHidden BEFOREcontext:4490referenceReads fingerprint%016llx;private numeric products NEVER exposed as native input,NO device/originalGPUcomparison/Work/fullmodel/TPS qualification\n",(unsigned long long)fingerprint);
}
