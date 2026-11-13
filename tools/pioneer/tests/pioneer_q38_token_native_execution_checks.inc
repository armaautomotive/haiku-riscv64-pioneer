/* SPDX-License-Identifier: MIT */
#if defined(PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCER_FIXTURE) && \
 (PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCER_FIXTURE != 1 || !defined(PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PEER_FIXTURE) \
  || !defined(PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE) || !defined(PIONEER_Q38_SYNTHETIC_EMBEDDING_TEST) \
  || defined(PIONEER_Q38_TOKEN_NATIVE_FIRST_MATRIX_FIXTURE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE))
#error Recurrent producer execution requires the isolated synthetic shared-peer fixture
#endif
#if defined(PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCERS_FIXTURE) && \
 (PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCERS_FIXTURE != 1 || !defined(PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCER_FIXTURE))
#error Recurrent producer cohort requires the isolated recurrent producer fixture
#endif
#if defined(PIONEER_Q38_TOKEN_NATIVE_SYNTHETIC_RECURRENT_MATRIX_FIXTURE) && \
 (PIONEER_Q38_TOKEN_NATIVE_SYNTHETIC_RECURRENT_MATRIX_FIXTURE != 1 || !defined(PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCERS_FIXTURE))
#error Synthetic recurrent matrix address check requires the isolated producer cohort
#endif
#if defined(PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_FIXTURE) && \
 (PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_FIXTURE != 1 || !defined(PIONEER_Q38_TOKEN_NATIVE_SYNTHETIC_RECURRENT_MATRIX_FIXTURE))
#error Recurrent matrix execution requires the isolated synthetic matrix fixture
#endif
#if defined(PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_COHORT_FIXTURE) && \
 (PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_COHORT_FIXTURE != 1 || !defined(PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_FIXTURE))
#error Recurrent matrix cohort requires the isolated matrix fixture
#endif
#if defined(PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_REUSE_FIXTURE) && \
 (PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_REUSE_FIXTURE != 1 || !defined(PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_COHORT_FIXTURE))
#error Recurrent matrix reuse requires the isolated cohort fixture
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE
#include "pioneer_q38_token_native_traffic_checks.inc"
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE
#include "pioneer_q38_token_native_embedding_checks.inc"
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE
#include "pioneer_q38_token_native_layer_checks.inc"
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_FIRST_MATRIX_FIXTURE
#include "pioneer_q38_token_native_first_matrix_checks.inc"
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE
#include "pioneer_q38_token_native_full_flow_checks.inc"
#else
#include "pioneer_q38_token_native_full_layer_checks.inc"
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE
#include "pioneer_q38_token_native_cancel_layer_checks.inc"
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_MANUAL_TTSYNC_FIXTURE
#include "pioneer_q38_token_native_manual_ttsync_checks.inc"
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_MANUAL_WATCHDOG_FIXTURE
#include "pioneer_q38_token_native_manual_watchdog_checks.inc"
#endif
static void CheckNativeExecution(Fixture& f){
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;
 const unsigned before=io.touches,rootResets=io.resets.size(),packets=io.runtimePackets;
 PioneerQ38TokenExecutionResult start,cancel;PioneerQ38TokenTerminalResult terminal;uint32 output=0x12345678;
 assert(!lockHeld && c.q38GdnCallThread==-1 && !state->Unsafe() && !state->Closed()
  && !StartQ38TokenNativeRoot(c,start) && !CancelQ38TokenNativeRoot(c,cancel) && !ObserveQ38TokenNativeTerminal(c,terminal)
  && !io.TokenMemoryReadChecked(0,PioneerQ38TokenLayout::CancelLocal,output) && output==0x12345678 && io.touches==before);
#if defined(PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT) || defined(PIONEER_Q38_QGWF_FRONTIER_NATIVE_INSPECTION)
 PioneerQ38TokenFrontierResult outside;
 assert(!ObserveQ38TokenNativeFrontier(c,outside) && !outside.started && io.touches==before);
#endif
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c && io.TokenModelSourceScope(false));
  assert(c.Unsafe() && !c.Unsafe(false,false,true)); // genuine fresh-call reentry into SAME running119 history
  const auto pristine=start;auto* words=reinterpret_cast<uint32*>(&start);
  for(unsigned i=0;i<16;i++){
   words[i]^=1;assert(!StartQ38TokenNativeRoot(c,start) && io.touches==before && !state->Unsafe());start=pristine;
  }
  assert(!io.executionCpu->steps && !io.executionCpu->markerWrites && !io.executionArms && !io.executionStarts);
  io.executionMode=true;
#ifdef PIONEER_Q38_TOKEN_NATIVE_ROOT_FRONTEND_FIXTURE
  assert(!io.rootFrontendSnapshots && !io.rootFrontendInjectedPCs);
  io.rootFrontendSnapshots=true;
#endif
  const bool okay=StartQ38TokenNativeRoot(c,start);
  if(!okay && !io.executionObserved)fprintf(stderr,"ROOT_START_DIAGNOSTIC phase%u worker%u word%u armed%u released%u ack%u CPUsteps%u/pc%x/markerWrites%u last%s:%x=%x;SIMULATED metadata ONLY\n",
   start.phase,start.worker,start.word,start.armed,start.released,start.acknowledged,io.executionCpu->steps,io.executionCpu->pc,
   io.executionCpu->markerWrites,io.lastOp,io.lastAddress,io.lastValue);
  if(io.executionFault && io.executionFault<=3){
   assert(!okay && start.started && start.unsafe && !start.acknowledged && io.executionObserved && state->Unsafe()
    && io.executionArms==1 && io.executionStarts==unsigned(io.executionFault!=1)
    && start.phase==(io.executionFault==1?3U:4U));
   if(io.executionFault==1)assert(io.touches==io.executionObserved && !io.executionCpu->steps);
   if(io.executionFault==3)assert(io.executionCpu->markerWrites==1 && io.executionCpu->steps);
   const unsigned touches=io.touches;PioneerQ38TokenExecutionResult again;
   assert(!StartQ38TokenNativeRoot(c,again) && !CancelQ38TokenNativeRoot(c,again) && io.touches==touches);
   f.Unknown();printf("PASS native ROOT startup fault%u/phase%u/SAME genuinely initialized+staged+actual119booted owner/ALL resources retained/no retry/reset/cleanup/following IO after observed uncertainty;SIMULATED endpoints,NOT model completion/STOP/END/physical/TPS\n",io.executionFault,start.phase);return;
  }
  assert(okay && start.started && !start.unsafe && start.armed==1 && start.released==1 && start.acknowledged==1
   && !start.cancelRequested && start.phase==5 && !state->Unsafe() && !state->Closed()
   && io.executionArms==1 && io.executionStarts==1 && io.executionCpu->markerWrites==1 && io.executionCpu->steps
   && io.executionCpu->minStack>=0x6e000 && !io.executionCpu->matrixInstructions && !io.executionCpu->floatOps
   && io.Raw(0,PioneerQ38GlobalChainLayout::R::Status+12)==PioneerQ38TokenLayout::Magic && c.q38Dispatch->rootRunning);
#if defined(PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT) || defined(PIONEER_Q38_QGWF_FRONTIER_NATIVE_INSPECTION)
  {
   const unsigned touches=io.touches,steps=io.executionCpu->steps,packets=io.runtimePackets;
   const auto resets=io.resets;PioneerQ38TokenFrontierResult metric;const auto pristineMetric=metric;
#ifdef PIONEER_Q38_TOKEN_NATIVE_FRONTIER_FAILURE_LINK_FIXTURE
   const unsigned linkReads=io.qflSavedLinkReads;
#endif
   auto* words=reinterpret_cast<uint32*>(&metric);
   for(unsigned i=0;i<16;i++){
    words[i]^=1;assert(!ObserveQ38TokenNativeFrontier(c,metric) && io.touches==touches && !state->Unsafe());metric=pristineMetric;
   }
   assert(ObserveQ38TokenNativeFrontier(c,metric) && metric.started && metric.ready && !metric.unsafe
    && !metric.committed && metric.prompts==2 && metric.limit==tokenNativePositions
    && !state->TerminalObserved() && !state->TerminalHeld() && !state->Closed() && !state->Unsafe()
    && io.resets==resets && io.executionCpu->steps==steps && io.runtimePackets==packets);
#ifdef PIONEER_Q38_TOKEN_NATIVE_FRONTIER_FAILURE_DETAIL_FIXTURE
   static_assert(PioneerQ38TokenFrontierFailureDetailSelection==1,"actual private diagnostic,not a flag-only old observer");
   for(auto value:metric.reserved)assert(!value);
#ifdef PIONEER_Q38_TOKEN_NATIVE_FRONTIER_FAILURE_LINK_FIXTURE
   static_assert(PioneerQ38TokenFrontierFailureLinkSelection==1,"actual private QFL1 observer");
   assert(io.qflSavedLinkReads==linkReads+1);
   puts("PASS QFL1 observer in SAME genuine851-source/120-journal/119-virgin-engine native history/fresh0+reserved ZERO/EXACTLY ONE extra checked local savedLINK read/no extra processor step,NoC packet,reset or lifecycle transition;positive ONLY,NOT native unsafe-failure,full-END,physical or TPS proof");
#else
   puts("PASS opt-in failure-detail observer in SAME genuine851-source/120-journal/119-virgin-engine native history/fresh0+all reserved ZERO/no extra processor step,NoC packet,reset or lifecycle transition;positive observation ONLY,NOT injected failure detail,full-END,physical or TPS proof");
#endif
#endif
   puts("PASS new frontier reader in SAME genuinely initialized851+staged120+journaled+virgin119booted+ROOT-entered native history/16 non-pristine outputs refused without IO/fresh0 before any model token/source+boot checked/no processor step,NoC packet,reset or lifecycle transition;SIMULATED OS+VM+NoC,NOT committed token/model/physical/TPS proof");
  }
#endif
  const auto pristineTerminal=terminal;auto* terminalWords=reinterpret_cast<uint32*>(&terminal);
  const unsigned beforeTerminal=io.touches;
  for(unsigned i=0;i<16;i++){
   terminalWords[i]^=1;assert(!ObserveQ38TokenNativeTerminal(c,terminal) && io.touches==beforeTerminal && !state->Unsafe());terminal=pristineTerminal;
  }
  assert(ObserveQ38TokenNativeTerminal(c,terminal) && terminal.started && !terminal.ready && !terminal.unsafe
   && !state->TerminalObserved() && !state->Closed()); // actual ZERO unpublished terminal,not a supplied completion
  puts("PASS native terminal observer pending after genuine ROOT startup marker;16 non-pristine results refused/no STOP or cleanup grant");
#ifdef PIONEER_Q38_TOKEN_NATIVE_ROOT_FRONTEND_FIXTURE
  for(unsigned sample=0;sample<6;sample++){
   PioneerQ38TokenTerminalResult pending;PioneerQ38TokenExecutionResult observation;
   assert(ObserveQ38TokenNativeTerminal(c,pending) && pending.started && !pending.ready && !pending.unsafe
    && ObserveQ38TokenNativeRoot(c,observation) && observation.acknowledged && !observation.unsafe
    && !io.Raw(0,PioneerQ38TokenLayout::TerminalLocal) && !state->TerminalObserved() && io.terminalHoldOrder.empty());
  }
  assert(io.rootFrontendInjectedPCs>=28);
  puts("PASS genuine virgin ROOT startup and unpublished terminal remain non-completion under RAW zero/UINTMAX/fault/terminal/outside frontend samples;actual engine/history unchanged;NO supplied ready or HOLD");
#endif
  {
   const unsigned beforeHold=io.touches;PioneerQ38TokenHoldResult hold;const auto pristineHold=hold;
   auto* holdWords=reinterpret_cast<uint32*>(&hold);
   for(unsigned i=0;i<16;i++){
    holdWords[i]^=1;assert(!HoldQ38TokenNativeStopped(c,hold) && io.touches==beforeHold);hold=pristineHold;
   }
   assert(!HoldQ38TokenNativeStopped(c,hold) && io.touches==beforeHold && !state->TerminalHeld() && !state->Unsafe());
   puts("PASS native terminal HOLD refuses pending ROOT and16 non-pristine results without IO/no supplied STOP or release grant");
   PioneerQ38TokenReadResult read;const auto pristineRead=read;auto* readWords=reinterpret_cast<uint32*>(&read);
   uint32_t output[64]={};
   for(unsigned i=0;i<16;i++){
    readWords[i]^=1;assert(!ReadQ38TokenNativeRecords(c,1,output,read) && io.touches==beforeHold);read=pristineRead;
   }
   for(unsigned count:{0U,1U,64U,65U,0xffffffffU}){
    assert(!ReadQ38TokenNativeRecords(c,count,count?output:nullptr,read) && io.touches==beforeHold);
   }
   for(auto value:output)assert(!value);
   puts("PASS native committed READ refuses pending ROOT,16 non-pristine results and5 counts without IO/output changes;NO token or HOLD capability supplied");
   PioneerQ38TokenRestoreResult restore;const auto pristineRestore=restore;auto* restoreWords=reinterpret_cast<uint32*>(&restore);
   for(unsigned i=0;i<16;i++){
    restoreWords[i]^=1;assert(!RestoreQ38TokenNativeL1(c,restore) && io.touches==beforeHold);restore=pristineRestore;
   }
   assert(!RestoreQ38TokenNativeL1(c,restore) && io.touches==beforeHold && !state->OriginalL1Restored());
   puts("PASS native original-L1 restoration refuses pending ROOT and16 non-pristine results without IO/no caller clean or HOLD grant");
   PioneerQ38TokenDispatchRestoreResult dispatch;const auto pristineDispatch=dispatch;auto* dispatchWords=reinterpret_cast<uint32*>(&dispatch);
   for(unsigned i=0;i<16;i++){
    dispatchWords[i]^=1;assert(!RestoreQ38TokenNativeDispatchImages(c,dispatch) && io.touches==beforeHold);dispatch=pristineDispatch;
   }
   assert(!RestoreQ38TokenNativeDispatchImages(c,dispatch) && io.touches==beforeHold && !state->DispatchImagesRestored());
   puts("PASS native earlier-dispatch L1 restoration refuses pending ROOT and16 non-pristine results without IO/no original or clean-state hint");
   PioneerQ38TokenPeerRestoreResult peer;const auto pristinePeer=peer;auto* peerWords=reinterpret_cast<uint32*>(&peer);
   for(unsigned i=0;i<16;i++){
    peerWords[i]^=1;assert(!RestoreQ38TokenNativePeerStores(c,peer) && io.touches==beforeHold);peer=pristinePeer;
   }
   assert(!RestoreQ38TokenNativePeerStores(c,peer) && io.touches==beforeHold && !state->PeerStoreOriginalsRestored());
   puts("PASS native119-peer local NIU+scratch restore refuses pending ROOT and16 non-pristine results without IO/no caller original/clean capability");
   PioneerQ38TokenRootStoreRestoreResult rootStore;const auto pristineRootStore=rootStore;
   auto* rootStoreWords=reinterpret_cast<uint32*>(&rootStore);
   for(unsigned i=0;i<16;i++){
    rootStoreWords[i]^=1;assert(!RestoreQ38TokenNativeRootStore(c,rootStore) && io.touches==beforeHold);rootStore=pristineRootStore;
   }
   assert(!RestoreQ38TokenNativeRootStore(c,rootStore) && io.touches==beforeHold && !state->RootStoreOriginalsRestored());
   uint32 rootValue=0xa5a55a5a;
   for(uint32 address:{0U,36U,40U,0x40U,0x100U,0x104U,UINT32_MAX})
    assert(!io.TokenRootNiuReadChecked(address,rootValue) && rootValue==0xa5a55a5a
     && !io.TokenRootNiuWrite(address,0) && io.touches==beforeHold);
   for(unsigned word:{0U,2047U,2048U,UINT32_MAX})
    assert(!io.TokenRootScratchReadChecked(word,rootValue) && rootValue==0xa5a55a5a
     && !io.TokenRootScratchWrite(word,0) && io.touches==beforeHold);
   puts("PASS native ROOT-local restore refuses genuine pending ROOT+16 non-pristine results/22 phase-or-range checked NIU+scratch refusals preserve output/NO IO or caller original/clean capability");
   for(unsigned w=0;w<120;w++){
    const uint32_t* original=io.Core(w).Q38PanelSavedWords();assert(original);
    for(unsigned offset:{0U,1U,unsigned(TT_Q38_PANEL_VIEW_WORDS)})
     assert(!Q38TokenExecutionOutput(c,original+offset,sizeof(dispatch)) && io.touches==beforeHold);
   }
   uint32_t aliasValue=0x13579bdf;
   for(unsigned w:{0U,1U,119U,120U})for(uint32_t address:{0x6000U,0x6e000U,0x6e001U,0xac000U,0xb0000U,0xffffffffU}){
    assert(!io.TokenDispatchImageReadChecked(w,address,aliasValue) && aliasValue==0x13579bdf
     && !io.TokenDispatchImageWrite(w,address,0x2468ace0) && io.touches==beforeHold);
   }
   puts("PASS native execution outputs exclude ALL120 earlier saved dispatch buffers/360 alias refusals and48 phase-or-range checked alias refusals preserve output with NO IO;NOT cleanup authority");
   for(unsigned w=0;w<120;w++)for(unsigned offset:{0U,1U,2047U})
    assert(!Q38TokenExecutionOutput(c,c.q38Dispatch->worker[w].Store().saved+offset,sizeof(peer)) && io.touches==beforeHold);
   for(unsigned w:{0U,1U,119U,120U}){
    assert(!io.TokenPeerStoreSelect(w) && io.touches==beforeHold);
    for(uint32_t address:{0U,36U,40U,0x100U,0x104U,0xffffffffU})
     assert(!io.TokenPeerNiuReadChecked(w,address,aliasValue) && aliasValue==0x13579bdf
      && !io.TokenPeerNiuWrite(w,address,0x2468ace0) && io.touches==beforeHold);
    for(unsigned index:{0U,2047U,2048U,0xffffffffU})
     assert(!io.TokenPeerScratchReadChecked(w,index,aliasValue) && aliasValue==0x13579bdf
      && !io.TokenPeerScratchWrite(w,index,0x2468ace0) && io.touches==beforeHold);
   }
   puts("PASS native outputs exclude ALL120 original store buffers/360 aliases+84 peer-store selection/phase/range refusals preserve output with NO IO;NOT a peer closure or shared-TLB grant");
  }
#ifdef PIONEER_Q38_TOKEN_NATIVE_INJECT_FAULT_MARKER
  {
   static_assert(PIONEER_Q38_TOKEN_NATIVE_INJECT_FAULT_MARKER==1,"explicit guard-corruption TEST only");
   namespace R=PioneerQ38GlobalChainLayout::R;namespace T=PioneerQ38TokenLayout;
   auto& cpu=*io.executionCpu;const auto faultResets=io.resets;
   assert(!cpu.faultSavedWrites && !cpu.faultErrorWrites && !io.Raw(0,R::Status+8));
   io.Raw(0,R::Boot)^=1;cpu.traffic=true; // deliberate boot-data corruption,NEVER a PC/status/history setter
   for(unsigned round=0;round<10000 && !cpu.faultErrorWrites;round++)io.Pause();
   assert(cpu.faultSavedWrites==1 && cpu.faultErrorWrites==1 && cpu.markerWrites==1
    && io.Raw(0,R::Status+8)==PioneerQ38TokenPrograms::FaultMarker
    && io.Raw(0,R::Status+32)==cpu.regs[1] && !io.Raw(0,T::TerminalLocal)
    && !cpu.submitted && !io.Core(0).pending && io.resets==faultResets && io.Core(0).coreReset==0x46800);
   const auto steps=cpu.steps;const uint32_t link=io.Raw(0,R::Status+32);
   PioneerQ38TokenTerminalResult bad;
   assert(!ObserveQ38TokenNativeTerminal(c,bad) && bad.started && bad.unsafe && !bad.ready
    && bad.faultError==PioneerQ38TokenPrograms::FaultMarker && bad.faultSavedLink==link
    && state->Unsafe() && !state->TerminalObserved() && !state->Closed() && cpu.steps==steps
    && io.lastAddress==R::Status+32 && io.lastValue==link && io.resets==faultResets);
   const unsigned touches=io.touches;PioneerQ38TokenTerminalResult again;PioneerQ38TokenExecutionResult cancel;
   PioneerQ38TokenHoldResult hold;
   assert(!ObserveQ38TokenNativeTerminal(c,again) && !CancelQ38TokenNativeRoot(c,cancel)
    && !HoldQ38TokenNativeStopped(c,hold) && io.touches==touches);
   f.Unknown();
   printf("PASS actual native virgin ROOT+119 worker histories/deliberate boot-data guard failure/ACTUAL fault trampoline+ordered savedLINK+error stores/native observer exports error%08x savedLINK%08x/owner retained/no terminal,HOLD,retry,following IO or END;SIMULATED endpoints,NOT model correctness/physical/Work/TPS\n",bad.faultError,bad.faultSavedLink);
   return;
  }
#endif
  if(io.terminalFault){
   namespace C=PioneerQ38GlobalChainLayout;namespace T=PioneerQ38TokenLayout;
   if(io.terminalFault==1)io.Raw(0,C::R::Status+8)=1;
   if(io.terminalFault==2)io.Raw(0,T::TerminalLocal)=T::TerminalMagic^1U;
   if(io.terminalFault==3)io.Core(0).source[11]^=1;
   if(io.terminalFault==4)io.Core(0).coreReset^=1;
#ifdef PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE
   if(io.terminalFault==5){
    io.Raw(0,C::R::Boot)^=1;io.executionCpu->traffic=true; // release HOST marker pause,not a PC/history setter
    for(unsigned round=0;round<10000 && io.executionCpu->pc!=0x60000;round++)io.Pause();
    assert(io.executionCpu->pc==0x60000 && io.executionCpu->markerWrites==1
     && !io.executionCpu->submitted && !io.Core(0).pending && !io.Raw(0,T::TerminalLocal)); // actual immutable Scope fault,NOT a supplied PC
   }
#endif
   PioneerQ38TokenTerminalResult bad;
   assert(!ObserveQ38TokenNativeTerminal(c,bad) && bad.started && bad.unsafe && !bad.ready && state->Unsafe());
   if(io.terminalFault==5)assert(io.executionFaultPcObserved && io.touches>io.executionFaultPcObserved
    && bad.word==0 && io.lastAddress==C::R::Boot && io.lastValue==io.Raw(0,C::R::Boot));
   const unsigned touches=io.touches;PioneerQ38TokenTerminalResult again;PioneerQ38TokenExecutionResult cancel;
   assert(!ObserveQ38TokenNativeTerminal(c,again) && !CancelQ38TokenNativeRoot(c,cancel) && io.touches==touches);
   printf("PASS native terminal-observation fault%u/actual started ROOT+119 genuine worker boots/status-error or malformed terminal/source/reset fault/ALL resources retained/no subsequent IO or retry/NO positive STOP/END grant\n",io.terminalFault);
   return;
  }
 }
 // Close the launch call completely. Cancellation must acquire a NEW real
 // native scope around this SAME live ROOT/119-worker lifetime.
 assert(!lockHeld && c.q38GdnCallThread==-1 && !state->Unsafe());
#ifdef PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE
 if(!CheckNativeTraffic(f))return;
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE
 if(!CheckNativeEmbedding(f))return;
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE
 CheckNativeLayerDispatch(f);
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCER_FIXTURE
 {
  namespace W=PioneerQ38SelectedGlobalWorker;namespace R=PioneerQ38GlobalChainLayout::R;
  auto& cpu=*io.executionCpu;auto& peer=*io.bootstrapPeer[0];
  assert(cpu.layerHeld && cpu.dispatchWorker==1 && io.Core(0).pending && peer.Idle() && peer.steps>0);
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
#ifdef PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCERS_FIXTURE
  cpu.recurrentProducersRunning=true;cpu.layerHeld=false;cpu.dispatchWorker=0;
  for(unsigned n=0;n<3000000 && !cpu.layerHeld && cpu.pc!=0x60000 && cpu.pc!=0x60004;n++)io.Pause();
  if(!cpu.layerHeld)fprintf(stderr,"RECURRENT_PRODUCERS_DIAGNOSTIC pc=%x steps=%u doors=%u submitted=%u completed=%u;NO result supplied\n",
   cpu.pc,cpu.steps,cpu.recurrentProducerDoorbells,cpu.submitted,cpu.completed);
  assert(cpu.layerHeld && cpu.recurrentProducerDoorbells==20 && cpu.dispatchWorker && io.Core(0).pending
   && io.Raw(cpu.dispatchWorker,W::Job+8)==1 && !state->Unsafe());
#ifdef PIONEER_Q38_TOKEN_NATIVE_SYNTHETIC_RECURRENT_MATRIX_FIXTURE
  {
   const unsigned worker=cpu.dispatchWorker;assert(worker==1 && cpu.original && cpu.original->Ready());
   std::array<uint32_t,64> job;for(unsigned i=0;i<64;i++)job[i]=io.Raw(worker,W::Job+i*4);
   PioneerQ38Bfp8WorkerJob::Job leaf;for(unsigned i=0;i<16;i++)leaf.words[i]=job[16+i];
   assert(job[0]==W::Magic && job[1]==2 && job[2]==1 && job[3]==worker && !job[5] && !job[6]
    && job[32]==0 && job[33]==64 && job[34]==10 && job[35]==8 && PioneerQ38Bfp8WorkerJob::Shape(leaf)
    && leaf.words[2]==5120 && leaf.words[4]==1 && leaf.words[6]==worker && leaf.words[11]==job[1]);
   std::array<uint8_t,1088> tile;assert(cpu.original->Read(leaf.words[5],leaf.words[3],tile.data(),tile.size()));
   unsigned nonzero=0;for(auto value:tile)nonzero+=value!=0;assert(nonzero);
   assert(cpu.original->Read(leaf.words[5],leaf.words[3]+159*1088,tile.data(),tile.size()));
   printf("PASS staged role-10 matrix job shape and first/last synthetic BFP8 tile address/worker%u/band%u;NO worker execution or model/TPS\n",worker,leaf.words[4]);
  }
#endif
  for(unsigned w=0;w<20;w++){
   const auto s=io.bootstrapPeer[w]->cpu->Stats();assert(io.bootstrapPeer[w]->Idle() && s.sequence==1 && !s.active && !s.pending && !s.matrixInstructions);
  }
  for(unsigned i=0;i<2560;i++){
   uint32_t lanes[2];for(unsigned j=0;j<2;j++){
    const uint32_t value=io.Raw(0,R::Input+(i*2+j)*4);lanes[j]=(value+0x7fff+((value>>16)&1))>>16;
   }
   assert(io.runtimeDram.at((0x20000/4)+i)==(lanes[0]|(lanes[1]<<16)));
  }
  printf("PASS 20 actual recurrent input producers on same booted peers/5120 synthetic BF16 values match independent observer/first matrix doorbell pending/ROOT steps%u;SIMULATED ONLY,NOT matrix/layer/token/physical/TPS\n",cpu.steps);
#ifdef PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_FIXTURE
  assert(cpu.dispatchWorker==1 && peer.Idle() && peer.cpu->Stats().sequence==1);
#ifdef PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_COHORT_FIXTURE
  cpu.recurrentMatrixRunning=true;cpu.layerHeld=false;cpu.dispatchWorker=0;
  for(unsigned n=0;n<6000000 && !cpu.layerHeld && cpu.pc!=0x60000 && cpu.pc!=0x60004;n++)io.Pause();
  if(!cpu.layerHeld){
   fprintf(stderr,"RECURRENT_MATRIX_COHORT_DIAGNOSTIC pc=%x steps=%u doors=%u submitted=%u completed=%u root-status=%x,%x,%x,%x,%x,%x;NO result supplied\n",
    cpu.pc,cpu.steps,cpu.recurrentMatrixDoorbells,cpu.submitted,cpu.completed,
    io.Raw(0,R::Status),io.Raw(0,R::Status+4),io.Raw(0,R::Status+8),io.Raw(0,R::Status+12),io.Raw(0,R::Status+16),io.Raw(0,R::Status+20));
   fprintf(stderr,"ROOT_RECENT_PC");for(unsigned i=0;i<64 && i<cpu.cohortRecentCount;i++)fprintf(stderr," %x",cpu.cohortRecentPC[(cpu.cohortRecentCount+i)%64]);fprintf(stderr,"\n");
   for(unsigned worker=1;worker<=2;worker++){
    auto& p=*io.bootstrapPeer[worker-1];const auto s=p.cpu->Stats();
    fprintf(stderr,"WORKER%u pc=%x sequence=%u active=%u pending=%u matrices=%u idle=%u fault=%u status=%x,%x,%x,%x\n",
     worker,s.pc,s.sequence,s.active,s.pending,s.matrixInstructions,unsigned(p.Idle()),unsigned(p.cpu->ObserveFaulted()),
     io.Raw(worker,W::Status),io.Raw(worker,W::Status+4),io.Raw(worker,W::Status+8),io.Raw(worker,W::Status+12));
   }
   for(unsigned row=0;row<64;row++){
    double expected=0;
    if(row<48)for(unsigned tile=0;tile<160;tile++)for(unsigned col=0;col<32;col++){
     const uint32_t value=io.Raw(0,R::Input+(tile*32+col)*4);
     const uint32_t bits=((value+0x7fff+((value>>16)&1))>>16)<<16;
     expected+=double(Float(bits))*double(int((row*7+tile*3+col*5)%17)-8)*0.0625;
    }
    const uint32_t raw=io.Raw(row/32+1,PioneerQ38Bfp8WorkerJob::Output+(row%32)*4);
    const float actual=Float(raw);
    if(!std::isfinite(actual) || std::fabs(double(actual)-expected)>0.001+std::fabs(expected)*0.0001)
     fprintf(stderr,"MATRIX_ROW_MISMATCH row=%u raw=%08x actual=%g expected=%g\n",row,raw,double(actual),expected);
   }
  }
  assert(cpu.layerHeld && cpu.recurrentMatrixDoorbells==2 && io.Core(0).pending && !state->Unsafe());
  for(unsigned worker=1;worker<=2;worker++){
   auto& matrixPeer=*io.bootstrapPeer[worker-1];const auto matrix=matrixPeer.cpu->Stats();
   assert(matrixPeer.Idle() && matrix.sequence==2 && !matrix.active && !matrix.pending && matrix.matrixInstructions==2560);
  }
  for(unsigned row=0;row<64;row++){
   double expected=0;
   if(row<48)for(unsigned tile=0;tile<160;tile++)for(unsigned col=0;col<32;col++){
    const uint32_t value=io.Raw(0,R::Input+(tile*32+col)*4);
    const uint32_t bits=((value+0x7fff+((value>>16)&1))>>16)<<16;
    expected+=double(Float(bits))*double(int((row*7+tile*3+col*5)%17)-8)*0.0625;
   }
   const float actual=Float(io.Raw(row/32+1,PioneerQ38Bfp8WorkerJob::Output+(row%32)*4));
   assert(std::isfinite(actual) && std::fabs(double(actual)-expected)<=0.001+std::fabs(expected)*0.0001);
  }
  printf("PASS recurrent BFP8 two-band cohort/64 outputs including 16 padded/5120 modeled matrix instructions;SIMULATED ONLY,NOT full layer/token/physical/TPS\n");
#ifdef PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_REUSE_FIXTURE
  fprintf(stderr,"NEXT_MATRIX_DIAGNOSTIC worker=%u operation=%u sequence=%u role=%u invocation=%u\n",cpu.dispatchWorker,
   io.Raw(cpu.dispatchWorker,W::Job+8),io.Raw(cpu.dispatchWorker,W::Job+4),io.Raw(cpu.dispatchWorker,W::Job+34*4),io.Raw(cpu.dispatchWorker,W::Job+7*4));
  assert(cpu.dispatchWorker==1 && io.Raw(1,W::Job+8)==1 && io.Raw(1,W::Job+34*4)==11);
  cpu.recurrentMatrixReuseRunning=true;cpu.layerHeld=false;cpu.dispatchWorker=0;
  for(unsigned n=0;n<6000000 && !cpu.layerHeld && cpu.pc!=0x60000 && cpu.pc!=0x60004;n++)io.Pause();
  if(!cpu.layerHeld)fprintf(stderr,"RECURRENT_MATRIX_REUSE_DIAGNOSTIC pc=%x steps=%u doors=%u submitted=%u completed=%u;NO result supplied\n",
   cpu.pc,cpu.steps,cpu.recurrentMatrixDoorbells,cpu.submitted,cpu.completed);
  assert(cpu.layerHeld && cpu.recurrentMatrixDoorbells==4 && io.Core(0).pending && !state->Unsafe());
  for(unsigned worker=1;worker<=2;worker++){
   auto& p=*io.bootstrapPeer[worker-1];const auto s=p.cpu->Stats();
   assert(p.Idle() && s.sequence==3 && !s.active && !s.pending && s.matrixInstructions==2560);
  }
  for(unsigned row=0;row<64;row++){
   double expected=0;
   if(row<48)for(unsigned tile=0;tile<160;tile++)for(unsigned col=0;col<32;col++){
    const uint32_t value=io.Raw(0,R::Input+(tile*32+col)*4);
    const uint32_t bits=((value+0x7fff+((value>>16)&1))>>16)<<16;
    expected+=double(Float(bits))*double(int((row*7+tile*3+col*5+3)%17)-8)*0.0625;
   }
   const float actual=Float(io.Raw(row/32+1,PioneerQ38Bfp8WorkerJob::Output+(row%32)*4));
   assert(std::isfinite(actual) && std::fabs(double(actual)-expected)<=0.001+std::fabs(expected)*0.0001);
  }
  printf("PASS second recurrent BFP8 matrix/two bands with reused input/64 independent synthetic outputs;SIMULATED ONLY,NOT full layer/token/physical/TPS\n");
#endif
#else
  cpu.layerHeld=false;cpu.RespondLayer();cpu.layerHeld=true; // deliver only the already-issued matrix doorbell
  assert(io.Raw(1,W::Doorbell)==2 && !io.Core(0).pending);
  for(unsigned n=0;n<1000000;n++){
   peer.Advance(256);const auto s=peer.cpu->Stats();
   if(peer.cpu->ObserveFaulted() || (peer.Idle() && s.sequence==2 && !s.active && !s.pending))break;
  }
  const auto matrix=peer.cpu->Stats();
  if(!peer.Idle() || matrix.sequence!=2 || matrix.active || matrix.pending || !matrix.matrixInstructions){
   fprintf(stderr,"RECURRENT_MATRIX_DIAGNOSTIC pc=%x steps=%u sequence=%u active=%u pending=%u matrices=%u faulted=%u status0=%x status1=%x status2=%x status3=%x;NO result supplied\n",
    matrix.pc,matrix.steps,matrix.sequence,matrix.active,matrix.pending,matrix.matrixInstructions,unsigned(peer.cpu->ObserveFaulted()),
    io.Raw(1,W::Status),io.Raw(1,W::Status+4),io.Raw(1,W::Status+8),io.Raw(1,W::Status+12));
   peer.cpu->ReportRecentInstructions();
  }
  assert(peer.Idle() && matrix.sequence==2 && !matrix.active && !matrix.pending && matrix.matrixInstructions>0);
  for(unsigned row=0;row<32;row++){
   double expected=0;
   for(unsigned tile=0;tile<160;tile++)for(unsigned col=0;col<32;col++){
    const uint32_t value=io.Raw(0,R::Input+(tile*32+col)*4);
    const uint32_t bits=((value+0x7fff+((value>>16)&1))>>16)<<16;
    expected+=double(Float(bits))*double(int((row*7+tile*3+col*5)%17)-8)*0.0625;
   }
   const float actual=Float(io.Raw(1,PioneerQ38Bfp8WorkerJob::Output+row*4));
   assert(std::isfinite(actual) && std::fabs(double(actual)-expected)<=0.001+std::fabs(expected)*0.0001);
  }
  printf("PASS first BFP8 matrix band on same simulated booted peer/32 outputs vs post-hoc observer/%u matrix instructions;NO full layer/token/physical/TPS\n",matrix.matrixInstructions);
#endif
#endif
#else
  cpu.layerHeld=false;cpu.RespondLayer();cpu.layerHeld=true; // deliver the already-issued packet; never seed state
  assert(io.Raw(1,W::Doorbell)==1 && !io.Core(0).pending);
  for(unsigned n=0;n<1000000;n++){
   peer.Advance(256);const auto s=peer.cpu->Stats();
   if(peer.Idle() && s.sequence==1 && !s.active && !s.pending)break;
  }
  const auto s=peer.cpu->Stats();assert(peer.Idle() && s.sequence==1 && !s.active && !s.pending && !s.matrixInstructions);
  for(unsigned i=0;i<128;i++){
   uint32_t lanes[2];for(unsigned j=0;j<2;j++){
    const uint32_t value=io.Raw(0,R::Input+(i*2+j)*4);
    lanes[j]=(value+0x7fff+((value>>16)&1))>>16;
   }
   assert(io.runtimeDram.at((0x20000/4)+i)==(lanes[0]|(lanes[1]<<16)));
  }
  printf("PASS actual first recurrent input producer on booted shared peer/256 synthetic values matched independent BF16 observer/%u worker steps;SIMULATED ONLY,NOT matrix/layer/token/physical/TPS\n",s.steps);
#endif
  return;
 }
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_FIRST_MATRIX_FIXTURE
#ifdef PIONEER_Q38_TOKEN_NATIVE_MANUAL_WATCHDOG_FIXTURE
 CheckNativeManualWatchdog(f);
#endif
 CheckNativeFirstMatrix(f);
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE
 CheckNativeFullFlow(f);return; // no cancel is injected;genuine three-token terminal path retains its native resources
#else
 CheckNativeFullLayer(f);
#ifdef PIONEER_Q38_TOKEN_NATIVE_MANUAL_TTSYNC_FIXTURE
 CheckNativeManualOverrideLoss(f);
#endif
#endif
#endif
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c && io.TokenModelSourceScope(false)
   && c.Unsafe() && !c.Unsafe(false,false,true));
  const unsigned beforeCancel=io.touches;const auto pristine=cancel;auto* words=reinterpret_cast<uint32*>(&cancel);
  for(unsigned i=0;i<16;i++){
   words[i]^=1;assert(!CancelQ38TokenNativeRoot(c,cancel) && io.touches==beforeCancel && !state->Unsafe());cancel=pristine;
  }
  io.executionDropCancel=io.executionFault==4;
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTROL_OPERATIONS_FIXTURE
 }
 const bool okay=CheckNativeControlOperation(f,PioneerQ38TokenControl::Cancel,true,cancel);
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
#else
  const bool okay=CancelQ38TokenNativeRoot(c,cancel);
#endif
  if(io.executionFault==4){
   assert(!okay && cancel.started && cancel.unsafe && !cancel.cancelRequested && io.executionCancelWrites==1
    && io.executionCancelObserved && io.touches==io.executionCancelObserved && state->Unsafe());
   const unsigned touches=io.touches;PioneerQ38TokenExecutionResult again;
   assert(!StartQ38TokenNativeRoot(c,again) && !CancelQ38TokenNativeRoot(c,again) && io.touches==touches);
   f.Unknown();puts("PASS native ROOT cancel lost-write fault/SAME actual119 worker boots+real ROOT startup instruction marker/one fixed32bit request attempt/exact readback refuses/ALLowners retained/no repeat write/reset/cleanup/following IO;SIMULATED endpoints,NOT actual cancel consumption/STOP/END/physical/TPS");return;
  }
  assert(okay && cancel.started && !cancel.unsafe && cancel.cancelRequested==1 && cancel.phase==7 && io.executionCancelWrites==1
   && io.Raw(0,PioneerQ38TokenLayout::CancelLocal)==PioneerQ38TokenLayout::CancelMagic && !state->Unsafe() && !state->Closed());
  const unsigned touches=io.touches;PioneerQ38TokenExecutionResult again;PioneerQ38TokenMemoryResult held;
  assert(!StartQ38TokenNativeRoot(c,again) && !CancelQ38TokenNativeRoot(c,again) && !CloseQ38TokenNativeHeld(c,held) && io.touches==touches);
 }
#ifdef PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE
 CheckNativeCancelLayer(f);return;
#endif
 assert(io.resets.size()==rootResets+2 && io.Core(0).coreReset==0x46800 && io.runtimePackets==packets
  && io.rootStarts==1010 && io.rootStops==1010 && io.rootHolds==1010 && !io.pciWrites && !io.HostReadMock::bmeWrites
  && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE
 assert(io.executionCpu->fullLayerComplete && !io.Core(0).pending);
 printf("PASS native fresh-call cancellation REQUEST after actual first-layer return/ROOT instructions%u writtenStack%u;prefix comparison still separate,NOT consumed cancellation/token/STOP/END/physical/Work/TPS\n",
#elif defined(PIONEER_Q38_TOKEN_NATIVE_FIRST_MATRIX_FIXTURE)
 assert(io.Core(0).pending && io.executionCpu->layerHeld && io.executionCpu->firstMatrixDoorbells==3);
 printf("PASS native ORIGINAL first48-output matrix/SAME3 actual worker tensor histories/fresh-call cancel while NEXT matrix doorbell PENDING/ROOT instructions%u writtenStack%u;NOT whole layer/token/cancel consumption/STOP/END/physical/Work/TPS\n",
#elif defined(PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE)
 assert(io.Core(0).pending && io.executionCpu->layerHeld && io.executionCpu->dispatchWorker
  && io.executionCpu->submitted==io.executionCpu->completed+1);
#ifdef PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY
 printf("PASS native SAME ROOT synthetic normalized layer0 input/fresh-call cancel WHILE first producer doorbell transfer is OUTSTANDING/one32bit request+exact readback/ROOT instructions%u writtenStack%u;NOT quantization/worker matrix/layer/token/cancel consumption/STOP/END/physical/Work/TPS\n",
#else
 printf("PASS native SAME genuine ROOT normalized+quantized layer0 input/fresh-call cancel WHILE actual first worker doorbell transfer is OUTSTANDING/one32bit request+exact readback/ROOT instructions%u writtenStack%u;NOT worker matrix/layer/token/cancel consumption/STOP/END/physical/Work/TPS\n",
#endif
#elif defined(PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE)
 assert(io.Core(0).pending && io.executionCpu->submitted==28 && io.executionCpu->completed==27);
#ifdef PIONEER_Q38_SYNTHETIC_EMBEDDING_TEST
 printf("PASS native SYNTHETIC EMBEDDING+simulated GDDR publication/SAME ROOT/119worker histories/fresh-call cancel while next layer0 metadata packet OUTSTANDING/one32bit request+exact readback+duplicate refusal/ROOT instructions%u writtenStack%u;NOT real model/layer/token/cancel consumption/STOP/END/physical/Work/TPS\n",
#else
 printf("PASS native original EMBEDDING+verified GDDR publication/SAME ROOT/119worker histories/fresh-call cancel while next layer0 metadata packet OUTSTANDING/one32bit request+exact readback+duplicate refusal/ROOT instructions%u writtenStack%u;NOT layer/token/cancel consumption/STOP/END/physical/Work/TPS\n",
#endif
#elif defined(PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE)
 assert(io.Core(0).pending && io.executionCpu->submitted==4 && io.executionCpu->completed==3);
 printf("PASS native SAME owner+119actual boots+ROOT actual immutable metadata NoC reads/fresh-call CANCEL WHILE learned NoC request is OUTSTANDING/one32bit monotonic request+exact readback+duplicate refusal/no command/reset/host packet or model product/ROOT instructions%u writtenStack%u;NOT actual cancel consumption/embedding/matrix/layer/token/STOP/END/physical/Work/TPS\n",
#else
 printf("PASS native SAME full851 owner+1010 zero jobs+fullheld journal+verified metadata packets+119 VIRGIN worker instruction histories/fresh-call reentry+current idle audit/actual ROOT ARM+release+%u startup instructions/genuine tokenROM marker/written stack%u/fresh-call one32bit monotonic cancel request/exact readback+duplicate refusal/ordinary+held cleanup excluded;ROOT test PAUSED at real marker store,NOT model arithmetic/cancel consumption/token/STOP/END/physical/Work/TPS proof\n",
#endif
  io.executionCpu->steps,0x70000-io.executionCpu->minStack);
}
