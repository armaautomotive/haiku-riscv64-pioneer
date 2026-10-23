/* SPDX-License-Identifier: MIT */
// Actual old native public control + original bytes + actual QGI1 emitted
// NoC/math. NOT a new native handoff/initialized-GDN owner or card launch.
#define PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CONTROL_FIXTURE_ONLY
#include "pioneer_q38_resident_decoder_validation_control_test.cpp"
#include "pioneer_q38_decoder_original_source.h"
#include "pioneer_q38_gdn_input_bridge.h"
int main(int argc,char** argv){
 assert(argc==9 || (argc==10 && !strcmp(argv[9],"--all-tokens")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned tokens=0;
 for(unsigned token:argc==10?std::vector<unsigned>{0,15,248319}:std::vector<unsigned>{0}){
  puts("ORIGINAL_GDN_INPUT_ALL_REFERENCES_BEFORE_NATIVE_CONTEXT;NO_DEVICE_ACCESS");
  auto source=OpenGdnInputOriginalSource(argv[7]);assert(source && source->Prepare(token) && source->ReferenceReads()==1672
   && !source->GdnInputReads() && source->SealBeforeNativeContext() && !source->SealBeforeNativeContext());
  DecoderControlFixture f(argv[1],argv[2],argv[3],argv[4],argv[5],argv[6],token);
  f.io.originalEmbeddingWeights=source->EmbeddingTile();f.io.originalRmsWeights=source->LearnedNorm();f.transport->LearnedSource(*source);
  auto reply=Request();reply->metadata.token=token;
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_DECODER_VALIDATE,reply.get(),sizeof(*reply))==B_OK);
  const auto& r=reply->metadata;auto* same=f.c->q38EmbeddingModel;
  assert(r.version==1 && r.token==token && !r.position && r.state==TT_RUNTIME_PASSED && r.closed && r.input_ready
   && !r.recovery_required && r.session && r.driver_generation==sReloadGeneration && r.source_pages==23235
   && r.saved_words==8281546 && r.restored_words==8281546 && reply->projections==4 && reply->peer_stopped==119 && reply->peer_held==119
   && reply->convolution_workers==120 && reply->convolution_jobs==640 && reply->convolution_channels==10240 && reply->convolution_held==120
   && reply->readback_pages==40 && reply->readback_words==40960 && reply->restored_words==8134072
   && same && same->Closed() && !f.c->Unsafe() && !locked && locks==releases && f.c->q38GdnCallThread==-1);
  assert(source->CheckEmbeddingRms(f.io.wholeWorkspace) && source->CheckConvolution(reply->values));
  for(unsigned role:{10U,11U,5U,6U})assert(source->CheckProjection(role,f.io.wholeWorkspace.data()+PioneerQ38ProjectionResultOffset(role)/4));
  printf("PASS ORIGINAL token%u actual native QPE1/QRN1/QPS1/QCB1 ALL120;NOW separate byte-only QGI1 endpoint simulation,NOT native ownership handoff\n",token);
  unsigned maxStack=0,maxSteps=0;
  for(unsigned half=0;half<96;half++){
   GdnInputEmulationResult result;
   assert(ExecuteGdnInputResidentBytes(argv[8],same->plan,half,same->bank,f.io.wholeWorkspace,*source,result)
    && result.packets==7 && source->CheckGdnInput(half,result.values.data()));
   maxStack=std::max(maxStack,result.stack);maxSteps=std::max(maxSteps,result.steps);
  }
  assert(source->ReferenceReads()==1672 && source->ProjectionReads()==164800 && source->ConvolutionReads()==640
   && source->GdnInputReads()==192 && !f.c->Unsafe() && !f.io.pciWrites && !f.io.HostReadMock::bmeWrites
   && EndFactory(*f.c) && f.c->q38Dispatch->closed && !sColdRecoveryRequired);
  printf("PASS ORIGINAL token%u QGI1 all96halves/672residentREADS/192originalA-dt64Breads/30912outputs/strictRELATIVEgates/maxStack%u/maxSteps%u;NO host numeric input/nativehandoff/stateinit/card/speedclaim\n",token,maxStack,maxSteps);tokens++;
 }
 printf("PASS ORIGINAL QGI1 integration tokens%u;raw source mapping and arithmetic ONLY,not native ownership/GDN/current-state/whole-model inference\n",tokens);
}
