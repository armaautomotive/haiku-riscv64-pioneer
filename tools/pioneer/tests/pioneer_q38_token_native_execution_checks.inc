/* SPDX-License-Identifier: MIT */
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
static void CheckNativeExecution(Fixture& f){
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;
 const unsigned before=io.touches,rootResets=io.resets.size(),packets=io.runtimePackets;
 PioneerQ38TokenExecutionResult start,cancel;PioneerQ38TokenTerminalResult terminal;uint32 output=0x12345678;
 assert(!lockHeld && c.q38GdnCallThread==-1 && !state->Unsafe() && !state->Closed()
  && !StartQ38TokenNativeRoot(c,start) && !CancelQ38TokenNativeRoot(c,cancel) && !ObserveQ38TokenNativeTerminal(c,terminal)
  && !io.TokenMemoryReadChecked(0,PioneerQ38TokenLayout::CancelLocal,output) && output==0x12345678 && io.touches==before);
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c && io.TokenModelSourceScope(false));
  assert(c.Unsafe() && !c.Unsafe(false,false,true)); // genuine fresh-call reentry into SAME running119 history
  const auto pristine=start;auto* words=reinterpret_cast<uint32*>(&start);
  for(unsigned i=0;i<16;i++){
   words[i]^=1;assert(!StartQ38TokenNativeRoot(c,start) && io.touches==before && !state->Unsafe());start=pristine;
  }
  assert(!io.executionCpu->steps && !io.executionCpu->markerWrites && !io.executionArms && !io.executionStarts);
  io.executionMode=true;const bool okay=StartQ38TokenNativeRoot(c,start);
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
  const auto pristineTerminal=terminal;auto* terminalWords=reinterpret_cast<uint32*>(&terminal);
  const unsigned beforeTerminal=io.touches;
  for(unsigned i=0;i<16;i++){
   terminalWords[i]^=1;assert(!ObserveQ38TokenNativeTerminal(c,terminal) && io.touches==beforeTerminal && !state->Unsafe());terminal=pristineTerminal;
  }
  assert(ObserveQ38TokenNativeTerminal(c,terminal) && terminal.started && !terminal.ready && !terminal.unsafe
   && !state->TerminalObserved() && !state->Closed()); // actual ZERO unpublished terminal,not a supplied completion
  puts("PASS native terminal observer pending after genuine ROOT startup marker;16 non-pristine results refused/no STOP or cleanup grant");
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
  }
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
   if(io.terminalFault==5)assert(io.executionFaultPcObserved && io.touches==io.executionFaultPcObserved);
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
#ifdef PIONEER_Q38_TOKEN_NATIVE_FIRST_MATRIX_FIXTURE
 CheckNativeFirstMatrix(f);
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE
 CheckNativeFullFlow(f);return; // no cancel is injected;genuine three-token terminal path retains its native resources
#else
 CheckNativeFullLayer(f);
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
  const bool okay=CancelQ38TokenNativeRoot(c,cancel);
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
 printf("PASS native SAME genuine ROOT normalized+quantized layer0 input/fresh-call cancel WHILE actual first worker doorbell transfer is OUTSTANDING/one32bit request+exact readback/ROOT instructions%u writtenStack%u;NOT worker matrix/layer/token/cancel consumption/STOP/END/physical/Work/TPS\n",
#elif defined(PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE)
 assert(io.Core(0).pending && io.executionCpu->submitted==28 && io.executionCpu->completed==27);
 printf("PASS native original EMBEDDING+verified GDDR publication/SAME ROOT/119worker histories/fresh-call cancel while next layer0 metadata packet OUTSTANDING/one32bit request+exact readback+duplicate refusal/ROOT instructions%u writtenStack%u;NOT layer/token/cancel consumption/STOP/END/physical/Work/TPS\n",
#elif defined(PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE)
 assert(io.Core(0).pending && io.executionCpu->submitted==4 && io.executionCpu->completed==3);
 printf("PASS native SAME owner+119actual boots+ROOT actual immutable metadata NoC reads/fresh-call CANCEL WHILE learned NoC request is OUTSTANDING/one32bit monotonic request+exact readback+duplicate refusal/no command/reset/host packet or model product/ROOT instructions%u writtenStack%u;NOT actual cancel consumption/embedding/matrix/layer/token/STOP/END/physical/Work/TPS\n",
#else
 printf("PASS native SAME full851 owner+1010 zero jobs+fullheld journal+4616 metadata packets+119 VIRGIN QGW1 instruction histories/fresh-call reentry+current idle audit/actual ROOT ARM+release+%u startup instructions/genuine tokenROM marker/written stack%u/fresh-call one32bit monotonic cancel request/exact readback+duplicate refusal/ordinary+held cleanup excluded;ROOT test PAUSED at real marker store,NOT model arithmetic/cancel consumption/token/STOP/END/physical/Work/TPS proof\n",
#endif
  io.executionCpu->steps,0x70000-io.executionCpu->minStack);
}
