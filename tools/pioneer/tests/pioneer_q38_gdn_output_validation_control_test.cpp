/* SPDX-License-Identifier: MIT */
// Actual DEBUG55 control/scope/native54/Close/END. OS/physical byte endpoints
// simulated;all compute instructions and ORIGINAL learned bytes are real.
#define PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CONTROL_FIXTURE_ONLY
#include "pioneer_q38_resident_decoder_validation_control_test.cpp"
#include "pioneer_q38_decoder_original_source.h"
static std::unique_ptr<tt_q38_gdn_output_validation> OutputRequest(){
 auto r=std::make_unique<tt_q38_gdn_output_validation>();r->metadata.version=1;return r;
}
struct OutputValidationCheckpoint {
 DecoderControlFixture& fixture;bool child=false;unsigned kind=0,word=0;
 static void Input(void* value){
  auto& self=*static_cast<OutputValidationCheckpoint*>(value);auto& f=self.fixture;
  assert(locked && f.c->q38GdnCallThread==currentThread && !f.transport->GdnValidationReads() && f.c->q38EmbeddingModel->decoder.GatedOutput());
  puts("DEBUG55_ORIGINAL_GDN_HELD_BEFORE_IMAGE_SWITCH;forking final-local-word fault");
  pid_t child=fork();assert(child>=0);
  if(!child){self.child=true;self.kind=100;f.transport->GdnReadbackFault(829631);return;}
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 static void Output(void* value){
  auto& self=*static_cast<OutputValidationCheckpoint*>(value);auto& f=self.fixture;
  if(self.child)return;
  assert(locked && f.c->q38GdnCallThread==currentThread && f.c->q38EmbeddingModel->decoder.GdnOutputCompleted()
   && f.transport->GdnValidationReads()==829632 && !f.transport->OutputNumericReads() && !f.transport->ValidationReads());
  puts("DEBUG55_ORIGINAL_OUTPUT_ALL119_MATRIX_JOBS_AND_STOP_ALL120HELD;forking18 actual readback-copy-restore faults");
  // Fork AFTER genuine original GDN+119matrixjobs+ALL120 physical holds.
  // Children continue ACTUAL controller/restore/copy/scope,not a test facade.
  for(unsigned kind=1;kind<=15;kind++){
   unsigned limit=kind==1?3072:kind==2?192:kind==3?5120:1;
   for(unsigned edge=0;edge<(limit==1?1U:2U);edge++){
    unsigned word=edge?limit-1:0;
    pid_t child=fork();assert(child>=0);
    if(!child){
     self.child=true;self.kind=kind;self.word=word;
     if(kind<=4)f.transport->OutputReadbackFault(kind,word);
     else if(kind<=14)f.transport->ValidationFault(kind-4,kind%2?1:5);
     else copyFailure=copies+1;
     return;
    }
    int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
   }
  }
 }
};
static void CheckOutputFailure(DecoderControlFixture& f,const tt_q38_gdn_output_validation& r,status_t status,const OutputValidationCheckpoint& fault){
 assert(status==(fault.kind==15?B_BAD_VALUE:B_OK) && sColdRecoveryRequired && f.c->dispatchOwnerRetained && f.io.retained
  && !locked && locks==releases && f.c->q38GdnCallThread==-1);
 if(fault.kind!=15){
  assert(r.metadata.recovery_required && !r.metadata.closed);
  const auto* data=reinterpret_cast<const uint8_t*>(&r);for(unsigned i=sizeof(r.metadata);i<sizeof(r);i++)assert(!data[i]);
 }
 if(fault.kind<=4)assert(f.transport->OutputReadbackFaultInjected());
 if(fault.kind>=5 && fault.kind<=14){
  unsigned page=fault.kind%2?1:5;
  assert(f.transport->ValidationFaultInjected() && f.transport->ValidationReads()==page);
  assert(f.transport->OutputNumericReads()==3264+(page-1)*1024+(fault.kind==6?1024:0));
 }
 if(fault.kind==100)assert(f.transport->GdnValidationReads()==829632 && !f.transport->OutputNumericReads() && !f.transport->ValidationReads());
 if(fault.kind==1)assert(f.transport->OutputNumericReads()==fault.word+1);
 if(fault.kind==2)assert(f.transport->OutputNumericReads()==3072+fault.word+1);
 if(fault.kind==3)assert(f.transport->OutputNumericReads()==3264+fault.word+1);
 f.UnknownNative(); // ALL borrowed host/clock/source/cohort owners,not just top-level flags
 unsigned a=f.io.touches,b=f.transport->Touches();auto again=OutputRequest();
 assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATE,again.get(),sizeof(*again))!=B_OK && !EndFactory(*f.c)
  && f.io.touches==a && f.transport->Touches()==b);
 printf("PASS ACTUAL DEBUG55 failure%u word%u/GDN%u/output%u/readPackets%u/ALL owners-snapshots-hosts retained/ZERO laterIO-reset-restore-retry-END\n",
  fault.kind,fault.word,f.transport->GdnValidationReads(),f.transport->OutputNumericReads(),f.transport->ValidationReads());
}
int main(int argc,char** argv){
 assert(argc==11 || (argc==12 && !strcmp(argv[11],"--faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;(void)&Request;
 puts("DEBUG55_ORIGINAL_SOURCE_AND_REFERENCES_BEFORE_CONTEXT;NO_DEVICE_ACCESS");
 auto source=OpenGdnOutputOriginalSource(argv[7]);assert(source && source->Prepare(0) && source->SealBeforeNativeContext());
 DecoderControlFixture f(argv[1],argv[2],argv[3],argv[4],argv[5],argv[6]);
 f.io.originalEmbeddingWeights=source->EmbeddingTile();f.io.originalRmsWeights=source->LearnedNorm();
 f.transport->GdnInputSource(argv[8]);f.transport->LearnedSource(*source);f.transport->OutputSources(argv[9],argv[10]);
 unsigned stripes=0,pages=0;
 for(unsigned i=0;i<f.plan->Count();i++){
  const auto* p=f.plan->Part(i);if(p->kind!=f.plan->WeightKind || p->layer || p->role!=13)continue;
  for(unsigned page=0;page<p->allocated/4096;page++)f.learned[p->bank*786432+p->offset/4096+page]=1;
  stripes++;pages+=p->allocated/4096;
 }
 assert(stripes==7 && pages==8640 && source->GdnOutputWeightsReady());
 unsigned a=f.io.touches,b=f.transport->Touches(),refused=0;tt_q38_embedding_support support={};
 assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATION_SUPPORT,&support,sizeof(support))==B_OK
  && support.version==1 && support.driver==55 && support.model_namespace==18 && support.inputs==838016
  && support.vocabulary==248320 && support.runtime_pages==512 && !support.no_cpu_reference && !support.model_inference);
 for(unsigned field=0;field<18;field++){
  if(field==1)continue;auto r=OutputRequest();uint32_t value=field?1:2;memcpy(reinterpret_cast<uint8_t*>(&r->metadata)+field*4,&value,4);
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
 }
 for(unsigned field=0;field<46;field++){
  auto r=OutputRequest();reinterpret_cast<uint8_t*>(r.get())[72+field*4]=1;
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
 }
 for(unsigned half=0;half<96;half++)for(unsigned at:{0U,321U,322U,8513U,8514U,8577U,8578U,8641U}){
  auto r=OutputRequest();r->values[half*8642+at]=1;
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
 }
 for(unsigned at:{829632U,832703U,832704U,832895U,832896U,838015U}){
  auto r=OutputRequest();r->values[at]=1;
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
 }
 for(unsigned op:{TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATION_SUPPORT,TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATE,
  TT_PCI_PROBE_Q38_GDN_RECURRENT_VALIDATE,TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATE,TT_PCI_PROBE_Q38_DECODER_VALIDATE,
  TT_PCI_PROBE_Q38_PROJECTION_VALIDATE,TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE,TT_PCI_PROBE_Q38_FFN_MODEL_INITIALIZE,
  TT_PCI_PROBE_Q38_FFN_GRAPH_RUN,TT_PCI_PROBE_Q38_WORK_RUN,TT_PCI_PROBE_Q38_LEGACY_WORK_RUN,TT_PCI_PROBE_Q38_DISPATCH_RUN}){
  auto r=OutputRequest();assert(ProbeControl(&f.cookie,op,r.get(),sizeof(*r))==B_NOT_ALLOWED);refused++;
 }
 for(unsigned kind=0;kind<11;kind++){
  auto r=OutputRequest();auto* cookie=&f.cookie;size_t bytes=sizeof(*r);
  if(kind==0)r->metadata.token=248320;if(kind==1)r->metadata.position=1;if(kind==2)bytes--;if(kind==3)bytes++;
  if(kind==4)cookie=nullptr;if(kind==5)currentTeam=8;if(kind==6)lockStatus=B_NOT_ALLOWED;if(kind==7)callerUid=1000;
  if(kind==8)copyFailure=copies+1;if(kind==9)failThreadInfo=true;if(kind==10)cookie->magic=0;
  assert(ProbeControl(cookie,TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATE,r.get(),bytes)!=B_OK);
  f.cookie.magic=0x50313030;currentTeam=7;lockStatus=B_OK;callerUid=0;copyFailure=0;failThreadInfo=false;refused++;
 }
 for(unsigned i=0;i<f.plan->Count();i++){
  const auto* p=f.plan->Part(i);if(p->kind!=f.plan->WeightKind || p->layer || p->role!=13)continue;
  for(unsigned page:{0U,p->allocated/4096-1})for(unsigned value:{0U,2U}){
   unsigned at=p->bank*786432+p->offset/4096+page;auto epoch=sQ38EmbeddingModelEpoch;f.learned[at]=value;auto r=OutputRequest();
   assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATE,r.get(),sizeof(*r))==B_NOT_ALLOWED
    && !f.c->q38EmbeddingModel && sQ38EmbeddingModelEpoch==epoch);f.learned[at]=1;refused++;
  }
 }
 assert(!f.c->q38EmbeddingModel && !locked && locks==releases && f.c->q38GdnCallThread==-1
  && f.io.touches==a && f.transport->Touches()==b);
 printf("PASS DEBUG55 actual public inert-refusals%u/root-team-copy/all fixed output fields/old namespaces/missing original ssm_out pages;ZERO endpoint IO or epoch\n",refused);
 OutputValidationCheckpoint checkpoint={f};
 if(argc==12){
  f.transport->GdnValidationCheckpoint(OutputValidationCheckpoint::Input,&checkpoint);
  f.transport->OutputValidationCheckpoint(OutputValidationCheckpoint::Output,&checkpoint);
 }
 auto reply=OutputRequest();status_t status=ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATE,reply.get(),sizeof(*reply));
 if(checkpoint.child){CheckOutputFailure(f,*reply,status,checkpoint);_exit(0);}
 const auto& r=reply->metadata;
 assert(status==B_OK && r.state==TT_RUNTIME_PASSED && r.closed && r.input_ready && !r.recovery_required && r.session
  && r.driver_generation==sReloadGeneration && r.source_pages==31878 && r.workspace_pages==512 && r.saved_words==8478154 && r.restored_words==8478154);
 assert(reply->state_runs==672 && reply->state_pages==73728 && reply->history_runs==336 && reply->history_pages==1680
  && reply->projections==4 && reply->peer_boots==119 && reply->peer_stopped==119 && reply->peer_held==119 && reply->root_starts==5 && reply->root_holds==5
  && reply->convolution_workers==120 && reply->convolution_jobs==640 && reply->convolution_channels==10240 && reply->convolution_held==120
  && reply->input_workers==96 && reply->input_installed==96 && reply->input_armed==96 && reply->input_released==96 && reply->input_completed==96 && reply->input_held==96
  && reply->layer_published==1 && reply->gated_published==1 && reply->readback_half_words==8642 && reply->readback_workers==96 && reply->readback_words==829632
  && reply->saved_words==8330680 && reply->verified_words==16661360 && reply->installed_words==1185912 && reply->restored_words==8330680
  && reply->output_installed==120 && reply->output_booted==119 && reply->output_root_runs==2 && reply->output_stopped==119 && reply->output_published==1
  && reply->output_pages==5 && reply->output_words==5120 && reply->operand_words==3072 && reply->scale_words==192);
 for(unsigned half=0;half<96;half++){
  const auto* values=reply->values+half*8642;assert(source->CheckGdnRecurrent(half,values,values+322,values+8514) && source->CheckGdnGatedNorm(half,values+8578));
  assert(!memcmp(values+8578,f.io.wholeWorkspace.data()+TT_Q38_GDN_GATED_OUTPUT/4+half*64,256));
 }
 assert(!memcmp(reply->values+829632,f.io.wholeWorkspace.data()+TT_Q38_PROJECTION_NATIVE_OFFSET/4,12288)
  && !memcmp(reply->values+832704,f.io.wholeWorkspace.data()+TT_Q38_PROJECTION_COMPACT_OFFSET/4,768)
  && !memcmp(reply->values+832896,f.io.wholeWorkspace.data()+TT_Q38_GDN_OUTPUT_RESULT/4,20480));
 f.transport->VerifyGdnOutput();
 assert(source->ReferenceReads()==1673 && source->GdnOutputReads()==61440 && f.transport->GdnValidationReads()==829632
  && f.transport->OutputNumericReads()==8384 && f.transport->ValidationReads()==5 && !locked && locks==releases && f.c->q38GdnCallThread==-1
  && f.c->q38EmbeddingModel->Closed() && !f.c->Unsafe() && !f.io.pciWrites && !f.io.HostReadMock::bmeWrites);
 a=f.io.touches;b=f.transport->Touches();auto again=OutputRequest();
 assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATE,again.get(),sizeof(*again))!=B_OK
  && f.io.touches==a && f.transport->Touches()==b && EndFactory(*f.c) && !sColdRecoveryRequired);
 puts("PASS ORIGINAL token0 ACTUAL DEBUG55 public global source-call-team-thread/73728+1680init/QPE1/QRN1/4QPS1/QCB1ALL120/QGT1ALL96/829632LOCAL values BEFORE image switch/SAME native54 fresh119boot/QOP1RUN+STOP/61440original tiles/ALL5120 independent FP32+FP64 rows/ALL120 physically held/3072native operands+192scales/5poisoned fixedNoCreads/complete8330680restore+verification/actualEND;SIMULATED_ENDPOINTS_NOT_PHYSICAL_TIMING_OR_INFERENCE_SPEED");
}
