/* SPDX-License-Identifier: MIT */
// Actual NODRV control over genuine native scopes+instruction producers.
// TEST raw endpoint adapters only;NO normal routing or physical permission.
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_RECURRENT_PREFIX_CONTROL_FIXTURE_ONLY
#include "pioneer_q38_recurrent_prefix_native_factory_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentPrefixValidationProtocol.h"
#include "native_prefix_snapshot_protocol.inc"
#include "native_prefix_snapshot.inc"
static constexpr int B_NO_MEMORY=-6;
static unsigned copies=0,failCopy=0;static bool copyChild=false,copyForked=false;
static unsigned copyTouches=0,copyDevice=0;static RecurrentPrefixNativeTransport* copyTransport=nullptr;
static void* copyCaller=nullptr;
static status_t user_memcpy(void* output,const void* input,size_t bytes){
 ++copies;
 if(copyCaller && output==copyCaller && input!=copyCaller && !copyChild && !copyForked && bytes==sizeof(tt_q38_recurrent_prefix_validation)
  && static_cast<const tt_q38_recurrent_prefix_validation*>(input)->metadata.state==TT_RUNTIME_PASSED){
  assert(!finalLockHeld && !sRuntimeOwner && finalDestroyed==1 && finalUnreserved==1 && !sColdRecoveryRequired && activeIO && copyTransport);
  copyTouches=activeIO->touches;copyDevice=copyTransport->Touches();
  copyTransport->PreserveClosedValidationObservation(static_cast<const tt_q38_recurrent_prefix_validation*>(input)->values,5120);
  assert(activeIO->touches==copyTouches && copyTransport->Touches()==copyDevice);
  pid_t child=fork();assert(child>=0);if(!child){copyChild=true;failCopy=copies;}
  else{int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));copyForked=true;}
 }
 if(copies==failCopy)return B_BAD_VALUE;memcpy(output,input,bytes);return B_OK;
}
static uint64 system_time(){static uint64 time=100;return ++time;}
static bool ObserveRawThenPrefixHeld(PioneerStoreContext& c,PioneerQ38RecurrentPrefixMemoryResult& result){
 auto& io=c.io;auto* transport=io.recurrentTransport;
 assert(transport && c.q38EmbeddingModel->Closed() && c.q38RecurrentPrefixNative->InitializationsClosed(c.banks,c.baseline.baseline.baseline.baseline.dram));
 std::vector<uint32> addresses,values;auto seed=[&](uint32 a,uint32 v){addresses.push_back(a);values.push_back(v);};
 seed(0x6000,io.entry);for(unsigned i=0;i<io.prefix.size();i++)seed(0x64000+i*4,io.prefix[i]);
 for(unsigned i=0;i<io.view.size();i++)seed(TT_Q38_PANEL_VIEW_BASE+i*4,io.view[i]);
 for(unsigned i=0;i<2048;i++)seed(0x17e000+i*4,io.destination[i]);
 uint32 source[16],command[10],counts[8];
 for(unsigned i=0;i<16;i++)source[i]=i?io.source[i]:io.config;
 for(unsigned i=0;i<10;i++)command[i]=io.command[i];
 for(unsigned i=0;i<8;i++)counts[i]=io.PanelIO::ResidentNocRead(PioneerQ38PanelCounter(i));
 transport->SeedRoot(addresses.data(),values.data(),unsigned(addresses.size()),source,command,counts);
 return PrepareQ38RecurrentPrefixNativeHeld(c,result);
} // only actual initialized output BYTES/registers,never a ready/phase setter
static bool ObserveRawThenPrefixExecute(PioneerStoreContext& c,PioneerQ38RecurrentPrefixResult& result){
 c.io.ConfigureRecurrentPrefixTransport();c.io.recurrentTransport->Observe(result);return PrepareQ38RecurrentPrefixNativeExecution(c,result);
}
static bool ObservePrefixReadbackThenClose(PioneerStoreContext& c,PioneerQ38RecurrentPrefixResult& execution,PioneerQ38RecurrentPrefixMemoryResult& memory,
 uint32_t* values,unsigned words,PioneerQ38RecurrentPrefixReadbackResult& readback){
 auto& io=c.io;io.recurrentTransport->VerifyRun();io.recurrentTransport->EnableValidationReadback();
 if(!CloseQ38RecurrentPrefixNativeValidation(c,execution,memory,values,words,readback))return false;
 // Explicit SIMULATED OS/PCI bookkeeping,not a native completed producer,
 // clock/end/permission facade. Actual final source audits still run.
 io.InitFinalVmViews();c.transaction.map_attempted=c.transaction.config_attempted=true;c.pci.version=1;c.pci.state=TT_PIONEER_MMIO_READ_OK;
 for(unsigned i=0;i<6;i++)c.pci.bars_before[i]=i%2?0:0x0c;
 c.pci.bridge_before[0]=0x1fff1;c.pci.node_id[0]=0x18611002;return true;
}
#define PrepareQ38RecurrentPrefixNativeHeld ObserveRawThenPrefixHeld
#define PrepareQ38RecurrentPrefixNativeExecution ObserveRawThenPrefixExecute
#define CloseQ38RecurrentPrefixNativeValidation ObservePrefixReadbackThenClose
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentPrefixValidationControl.h"
#undef CloseQ38RecurrentPrefixNativeValidation
#undef PrepareQ38RecurrentPrefixNativeExecution
#undef PrepareQ38RecurrentPrefixNativeHeld
static std::unique_ptr<tt_q38_recurrent_prefix_validation> Request(){
 auto r=std::make_unique<tt_q38_recurrent_prefix_validation>();r->metadata.version=1;return r;
}
int main(int argc,char** argv){
 assert(argc==6 && (!strcmp(argv[5],"--control-preflight") || !strcmp(argv[5],"--control-execute")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 (void)&AcquireQ38DeltaLock;(void)&NativeSealedFinalEnd;
 RecurrentBindingFixture f(argv[1],argv[2]);auto& c=*f.c;auto& io=f.io;
 RecurrentPrefixNativeTransport transport(argv[3],argv[4]);io.recurrentTransport=&transport;copyTransport=&transport;
 auto touches=io.touches,physical=transport.Touches();auto epoch=sQ38EmbeddingModelEpoch;
 tt_q38_embedding_support support={};
 assert(Q38RecurrentPrefixValidationControl(&f.cookie,TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATION_SUPPORT,&support,sizeof(support))==B_OK
  && support.version==1 && support.driver==55 && support.model_namespace==24 && support.inputs==5120 && support.vocabulary==248320
  && support.runtime_pages==512 && !support.no_cpu_reference && !support.model_inference
  && io.touches==touches && transport.Touches()==physical && sQ38EmbeddingModelEpoch==epoch && !finalLockHeld && c.q38GdnCallThread==-1);
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned missing=0;
 for(unsigned i=0;i<plan->Count();i++){
  const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer>=3)continue;
  unsigned first=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
  for(unsigned page:{first,first+p->allocated/4096-1}){
   assert(f.learned[page]==1);f.learned[page]=0;auto r=Request();
   assert(Q38RecurrentPrefixValidationControl(&f.cookie,TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATE,r.get(),sizeof(*r))!=B_OK);
   assert(!c.q38EmbeddingModel && !c.q38RecurrentPrefixNative && io.touches==touches && transport.Touches()==physical
    && sQ38EmbeddingModelEpoch==epoch && !c.dispatchOwnerRetained && !sColdRecoveryRequired && !finalLockHeld && c.q38GdnCallThread==-1);
   f.learned[page]=1;missing++;
  }
 }
 assert(missing==336);
 for(unsigned at=0;at<96;at++){
  auto r=Request();reinterpret_cast<uint32_t*>(r.get())[at]^=1;
  assert(Q38RecurrentPrefixValidationControl(&f.cookie,TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);
 }
 auto request=Request();failCopy=copies+1;
 assert(Q38RecurrentPrefixValidationControl(&f.cookie,TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATE,request.get(),sizeof(*request))==B_BAD_VALUE);failCopy=0;
 assert(Q38RecurrentPrefixValidationControl(nullptr,TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATE,request.get(),sizeof(*request))==B_BAD_VALUE);
 assert(Q38RecurrentPrefixValidationControl(&f.cookie,TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATE,nullptr,sizeof(*request))==B_BAD_VALUE);
 for(unsigned op:{0U,1000U+0x6402,1000U+0x6403,UINT32_MAX})
  assert(Q38RecurrentPrefixValidationControl(&f.cookie,op,request.get(),sizeof(*request))==B_BAD_VALUE);
 currentTeam++;assert(Q38RecurrentPrefixValidationControl(&f.cookie,TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATE,request.get(),sizeof(*request))==B_NOT_ALLOWED);currentTeam--;
 assert(!c.q38EmbeddingModel && !c.q38RecurrentPrefixNative && !finalLockHeld && c.q38GdnCallThread==-1
  && io.touches==touches && transport.Touches()==physical && sQ38EmbeddingModelEpoch==epoch);
 tt_resident_session_result ignored={};assert(Q38RecurrentPrefixValidationEnd(&f.cookie,nullptr,{1,2,3,4},ignored)==B_NOT_ALLOWED
  && !ignored.state && !c.dispatchOwnerRetained && !finalLockHeld && io.touches==touches && transport.Touches()==physical);
 puts("PASS QPX actual NODRV control preflight/336 ALL3layer learned endpoints/96header mutations/copy+team+null+old-op refusal/locked END context rebinding;ZERO model publication,epoch advance or card IO;NOT public routing/native RUN/physical qualification");
 if(!strcmp(argv[5],"--control-preflight")){
  // The shared full-END fixture deliberately asserts on ordinary C++
  // destruction unless actual END ran. Preflight performed ZERO device IO
  // and created NO native model. Exit this OS simulation without forging
  // a closed/destruction seal;this is not a cleanup qualification.
  io.recurrentTransport=nullptr;fflush(nullptr);_exit(0);
 }
 assert(f.c.release()==&c);auto pristine=Request();copyCaller=request.get();
 status_t status=Q38RecurrentPrefixValidationControl(&f.cookie,TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATE,request.get(),sizeof(*request));
 if(copyChild){
  assert(status==B_BAD_VALUE && !memcmp(request.get(),pristine.get(),sizeof(*request)) && !f.cookie.context && !sRuntimeOwner
   && finalDestroyed==1 && finalUnreserved==1 && !sColdRecoveryRequired && !finalLockHeld
   && io.touches==copyTouches && transport.Touches()==copyDevice);
  auto retry=Request();assert(Q38RecurrentPrefixValidationControl(&f.cookie,TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATE,retry.get(),sizeof(*retry))!=B_OK
   && io.touches==copyTouches && transport.Touches()==copyDevice);
  puts("PASS QPX user-copy failure AFTER actual complete-END/context destruction/caller output unchanged/ZERO following card IO or duplicate END;known-clean card NOT quarantined for CPU copy failure");fflush(nullptr);_exit(0);
 }
 if(status!=B_OK || !request->metadata.closed)fprintf(stderr,"QPX_CONTROL_FAILED status%d state%u closed%u recovery%u context%p\n",
  status,request->metadata.state,request->metadata.closed,request->metadata.recovery_required,static_cast<void*>(f.cookie.context));
 assert(status==B_OK && copyForked && request->metadata.closed && request->metadata.input_ready && !request->metadata.recovery_required
  && request->metadata.source_pages==205785 && request->layers_completed==3 && request->matrices_completed==24 && request->root_stop_request==4
  && request->final_end_state==TT_RESIDENT_SESSION_CLOSED && request->pci_restored==1 && request->host_released==1 && !request->capabilities
  && !f.cookie.context && !sRuntimeOwner && finalDestroyed==1 && finalUnreserved==1 && !sColdRecoveryRequired && !finalLockHeld);
 transport.VerifyClosed();transport.VerifyReadback(request->values,5120);
 auto before=transport.Touches();auto retry=Request();assert(Q38RecurrentPrefixValidationControl(&f.cookie,TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATE,retry.get(),sizeof(*retry))!=B_OK && transport.Touches()==before);
 puts("PASS ACTUAL QPX NODRV control/SAME private native QPE1+672GDN+336history/3RUN+realSTOP+120holds+5READ/fullrestore/release actual compute seal/locked final-END+fresh audit+clock+30VM+PCI+3hosts+context destruction BEFORE user copy;SIMULATED raw learned/OS,NOT original-model,physical,Work or TPS qualification");
 io.recurrentTransport=nullptr;
}
