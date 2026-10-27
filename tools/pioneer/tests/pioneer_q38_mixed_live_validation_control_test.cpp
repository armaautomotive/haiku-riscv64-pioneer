/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_MIXED_LIVE_CONTROL_FIXTURE_ONLY
#include "pioneer_q38_mixed_live_native_factory_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveValidationProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
static constexpr int B_NO_MEMORY=-6;
static unsigned copies=0,failCopy=0;static bool copyChild=false,copyForked=false;
static unsigned copyTouches=0,copyDevice=0;static MixedLiveNativeTransport* copyTransport=nullptr;
static status_t user_memcpy(void* out,const void* in,size_t bytes){
 ++copies;
 if(finalLockHeld && !copyChild && !copyForked && bytes==sizeof(tt_q38_mixed_live_validation)
  && static_cast<const tt_q38_mixed_live_validation*>(in)->metadata.state==TT_RUNTIME_PASSED){
  assert(finalLockHeld && activeIO && copyTransport);
  copyTouches=activeIO->touches;copyDevice=copyTransport->Touches();
  pid_t pid=fork();assert(pid>=0);
  if(!pid){copyChild=true;failCopy=copies;}
  else{int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));copyForked=true;}
 }
 if(copies==failCopy)return B_BAD_VALUE;
 memcpy(out,in,bytes);return B_OK;
}
static uint64 system_time(){static uint64 time=100;return ++time;}
// TEST endpoint handoff only. Copy actual post-QPE1 raw bytes/controls,
// never a phase/producer/ready grant or expected numerical product.
static bool ObserveRawThenHeld(PioneerStoreContext& c,PioneerQ38MixedLiveMemoryResult& r){
 auto& io=c.io;auto* transport=io.mixedLiveTransport;assert(transport && c.q38EmbeddingModel->Closed());
 std::vector<uint32> addresses,values;auto seed=[&](uint32 a,uint32 v){addresses.push_back(a);values.push_back(v);};
 seed(0x6000,io.entry);
 for(unsigned i=0;i<io.prefix.size();i++)seed(0x64000+i*4,io.prefix[i]);
 for(unsigned i=0;i<io.view.size();i++)seed(TT_Q38_PANEL_VIEW_BASE+i*4,io.view[i]);
 for(unsigned i=0;i<2048;i++)seed(0x17e000+i*4,io.destination[i]);
 uint32 source[16],command[10],counts[8];
 for(unsigned i=0;i<16;i++)source[i]=i?io.source[i]:io.config;
 for(unsigned i=0;i<10;i++)command[i]=io.command[i];
 for(unsigned i=0;i<8;i++)counts[i]=io.PanelIO::ResidentNocRead(PioneerQ38PanelCounter(i));
 transport->SeedRoot(addresses.data(),values.data(),unsigned(addresses.size()),source,command,counts);
 return PrepareQ38MixedLiveNativeHeld(c,r); // actual unchanged native function
}
static bool ObserveRawThenExecute(PioneerStoreContext& c,PioneerQ38MixedLiveResult& r){
 c.io.ConfigureMixedLiveTestTransport();c.io.mixedLiveTransport->Observe(r);
 return PrepareQ38MixedLiveNativeExecution(c,r); // actual unchanged native function
}
#define PrepareQ38MixedLiveNativeHeld ObserveRawThenHeld
#define PrepareQ38MixedLiveNativeExecution ObserveRawThenExecute
#define dprintf(...) fprintf(stderr,__VA_ARGS__)
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveValidationControl.h"
#undef dprintf
#undef PrepareQ38MixedLiveNativeExecution
#undef PrepareQ38MixedLiveNativeHeld
static unsigned otherRoutes=0;static int callerUid=0;
static status_t DriverReloadControl(PioneerProbeCookie*,uint32,void*,size_t){otherRoutes++;return B_NOT_ALLOWED;}
static status_t Q38BankControl(PioneerProbeCookie*,uint32,void*,size_t){otherRoutes++;return B_NOT_ALLOWED;}
static status_t Q38SpanControl(PioneerProbeCookie*,uint32,void*,size_t){otherRoutes++;return B_NOT_ALLOWED;}
#define geteuid() callerUid
#include "native_mixed_control_routing.inc"
#undef geteuid
static std::unique_ptr<tt_q38_mixed_live_validation> Request(){auto r=std::make_unique<tt_q38_mixed_live_validation>();r->metadata.version=1;return r;}
int main(int argc,char** argv){
 assert(argc==7);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 (void)&AcquireQ38DeltaLock;
 NativeFactoryFixture f(argv[1],argv[2]);auto& c=*f.c;auto& io=f.io;
 MixedLiveNativeTransport transport(argv[3],argv[4],argv[5],argv[6]);io.mixedLiveTransport=&transport;copyTransport=&transport;
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
 // Preflight ALL53 learned spans,first and last page missing. Every
 // partial upload must refuse BEFORE an embedding model/epoch/card IO.
 unsigned missing=0;
 for(unsigned i=0;i<plan->Count();i++){
  const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer!=3)continue;
  unsigned first=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
  for(unsigned j=0;j<p->allocated/4096;j++)f.learned[first+j]=1;
 }
 auto touches=io.touches,device=transport.Touches();auto epoch=sQ38EmbeddingModelEpoch;
 for(unsigned i=0;i<plan->Count();i++){
  const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer!=3)continue;
  unsigned first=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
  for(unsigned page:{first,first+p->allocated/4096-1}){
   f.learned[page]=0;auto r=Request();assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATE,r.get(),sizeof(*r))!=B_OK);
   assert(!c.q38EmbeddingModel && !c.q38MixedLiveNative && io.touches==touches && transport.Touches()==device && sQ38EmbeddingModelEpoch==epoch
    && !c.dispatchOwnerRetained && !sColdRecoveryRequired && !finalLockHeld && c.q38GdnCallThread==-1);
   f.learned[page]=1;missing++;
  }
 }
 for(unsigned at=0;at<64;at++){
  auto r=Request();reinterpret_cast<uint32_t*>(r.get())[at]^=1;
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);
 }
 for(unsigned op=0;op<65536;op++){
  bool allowed=op==TT_PCI_PROBE_RELOAD_STATUS || op==TT_PCI_PROBE_RELOAD_QUIESCE || op==TT_PCI_PROBE_Q38_BANK_SUPPORT
   || op==TT_PCI_PROBE_Q38_BANK_BEGIN || op==TT_PCI_PROBE_Q38_BANK_END || op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT
   || op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD || op==TT_PCI_PROBE_Q38_SPAN_END || op==TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATE
   || op==TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATION_SUPPORT;
  if(!allowed)assert(ProbeControl(&f.cookie,op,nullptr,0)==B_NOT_ALLOWED);
 }
 assert(!otherRoutes && io.touches==touches && transport.Touches()==device);
 auto reply=Request();auto pristine=Request();
 failCopy=copies+1;assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATE,reply.get(),sizeof(*reply))==B_BAD_VALUE);failCopy=0;
 callerUid=1;assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATE,reply.get(),sizeof(*reply))==B_NOT_ALLOWED);callerUid=0;
 assert(!c.q38EmbeddingModel && io.touches==touches && transport.Touches()==device);
 printf("PASS DEBUG70 actual public control preflight:ALL%umissing learned endpoints/64header mutations/65526closed opcodes/copy+root admission;ZERO model publication,epoch advance or card IO\n",missing);
 status_t status=ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATE,reply.get(),sizeof(*reply));
 if(copyChild){
  assert(status==B_BAD_VALUE && !memcmp(reply.get(),pristine.get(),sizeof(*reply)) && c.dispatchOwnerRetained && sColdRecoveryRequired
   && c.q38MixedLiveNative && c.q38MixedLiveNative->Unsafe() && c.q38Clock.Unsafe() && !finalLockHeld && c.q38GdnCallThread==-1
   && io.touches==copyTouches && transport.Touches()==copyDevice);
  auto retry=Request();assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATE,retry.get(),sizeof(*retry))!=B_OK);
  tt_resident_session_result ignored={};assert(NativeSealedFinalEnd(&f.cookie,ignored)!=B_OK || ignored.state!=TT_RESIDENT_SESSION_CLOSED);
  assert(io.touches==copyTouches && transport.Touches()==copyDevice && c.dispatchOwnerRetained);
  puts("PASS DEBUG70 final user-copy failure AFTER real full instruction/readback/restore:ALL owners quarantined/NO subsequent device IO,END,reset or retry");fflush(nullptr);_exit(0);
 }
 assert(status==B_OK && copyForked && reply->metadata.closed && reply->metadata.input_ready && !reply->metadata.recovery_required
  && reply->cohort_completed==119 && reply->cohort_stopped==119 && reply->cohort_held==120 && reply->root_runs==1 && reply->root_holds==1
  && reply->readback_pages==5 && reply->readback_words==5120 && reply->packets==4 && c.q38MixedLiveNative->Closed()
  && !c.dispatchOwnerRetained && !sColdRecoveryRequired && !finalLockHeld && c.q38GdnCallThread==-1);
 transport.VerifyClosed();auto before=transport.Touches();auto clockBefore=c.q38Clock.IO();
 auto duplicate=Request();assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATE,duplicate.get(),sizeof(*duplicate))!=B_OK
  && !c.dispatchOwnerRetained && transport.Touches()==before && c.q38Clock.IO()==clockBefore);
 puts("PASS DEBUG70 actual public handler/REAL call scope/QPE1/ALL7matrices/119STOP+ROOT/120holds/5page5120capture/full restore+second proof;duplicate inert;SYNTHETIC raw weights/OS ONLY");
 // Same omitted INITIAL OS/PCI mapping baseline DATA as the existing
 // native final-END fixture. Do NOT manufacture native/clock/STOP/closed
 // state or weaken END. These simulated handles/register snapshots are
 // not the real kernel's initial bank-session constructor qualification.
 io.InitFinalVmViews();c.transaction.map_attempted=c.transaction.config_attempted=true;
 c.pci.version=1;c.pci.state=TT_PIONEER_MMIO_READ_OK;
 for(unsigned i=0;i<6;i++)c.pci.bars_before[i]=i%2?0:0x0c;
 c.pci.bridge_before[0]=0x1fff1;c.pci.node_id[0]=0x18611002;
 assert(f.c.release()==&c);tt_resident_session_result end={};assert(NativeSealedFinalEnd(&f.cookie,end)==B_OK);
 if(f.cookie.context || end.state!=TT_RESIDENT_SESSION_CLOSED || !end.host_released || !end.pci_restored || end.cold)
  fprintf(stderr,"DEBUG70_END_FAILURE context%u state%u host%u pci%u cold%u phase%u removed%zu destroyed%u unreserved%u audit%u;CPU_METADATA_ONLY\n",
   unsigned(f.cookie.context!=nullptr),end.state,end.host_released,end.pci_restored,end.cold,
   f.cookie.context?f.cookie.context->q38EndPhase:0,finalRemoved.size(),finalDestroyed,finalUnreserved,finalAuditCalls);
 assert(!f.cookie.context && !sRuntimeOwner && finalDestroyed==1 && finalUnreserved==1 && finalRemoved.size()==33
  && end.state==TT_RESIDENT_SESSION_CLOSED && end.host_released && end.pci_restored && !end.cold && !finalLockHeld);
 puts("PASS DEBUG70 actual explicit END seal/real clock/fresh audit/final VM+PCI+host/context release;NOT original-byte or physical/full-model/TPS qualification");
 io.mixedLiveTransport=nullptr;return 0;
}
