/* SPDX-License-Identifier: MIT */
// ACTUAL public control/cookie/global scope/native ownership/instructions.
// ORIGINAL bytes and independent BEFORE-context references are in a separate
// compilation unit. Only physical/OS endpoints simulated,NOT card speed.
#define PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CONTROL_FIXTURE_ONLY
#include "pioneer_q38_resident_decoder_validation_control_test.cpp"
#include "pioneer_q38_decoder_original_source.h"
#if defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT)
#error CPU observers MUST NOT enter the actual native controller compilation unit
#endif
int main(int argc,char** argv){
 assert(argc==8 || (argc==9 && (!strcmp(argv[8],"--all-tokens") || !strcmp(argv[8],"--frontend-pc"))));setvbuf(stdout,nullptr,_IOLBF,0);
 const bool frontendPc=argc==9 && !strcmp(argv[8],"--frontend-pc");
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned tests=0;
 for(unsigned token:argc==9?std::vector<unsigned>{0,15,248319}:std::vector<unsigned>{0}){
  puts("DEBUG_ORIGINAL_FULL_SHA_AND_ALL_CPU_REFERENCES_BEFORE_NATIVE_CONTEXT;NO_DEVICE_ACCESS");
  auto source=OpenDecoderOriginalSource(argv[7]);assert(source && source->Prepare(token) && source->ReferenceReads()==1672);
  assert(source->SealBeforeNativeContext() && !source->SealBeforeNativeContext() && !source->Prepare(token));
  printf("PASS ORIGINAL token%u SAME_FILE_SHA_c600de03_ALL851TENSORS/all1672independent_reference_reads BEFORE native context\n",token);
  DecoderControlFixture f(argv[1],argv[2],argv[3],argv[4],argv[5],argv[6],token);
  f.io.originalEmbeddingWeights=source->EmbeddingTile();f.io.originalRmsWeights=source->LearnedNorm();f.transport->LearnedSource(*source);
  if(frontendPc)f.transport->DecoderFault(15); // TEST endpoint snapshots ONLY,not numeric/authority inputs
  auto reply=Request();reply->metadata.token=token;
  puts("ORIGINAL_ACTUAL_SINGLE_PUBLIC_CALL_HISTORY336X5_ZERO512_QPE1_QRN1_QPS1_QCB1_READBACK_RESTORE");
  int status=ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_DECODER_VALIDATE,reply.get(),sizeof(*reply));
  const auto& r=reply->metadata;auto* same=f.c->q38EmbeddingModel;
  assert(status==B_OK && r.version==1 && r.token==token && !r.position && r.state==TT_RUNTIME_PASSED && r.closed && r.input_ready
   && !r.recovery_required && r.session && r.driver_generation==sReloadGeneration && r.source_pages==23235
   && r.saved_words==8281546 && r.restored_words==8281546 && reply->projections==4 && reply->peer_stopped==119 && reply->peer_held==119
   && reply->convolution_workers==120 && reply->convolution_jobs==640 && reply->convolution_channels==10240 && reply->convolution_held==120
   && reply->readback_pages==40 && reply->readback_words==40960 && reply->restored_words==8134072
   && same && same->Closed() && !f.c->Unsafe() && !locked && locks==releases && f.c->q38GdnCallThread==-1);
  // No device I/O here. Compare only captured bytes to CPU references
  // computed BEFORE any native context,never to products supplied to kernels.
  bool matches=source->Ready() && source->CheckEmbeddingRms(f.io.wholeWorkspace);
  for(unsigned role:{10U,11U,5U,6U})matches=matches && source->CheckProjection(role,f.io.wholeWorkspace.data()+PioneerQ38ProjectionResultOffset(role)/4);
  matches=matches && source->CheckConvolution(reply->values);
  if(!matches)fprintf(stderr,"STOP ORIGINAL decoder numeric mismatch token%u;NO END/restore/device retry\n",token);
  assert(matches && source->ReferenceReads()==1672 && source->ProjectionReads()==164800 && source->ConvolutionReads()==640);
  for(unsigned i=0;i<40960;i++)assert(reply->values[i]==f.io.wholeWorkspace[(i<10240?TT_Q38_CONV_OUTPUT:TT_Q38_CONV_NEXT)/4+(i<10240?i:i-10240)]);
  assert(f.transport->DecoderStarts()==120 && f.transport->ValidationReads()==40 && !f.io.pciWrites && !f.io.HostReadMock::bmeWrites
   && EndFactory(*f.c) && f.c->q38Dispatch->closed && same->Closed() && !f.c->Unsafe() && !sColdRecoveryRequired && source->Ready());
  assert(f.transport->DecoderFrontendSnapshots()==(frontendPc?360U:0U));
  if(frontendPc)puts("PASS ORIGINAL all120 actual QCB1 workers with360speculative6403c snapshots;allindependent numerics/receipts/holds/readback/restore/END unchanged");
  printf("PASS ORIGINAL token%u actual native SAME context/control/leases/history/QPE1/QRN1/QPS1all4/QCB1all120/640jobs/40poisoned_pages/10240SiLU+30720exact_next/full8134072word_restore/actual_END;1672BEFORE-context_reference_reads/164800ORIGINAL_Q8_tiles/640ORIGINAL_F32_tiles;SIMULATED_NOT_CARD_OR_MODEL_SPEED\n",token);tests++;
 }
 printf("PASS ORIGINAL native decoder integration tokens%u;NO synthetic learned bytes/numeric device inputs/reference compilation in native controller;NO_INSTALL_PHYSICAL_CARD_OR_SPEED_CLAIM\n",tests);
}
