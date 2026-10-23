/* SPDX-License-Identifier: MIT */
// ACTUAL new root-only public control/routing/global call lock/factories/
// instructions/full native END. Only OS/physical endpoints are simulated.
#define PIONEER_Q38_PROJECTION_NATIVE_FACTORY_ONLY
#define B_DEVICE_OP_CODES_END 1000
#include "pioneer_q38_projection_execution_native_factory_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionValidationProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnModelProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnGraphProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38WorkProtocol.h"
using status_t=int;using thread_id=int;
enum{B_BAD_VALUE=-10,B_NOT_ALLOWED=-11,B_DEV_NOT_READY=-12,B_BUSY=-13,B_NO_MEMORY=-14,sLock=91};
struct thread_info{int team;};
static unsigned locks=0,releases=0,copies=0,copyFailure=0;
static int callerTeam=7,callerUid=0,lockStatus=B_OK;static bool locked=false;
static status_t get_thread_info(int id,thread_info* info){if(id<0)return B_BAD_VALUE;info->team=callerTeam;return B_OK;}
static status_t acquire_sem(int sem){assert(sem==sLock);if(lockStatus!=B_OK)return lockStatus;if(locked)return B_NOT_ALLOWED;locks++;locked=true;return B_OK;}
static void release_sem(int sem){assert(sem==sLock && locked);locked=false;releases++;}
static status_t user_memcpy(void* out,const void* in,size_t bytes){if(++copies==copyFailure)return B_BAD_VALUE;memcpy(out,in,bytes);return B_OK;}
static uint64 system_time(){static uint64 time=100;return ++time;}
#define TT_PIONEER_DRIVER_RELOAD
#define TT_PIONEER_EXPERIMENTAL_Q38_BANKS
#include "projection_validation_probe_lock.inc"
#include "projection_validation_call_scope.inc"
#define dprintf(...) fprintf(stderr,__VA_ARGS__) // captured OS log only;never device reads
#include "projection_validation_control.inc"
#define geteuid() callerUid
#include "projection_validation_route_prelude.inc"
#include "projection_validation_routing.inc"
 return B_BAD_VALUE;
}
#undef geteuid
struct ProjectionControlFixture:ProjectionNativeFixture {
 ProjectionControlFixture(const char* root,const char* peer,const char* zero,const char* embedding,const char* rms)
  :ProjectionNativeFixture(zero,embedding,rms){
  c->q38GdnCallThread=-1;callerTeam=7;callerUid=0;lockStatus=B_OK;locks=releases=copies=copyFailure=0;locked=false;
  // A single native call hands its real post-QRN1 DEVICE bytes/controls to
  // the projection endpoint lazily. No source/context/owner proof replaced.
  transport.reset(new ProjectionNativeTransport(root,peer,io.wholeWorkspace));io.projectionTransport=transport.get();io.projectionSeedRequired=true;
 }
};
static std::unique_ptr<tt_q38_projection_validation> Request(){
 auto r=std::make_unique<tt_q38_projection_validation>();r->metadata.version=1;r->metadata.position=8191;return r;
}
int main(int argc,char** argv){
 assert(argc==6 || argc==7);setvbuf(stdout,nullptr,_IOLBF,0);int fault=argc==7?atoi(argv[6]):-1;
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 ProjectionControlFixture f(argv[1],argv[2],argv[3],argv[4],argv[5]);
 unsigned refused=0;
 if(fault<0){
  unsigned touched=f.io.touches,d=f.transport->Touches();tt_q38_embedding_support support={};
  auto* context=f.cookie.context;f.cookie.context=nullptr;
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_PROJECTION_VALIDATION_SUPPORT,&support,sizeof(support))==B_OK);
  f.cookie.context=context;
  assert(support.version==1 && support.driver==55 && support.model_namespace==13 && support.inputs==16480
   && support.vocabulary==248320 && support.runtime_pages==512 && !support.no_cpu_reference && !support.model_inference);
  for(unsigned field=0;field<18;field++){
   if(field==1 || field==2)continue;auto r=Request();uint32 value=field?1:2;
   memcpy(reinterpret_cast<uint8*>(&r->metadata)+field*4,&value,4);
   assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_PROJECTION_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
  }
  for(unsigned field=0;field<12;field++){
   auto r=Request();reinterpret_cast<uint8*>(r.get())[sizeof(r->metadata)+field*4]=1;
   assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_PROJECTION_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
  }
  for(unsigned page=0;page<18;page++)for(unsigned at:{0U,1023U}){
   auto r=Request();r->values[page*1024+at]=1;
   assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_PROJECTION_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
  }
  for(unsigned op:{TT_PCI_PROBE_Q38_EMBEDDING_SUPPORT,TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE,
   TT_PCI_PROBE_Q38_EMBEDDING_VALIDATION_SUPPORT,TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE,
   TT_PCI_PROBE_Q38_RMS_VALIDATION_SUPPORT,TT_PCI_PROBE_Q38_RMS_VALIDATE,TT_PCI_PROBE_Q38_FFN_MODEL_SUPPORT,
   TT_PCI_PROBE_Q38_FFN_MODEL_INITIALIZE,TT_PCI_PROBE_Q38_FFN_GRAPH_SUPPORT,TT_PCI_PROBE_Q38_FFN_GRAPH_RUN,
   TT_PCI_PROBE_Q38_WORK_SUPPORT,TT_PCI_PROBE_Q38_WORK_RUN,TT_PCI_PROBE_Q38_LEGACY_WORK_SUPPORT,
   TT_PCI_PROBE_Q38_LEGACY_WORK_RUN,TT_PCI_PROBE_Q38_DISPATCH_SUPPORT,TT_PCI_PROBE_Q38_DISPATCH_RUN}){
   auto r=Request();assert(ProbeControl(&f.cookie,op,r.get(),sizeof(*r))==B_NOT_ALLOWED);refused++;
  }
  for(unsigned kind=0;kind<9;kind++){
   auto r=Request();auto* cookie=&f.cookie;size_t size=sizeof(*r);
   if(kind==0)r->metadata.token=248320;if(kind==1)r->metadata.position=8192;if(kind==2)size--;if(kind==3)size++;
   if(kind==4)cookie=nullptr;if(kind==5)callerTeam=8;if(kind==6)lockStatus=B_NOT_ALLOWED;if(kind==7)callerUid=1000;if(kind==8)copyFailure=copies+1;
   assert(ProbeControl(cookie,TT_PCI_PROBE_Q38_PROJECTION_VALIDATE,r.get(),size)!=B_OK);
   callerTeam=7;callerUid=0;lockStatus=B_OK;copyFailure=0;refused++;
  }
  assert(f.io.touches==touched && f.transport->Touches()==d && !f.c->q38EmbeddingModel && !locked && f.c->q38GdnCallThread==-1);
  printf("QPS1_DEBUG_PUBLIC_INERT_REFUSALS%u/root/team/copy/ALL output fields/old namespaces,NO endpoint IO\n",refused);
 }else{
  assert(fault<8);
  if(fault<2)f.io.zeroFailRun=unsigned(fault+1);if(fault==2)f.io.failMap=true;if(fault==3)f.io.embeddingPacketFault=22;
  if(fault==4)f.io.rmsPacketFault=8;if(fault==5)f.io.projectionFailMap=1;if(fault==6)f.transport->ValidationFault(2,18);if(fault==7)copyFailure=2;
 }
 auto reply=Request();puts("QPS1_DEBUG_ACTUAL_PUBLIC_SCOPE_ZERO_QPE1_QRN1_PROJECTIONS_READBACK");
 status_t status=ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_PROJECTION_VALIDATE,reply.get(),sizeof(*reply));
 if(fault>=0){
  assert(status==(fault==7?B_BAD_VALUE:B_OK) && sColdRecoveryRequired && f.c->dispatchOwnerRetained && f.io.retained
   && !locked && locks==releases && f.c->q38GdnCallThread==-1);
  if(fault!=7){assert(reply->metadata.recovery_required && !reply->metadata.closed);
   const auto* bytes=reinterpret_cast<const uint8*>(reply.get());for(unsigned i=sizeof(reply->metadata);i<sizeof(*reply);i++)assert(!bytes[i]);}
  unsigned n=f.io.touches,d=f.transport->Touches();auto again=Request();
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_PROJECTION_VALIDATE,again.get(),sizeof(*again))!=B_OK
   && !EndFactory(*f.c) && f.io.touches==n && f.transport->Touches()==d);
  printf("PASS actual QPS1 public DEBUG fault%d,ALL retained/no following I/O/END/restore\n",fault);return 0;
 }
 const auto& r=reply->metadata;
 printf("QPS1_DEBUG_PUBLIC_CAPTURED status%d state%u close%u recovery%u source%u saved%u restored%u runs%u boots%u stops%u held%u roots%u/%u read%u/%u memory%u/%u/%u/%u model_closed%u unsafe%u lock%u locks%u/releases%u thread%d\n",
  status,r.state,r.closed,r.recovery_required,r.source_pages,r.saved_words,r.restored_words,reply->projections,reply->peer_boots,reply->peer_stopped,
  reply->peer_held,reply->root_starts,reply->root_holds,reply->readback_pages,reply->readback_words,reply->saved_words,reply->verified_words,
  reply->installed_words,reply->restored_words,unsigned(f.c->q38EmbeddingModel && f.c->q38EmbeddingModel->Closed()),unsigned(f.c->Unsafe()),unsigned(locked),locks,releases,f.c->q38GdnCallThread);
 assert(status==B_OK && r.state==TT_RUNTIME_PASSED && r.closed && r.input_ready && !r.recovery_required && r.session
  && r.driver_generation==sReloadGeneration && r.source_pages==23193 && r.workspace_pages==512 && r.saved_words==8281546 && r.restored_words==8281546
  && reply->projections==4 && reply->peer_boots==119 && reply->peer_stopped==119 && reply->peer_held==119
  && reply->root_starts==5 && reply->root_holds==5 && reply->readback_pages==18 && reply->readback_words==18432
  && reply->saved_words==8134072 && reply->verified_words==16268144 && reply->installed_words==989304 && reply->restored_words==8134072
  && f.c->q38EmbeddingModel->Closed() && !f.c->Unsafe() && !locked && locks==releases && f.c->q38GdnCallThread==-1);
 for(unsigned i=0;i<18432;i++)assert(reply->values[i]==f.io.wholeWorkspace[TT_Q38_PROJECTION_RESULT_BASE/4+i]);
 assert(f.transport->ValidationReads()==18 && !f.io.pciWrites && !f.io.HostReadMock::bmeWrites && EndFactory(*f.c));
 printf("PASS actual QPS1 root-only public DEBUG control/routing/global scope/4projections/18fresh poisoned readbacks/ALL18432words/full restore/actual END/refusals%u;SIMULATED_OS_HARDWARE_NOT_PHYSICAL_CARD_OR_SPEED\n",refused);
}
