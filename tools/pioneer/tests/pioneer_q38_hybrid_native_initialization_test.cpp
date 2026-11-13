/* SPDX-License-Identifier: MIT */
// Real sealed hybrid receipt, native bind and three native initializers.
// Zero-program instructions run against simulated OS/MMIO/DRAM endpoints.
// No actual card, learned weights, model execution or speed measurement.
#define main pioneer_original_initialization_fixture_main
#include "pioneer_q38_token_native_initialization_test.cpp"
#undef main
#ifdef PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY
static_assert(PioneerQ38TokenNativeProfile==13 && PioneerQ38TokenPrograms::Family==0x4d505451
 && PioneerQ38SelectedGlobalWorker::Magic==0x46574751,"recurrent QTPM inspection identity required");
#elif defined(PIONEER_Q38_QGWF_UNROLL_NATIVE_INSPECTION)
static_assert(PioneerQ38TokenNativeProfile==13 && PioneerQ38TokenPrograms::Family==0x4b505451
 && PioneerQ38SelectedGlobalWorker::Magic==0x46574751,"unrolled QGWF frontier inspection identity required");
#elif defined(PIONEER_Q38_QGWF_FRONTIER_NATIVE_INSPECTION)
static_assert(PioneerQ38TokenNativeProfile==13 && PioneerQ38TokenPrograms::Family==0x4a505451
 && PioneerQ38SelectedGlobalWorker::Magic==0x46574751,"QGWF frontier inspection identity required");
#elif defined(PIONEER_Q38_QGWF_NATIVE_INSPECTION)
static_assert(PioneerQ38TokenNativeProfile==13 && PioneerQ38TokenPrograms::Family==0x49505451
 && PioneerQ38SelectedGlobalWorker::Magic==0x46574751,"QGWF inspection identity required");
#elif defined(PIONEER_Q38_QGWE_NATIVE_INSPECTION)
static_assert(PioneerQ38TokenNativeProfile==13 && PioneerQ38TokenPrograms::Family==0x48505451
 && PioneerQ38SelectedGlobalWorker::Magic==0x45574751,"QGWE inspection identity required");
#elif defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)
static_assert(PioneerQ38TokenNativeProfile==13 && PioneerQ38TokenPrograms::Family==0x47505451
 && PioneerQ38SelectedGlobalWorker::Magic==0x44574751,"QGWD inspection identity required");
#elif defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION)
static_assert(PioneerQ38TokenNativeProfile==8 && PioneerQ38TokenPrograms::Family==0x38505451
 && PioneerQ38SelectedGlobalWorker::Magic==0x38574751,
 "native initialization must use the QGW8 inspection image and grammar");
#elif defined(PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION)
static_assert(PioneerQ38TokenNativeProfile==7 && PioneerQ38TokenPrograms::Family==0x37505451
 && PioneerQ38SelectedGlobalWorker::Magic==0x37574751,
 "native initialization must use the QGW7 inspection image and grammar");
#else
static_assert(PioneerQ38TokenNativeProfile==5 && PioneerQ38TokenPrograms::Family==0x35505451
 && PioneerQ38SelectedGlobalWorker::Magic==0x35574751,
 "ordinary hybrid initialization must retain the QGW5 identity");
#endif

int main(int argc,char** argv){
 assert(argc==2 || (argc==3 && (!strcmp(argv[2],"--held") || !strcmp(argv[2],"--runtime") || !strcmp(argv[2],"--bootstrap") || !strcmp(argv[2],"--execution") || !strcmp(argv[2],"--execution-traffic")
#ifdef PIONEER_Q38_QGW8_NATIVE_INSPECTION
  || !strcmp(argv[2],"--faults")
#ifdef PIONEER_Q38_QGW8_TRACE_INIT_INSPECTION
  || !strcmp(argv[2],"--runtime-faults")
#endif
#endif
  ))
  || (argc==4 && (!strcmp(argv[2],"--execution-embedding") || !strcmp(argv[2],"--execution-layer-dispatch"))));
#ifndef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
#ifdef PIONEER_Q38_QGW8_NATIVE_INSPECTION
 assert(argc==2 || (argc==3 && !strcmp(argv[2],"--faults")));
#else
 assert(argc==2);
#endif
#endif
#ifndef PIONEER_Q38_TOKEN_NATIVE_RUNTIME_FIXTURE
 assert(argc==2 || (strcmp(argv[2],"--runtime") && strcmp(argv[2],"--bootstrap") && strcmp(argv[2],"--execution") && strcmp(argv[2],"--execution-traffic") && strcmp(argv[2],"--execution-embedding")));
#endif
#ifndef PIONEER_Q38_TOKEN_NATIVE_BOOTSTRAP_FIXTURE
 assert(argc==2 || (strcmp(argv[2],"--bootstrap") && strcmp(argv[2],"--execution") && strcmp(argv[2],"--execution-traffic") && strcmp(argv[2],"--execution-embedding")));
#endif
#ifndef PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE
 assert(argc==2 || (strcmp(argv[2],"--execution") && strcmp(argv[2],"--execution-traffic") && strcmp(argv[2],"--execution-embedding")));
#endif
#ifndef PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE
 assert(argc==2 || (strcmp(argv[2],"--execution-traffic") && strcmp(argv[2],"--execution-embedding")));
#endif
#ifdef PIONEER_Q38_SYNTHETIC_EMBEDDING_TEST
 assert(argc==4 && (!strcmp(argv[2],"--execution-embedding") || !strcmp(argv[2],"--execution-layer-dispatch")));
 Q38EmbeddingRawTestBridge original;assert(original.Open(argv[3]) && original.Ready());
#else
 assert(argc!=4);
#endif
 std::vector<uint32> hashes(PioneerResidentBanks::Pages);
 Fixture f(argv[1]);auto& c=*f.c;auto& io=f.io;
#ifdef PIONEER_Q38_SYNTHETIC_EMBEDDING_TEST
 io.executionCpu->original=&original;
#endif
 std::fill(f.learned.begin(),f.learned.end(),0);c.hashes=hashes.data();c.initialized=f.learned.data();c.span={};
#ifdef PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY
 c.q38SmallRecurrentReceipt=new PioneerQ38SmallRecurrentUploadReceiptInspection;auto& receipt=*c.q38SmallRecurrentReceipt;
#elif defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)
 c.q38QgwdReceipt=new PioneerQ38DenseQ8UploadReceiptInspection;auto& receipt=*c.q38QgwdReceipt;
#elif defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION)
 c.q38Qgw8Receipt=new PioneerQ38Qgw8UploadReceipt;auto& receipt=*c.q38Qgw8Receipt;
#else
 c.q38HybridReceipt=new PioneerQ38HybridUploadReceipt;auto& receipt=*c.q38HybridReceipt;
#endif
 assert(receipt.Begin(&c,&f.cookie,c.hashes,c.initialized,f.cookie.team,sReloadGeneration,c.banks,f.proof,f.learned.size()));
 unsigned bank=0,length=0,spans=0;uint64 offset=0,total=0;
 while(receipt.Next(bank,offset,length)){
  assert(receipt.Before(&c,c.hashes,c.initialized,&c.span,bank,offset,length));
  const unsigned first=unsigned(uint64(bank)*(PioneerResidentBanks::Capacity/4096)+offset/4096);
  for(unsigned p=0;p<length/4096;p++){
   const unsigned index=first+p;const uint32 hash=index*2654435761U;
   f.learned[index]=1;hashes[index]=hash;assert(receipt.RecordPage(index,hash));
  }
  c.span.saved=c.span.ready=c.span.dirty=true;c.span.nonce=++spans;assert(receipt.After());total+=length;
 }
 c.span.closed=true;c.span.ready=c.span.dirty=false;
#ifdef PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY
 assert(spans==21475 && total==UINT64_C(20754878464) && receipt.Seal(c.span));
#elif defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)
 assert(spans==21907 && total==UINT64_C(21094223872) && receipt.Seal(c.span));
#elif defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION)
 assert(spans==22035 && total==UINT64_C(21167624192) && receipt.Seal(c.span));
#else
 assert(spans==22005 && total==UINT64_C(21137039360) && receipt.Seal(c.span));
#endif
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();
#ifdef PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY
 assert(PioneerQ38SmallRecurrentNativeBinding::DeriveLogical(c,&f.cookie,sReloadGeneration,*plan));
#elif defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)
 assert(PioneerQ38QgwdNativeBinding::DeriveLogical(c,&f.cookie,sReloadGeneration,*plan));
#elif defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION)
 assert(receipt.DeriveBoundCanonical(*plan,&c,&f.cookie,c.hashes,c.initialized,f.cookie.team,sReloadGeneration));
#else
 assert(receipt.DeriveBoundLayout(*plan,&c,&f.cookie,c.hashes,c.initialized,f.cookie.team,sReloadGeneration));
#endif
 unsigned pages=0;
 for(unsigned i=0;i<672;i++){
  io.recurrent[i]=plan->RecurrentRegions()[i];io.recurrent[i].offset+=plan->Base;
  io.recurrentFirst[i]=pages;pages+=io.recurrent[i].heads*16;
 }
 assert(pages==73728);unsigned n=0;
 for(unsigned i=0;i<plan->Count();i++){
  const auto* p=plan->Part(i);if(p->kind!=plan->ConvKind)continue;
  assert(n<336);io.history[n++]={p->bank,plan->Base+p->offset,PioneerQ38ProjectionRoute(p->bank),0,p->allocated/4096,p->allocated};
 }
 assert(n==336);io.workspaceAddress=plan->Base+plan->Workspace(0)->offset;io.workspaceBank=0;
 const auto before=f.learned;assert(f.Bind());
 PioneerQ38StateZeroResult recurrent;PioneerQ38ConvHistoryZeroResult history;PioneerQ38ProjectionZeroResult projection;
#ifdef PIONEER_Q38_QGW8_NATIVE_INSPECTION
 if(argc==3 && !strcmp(argv[2],"--faults")){
  io.failRun=1;assert(!f.Recurrent(recurrent));f.Unknown();assert(f.learned==before);
  puts("PASS QGW8 native initializer failure retains/quarantines simulated owners; NO DEVICE OR INFERENCE");return 0;
 }
#endif
 assert(f.Recurrent(recurrent) && f.History(history) && f.Projection(projection));
 assert(c.q38TokenNative && c.q38TokenNative->InitializationsClosed(c.banks,f.proof)
  && !c.q38TokenNative->Unsafe() && !c.banks.Quarantined() && f.learned==before && !lockHeld);
 const unsigned touches=io.touches;
 assert(!f.Recurrent(recurrent) && !f.History(history) && !f.Projection(projection) && io.touches==touches);
 assert(!c.q38TokenNative->Unsafe() && !c.banks.Quarantined() && f.learned==before);
#ifdef PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY
 puts("PASS recurrent sealed receipt/native bind/three initializers, actual zero RV32, 73728+1680+512 pages; SIMULATED ENDPOINTS ONLY");
#elif defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)
 puts("PASS QGWD sealed receipt/native bind/three initializers, actual zero RV32, 73728+1680+512 pages; SIMULATED ENDPOINTS ONLY");
#elif defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION)
 puts("PASS QGW8 sealed receipt/native bind/three initializers, actual zero RV32, 73728+1680+512 pages; SIMULATED ENDPOINTS ONLY");
#else
 puts("PASS real sealed hybrid receipt/bind/three initializers, actual zero RV32, 73728+1680+512 pages; SIMULATED ENDPOINTS ONLY");
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
#ifdef PIONEER_Q38_TOKEN_NATIVE_RUNTIME_FIXTURE
 io.runtimeWorkspace=io.workspaceAddress;
#ifdef PIONEER_Q38_QGW8_TRACE_INIT_INSPECTION
 if(argc==3 && !strcmp(argv[2],"--runtime-faults"))io.runtimeFault=2;
#endif
#endif
 if(argc>=3)CheckNativeHeld(f,0);
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE
 if(argc>=3 && (!strcmp(argv[2],"--execution") || !strcmp(argv[2],"--execution-traffic") || !strcmp(argv[2],"--execution-embedding") || !strcmp(argv[2],"--execution-layer-dispatch")))CheckNativeExecution(f);
#endif
}
