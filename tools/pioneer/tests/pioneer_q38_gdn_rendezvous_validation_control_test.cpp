/* SPDX-License-Identifier: MIT */
// Actual DEBUG52 root/team/copy/global call/native owner/Close/END.
// Only OS/physical/learned byte endpoints simulated;NOT physical speed.
#define PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CONTROL_FIXTURE_ONLY
#include "pioneer_q38_resident_decoder_validation_control_test.cpp"
#include "pioneer_q38_decoder_original_source.h"
static std::unique_ptr<tt_q38_gdn_rendezvous_validation> RendezvousRequest(unsigned token=0){
 auto r=std::make_unique<tt_q38_gdn_rendezvous_validation>();r->metadata.version=1;r->metadata.token=token;return r;
}
struct RendezvousFaultCheckpoint {
 DecoderControlFixture& fixture;bool child=false;unsigned fault=UINT32_MAX;
 static void AtFirstRead(void* value){
  auto& self=*static_cast<RendezvousFaultCheckpoint*>(value);auto& f=self.fixture;
  assert(locked && f.c->q38GdnCallThread==currentThread && f.c->q38EmbeddingModel->decoder.GdnLayerCompleted()
   && f.c->q38EmbeddingModel->MatchesInitializedGdn(f.owner.banks,f.owner.dram) && !f.transport->GdnValidationReads());
  for(unsigned fault:{0U,1U,2U,3U,4U,5U,6U,7U,8U,9U,12U,13U,14U}){
   pid_t child=fork();assert(child>=0);
   if(!child){
    self.child=true;self.fault=fault;const unsigned boundary[]={0,321,322,8513,8514,8641,829631};
    if(fault<7)f.transport->GdnReadbackFault(boundary[fault]);
    if(fault==7)copyFailure=copies+1; // request copy already completed;fail actual reply copy
    if(fault==8)f.transport->GdnValidationRootFault();if(fault==9)f.transport->GdnValidationRestoreFault();
    if(fault>=12)f.transport->GdnValidationStatusFault(fault-12);
    return; // continue ACTUAL controller/Close/copy/scope destruction in child
   }
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
 }
};
static void CheckRendezvousFault(DecoderControlFixture& f,const tt_q38_gdn_rendezvous_validation& reply,status_t status,unsigned fault){
 assert(status==(fault==7?B_BAD_VALUE:B_OK) && sColdRecoveryRequired && f.c->dispatchOwnerRetained && f.io.retained
  && !locked && locks==releases && f.c->q38GdnCallThread==-1);
 if(fault!=7){
  assert(reply.metadata.recovery_required && !reply.metadata.closed);
  const auto* bytes=reinterpret_cast<const uint8_t*>(&reply);for(unsigned i=sizeof(reply.metadata);i<sizeof(reply);i++)assert(!bytes[i]);
 }
 const unsigned boundary[]={0,321,322,8513,8514,8641,829631};
 if(fault<7)assert(f.transport->GdnValidationReads()==boundary[fault]+1);
 if(fault==9)assert(f.transport->GdnValidationRestoreInjected() && f.transport->GdnValidationReads()==829632);
 if(fault>=12)assert(f.transport->GdnValidationReads()==8642);
 unsigned n=f.io.touches,d=f.transport->Touches();auto again=RendezvousRequest();
 assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATE,again.get(),sizeof(*again))!=B_OK
  && !EndFactory(*f.c) && f.io.touches==n && f.transport->Touches()==d);
 printf("PASS actual QGT DEBUG public fault%u/ALL retained/ZERO laterIO\n",fault);
}
int main(int argc,char** argv){
 assert(argc==9 || (argc==10 && !strcmp(argv[9],"--faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;(void)&Request;
 puts("QGT_DEBUG_ORIGINAL_REFERENCES_BEFORE_CONTEXT;NO_DEVICE_ACCESS");
 auto source=OpenGdnGatedNativeOriginalSource(argv[7]);assert(source && source->Prepare(0) && source->SealBeforeNativeContext());
 DecoderControlFixture f(argv[1],argv[2],argv[3],argv[4],argv[5],argv[6]);
 f.io.originalEmbeddingWeights=source->EmbeddingTile();f.io.originalRmsWeights=source->LearnedNorm();
 f.transport->GdnInputSource(argv[8]);f.transport->LearnedSource(*source);
 unsigned touched=f.io.touches,device=f.transport->Touches(),refused=0;tt_q38_embedding_support support={};
 assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATION_SUPPORT,&support,sizeof(support))==B_OK
  && support.version==1 && support.driver==55 && support.model_namespace==17 && support.inputs==829632 && support.vocabulary==248320
  && support.runtime_pages==512 && !support.no_cpu_reference && !support.model_inference);
 for(unsigned field=0;field<18;field++){
  if(field==1)continue;auto r=RendezvousRequest();uint32_t value=field?1:2;memcpy(reinterpret_cast<uint8_t*>(&r->metadata)+field*4,&value,4);
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
 }
 for(unsigned field=0;field<30;field++){
  auto r=RendezvousRequest();reinterpret_cast<uint8_t*>(r.get())[sizeof(r->metadata)+field*4]=1;
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
 }
 for(unsigned half=0;half<96;half++)for(unsigned at:{0U,321U,322U,8513U,8514U,8577U,8578U,8641U}){
  auto r=RendezvousRequest();r->values[half*8642+at]=1;
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
 }
 for(unsigned op:{TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATION_SUPPORT,TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATE,
  TT_PCI_PROBE_Q38_DECODER_VALIDATION_SUPPORT,TT_PCI_PROBE_Q38_DECODER_VALIDATE,TT_PCI_PROBE_Q38_PROJECTION_VALIDATE,
  TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE,TT_PCI_PROBE_Q38_FFN_MODEL_INITIALIZE,TT_PCI_PROBE_Q38_FFN_GRAPH_RUN,
  TT_PCI_PROBE_Q38_WORK_RUN,TT_PCI_PROBE_Q38_LEGACY_WORK_RUN,TT_PCI_PROBE_Q38_DISPATCH_RUN}){
  auto r=RendezvousRequest();assert(ProbeControl(&f.cookie,op,r.get(),sizeof(*r))==B_NOT_ALLOWED);refused++;
 }
 for(unsigned kind=0;kind<11;kind++){
  auto r=RendezvousRequest();auto* cookie=&f.cookie;size_t bytes=sizeof(*r);
  if(kind==0)r->metadata.token=248320;if(kind==1)r->metadata.position=1;if(kind==2)bytes--;if(kind==3)bytes++;
  if(kind==4)cookie=nullptr;if(kind==5)currentTeam=8;if(kind==6)lockStatus=B_NOT_ALLOWED;if(kind==7)callerUid=1000;
  if(kind==8)copyFailure=copies+1;if(kind==9)failThreadInfo=true;if(kind==10)cookie->magic=0;
  assert(ProbeControl(cookie,TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATE,r.get(),bytes)!=B_OK);
  f.cookie.magic=0x50313030;currentTeam=7;lockStatus=B_OK;callerUid=0;copyFailure=0;failThreadInfo=false;refused++;
 }
 assert(!f.c->q38EmbeddingModel && !locked && locks==releases && f.c->q38GdnCallThread==-1
  && f.io.touches==touched && f.transport->Touches()==device);
 printf("QGT_DEBUG_ACTUAL_PUBLIC_INERT_REFUSALS%u,ZERO endpoint IO\n",refused);
 RendezvousFaultCheckpoint checkpoint={f};
 if(argc==10)for(unsigned fault=10;fault<12;fault++){
  pid_t child=fork();assert(child>=0);
  if(!child){
   if(fault==10)f.io.recurrentFailRun=1;if(fault==11)f.io.projectionFailMap=1;
   auto reply=RendezvousRequest();status_t status=ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATE,reply.get(),sizeof(*reply));
   CheckRendezvousFault(f,*reply,status,fault);_exit(0);
  }
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 if(argc==10)f.transport->GdnValidationCheckpoint(RendezvousFaultCheckpoint::AtFirstRead,&checkpoint);
 auto reply=RendezvousRequest();status_t status=ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATE,reply.get(),sizeof(*reply));
 if(checkpoint.child){CheckRendezvousFault(f,*reply,status,checkpoint.fault);_exit(0);}
 assert(status==B_OK);
 const auto& r=reply->metadata;
 assert(r.version==1 && !r.token && !r.position && r.state==TT_RUNTIME_PASSED && r.closed && r.input_ready && !r.recovery_required
  && r.session && r.driver_generation==sReloadGeneration && r.source_pages==23238 && r.workspace_pages==512
  && r.saved_words==8478154 && r.restored_words==8478154 && reply->state_runs==672 && reply->state_pages==73728
  && reply->history_runs==336 && reply->history_pages==1680 && reply->projections==4 && reply->peer_boots==119
  && reply->peer_stopped==119 && reply->peer_held==119 && reply->root_starts==5 && reply->root_holds==5
  && reply->convolution_workers==120 && reply->convolution_jobs==640 && reply->convolution_channels==10240 && reply->convolution_held==120
  && reply->input_workers==96 && reply->input_installed==96 && reply->input_armed==96 && reply->input_released==96
  && reply->input_completed==96 && reply->input_held==96 && reply->layer_published==1 && reply->gated_published==1 && reply->readback_half_words==8642 && reply->readback_workers==96 && reply->readback_words==829632
  && reply->saved_words==8330680 && reply->verified_words==16661360
  && reply->installed_words==1185912
  && reply->restored_words==8330680
  && f.transport->GdnValidationReads()==829632 && !locked && locks==releases && f.c->q38GdnCallThread==-1
  && f.c->q38EmbeddingModel->Closed() && !f.c->Unsafe() && !f.c->q38EmbeddingModel->decoder.GdnLayerCompleted());
 for(unsigned half=0;half<96;half++){
  const auto* values=reply->values+half*8642;assert(source->CheckGdnRecurrent(half,values,values+322,values+8514) && source->CheckGdnGatedNorm(half,values+8578));
  auto* m=f.c->q38EmbeddingModel;auto* g=m->graph.Recurrent(m->identity);PioneerQ38GdnBankExtent span;uint32_t remote[8192];
  assert(g && g->InitializeHalf(m->identity,0,1,half,span) && f.io.ReadGdnState(span.bank,span.address,remote,8192)
   && !memcmp(remote,values+322,sizeof(remote)) && g->InitializeHalf(m->identity,0,0,half,span) && f.io.ReadGdnState(span.bank,span.address,remote,8192));
  for(auto word:remote)assert(!word);
 }
 unsigned n=f.io.touches,d=f.transport->Touches();auto again=RendezvousRequest();
 assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATE,again.get(),sizeof(*again))!=B_OK
  && f.io.touches==n && f.transport->Touches()==d && !f.c->Unsafe());
 assert(source->ReferenceReads()==1673 && source->GdnInputReads()==192 && source->GdnNormReads()==96 && !f.io.pciWrites && !f.io.HostReadMock::bmeWrites
  && EndFactory(*f.c) && f.c->q38Dispatch->closed && !sColdRecoveryRequired);
#ifdef PIONEER_Q38_CONV_SILU_PRECISION_NATIVE
 puts("PASS ORIGINAL token0 ACTUAL DEBUG64 preciseQCB1 public root/team/thread/copy/global scope/real73728pageinit/QCB1ALL120->ONE_QGT1ALL96/829632fixedLOCALvalues/strict independent references/NEXT_GDDR_exact/CURRENT_immutable/fullrestore/actualEND;SIMULATED_NOT_PHYSICAL_OR_SPEED");
#else
 puts("PASS ORIGINAL token0 ACTUAL DEBUG52 public root/team/thread/copy/global scope/real73728pageinit/QCB1ALL120->ONE_QGT1ALL96/829632fixedLOCALvalues/strict independent references/NEXT_GDDR_exact/CURRENT_immutable/fullrestore/actualEND;SIMULATED_NOT_PHYSICAL_OR_SPEED");
#endif
}
