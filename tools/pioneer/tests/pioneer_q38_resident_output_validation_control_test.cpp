/* SPDX-License-Identifier: MIT */
// Actual public control/copy/global lock/call scope/native factories/END.
// OS/learned/physical BYTE endpoints TEST;NO physical/model-speed claim.
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
#define PIONEER_Q38_RMS_FACTORY_FIXTURE_ONLY
#include "pioneer_q38_learned_rms_native_factory_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputValidationProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include <algorithm>
#include <sys/wait.h>
#include <unistd.h>
#ifdef PIONEER_Q38_TEST_RESIDENT_OUTPUT_ORIGINAL
#include "pioneer_q38_decoder_original_source.h"
#endif
using status_t=int;using thread_id=int;
enum {B_BAD_VALUE=-10,B_NOT_ALLOWED=-11,B_DEV_NOT_READY=-12,B_BUSY=-13,B_NO_MEMORY=-14,sLock=91};
static unsigned locks=0,releases=0,copies=0,copyFailure=0,unexpectedRoutes=0;
static int callerUid=0,lockStatus=B_OK;static bool locked=false;
static void (*copyCheckpoint)(void*,const void*,size_t)=nullptr;static void* copyContext=nullptr;
static status_t acquire_sem(int sem){assert(sem==sLock);if(lockStatus!=B_OK)return lockStatus;if(locked)return B_NOT_ALLOWED;locks++;locked=true;return B_OK;}
static void release_sem(int sem){assert(sem==sLock && locked);locked=false;releases++;}
static status_t user_memcpy(void* out,const void* in,size_t bytes){
 ++copies;if(copyCheckpoint)copyCheckpoint(copyContext,in,bytes);
 if(copies==copyFailure)return B_BAD_VALUE;memcpy(out,in,bytes);return B_OK;
}
static uint64 system_time(){static uint64 time=100;return ++time;}
#include "debug68_probe_lock.inc"
#include "debug68_call_scope.inc"
#define dprintf(...) fprintf(stderr,__VA_ARGS__)
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputValidationControl.h"
static status_t DriverReloadControl(PioneerProbeCookie*,uint32,void*,size_t){unexpectedRoutes++;return B_NOT_ALLOWED;}
static status_t Q38BankControl(PioneerProbeCookie*,uint32,void*,size_t){unexpectedRoutes++;return B_NOT_ALLOWED;}
static status_t Q38SpanControl(PioneerProbeCookie*,uint32,void*,size_t){unexpectedRoutes++;return B_NOT_ALLOWED;}
#define geteuid() callerUid
#include "debug68_routing.inc"
#undef geteuid
static std::unique_ptr<tt_q38_resident_output_validation> Request(){
 auto r=std::make_unique<tt_q38_resident_output_validation>();r->metadata.version=1;return r;
}
struct ReplyCopyCheckpoint {
 NativeFactoryFixture& fixture;ProjectionNativeTransport& transport;
 bool child=false,invoked=false;unsigned touches=0,device=0;
 static void Copy(void* context,const void* input,size_t bytes){
  auto& self=*static_cast<ReplyCopyCheckpoint*>(context);
  if(self.child || bytes!=sizeof(tt_q38_resident_output_validation))return;
  const auto& r=*static_cast<const tt_q38_resident_output_validation*>(input);
  if(r.metadata.state!=TT_RUNTIME_PASSED)return;
  auto& c=*self.fixture.c;
  assert(!self.invoked && locked && c.q38GdnCallThread==currentThread && r.metadata.closed
   && r.readback_pages==1592 && r.readback_words==1629120
   && c.q38ResidentOutputNative->Closed(c.banks,self.fixture.owner.dram));
  self.touches=self.fixture.io.touches;self.device=self.transport.Touches();
  puts("DEBUG68_ACTUAL_FULL_PIPELINE_READBACK_RESTORE_COMPLETE_BEFORE_USER_COPY;forking final-copy failure");
  pid_t pid=fork();assert(pid>=0);
  if(!pid){self.child=true;copyFailure=copies;return;}
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));
  assert(self.fixture.io.touches==self.touches && self.transport.Touches()==self.device);
  self.invoked=true;copyCheckpoint=nullptr;
 }
 void CheckFailure(status_t status,const tt_q38_resident_output_validation& reply){
  auto& f=fixture;auto& c=*f.c;auto* state=c.q38ResidentOutputNative;
  assert(child && status==B_BAD_VALUE && PioneerQ38ResidentOutputValidationRequest(reply)
   && state && state->Unsafe() && c.q38EmbeddingModel->Unsafe() && c.dispatchOwnerRetained && sColdRecoveryRequired
   && c.banks.Quarantined() && c.lease.recovery_required && c.bulk.unsafe && f.transfer.quarantined && f.wide.quarantined
   && c.q38Clock.quarantined && f.io.PioneerHostPageIO::quarantined
   && !locked && locks==releases && c.q38GdnCallThread==-1
   && f.io.touches==touches && transport.Touches()==device);
  auto retry=Request();
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATE,retry.get(),sizeof(*retry))!=B_OK
   && !EndFactory(c) && !EndFactory(c) && f.io.touches==touches && transport.Touches()==device);
  puts("PASS ACTUAL DEBUG68 final-user-copy failure AFTER full native compute/readback/restore:ALL owners/maps/snapshots/clock/hosts retained,call seal released,ZERO laterIO-reset-restore-retry-END");
 }
};
int main(int argc,char** argv){
 setvbuf(stdout,nullptr,_IOLBF,0);
#ifdef PIONEER_Q38_TEST_RESIDENT_OUTPUT_ORIGINAL
 assert(argc==9);
 puts("ORIGINAL_COMBINED_FULL_SHA_AND_INDEPENDENT_REFERENCES_BEFORE_NATIVE_CONTEXT");
 auto original=OpenFfnResidualOriginalSource(argv[8]);
 assert(original && original->Prepare(0) && original->SealBeforeNativeContext() && original->FfnWeightsReady());
#else
 assert(argc==8);
#endif
 (void)&AcquireQ38DeltaLock; // Actual global-lock helper;legacy delta route is closed in DEBUG68.
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 NativeFactoryFixture f(argv[3],argv[4]);f.io.rmsImage=argv[5];f.c->q38Clock.value.busy_mhz=1400;
#ifdef PIONEER_Q38_TEST_RESIDENT_OUTPUT_ORIGINAL
 f.io.originalEmbeddingWeights=original->EmbeddingTile();f.io.originalRmsWeights=original->LearnedNorm();
#endif
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
 for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);
  if(p->kind!=plan->WeightKind || p->layer || p->role>13)continue;
  unsigned first=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
  for(unsigned page=0;page<p->allocated/4096;page++)f.learned[first+page]=1;
 }
 f.c->q38GdnCallThread=-1;currentTeam=7;currentThread=42;
 ProjectionNativeTransport transport(argv[1],argv[2],f.io.wholeWorkspace);f.io.projectionTransport=&transport;f.io.projectionSeedRequired=true;
 transport.ConfigureResidentOutputControl(argv[6],argv[7],f.io);
#ifdef PIONEER_Q38_TEST_RESIDENT_OUTPUT_ORIGINAL
 transport.LearnedSource(*original);
#endif
 auto& c=*f.c;unsigned touched=f.io.touches,device=transport.Touches(),epoch=unsigned(sQ38EmbeddingModelEpoch),refused=0;
 tt_q38_embedding_support support={};
 assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATION_SUPPORT,&support,sizeof(support))==B_OK
  && support.version==1 && support.driver==55 && support.model_namespace==19 && support.inputs==1629120
  && support.vocabulary==248320 && support.runtime_pages==512 && !support.no_cpu_reference && !support.model_inference);
 for(unsigned field=0;field<18;field++){
  if(field==1)continue;auto r=Request();uint32_t v=field?1:2;memcpy(reinterpret_cast<uint8_t*>(&r->metadata)+field*4,&v,4);
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
 }
 for(unsigned word=0;word<46;word++){
  auto r=Request();reinterpret_cast<uint8_t*>(r.get())[72+word*4]=1;
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
 }
 for(unsigned at:{0U,10239U,10240U,40959U,40960U,47103U,47104U,50175U,50176U,50943U,50944U,51135U,51136U,56255U,56256U,842687U,842688U,1629119U}){
  auto r=Request();r->values[at]=1;
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
 }
 for(unsigned bad=0;bad<11;bad++){
  auto r=Request();auto* cookie=&f.cookie;size_t bytes=sizeof(*r);
  if(bad==0)r->metadata.token=248320;if(bad==1)r->metadata.position=1;if(bad==2)bytes--;if(bad==3)bytes++;
  if(bad==4)cookie=nullptr;if(bad==5)currentTeam=8;if(bad==6)lockStatus=B_NOT_ALLOWED;if(bad==7)callerUid=1000;
  if(bad==8)copyFailure=copies+1;if(bad==9)failThreadInfo=true;if(bad==10)f.cookie.magic=0;
  assert(ProbeControl(cookie,TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATE,r.get(),bytes)!=B_OK);refused++;
  f.cookie.magic=0x50313030;currentTeam=7;lockStatus=B_OK;callerUid=0;copyFailure=0;failThreadInfo=false;
 }
 for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer || p->role>13)continue;
  for(unsigned relative:{0U,p->allocated/4096-1})for(uint8_t value:{uint8_t(0),uint8_t(2)}){
   unsigned at=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096+relative;assert(f.learned[at]==1);f.learned[at]=value;
   auto r=Request();assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATE,r.get(),sizeof(*r))==B_NOT_ALLOWED);
   f.learned[at]=1;refused++;
  }
 }
 assert(!c.q38EmbeddingModel && !c.q38ResidentOutputNative && !c.dispatchOwnerRetained && !locked && locks==releases
  && c.q38GdnCallThread==-1 && sQ38EmbeddingModelEpoch==epoch && f.io.touches==touched && transport.Touches()==device && !unexpectedRoutes);
 printf("PASS ACTUAL DEBUG68 public inert-refusals%u/root-team-copy/complete fixed request/missing ALL56 layer0 norm+FFN+projection+Conv+GDN+output stripes;ZERO IO/epoch/model publication\n",refused);
 puts("BEGIN ACTUAL DEBUG68 public call:source scope/ALL initialization/prefix/new combined programs/readback/restore/copy");
 ReplyCopyCheckpoint checkpoint{f,transport};copyCheckpoint=ReplyCopyCheckpoint::Copy;copyContext=&checkpoint;
 auto reply=Request();status_t status=ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATE,reply.get(),sizeof(*reply));
 if(checkpoint.child){checkpoint.CheckFailure(status,*reply);_exit(0);}
 const auto& r=reply->metadata;
 if(status!=B_OK || r.state!=TT_RUNTIME_PASSED)fprintf(stderr,"DEBUG68_CONTROL_FAILED status%d state%u cold%u unsafe%u output%u source%u\n",
  status,r.state,unsigned(sColdRecoveryRequired),unsigned(c.Unsafe()),reply->readback_words,r.source_pages);
 assert(status==B_OK && r.state==TT_RUNTIME_PASSED && r.closed && r.input_ready && !r.recovery_required && r.session
  && r.driver_generation==sReloadGeneration && r.source_pages==68603
  && r.source_pages==unsigned(std::count(f.learned.begin(),f.learned.end(),uint8_t(1)))
  && r.workspace_pages==512 && r.saved_words==19232898 && r.restored_words==19232898 && checkpoint.invoked);
 assert(reply->state_runs==672 && reply->state_pages==73728 && reply->history_runs==336 && reply->history_pages==1680
  && reply->projection_runs==4 && reply->projection_boots==119 && reply->projection_root_runs==5 && reply->projection_root_holds==5
  && reply->cohort_boots==119 && reply->cohort_root_runs==1 && reply->cohort_root_holds==1 && reply->cohort_stopped==119 && reply->cohort_held==119
  && reply->convolution_jobs==640 && reply->gdn_halves==96 && reply->output_chunks==24 && reply->output_rows==5120
  && reply->readback_pages==1592 && reply->readback_words==1629120 && !reply->current_slot && reply->next_slot==1
  && reply->saved_words==TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS && reply->verified_words==2*TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS
  && reply->installed_words==TT_Q38_RESIDENT_OUTPUT_CODE_WORDS && reply->restored_words==TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS
  && reply->prefix_saved_words==TT_Q38_PROJECTION_GRAPH_SAVED_WORDS && reply->prefix_verified_words==2*TT_Q38_PROJECTION_GRAPH_SAVED_WORDS
  && reply->prefix_installed_words==TT_Q38_PROJECTION_GRAPH_CODE_WORDS && reply->prefix_restored_words==TT_Q38_PROJECTION_GRAPH_SAVED_WORDS);
 for(auto reserved:reply->reserved_reply)assert(!reserved);
 assert(!locked && locks==releases && c.q38GdnCallThread==-1 && !c.Unsafe() && !sColdRecoveryRequired && !unexpectedRoutes);
 transport.VerifyResidentOutput();transport.VerifyResidentReadback(reply->values,TT_Q38_RESIDENT_OUTPUT_VALIDATION_WORDS);
#ifdef PIONEER_Q38_TEST_RESIDENT_OUTPUT_ORIGINAL
 touched=f.io.touches;device=transport.Touches();
 assert(original->ReferenceReads()==1673 && original->ProjectionReads()==164800 && original->ConvolutionReads()==640
  && original->GdnInputReads()==192 && original->GdnNormReads()==96 && original->GdnOutputReads()==61440 && !original->FfnReads()
  && original->CheckResidentOutput(*reply));
 unsigned mutations=0;
 for(unsigned at:{0U,10239U,10240U,40959U,40960U,47103U,47104U,50175U,50176U,50183U,50919U,50944U,51135U,51136U,56255U,56256U,842687U,842688U,1629119U}){
  auto saved=reply->values[at];reply->values[at]=0x7fc00000;
  assert(!original->CheckResidentOutput(*reply));reply->values[at]=saved;mutations++;
 }
 assert(mutations==19 && original->CheckResidentOutput(*reply) && f.io.touches==touched && transport.Touches()==device);
 puts("PASS ORIGINAL token0 ACTUAL DEBUG68 combined machine:authenticated sameGGUF/ALL independent Conv+history+next-state+gated checks/ALL3072native operands+192scales+5120raw-GGUF-row outputs/19nonfinite-or-current-state corruptions refused;NO CPU products fed to native code");
#endif
 touched=f.io.touches;device=transport.Touches();auto again=Request();
 assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATE,again.get(),sizeof(*again))==B_NOT_ALLOWED
  && f.io.touches==touched && transport.Touches()==device && EndFactory(c) && !sColdRecoveryRequired);
#ifdef PIONEER_Q38_TEST_RESIDENT_OUTPUT_ORIGINAL
 puts("PASS ORIGINAL token0 ACTUAL DEBUG68 public/native complete lifetime/1592fixed reads+ALL1629120words/full43.8MB restore+secondproof/independent original-GGUF observer/reply copy+failure containment/one-shot refusal/actual END;OS+physical endpoints TEST,NOT silicon/full64layer/tokenloop/Work/rate qualification");
#else
 puts("PASS ACTUAL DEBUG68 public root/team/request copy/real global lock+call seal/private native source+zero+prefix/ACTUAL full combined programs/1592fixed reads+ALL1629120words/full43.8MB restore+secondproof/reply copy/scope release/one-shot refusal/actual END;OS+learned+physical bytes TEST,NOT original-GGUF/silicon/Work/rate qualification");
#endif
}
