/* SPDX-License-Identifier: MIT */
// Actual public control+routing+call scope over the ACTUAL factory/VM fixture.
// No ready/controller callback forgery. OS endpoints alone are mocked.
#define PIONEER_Q38_EMBEDDING_FACTORY_FIXTURE_ONLY
#define B_DEVICE_OP_CODES_END 1000
#include "pioneer_q38_embedding_native_factory_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38EmbeddingProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnModelProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnGraphProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38WorkProtocol.h"
using status_t=int;using thread_id=int;
static constexpr int B_BAD_VALUE=-10,B_NOT_ALLOWED=-11,B_DEV_NOT_READY=-12,B_BUSY=-13,sLock=91;
struct thread_info{int team;};
static unsigned locks=0,releases=0,copies=0,copyFailure=0;
static int callerTeam=7,lockStatus=B_OK;
static bool locked=false;
static status_t get_thread_info(int id,thread_info* info){if(id<0)return B_BAD_VALUE;info->team=callerTeam;return B_OK;}
static status_t acquire_sem(int sem){
 assert(sem==sLock);
 locks++;if(lockStatus!=B_OK)return lockStatus;
 if(locked)return B_NOT_ALLOWED;
 locked=true;return B_OK;
}
static void release_sem(int sem){assert(sem==sLock && locked);locked=false;releases++;}
static status_t user_memcpy(void* out,const void* in,size_t bytes){if(++copies==copyFailure)return B_BAD_VALUE;memcpy(out,in,bytes);return B_OK;}
static uint64 system_time(){static uint64 time=100;return ++time;}
#define TT_PIONEER_DRIVER_RELOAD
#define TT_PIONEER_EXPERIMENTAL_Q38_BANKS
#include "embedding_factory_probe_lock.inc"
#include "embedding_factory_call_scope.inc"
#include "embedding_factory_control.inc"
static status_t Route(PioneerProbeCookie* instance,uint32 operation,void* data,size_t length){
#include "embedding_factory_routing.inc"
 return B_BAD_VALUE;
}
static tt_q38_embedding_produce Request(unsigned token=0){tt_q38_embedding_produce r={};r.version=1;r.token=token;r.position=8191;return r;}
static void ResetScope(NativeFactoryFixture& f){
 f.c->q38GdnCallThread=-1;callerTeam=7;lockStatus=B_OK;locks=releases=copies=copyFailure=0;locked=false;
}
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned positives=0,refusals=0,faults=0;
 for(unsigned bank=0;bank<7;bank++){
  unsigned token=PioneerQ38EmbeddingFirst(bank)+PioneerQ38EmbeddingRows(bank)-1;
  NativeFactoryFixture f(argv[1],argv[2],token,0);ResetScope(f);unsigned touches=f.io.touches;
  tt_q38_embedding_support s={};assert(Route(&f.cookie,TT_PCI_PROBE_Q38_EMBEDDING_SUPPORT,&s,sizeof(s))==B_OK
   && s.version==1 && s.driver==55 && s.model_namespace==10 && s.no_cpu_reference==1 && !s.model_inference && f.io.touches==touches);
  auto r=Request(token);assert(Route(&f.cookie,TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE,&r,sizeof(r))==B_OK
   && r.state==TT_RUNTIME_PASSED && r.closed==1 && r.input_ready==1 && !r.recovery_required
   && r.source_pages==12 && r.workspace_pages==512 && r.saved_words==73737 && r.restored_words==73737
   && r.session && r.driver_generation==sReloadGeneration && r.elapsed_us==1
   && f.c->q38EmbeddingModel->Closed() && f.c->q38GdnCallThread==-1 && !locked && locks==releases && EndFactory(*f.c));
  touches=f.io.touches;auto again=Request(token);
  assert(Route(&f.cookie,TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE,&again,sizeof(again))==B_NOT_ALLOWED && f.io.touches==touches);
  assert(AcquireProbeLock(&f.cookie,true)==B_NOT_ALLOWED && !locked && f.io.touches==touches);
  assert(AcquireQ38GdnSessionLock(&f.cookie)==B_OK && locked);release_sem(sLock);
  positives++;
 }
 {NativeFactoryFixture f(argv[1],argv[2]);ResetScope(f);unsigned touches=f.io.touches;
  // Real public SUPPORT must work before BANK_BEGIN/context allocation.
  f.cookie.context=nullptr;tt_q38_embedding_support s={};
  assert(Route(&f.cookie,TT_PCI_PROBE_Q38_EMBEDDING_SUPPORT,&s,sizeof(s))==B_OK && s.driver==55 && !locked);
  f.cookie.context=f.c.get();
  for(unsigned field=0;field<18;field++){
   if(field==1 || field==2)continue;
   auto r=Request();auto* words=reinterpret_cast<uint32*>(&r);words[field]=field==0?2:1;
   assert(Route(&f.cookie,TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE,&r,sizeof(r))==B_BAD_VALUE);refusals++;
  }
  for(unsigned test=0;test<7;test++){
   auto r=Request();size_t bytes=sizeof(r);PioneerProbeCookie* cookie=&f.cookie;
   if(test==0)r.token=248320;if(test==1)r.position=8192;if(test==2)bytes--;if(test==3)bytes++;
   if(test==4)cookie=nullptr;if(test==5)callerTeam=8;if(test==6)lockStatus=B_NOT_ALLOWED;
   assert(Route(cookie,TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE,&r,bytes)!=B_OK);callerTeam=7;lockStatus=B_OK;refusals++;
  }
  for(unsigned op:{TT_PCI_PROBE_Q38_FFN_MODEL_SUPPORT,TT_PCI_PROBE_Q38_FFN_MODEL_INITIALIZE,
    TT_PCI_PROBE_Q38_FFN_GRAPH_SUPPORT,TT_PCI_PROBE_Q38_FFN_GRAPH_RUN,TT_PCI_PROBE_Q38_WORK_SUPPORT,
    TT_PCI_PROBE_Q38_WORK_RUN,TT_PCI_PROBE_Q38_DISPATCH_SUPPORT,TT_PCI_PROBE_Q38_DISPATCH_RUN}){
   auto r=Request();assert(Route(&f.cookie,op,&r,sizeof(r))==B_NOT_ALLOWED);refusals++;
  }
  assert(f.io.touches==touches && !f.c->q38EmbeddingModel && !locked && f.c->q38GdnCallThread==-1);
 }
 for(unsigned fault=0;fault<5;fault++){
  NativeFactoryFixture f(argv[1],argv[2]);ResetScope(f);auto r=Request();
  if(fault==0)f.io.zeroFailRun=1;if(fault==1)f.io.zeroFailRun=2;if(fault==2)f.io.failMap=true;
  if(fault==3)f.io.embeddingPacketFault=22;if(fault==4)copyFailure=2;
  auto status=Route(&f.cookie,TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE,&r,sizeof(r));
  assert(status==(fault==4?B_BAD_VALUE:B_OK) && sColdRecoveryRequired && f.c->dispatchOwnerRetained
   && f.owner.banks.Quarantined() && f.io.retained && f.c->q38GdnCallThread==-1 && !locked && locks==releases);
  if(fault!=4)assert(r.state==TT_RUNTIME_COMPLETION_FAILED && r.recovery_required && !r.closed && !r.input_ready);
  unsigned touches=f.io.touches;auto again=Request();
  assert(Route(&f.cookie,TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE,&again,sizeof(again))!=B_OK && !EndFactory(*f.c) && f.io.touches==touches);faults++;
 }
 printf("PASS ACTUAL native public embedding control/call-scope/routing:positive%u/refusals%u/faults%u;ALL7 source banks actual zero+QPE1 instructions,real factory ownership/END,request replies cannot become ready authority,failed user-copy CLOSE ONLY retained;SIMULATED_OS_HARDWARE_NOT_CARD_OR_MODEL_SPEED\n",positives,refusals,faults);
}
