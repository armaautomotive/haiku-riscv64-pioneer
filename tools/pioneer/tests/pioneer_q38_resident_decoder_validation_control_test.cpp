/* SPDX-License-Identifier: MIT */
// ACTUAL root/team/control/routing/call-lock/factories/instructions/END.
// Only OS and physical/learned byte endpoints simulated;NOT model/card speed.
#define PIONEER_Q38_RESIDENT_DECODER_NATIVE_FACTORY_ONLY
#define B_DEVICE_OP_CODES_END 1000
#include "pioneer_q38_resident_decoder_native_factory_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentDecoderValidationProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionValidationProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnModelProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnGraphProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38WorkProtocol.h"
using status_t=int;using thread_id=int;
enum{B_BAD_VALUE=-10,B_NOT_ALLOWED=-11,B_DEV_NOT_READY=-12,B_BUSY=-13,B_NO_MEMORY=-14,sLock=91};
static unsigned locks=0,releases=0,copies=0,copyFailure=0;
static int callerUid=0,lockStatus=B_OK;static bool locked=false;
static status_t acquire_sem(int sem){assert(sem==sLock);if(lockStatus!=B_OK)return lockStatus;if(locked)return B_NOT_ALLOWED;locks++;locked=true;return B_OK;}
static void release_sem(int sem){assert(sem==sLock && locked);locked=false;releases++;}
static status_t user_memcpy(void* out,const void* in,size_t bytes){if(++copies==copyFailure)return B_BAD_VALUE;memcpy(out,in,bytes);return B_OK;}
static uint64 system_time(){static uint64 time=100;return ++time;}
#define TT_PIONEER_DRIVER_RELOAD
#define TT_PIONEER_EXPERIMENTAL_Q38_BANKS
#include "decoder_validation_probe_lock.inc"
#include "decoder_validation_call_scope.inc"
#define dprintf(...) fprintf(stderr,__VA_ARGS__)
#include "decoder_validation_control.inc"
#define geteuid() callerUid
#include "decoder_validation_route_prelude.inc"
#include "decoder_validation_routing.inc"
 return B_BAD_VALUE;
}
#undef geteuid
struct DecoderControlFixture:DecoderNativeFixture {
 std::unique_ptr<PioneerQ38CanonicalModelPlan> plan;
 DecoderControlFixture(const char* root,const char* peer,const char* zero,const char* embedding,const char* rms,const char* conv,unsigned selectedToken=0)
  :DecoderNativeFixture(zero,embedding,rms,selectedToken),plan(new PioneerQ38CanonicalModelPlan){
  assert(plan->Build());c->q38GdnCallThread=-1;currentTeam=7;callerUid=0;lockStatus=B_OK;locks=releases=copies=copyFailure=0;locked=false;
  transport.reset(new ProjectionNativeTransport(root,peer,io.wholeWorkspace));io.projectionTransport=transport.get();io.projectionSeedRequired=true;
  transport->DecoderSources(conv,*plan,0,io); // canonical DATA/byte endpoints only,NO owner or completion facade
 }
};
static std::unique_ptr<tt_q38_resident_decoder_validation> Request(){
 auto r=std::make_unique<tt_q38_resident_decoder_validation>();r->metadata.version=1;return r;
}
#ifndef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CONTROL_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==7 || argc==8);setvbuf(stdout,nullptr,_IOLBF,0);int fault=argc==8?atoi(argv[7]):-1;
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 DecoderControlFixture f(argv[1],argv[2],argv[3],argv[4],argv[5],argv[6]);unsigned refused=0;
 if(fault<0){
  unsigned touched=f.io.touches,d=f.transport->Touches();tt_q38_embedding_support support={};
  auto* context=f.cookie.context;f.cookie.context=nullptr;
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_DECODER_VALIDATION_SUPPORT,&support,sizeof(support))==B_OK);f.cookie.context=context;
  assert(support.version==1 && support.driver==55 && support.model_namespace==14 && support.inputs==10240
   && support.vocabulary==248320 && support.runtime_pages==512 && !support.no_cpu_reference && !support.model_inference);
  for(unsigned field=0;field<18;field++){
   if(field==1)continue;auto r=Request();uint32 value=field?1:2;memcpy(reinterpret_cast<uint8*>(&r->metadata)+field*4,&value,4);
   assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_DECODER_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
  }
  for(unsigned field=0;field<18;field++){
   auto r=Request();reinterpret_cast<uint8*>(r.get())[sizeof(r->metadata)+field*4]=1;
   assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_DECODER_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
  }
  for(unsigned page=0;page<40;page++)for(unsigned at:{0U,1023U}){
   auto r=Request();r->values[page*1024+at]=1;
   assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_DECODER_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refused++;
  }
  for(unsigned op:{TT_PCI_PROBE_Q38_PROJECTION_VALIDATION_SUPPORT,TT_PCI_PROBE_Q38_PROJECTION_VALIDATE,
   TT_PCI_PROBE_Q38_EMBEDDING_SUPPORT,TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE,TT_PCI_PROBE_Q38_EMBEDDING_VALIDATION_SUPPORT,
   TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE,TT_PCI_PROBE_Q38_RMS_VALIDATION_SUPPORT,TT_PCI_PROBE_Q38_RMS_VALIDATE,
   TT_PCI_PROBE_Q38_FFN_MODEL_SUPPORT,TT_PCI_PROBE_Q38_FFN_MODEL_INITIALIZE,TT_PCI_PROBE_Q38_FFN_GRAPH_SUPPORT,
   TT_PCI_PROBE_Q38_FFN_GRAPH_RUN,TT_PCI_PROBE_Q38_WORK_SUPPORT,TT_PCI_PROBE_Q38_WORK_RUN,
   TT_PCI_PROBE_Q38_LEGACY_WORK_SUPPORT,TT_PCI_PROBE_Q38_LEGACY_WORK_RUN,TT_PCI_PROBE_Q38_DISPATCH_SUPPORT,TT_PCI_PROBE_Q38_DISPATCH_RUN}){
   auto r=Request();assert(ProbeControl(&f.cookie,op,r.get(),sizeof(*r))==B_NOT_ALLOWED);refused++;
  }
  for(unsigned kind=0;kind<11;kind++){
   auto r=Request();auto* cookie=&f.cookie;size_t size=sizeof(*r);
   if(kind==0)r->metadata.token=248320;if(kind==1)r->metadata.position=8192;if(kind==2)size--;if(kind==3)size++;
   if(kind==4)cookie=nullptr;if(kind==5)currentTeam=8;if(kind==6)lockStatus=B_NOT_ALLOWED;if(kind==7)callerUid=1000;
   if(kind==8)copyFailure=copies+1;if(kind==9)failThreadInfo=true;if(kind==10)r->metadata.position=1;
   assert(ProbeControl(cookie,TT_PCI_PROBE_Q38_DECODER_VALIDATE,r.get(),size)!=B_OK);
   currentTeam=7;callerUid=0;lockStatus=B_OK;copyFailure=0;failThreadInfo=false;refused++;
  }
  assert(f.io.touches==touched && f.transport->Touches()==d && !f.c->q38EmbeddingModel && !locked && f.c->q38GdnCallThread==-1);
  printf("DECODER_DEBUG_PUBLIC_INERT_REFUSALS%u root/team/copy/position/ALL output fields/old namespaces,NO endpoint IO\n",refused);
 }else{
  assert(fault<9);
  if(fault==0)f.io.packetFault=1;if(fault==1)f.io.zeroFailRun=337;if(fault==2)f.io.failMap=true;
  if(fault==3)f.io.embeddingPacketFault=22;if(fault==4)f.io.rmsPacketFault=8;if(fault==5)f.io.projectionFailMap=1;
  if(fault==6)f.transport->ValidationFault(2,40);if(fault==7)copyFailure=2;if(fault==8)f.transport->DecoderFault(1);
 }
 auto reply=Request();puts("DECODER_DEBUG_ACTUAL_PUBLIC_SCOPE_HISTORY336X5_ZERO512_QPE1_QRN1_QPS1_QCB1_READBACK");
 status_t status=ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_DECODER_VALIDATE,reply.get(),sizeof(*reply));
 if(fault>=0){
  assert(status==(fault==7?B_BAD_VALUE:B_OK) && sColdRecoveryRequired && f.c->dispatchOwnerRetained && f.io.retained
   && !locked && locks==releases && f.c->q38GdnCallThread==-1);
  if(fault!=7){assert(reply->metadata.recovery_required && !reply->metadata.closed);
   const auto* bytes=reinterpret_cast<const uint8*>(reply.get());for(unsigned i=sizeof(reply->metadata);i<sizeof(*reply);i++)assert(!bytes[i]);}
  unsigned n=f.io.touches,d=f.transport->Touches();auto again=Request();
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_DECODER_VALIDATE,again.get(),sizeof(*again))!=B_OK
   && !EndFactory(*f.c) && f.io.touches==n && f.transport->Touches()==d);
  printf("PASS actual retained-convolution public DEBUG fault%d ALL retained/no following I/O/END/restore\n",fault);return 0;
 }
 const auto& r=reply->metadata;
 assert(status==B_OK && r.state==TT_RUNTIME_PASSED && r.closed && r.input_ready && !r.recovery_required && r.session
  && r.driver_generation==sReloadGeneration && r.source_pages==23235 && r.workspace_pages==512 && r.saved_words==8281546 && r.restored_words==8281546
  && reply->history_runs==336 && reply->history_pages==1680 && reply->projections==4 && reply->peer_boots==119
  && reply->peer_stopped==119 && reply->peer_held==119 && reply->root_starts==5 && reply->root_holds==5
  && reply->convolution_workers==120 && reply->convolution_jobs==640 && reply->convolution_channels==10240 && reply->convolution_held==120
  && reply->readback_pages==40 && reply->readback_words==40960 && reply->saved_words==8134072 && reply->verified_words==16268144
  && reply->installed_words==989304 && reply->restored_words==8134072 && f.c->q38EmbeddingModel->Closed() && !f.c->Unsafe()
  && !locked && locks==releases && f.c->q38GdnCallThread==-1);
 for(unsigned i=0;i<40960;i++)assert(reply->values[i]==f.io.wholeWorkspace[(i<10240?TT_Q38_CONV_OUTPUT:TT_Q38_CONV_NEXT)/4+(i<10240?i:i-10240)]);
 assert(f.transport->ValidationReads()==40 && !f.io.pciWrites && !f.io.HostReadMock::bmeWrites && EndFactory(*f.c));
 printf("PASS actual tag44 root-only DEBUG control/routing/global scope/history336x5/4projections/QCB1ALL120/40fresh poisoned reads/ALL40960words/full restore/actual END/refusals%u;SIMULATED_NOT_ORIGINAL_GGUF_PHYSICAL_CARD_OR_SPEED\n",refused);
}
#endif
