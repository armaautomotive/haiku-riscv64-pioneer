/* SPDX-License-Identifier: MIT */
// Genuine NEW native lifecycle+emitted full-layer programs,ALL ORIGINAL
// learned BYTES. Independent CPU reference is a separately compiled TU,
// prepared BEFORE the native context. Simulated OS/PCI endpoints only.
#define PIONEER_Q38_LAYER_READBACK_FIXTURE_ONLY
#include "pioneer_q38_resident_layer_native_readback_test.cpp"
#include "pioneer_q38_decoder_original_source.h"
#include "../PioneerQ38ResidentLayerComparison.h"
int main(int argc,char** argv){
 assert(argc==9);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 (void)&ForkActualNativeLayerTerminal;(void)&FinishNativeOutput;
 puts("ORIGINAL_COMPLETE_LAYER_FULL_SHA_AND_4810_IDEAL_AND_INSTRUCTION_REFERENCE_READS_BEFORE_ANY_NATIVE_CONTEXT");
 auto original=OpenResidentLayerOriginalSource(argv[8]);assert(original && original->Prepare(0) && original->SealBeforeNativeContext()
  && original->ResidentLayerReferenceReady() && original->ReferenceReads()==4810);
 auto fingerprint=original->ResidentLayerReferenceFingerprint();assert(fingerprint);
 auto instructionFingerprint=original->ResidentLayerInstructionReferenceFingerprint();assert(instructionFingerprint && instructionFingerprint!=fingerprint);
 uint64_t referenceStages[5],actualStages[5];for(unsigned i=0;i<5;i++)assert(original->ResidentLayerStageFingerprint(i,referenceStages[i]));
 uint64_t instructionStages[5];for(unsigned i=0;i<5;i++)assert(original->ResidentLayerInstructionStageFingerprint(i,instructionStages[i]));
 NativeBindingFixture f(argv[3],argv[4]);f.io.rmsImage=argv[5];auto& c=*f.c;auto& m=*c.q38EmbeddingModel;c.q38Clock.value.busy_mhz=1400;
 f.io.originalEmbeddingWeights=original->EmbeddingTile();f.io.originalRmsWeights=original->LearnedNorm();
 assert(BindQ38ResidentLayerNativeState(c));PioneerQ38StateZeroResult gdn;PioneerQ38ConvHistoryZeroResult history;PioneerQ38LearnedRmsResult rms;
 assert(InitializeQ38ResidentLayerRecurrent(c,gdn) && InitializeQ38ResidentLayerHistory(c,history)
  && f.Initialize() && f.Run() && RunQ38LearnedRmsModel(c,rms) && original->CheckEmbeddingRms(f.io.wholeWorkspace));
 std::vector<uint32> addresses(73737),values(73737);uint32 sources[16],commands[10],counts[8];
 for(unsigned i=0;i<73737;i++){addresses[i]=PioneerQ38EmbeddingSnapshotAddress(i);values[i]=f.io.RawSlot(addresses[i]);}
 for(unsigned i=0;i<16;i++)sources[i]=i?f.io.source[i]:f.io.config;
 for(unsigned i=0;i<10;i++)commands[i]=f.io.command[i];for(unsigned i=0;i<8;i++)counts[i]=f.io.PanelIO::ResidentNocRead(PioneerQ38SpanCounter(i));
 ProjectionNativeTransport transport(argv[1],argv[2],f.io.wholeWorkspace);transport.SeedRoot(addresses.data(),values.data(),unsigned(values.size()),sources,commands,counts);
 transport.LearnedSource(*original);f.io.projectionTransport=&transport;PioneerQ38ProjectionGraphResult projection;transport.Observe(projection);
 assert(PrepareQ38ProjectionExecution(c,projection));
 for(unsigned role:{10U,11U,5U,6U}){assert(RunQ38ProjectionExecution(c,role,projection));transport.VerifyOutput();
  assert(original->CheckProjection(role,f.io.wholeWorkspace.data()+PioneerQ38ProjectionResultOffset(role)/4));
 }
 assert(CloseQ38ProjectionExecution(c,projection) && m.Closed() && m.projection.ResidentInput());
 transport.ResidentLayerSources(argv[6],argv[7],f.io);PioneerQ38ResidentLayerResult output;
 assert(PrepareQ38ResidentLayerNative(c,output));transport.ResidentLayerFault(0,output);
 bool ran=RunQ38ResidentLayerNative(c,output);
 if(!ran){fprintf(stderr,"ORIGINAL_LAYER_RUN_FAILED phase%u worker%u word%u unsafe%u rootStarts%u rootHolds%u\n",
  output.phase,output.worker,output.word,output.unsafe,output.rootStarts,output.rootHolds);transport.ResidentLayerDiagnostic();}
 assert(ran);transport.VerifyResidentLayer();
 std::vector<uint32_t> observed;FinishLayerReadback(f,transport,output,0,0,0,&observed);
 assert(observed.size()==PioneerQ38ResidentLayerComparison::Words && original->ResidentLayerReferenceReady()
  && original->ReferenceReads()==4810 && original->ResidentLayerReferenceFingerprint()==fingerprint
  && original->ResidentLayerInstructionReferenceFingerprint()==instructionFingerprint
  && original->ConvolutionReads()==640 && original->GdnInputReads()==192 && original->GdnNormReads()==96
  && original->ResidualRmsReads()==2 && original->FfnReads()==65280 && original->GdnOutputReads()==61440);
 PioneerQ38ResidentLayerComparison comparison;assert(original->CompareResidentLayer(observed.data(),unsigned(observed.size()),&comparison));
 transport.ResidentLayerStageFingerprints(actualStages);
 for(unsigned i=0;i<5;i++)printf("ORIGINAL_STAGE_FINGERPRINT stage%u reference%016llx actual%016llx equal%u;OBSERVED_STORES_ONLY_NOT_GPU_INPUT\n",
  i,(unsigned long long)referenceStages[i],(unsigned long long)actualStages[i],unsigned(referenceStages[i]==actualStages[i]));
 printf("ORIGINAL_QLV1_COMPARE checked%u nonfinite%u conv%u history%u gated%u projectionBits%u current%u next%u finalBits%u projectionMaxAbs%.12g finalMaxAbs%.12g finalRelativeL2%.12g passed%u;PRE_CONTEXT_REFERENCES_NEVER_SEEDED_FROM_GPU\n",
  comparison.checked,comparison.nonfinite,comparison.convErrors,comparison.historyErrors,comparison.gatedErrors,comparison.projectionDifferences,
  comparison.currentErrors,comparison.nextErrors,comparison.finalDifferences,comparison.projectionMaxAbsolute,comparison.finalMaxAbsolute,comparison.finalRelativeL2,comparison.passed);
 // Preserve ideal-libm STRICT report,including its failure. It is NOT
 // relabelled as a bit-equality pass or replaced with a fitted tolerance.
 // The separate instruction contract was predicted pre-context,already
 // bounded per-value against libm,and MUST match every stage fingerprint
 // and all final words bit-for-bit. Fingerprints are diagnostic DATA only.
 PioneerQ38ResidentLayerComparison instruction;
 assert(original->CompareResidentLayerInstruction(observed.data(),unsigned(observed.size()),&instruction));
 for(unsigned i=0;i<5;i++){
  printf("ORIGINAL_INSTRUCTION_STAGE stage%u reference%016llx actual%016llx equal%u;DISTINCT_FROM_IDEAL_LIBM\n",
   i,(unsigned long long)instructionStages[i],(unsigned long long)actualStages[i],unsigned(instructionStages[i]==actualStages[i]));
  assert(instructionStages[i]==actualStages[i]);
 }
 printf("ORIGINAL_INSTRUCTION_QLV1_COMPARE checked%u nonfinite%u conv%u history%u gated%u projectionBits%u current%u next%u finalBits%u finalMaxAbs%.12g passed%u;IDEAL_LIBM_STRICT_PASSED%u\n",
  instruction.checked,instruction.nonfinite,instruction.convErrors,instruction.historyErrors,instruction.gatedErrors,
  instruction.projectionDifferences,instruction.currentErrors,instruction.nextErrors,instruction.finalDifferences,instruction.finalMaxAbsolute,instruction.passed,comparison.passed);
 assert(instruction.passed);puts("PASS ORIGINAL authenticated complete FIRST recurrent layer/fullNEWnativecontext+allprograms/QLV1/restore/END/independent precontext instruction-contract CPU reference;IDEAL_LIBM_REPORT_RETAINED_SEPARATELY;SIMULATED OS/byte endpoints,NOT physical/full64model/Work/inference-rate qualification");
}
