/* SPDX-License-Identifier: MIT */
static bool CheckNativeRuntime(Fixture& f){
 auto& c=*f.c;auto& io=f.io;PioneerQ38TokenRuntimeResult result;auto* state=c.q38TokenNative;
 const unsigned before=io.touches,starts=io.rootStarts,stops=io.rootStops,holds=io.rootHolds,resets=io.resets.size();
 io.runtimeMode=true;
 const auto pristine=result;auto* words=reinterpret_cast<uint32*>(&result);
 for(unsigned i=0;i<16;i++){
  words[i]^=1;assert(!PrepareQ38TokenNativeRuntime(c,result) && io.touches==before && !state->Unsafe() && !io.runtimePackets);result=pristine;
 }
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE
 {
  auto* original=io.BulkSavedWords();uint32 saved[16];memcpy(saved,original,sizeof(saved));
  memcpy(original,&pristine,sizeof(pristine));
  auto& alias=*reinterpret_cast<PioneerQ38TokenRuntimeResult*>(original);
  assert(!PrepareQ38TokenNativeRuntime(c,alias) && io.touches==before && !state->Unsafe() && !io.runtimePackets);
  memcpy(original,saved,sizeof(saved));
 } // separately allocated original buffer is rejected BEFORE runtime allocation/IO
#endif
 tokenChildFail=1;tokenChildAttempts=0;
 assert(!PrepareQ38TokenNativeRuntime(c,result) && tokenChildAttempts==1 && io.touches==before && !state->Unsafe() && !io.runtimePackets);
 tokenChildFail=0;
 const unsigned pciEvents=io.nativePci.events;assert(!state->PciSnapshotCaptured());
 if(io.pciCaptureFault)c.Pci().bars_before[5]^=1U;
 const bool okay=PrepareQ38TokenNativeRuntime(c,result);
 if(io.pciCaptureFault){
  assert(!okay && result.unsafe && !result.staged && !result.started && state->Unsafe() && !state->PciSnapshotCaptured()
   && io.runtimePackets==0 && io.touches==before && io.nativePci.events==pciEvents && io.nativePci.writes==11);
  PioneerQ38TokenRuntimeResult again;assert(!PrepareQ38TokenNativeRuntime(c,again) && io.touches==before && io.nativePci.events==pciEvents);
  f.Unknown();puts("PASS native SAME genuine1010 initializer+120 new preparations+held-image histories/invalid actual PCI BEGIN original refused BEFORE first runtime packet or worker launch/NO following IO/retry/ALL owners retained;NOT successful model or PCI END/physical/Work/TPS qualification");return false;
 }
 if(io.runtimeFault){
  assert(!okay && result.started && result.unsafe && !result.staged && io.runtimeObserved && state->Unsafe()
   && io.runtimePackets==(io.runtimeFault==2?4616U:1U));
  // Generation loss is detected before ANY following MMIO. Counter/data
  // faults are observed by the actual production checked reads.
  if(io.runtimeFault==1)assert(io.touches==io.runtimeObserved);
  unsigned touches=io.touches;PioneerQ38TokenRuntimeResult again;
  assert(!PrepareQ38TokenNativeRuntime(c,again) && io.touches==touches);f.Unknown();
  printf("PASS native held runtime fault%u/phase%u/chunk%u/packets%u/actual generation+counter+last-readback faults/ALL owners retained/no retry/cleanup;SIMULATED endpoints,NOT model RUN/physical/TPS\n",io.runtimeFault,result.phase,result.chunk,result.packets);
  return false;
 }
 if(!okay)fprintf(stderr,"RUNTIME_PREPARE_DIAGNOSTIC phase%u worker%u chunk%u word%u packets%u writes%u reads%u unsafe%u last%s:%x=%x;SIMULATED metadata ONLY\n",
  result.phase,result.worker,result.chunk,result.word,result.packets,result.writes,result.reads,result.unsafe,io.lastOp,io.lastAddress,io.lastValue);
 assert(okay && result.started && result.staged && !result.unsafe && result.packets==4616 && result.writes==2308 && result.reads==2308
  && state->PciSnapshotCaptured() && io.nativePci.events==pciEvents && io.nativePci.writes==11
  && result.bytesWritten==37782528 && result.bytesRead==37782528 && result.boots==120 && !state->Unsafe() && !state->Closed()
  && io.runtimePackets==4616 && io.runtimeWrites==2308 && io.runtimeReads==2308
  && io.rootStarts==starts && io.rootStops==stops && io.rootHolds==holds && io.resets.size()==resets
  && !io.pciWrites && !io.HostReadMock::bmeWrites && !io.HostReadMock::quarantined && !io.wideQuarantined);
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();auto startup=std::make_unique<PioneerQ38TokenStartupPlan>();
 const uint32 prompt[]={248319,17};const PioneerQ38GdnIdentity id={sQ38TokenModelEpoch,1,1,sReloadGeneration};
 assert(canonical->Build() && startup->Build(*canonical,0,id,&c.q38Dispatch->authority.source[0][0],prompt,2,3,false,248318,248317));
 auto plan=std::make_unique<PioneerQ38TokenRuntimePlan>();assert(plan->Bind(*canonical,*startup,id,&c.q38Dispatch->authority.source[0][0]));
 std::vector<uint32> expected(67108864/4,0x13579bdf);std::array<uint8,16384> bytes;
 std::fill(expected.begin(),expected.begin()+512*1024,0); // independent expected initialization,NEVER supplied to device fabric
 for(unsigned n=0;n<PioneerQ38TokenRuntimePlan::Chunks;n++){
  PioneerQ38TokenRuntimePlan::Chunk d;assert(plan->Describe(n,d) && plan->Read(n,bytes.data(),d.bytes));
  for(unsigned i=0;i<d.bytes/4;i++)expected.at((d.address-io.runtimeWorkspace)/4+i)=
   uint32(bytes[i*4])|uint32(bytes[i*4+1])<<8|uint32(bytes[i*4+2])<<16|uint32(bytes[i*4+3])<<24;
 }
 assert(io.runtimeDram==expected); // ALL bytes INCLUDING untouched slack,NOT merely receipts
 for(unsigned w=0;w<120;w++){
  std::array<uint32,64> boot;assert(startup->ReadBoot(w,boot.data()));
  for(unsigned i=0;i<64;i++)assert(io.Raw(w,(w?unsigned(PioneerQ38GlobalWorkerLayout::Boot):unsigned(PioneerQ38GlobalChainLayout::R::Boot))+i*4)==boot[i]);
  assert(io.Core(w).coreReset==0x47800 && (!w || io.Core(w).resets.empty()));
 }
 const unsigned touches=io.touches;PioneerQ38TokenRuntimeResult again;
 assert(!PrepareQ38TokenNativeRuntime(c,again) && io.touches==touches);
 puts("PASS SAME genuine full851 initialization+120 held histories/actual PCI BEGIN originals captured by native factory BEFORE launch/no new PCI access/actual native runtime scope+factory/ALL2308 bounded metadata+frozen rawRoPE+prompt+zero-record chunks/4616 exact NoC packets+complete readback/37782528B vs independently built startup+ALL untouched slack/120 actual boot DATA/NO reset/start/CPU model products/host BME;SIMULATED endpoints,NOT initialized KV rows/model RUN/STOP/END/physical/TPS");
 return true;
}
