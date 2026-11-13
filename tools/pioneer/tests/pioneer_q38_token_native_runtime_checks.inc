/* SPDX-License-Identifier: MIT */
static bool CheckNativeRuntime(Fixture& f){
 auto& c=*f.c;auto& io=f.io;PioneerQ38TokenRuntimeResult result;auto* state=c.q38TokenNative;
 const unsigned before=io.touches,starts=io.rootStarts,stops=io.rootStops,holds=io.rootHolds,resets=io.resets.size();
#ifdef PIONEER_Q38_TOKEN_NATIVE_MATRIX_PHASE_READ_FIXTURE
 namespace MatrixData=PioneerQ38TokenMatrixPhaseData;
 std::vector<uint32_t> slotBefore(119*18);unsigned originalNonzero=0;
 for(unsigned w=1;w<120;w++)for(unsigned i=0;i<18;i++){
  const uint32_t value=io.Raw(w,MatrixData::Address-4+i*4);slotBefore[(w-1)*18+i]=value;
  if(i && i<17)originalNonzero+=value!=0;
 }
 assert(originalNonzero); // arbitrary native backing,not a supplied ready state
#endif
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
   && io.runtimePackets==(io.runtimeFault==2?2U*PioneerQ38SelectedTokenRuntimePlan::Chunks:1U));
  // Generation loss is detected before ANY following MMIO. Counter/data
  // faults are observed by the actual production checked reads.
  if(io.runtimeFault==1)assert(io.touches==io.runtimeObserved);
  unsigned touches=io.touches;PioneerQ38TokenRuntimeResult again;
  assert(!PrepareQ38TokenNativeRuntime(c,again) && io.touches==touches);f.Unknown();
  printf("PASS native held runtime fault%u/phase%u/chunk%u/packets%u/owners retained/no retry; SIMULATED endpoints, NOT cleanup/model RUN/physical/TPS\n",io.runtimeFault,result.phase,result.chunk,result.packets);
  return false;
 }
 if(!okay)fprintf(stderr,"RUNTIME_PREPARE_DIAGNOSTIC phase%u worker%u chunk%u word%u packets%u writes%u reads%u unsafe%u last%s:%x=%x;SIMULATED metadata ONLY\n",
  result.phase,result.worker,result.chunk,result.word,result.packets,result.writes,result.reads,result.unsafe,io.lastOp,io.lastAddress,io.lastValue);
 assert(okay && result.started && result.staged && !result.unsafe && result.packets==2*PioneerQ38SelectedTokenRuntimePlan::Chunks && result.writes==PioneerQ38SelectedTokenRuntimePlan::Chunks && result.reads==PioneerQ38SelectedTokenRuntimePlan::Chunks
  && state->PciSnapshotCaptured() && io.nativePci.events==pciEvents && io.nativePci.writes==11
  && result.bytesWritten==PioneerQ38SelectedTokenRuntimePlan::PayloadBytes && result.bytesRead==PioneerQ38SelectedTokenRuntimePlan::PayloadBytes && result.boots==120 && !state->Unsafe() && !state->Closed()
  && io.runtimePackets==2*PioneerQ38SelectedTokenRuntimePlan::Chunks && io.runtimeWrites==PioneerQ38SelectedTokenRuntimePlan::Chunks && io.runtimeReads==PioneerQ38SelectedTokenRuntimePlan::Chunks
  && io.rootStarts==starts && io.rootStops==stops && io.rootHolds==holds && io.resets.size()==resets
  && !io.pciWrites && !io.HostReadMock::bmeWrites && !io.HostReadMock::quarantined && !io.wideQuarantined);
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();auto startup=std::make_unique<PioneerQ38TokenStartupPlan>();
 const uint32 prompt[]={248319,17};const PioneerQ38GdnIdentity id={sQ38TokenModelEpoch,1,1,sReloadGeneration};
#ifdef PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY
 assert(PioneerQ38SmallRecurrentNativeBinding::DeriveLogical(c,&f.cookie,sReloadGeneration,*canonical));
#elif defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)
 assert(PioneerQ38QgwdNativeBinding::DeriveLogical(c,&f.cookie,sReloadGeneration,*canonical));
#elif defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION)
 assert(c.q38Qgw8Receipt && c.q38Qgw8Receipt->DeriveBoundCanonical(*canonical,&c,&f.cookie,c.hashes,c.initialized,f.cookie.team,sReloadGeneration));
#elif defined(PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY)
 assert(c.q38HybridReceipt && c.q38HybridReceipt->DeriveBoundLayout(*canonical,&c,&f.cookie,c.hashes,c.initialized,f.cookie.team,sReloadGeneration));
#else
 assert(canonical->Build());
#endif
 assert(startup->Build(*canonical,0,id,&c.q38Dispatch->authority.source[0][0],prompt,2,tokenNativePositions,false,248318,248317));
 auto plan=std::make_unique<PioneerQ38SelectedTokenRuntimePlan>();
#ifdef PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY
 assert(plan->Bind(*canonical,*startup,id,&c.q38Dispatch->authority.source[0][0],*c.q38SmallRecurrentReceipt,
  &c,&f.cookie,c.hashes,c.initialized,f.cookie.team,sReloadGeneration));
#else
 assert(plan->Bind(*canonical,*startup,id,&c.q38Dispatch->authority.source[0][0]));
#endif
 std::vector<uint32> expected(67108864/4,0x13579bdf);std::array<uint8,16384> bytes;
 std::fill(expected.begin(),expected.begin()+512*1024,0); // independent expected initialization,NEVER supplied to device fabric
 for(unsigned n=0;n<PioneerQ38SelectedTokenRuntimePlan::Chunks;n++){
  PioneerQ38TokenRuntimePlan::Chunk d;assert(plan->Describe(n,d) && plan->Read(n,bytes.data(),d.bytes));
  for(unsigned i=0;i<d.bytes/4;i++)expected.at((d.address-io.runtimeWorkspace)/4+i)=
   uint32(bytes[i*4])|uint32(bytes[i*4+1])<<8|uint32(bytes[i*4+2])<<16|uint32(bytes[i*4+3])<<24;
 }
 assert(io.runtimeDram==expected); // ALL bytes INCLUDING untouched slack,NOT merely receipts
#ifdef PIONEER_Q38_TOKEN_NATIVE_MATRIX_PHASE_READ_FIXTURE
 for(unsigned w=1;w<120;w++)for(unsigned i=0;i<18;i++){
  const uint32_t expectedWord=i && i<17?0:slotBefore[(w-1)*18+i];
  assert(io.Raw(w,MatrixData::Address-4+i*4)==expectedWord);
 }
 puts("OBSERVED actual native prepare clears ALL119 exact16word timing slots from nonzero arbitrary backing and preserves BOTH neighbors;no core start,NoC packet or invented metric;original full-END restoration still required");
#endif
 for(unsigned w=0;w<120;w++){
  std::array<uint32,64> boot;assert(startup->ReadBoot(w,boot.data()));
  for(unsigned i=0;i<64;i++)assert(io.Raw(w,(w?unsigned(PioneerQ38SelectedGlobalWorker::Boot):unsigned(PioneerQ38GlobalChainLayout::R::Boot))+i*4)==boot[i]);
  assert(io.Core(w).coreReset==0x47800 && (!w || io.Core(w).resets.empty()));
 }
 const unsigned touches=io.touches;PioneerQ38TokenRuntimeResult again;
 assert(!PrepareQ38TokenNativeRuntime(c,again) && io.touches==touches);
 printf("PASS native runtime %u chunks/%u simulated packets/%u bytes complete readback and untouched slack/120 boot records; SIMULATED endpoints, NOT model RUN/physical/TPS\n",unsigned(PioneerQ38SelectedTokenRuntimePlan::Chunks),unsigned(2*PioneerQ38SelectedTokenRuntimePlan::Chunks),unsigned(PioneerQ38SelectedTokenRuntimePlan::PayloadBytes));
 return true;
}
