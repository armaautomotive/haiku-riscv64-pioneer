/* SPDX-License-Identifier: MIT */
// ACTUAL new public root/team/copy/global scope/native factories/END.
// Only OS/physical/learned byte endpoints simulated;NOT physical card speed.
#define PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CONTROL_FIXTURE_ONLY
#include "pioneer_q38_resident_decoder_validation_control_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnInputValidationProtocol.h"
#include "pioneer_q38_decoder_original_source.h"
static std::unique_ptr<tt_q38_gdn_input_validation> GdnRequest(unsigned token=0){
 auto r=std::make_unique<tt_q38_gdn_input_validation>();r->metadata.version=1;r->metadata.token=token;return r;
}
int main(int argc,char** argv){
 assert(argc==9 || (argc==10 && !strcmp(argv[9],"--faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;(void)&Request;
 puts("GDN_DEBUG_ALL_ORIGINAL_REFERENCES_BEFORE_NATIVE_CONTEXT;NO_DEVICE_ACCESS");
 auto source=OpenGdnInputOriginalSource(argv[7]);assert(source && source->Prepare(0) && source->SealBeforeNativeContext());
 DecoderControlFixture f(argv[1],argv[2],argv[3],argv[4],argv[5],argv[6]);
 f.io.originalEmbeddingWeights=source->EmbeddingTile();f.io.originalRmsWeights=source->LearnedNorm();
 f.transport->GdnInputSource(argv[8]);f.transport->LearnedSource(*source);
 unsigned touched=f.io.touches,device=f.transport->Touches(),refused=0;tt_q38_embedding_support support={};
 assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATION_SUPPORT,&support,sizeof(support))==B_OK
  && support.version==1 && support.driver==55 && support.model_namespace==15 && support.inputs==30912 && support.vocabulary==248320
  && support.runtime_pages==512 && !support.no_cpu_reference && !support.model_inference);
 for(unsigned field=0;field<18;field++){
  if(field==1)continue;auto r=GdnRequest();uint32_t value=field?1:2;memcpy(reinterpret_cast<uint8_t*>(&r->metadata)+field*4,&value,4);
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
 }
 for(unsigned field=0;field<24;field++){
  auto r=GdnRequest();reinterpret_cast<uint8_t*>(r.get())[sizeof(r->metadata)+field*4]=1;
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
 }
 for(unsigned half=0;half<96;half++)for(unsigned at:{0U,321U}){
  auto r=GdnRequest();r->values[half*322+at]=1;
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
 }
 for(unsigned op:{TT_PCI_PROBE_Q38_DECODER_VALIDATION_SUPPORT,TT_PCI_PROBE_Q38_DECODER_VALIDATE,
  TT_PCI_PROBE_Q38_PROJECTION_VALIDATION_SUPPORT,TT_PCI_PROBE_Q38_PROJECTION_VALIDATE,
  TT_PCI_PROBE_Q38_EMBEDDING_SUPPORT,TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE,TT_PCI_PROBE_Q38_EMBEDDING_VALIDATION_SUPPORT,
  TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE,TT_PCI_PROBE_Q38_RMS_VALIDATION_SUPPORT,TT_PCI_PROBE_Q38_RMS_VALIDATE,
  TT_PCI_PROBE_Q38_FFN_MODEL_SUPPORT,TT_PCI_PROBE_Q38_FFN_MODEL_INITIALIZE,TT_PCI_PROBE_Q38_FFN_GRAPH_SUPPORT,TT_PCI_PROBE_Q38_FFN_GRAPH_RUN,
  TT_PCI_PROBE_Q38_WORK_SUPPORT,TT_PCI_PROBE_Q38_WORK_RUN,TT_PCI_PROBE_Q38_LEGACY_WORK_SUPPORT,TT_PCI_PROBE_Q38_LEGACY_WORK_RUN,
  TT_PCI_PROBE_Q38_DISPATCH_SUPPORT,TT_PCI_PROBE_Q38_DISPATCH_RUN}){
  auto r=GdnRequest();assert(ProbeControl(&f.cookie,op,r.get(),sizeof(*r))==B_NOT_ALLOWED);refused++;
 }
 for(unsigned kind=0;kind<11;kind++){
  auto r=GdnRequest();auto* cookie=&f.cookie;size_t bytes=sizeof(*r);
  if(kind==0)r->metadata.token=248320;if(kind==1)r->metadata.position=1;if(kind==2)bytes--;if(kind==3)bytes++;
  if(kind==4)cookie=nullptr;if(kind==5)currentTeam=8;if(kind==6)lockStatus=B_NOT_ALLOWED;if(kind==7)callerUid=1000;
  if(kind==8)copyFailure=copies+1;if(kind==9)failThreadInfo=true;if(kind==10)cookie->magic=0;
  assert(ProbeControl(cookie,TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATE,r.get(),bytes)!=B_OK);
  f.cookie.magic=0x50313030;currentTeam=7;lockStatus=B_OK;callerUid=0;copyFailure=0;failThreadInfo=false;refused++;
 }
 assert(!f.c->q38EmbeddingModel && !locked && locks==releases && f.c->q38GdnCallThread==-1
  && f.io.touches==touched && f.transport->Touches()==device);
 printf("GDN_DEBUG_ACTUAL_PUBLIC_INERT_REFUSALS%u,NO endpoint IO\n",refused);
 if(argc==10)for(unsigned fault=1;fault<=9;fault++){
  pid_t child=fork();assert(child>=0);
  if(!child){
   if(fault<=4)f.transport->GdnInputFault(fault==1?1:fault==2?3:fault==3?13:14);
   if(fault==5)f.transport->GdnReadbackFault(0);if(fault==6)f.transport->GdnReadbackFault(30911);
   if(fault==7)copyFailure=copies+2;if(fault==8)f.io.zeroFailRun=337;if(fault==9)f.io.projectionFailMap=1;
   auto reply=GdnRequest();status_t status=ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATE,reply.get(),sizeof(*reply));
   assert(status==(fault==7?B_BAD_VALUE:B_OK) && sColdRecoveryRequired && f.c->dispatchOwnerRetained && f.io.retained
    && !locked && locks==releases && f.c->q38GdnCallThread==-1);
   if(fault!=7){
    assert(reply->metadata.recovery_required && !reply->metadata.closed);
    const auto* bytes=reinterpret_cast<const uint8_t*>(reply.get());for(unsigned i=sizeof(reply->metadata);i<sizeof(*reply);i++)assert(!bytes[i]);
   }
   if(fault<=4)assert(f.transport->GdnInputFaultInjected());
   unsigned n=f.io.touches,d=f.transport->Touches();auto again=GdnRequest();
   assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATE,again.get(),sizeof(*again))!=B_OK
    && !EndFactory(*f.c) && f.io.touches==n && f.transport->Touches()==d);
   printf("PASS actual GDN DEBUG public fault%u/ALL retained/zero later IO\n",fault);_exit(0);
  }
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 auto reply=GdnRequest();assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATE,reply.get(),sizeof(*reply))==B_OK);
 const auto& r=reply->metadata;
 assert(r.version==1 && !r.token && !r.position && r.state==TT_RUNTIME_PASSED && r.closed && r.input_ready && !r.recovery_required
  && r.session && r.driver_generation==sReloadGeneration && r.source_pages==23237 && r.workspace_pages==512
  && r.saved_words==8281546 && r.restored_words==8281546 && reply->history_runs==336 && reply->history_pages==1680
  && reply->projections==4 && reply->peer_boots==119 && reply->peer_stopped==119 && reply->peer_held==119
  && reply->root_starts==5 && reply->root_holds==5 && reply->convolution_workers==120 && reply->convolution_jobs==640
  && reply->convolution_channels==10240 && reply->convolution_held==120 && reply->input_workers==96 && reply->input_installed==96
  && reply->input_armed==96 && reply->input_released==96 && reply->input_completed==96 && reply->input_held==96
  && reply->readback_workers==96 && reply->readback_words==30912 && reply->saved_words==8134072 && reply->verified_words==16268144
  && reply->installed_words==989304 && reply->restored_words==8134072 && f.transport->GdnValidationReads()==30912
  && !locked && locks==releases && f.c->q38GdnCallThread==-1 && f.c->q38EmbeddingModel->Closed() && !f.c->Unsafe());
 for(unsigned half=0;half<96;half++)assert(source->CheckGdnInput(half,reply->values+half*322));
 assert(source->ReferenceReads()==1672 && source->ProjectionReads()==164800 && source->ConvolutionReads()==640
  && source->GdnInputReads()==192 && !f.io.pciWrites && !f.io.HostReadMock::bmeWrites && EndFactory(*f.c)
  && f.c->q38Dispatch->closed && !sColdRecoveryRequired);
 puts("PASS ORIGINAL token0 ACTUAL DEBUG46 public root/team/thread/copy/global semaphore/source/native SAMEowner QCB1ALL120->QGI1ALL96/30912fixed LOCALreads/strict original references/restore/actual END;SIMULATED_NOT_PHYSICAL_CARD_STATE_INIT_OR_SPEED");
}
