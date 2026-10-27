/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_RECURRENT_BLOCK_CONTROL_FIXTURE_ONLY
#include "pioneer_q38_recurrent_block_native_factory_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentBlockValidationProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
static constexpr int B_NO_MEMORY=-6;
static unsigned copies=0,failCopy=0;static bool copyChild=false,copyForked=false;
static unsigned copyTouches=0,copyDevice=0;static RecurrentBlockNativeTransport* copyTransport=nullptr;
static status_t user_memcpy(void* out,const void* in,size_t bytes){
 ++copies;
 if(finalLockHeld && !copyChild && !copyForked && bytes==sizeof(tt_q38_recurrent_block_validation)
  && static_cast<const tt_q38_recurrent_block_validation*>(in)->metadata.state==TT_RUNTIME_PASSED){
  assert(activeIO && copyTransport);copyTouches=activeIO->touches;copyDevice=copyTransport->Touches();
  copyTransport->PreserveClosedValidationObservation(static_cast<const tt_q38_recurrent_block_validation*>(in)->values,5120);
  assert(activeIO->touches==copyTouches && copyTransport->Touches()==copyDevice);
  pid_t pid=fork();assert(pid>=0);if(!pid){copyChild=true;failCopy=copies;}
  else{int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));copyForked=true;}
 }
 if(copies==failCopy)return B_BAD_VALUE;memcpy(out,in,bytes);return B_OK;
}
static uint64 system_time(){static uint64 time=100;return ++time;}
// Observe ONLY actual post-QPE1/672GDN/336history raw bytes/registers. No
// ready,phase,nonce,STOP,hold or expected numerical product is supplied.
static bool ObserveRawThenHeld(PioneerStoreContext& c,PioneerQ38RecurrentBlockMemoryResult& r){
 auto& io=c.io;auto* transport=io.recurrentTransport;
 assert(transport && c.q38EmbeddingModel->Closed() && c.q38RecurrentBlockNative->InitializationsClosed(c.banks,c.baseline.baseline.baseline.baseline.dram));
 std::vector<uint32> addresses,values;auto seed=[&](uint32 a,uint32 v){addresses.push_back(a);values.push_back(v);};
 seed(0x6000,io.entry);for(unsigned i=0;i<io.prefix.size();i++)seed(0x64000+i*4,io.prefix[i]);
 for(unsigned i=0;i<io.view.size();i++)seed(TT_Q38_PANEL_VIEW_BASE+i*4,io.view[i]);
 for(unsigned i=0;i<2048;i++)seed(0x17e000+i*4,io.destination[i]);
 uint32 source[16],command[10],counts[8];
 for(unsigned i=0;i<16;i++)source[i]=i?io.source[i]:io.config;
 for(unsigned i=0;i<10;i++)command[i]=io.command[i];
 for(unsigned i=0;i<8;i++)counts[i]=io.PanelIO::ResidentNocRead(PioneerQ38PanelCounter(i));
 transport->SeedRoot(addresses.data(),values.data(),unsigned(addresses.size()),source,command,counts);
 return PrepareQ38RecurrentBlockNativeHeld(c,r);
}
static bool ObserveRawThenExecute(PioneerStoreContext& c,PioneerQ38RecurrentBlockResult& r){
 c.io.ConfigureRecurrentBlockTransport();c.io.recurrentTransport->Observe(r);return PrepareQ38RecurrentBlockNativeExecution(c,r);
}
#define PrepareQ38RecurrentBlockNativeHeld ObserveRawThenHeld
#define PrepareQ38RecurrentBlockNativeExecution ObserveRawThenExecute
#define dprintf(...) fprintf(stderr,__VA_ARGS__)
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentBlockValidationControl.h"
#undef dprintf
#undef PrepareQ38RecurrentBlockNativeExecution
#undef PrepareQ38RecurrentBlockNativeHeld
static unsigned otherRoutes=0;static int callerUid=0;
static status_t DriverReloadControl(PioneerProbeCookie*,uint32,void*,size_t){otherRoutes++;return B_NOT_ALLOWED;}
static status_t Q38BankControl(PioneerProbeCookie*,uint32,void*,size_t){otherRoutes++;return B_NOT_ALLOWED;}
static status_t Q38SpanControl(PioneerProbeCookie*,uint32,void*,size_t){otherRoutes++;return B_NOT_ALLOWED;}
#define geteuid() callerUid
#include "native_recurrent_control_routing.inc"
#undef geteuid
static std::unique_ptr<tt_q38_recurrent_block_validation> Request(){
 auto r=std::make_unique<tt_q38_recurrent_block_validation>();r->metadata.version=1;return r;
}
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;(void)&AcquireQ38DeltaLock;
 RecurrentBindingFixture f(argv[1],argv[2]);auto& c=*f.c;auto& io=f.io;
 RecurrentBlockNativeTransport transport(argv[3],argv[4]);transport.EnableValidationReadback();io.recurrentTransport=&transport;copyTransport=&transport;
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned missing=0;
 auto touches=io.touches,physical=transport.Touches();auto epoch=sQ38EmbeddingModelEpoch;
 tt_q38_embedding_support support={};
 assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATION_SUPPORT,&support,sizeof(support))==B_OK
  && support.version==1 && support.driver==55 && support.model_namespace==23 && support.inputs==5120
  && support.vocabulary==248320 && support.runtime_pages==512 && !support.no_cpu_reference && !support.model_inference
  && io.touches==touches && transport.Touches()==physical && sQ38EmbeddingModelEpoch==epoch);
 for(unsigned i=0;i<plan->Count();i++){
  const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer)continue;
  unsigned first=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
  for(unsigned page:{first,first+p->allocated/4096-1}){
   assert(f.learned[page]==1);f.learned[page]=0;auto r=Request();
   assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATE,r.get(),sizeof(*r))!=B_OK);
   assert(!c.q38EmbeddingModel && !c.q38RecurrentBlockNative && io.touches==touches && transport.Touches()==physical
    && sQ38EmbeddingModelEpoch==epoch && !c.dispatchOwnerRetained && !sColdRecoveryRequired && !finalLockHeld && c.q38GdnCallThread==-1);
   f.learned[page]=1;missing++;
  }
 }
 for(unsigned at=0;at<80;at++){
  auto r=Request();reinterpret_cast<uint32_t*>(r.get())[at]^=1;
  assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);
 }
 for(unsigned op=0;op<65536;op++){
  bool allowed=op==TT_PCI_PROBE_RELOAD_STATUS || op==TT_PCI_PROBE_RELOAD_QUIESCE || op==TT_PCI_PROBE_Q38_BANK_SUPPORT
   || op==TT_PCI_PROBE_Q38_BANK_BEGIN || op==TT_PCI_PROBE_Q38_BANK_END || op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT
   || op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD || op==TT_PCI_PROBE_Q38_SPAN_END || op==TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATE
   || op==TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATION_SUPPORT;
  if(!allowed)assert(ProbeControl(&f.cookie,op,nullptr,0)==B_NOT_ALLOWED);
 }
 assert(missing==112 && !otherRoutes && io.touches==touches && transport.Touches()==physical);
 auto reply=Request();auto pristine=Request();failCopy=copies+1;
 assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATE,reply.get(),sizeof(*reply))==B_BAD_VALUE);failCopy=0;
 callerUid=1;assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATE,reply.get(),sizeof(*reply))==B_NOT_ALLOWED);callerUid=0;
 assert(!c.q38EmbeddingModel && io.touches==touches && transport.Touches()==physical);
 puts("PASS DEBUG72 actual public preflight/ALL112 missing learned endpoints/80header mutations/65526closed opcodes/copy+root admission;ZERO model publication,epoch advance or card IO");
 status_t status=ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATE,reply.get(),sizeof(*reply));
 if(copyChild){
  assert(status==B_BAD_VALUE && !memcmp(reply.get(),pristine.get(),sizeof(*reply)) && c.dispatchOwnerRetained && sColdRecoveryRequired
   && c.q38RecurrentBlockNative && c.q38RecurrentBlockNative->Unsafe() && c.q38Clock.Unsafe() && !finalLockHeld && c.q38GdnCallThread==-1
   && io.touches==copyTouches && transport.Touches()==copyDevice);
  auto retry=Request();assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATE,retry.get(),sizeof(*retry))!=B_OK);
  tt_resident_session_result ignored={};assert(NativeSealedFinalEnd(&f.cookie,ignored)!=B_OK || ignored.state!=TT_RESIDENT_SESSION_CLOSED);
  assert(io.touches==copyTouches && transport.Touches()==copyDevice && c.dispatchOwnerRetained);
  puts("PASS DEBUG72 user-copy failure AFTER real instruction/native readback/restore:ALL owners retained/ZERO following card IO,END,reset or retry");fflush(nullptr);_exit(0);
 }
 if(status!=B_OK || !reply->metadata.closed)fprintf(stderr,"DEBUG72_HANDLER_FAILED status%d state%u closed%u recovery%u\n",status,reply->metadata.state,reply->metadata.closed,reply->metadata.recovery_required);
 assert(status==B_OK && copyForked && reply->metadata.closed && reply->metadata.input_ready && !reply->metadata.recovery_required
  && reply->recurrent_regions==672 && reply->history_regions==336 && reply->recurrent_pages==73728 && reply->history_pages==1680
  && reply->cohort_completed==119 && reply->cohort_stopped==119 && reply->cohort_held==120 && reply->root_runs==1 && reply->root_holds==1
  && reply->readback_pages==5 && reply->readback_words==5120 && reply->packets==25 && c.q38RecurrentBlockNative->Closed()
  && !c.dispatchOwnerRetained && !sColdRecoveryRequired && !finalLockHeld && c.q38GdnCallThread==-1);
 // The handler has deleted its result set. The observer must now use its
 // exact closed snapshot,not a stale pointer into that freed allocation.
 transport.VerifyClosed();transport.VerifyValidationReadback(reply->values,5120);
 auto before=transport.Touches();auto clockBefore=c.q38Clock.IO();auto duplicate=Request();
 assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATE,duplicate.get(),sizeof(*duplicate))!=B_OK
  && !c.dispatchOwnerRetained && transport.Touches()==before && c.q38Clock.IO()==clockBefore);
 puts("PASS DEBUG72 actual public handler/real callscope/QPE1+672GDN+336history/wholeQRB/119STOP+ROOT/120holds/5READ/full restore+secondproof/duplicate inert;SYNTHETIC raw learned/OS endpoints,NOT physical/original/Work/TPS");
 io.InitFinalVmViews();c.transaction.map_attempted=c.transaction.config_attempted=true;c.pci.version=1;c.pci.state=TT_PIONEER_MMIO_READ_OK;
 for(unsigned i=0;i<6;i++)c.pci.bars_before[i]=i%2?0:0x0c;c.pci.bridge_before[0]=0x1fff1;c.pci.node_id[0]=0x18611002;
 assert(f.c.release()==&c);tt_resident_session_result end={};assert(NativeSealedFinalEnd(&f.cookie,end)==B_OK);
 assert(!f.cookie.context && !sRuntimeOwner && finalDestroyed==1 && finalUnreserved==1 && finalRemoved.size()==33
  && end.state==TT_RESIDENT_SESSION_CLOSED && end.host_released && end.pci_restored && !end.cold && !finalLockHeld);
 puts("PASS DEBUG72 actual explicit END seal/real clock/fresh audit/final VM+PCI+host/context release;NOT physical/full-model/TPS qualification");
 io.recurrentTransport=nullptr;
}
