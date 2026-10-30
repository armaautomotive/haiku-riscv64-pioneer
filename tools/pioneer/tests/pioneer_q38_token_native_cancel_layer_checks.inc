/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Explicit native first-layer cancellation TEST required
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE
#include "pioneer_q38_token_native_hold_layer_checks.inc"
#endif
static void CheckNativeCancelLayer(Fixture& f){
 namespace T=PioneerQ38TokenLayout;namespace C=PioneerQ38GlobalChainLayout;namespace W=PioneerQ38SelectedGlobalWorker;
 auto& c=*f.c;auto& io=f.io;auto& cpu=*io.executionCpu;auto* state=c.q38TokenNative;
 assert(!lockHeld && cpu.fullLayerComplete && !state->Unsafe() && !state->Closed()
  && io.executionCancelWrites==1 && io.Raw(0,T::CancelLocal)==T::CancelMagic);
 const unsigned resets=io.resets.size(),packets=io.runtimePackets;unsigned bucket=cpu.steps/10000000;
 cpu.ResumeFirstLayerCancellation();
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTROL_OPERATIONS_FIXTURE
 PioneerQ38TokenTerminalResult pending;
 assert(CheckNativeControlOperation(f,PioneerQ38TokenControl::Observe,false,pending)
  && pending.started && !pending.ready && !pending.unsafe);
#endif
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
#ifndef PIONEER_Q38_TOKEN_NATIVE_CONTROL_OPERATIONS_FIXTURE
  PioneerQ38TokenTerminalResult pending;
  assert(ObserveQ38TokenNativeTerminal(c,pending) && pending.started && !pending.ready && !pending.unsafe);
#endif
  for(unsigned round=0;round<1000000 && cpu.pc!=0x60000 && cpu.pc!=0x60004;round++){
   io.Pause();if(cpu.steps/10000000!=bucket){bucket=cpu.steps/10000000;
    printf("OBSERVED native first-layer cancellation ROOTinstructions%u pc%x packets%u;NOT terminal/END qualification\n",cpu.steps,cpu.pc,cpu.layerPackets);}
  }
  if(cpu.pc!=0x60004)fprintf(stderr,"CANCEL_LAYER_DIAGNOSTIC ROOTpc%x steps%u packets%u phase%x;NO terminal supplied\n",
   cpu.pc,cpu.steps,cpu.layerPackets,io.Raw(0,C::R::Status+20));
  assert(cpu.pc==0x60004 && !io.Core(0).pending && !cpu.fullLayerComplete && !state->Unsafe() && !state->Closed());
 }
 const uint32 terminal[]={T::TerminalMagic,1,3,0,0,119,1,0,0,1,1,0};
 for(unsigned i=0;i<12;i++)assert(io.Raw(0,T::TerminalLocal+i*4)==terminal[i]);
 const unsigned rootPackets=cpu.layerPackets,instructions=cpu.steps;
 for(unsigned i=0;i<2048;i++){cpu.StepScalar();assert(cpu.pc==0x60004);}
 assert(cpu.steps==instructions+2048 && cpu.layerPackets==rootPackets);
 uint64 peerInstructions=0;
 for(unsigned w=1;w<120;w++){
  auto& p=*io.bootstrapPeer[w-1];const auto before=p.cpu->Stats();assert(p.cpu->ObserveStopped() && !before.active && !before.pending
   && io.Raw(w,W::Job+5*4)==0 && io.Raw(w,W::Job+6*4)==0 && io.Raw(w,W::Job+7*4)==9998);
  p.cpu->AdvanceStopped(2048);p.Observe();const auto after=p.cpu->Stats();
  assert(after.totalSteps==before.totalSteps+2048 && after.requests==before.requests && after.responses==before.responses && p.cpu->ObserveStopped());
  peerInstructions+=after.totalSteps;
 }
 PioneerQ38TokenTerminalResult observed;
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTROL_OPERATIONS_FIXTURE
 const bool okay=CheckNativeControlOperation(f,PioneerQ38TokenControl::Observe,true,observed);
#endif
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
#ifndef PIONEER_Q38_TOKEN_NATIVE_CONTROL_OPERATIONS_FIXTURE
  const bool okay=ObserveQ38TokenNativeTerminal(c,observed);
#endif
  if(!okay)fprintf(stderr,"TERMINAL_OBSERVER_DIAGNOSTIC phase%u worker%u word%u firstPC%x secondPC%x unsafe%u;NO completion supplied\n",
   observed.phase,observed.worker,observed.word,observed.firstPC,observed.secondPC,observed.unsafe);
  assert(okay && observed.ready && observed.cancelled && !observed.unsafe && !observed.committed && !observed.position
   && observed.completedLayers==1 && observed.stage==1 && state->TerminalObserved() && !state->Closed());
  const unsigned touches=io.touches;PioneerQ38TokenTerminalResult again;PioneerQ38TokenExecutionResult cancel;
  PioneerQ38TokenMemoryResult held;
  assert(!ObserveQ38TokenNativeTerminal(c,again) && !CancelQ38TokenNativeRoot(c,cancel)
   && !CloseQ38TokenNativeHeld(c,held) && io.touches==touches); // no duplicate observation/request or legacy cleanup
 }
 assert(io.resets.size()==resets && io.runtimePackets==packets && cpu.layerPackets==rootPackets && io.executionCancelWrites==1
  && !io.pciWrites && !io.HostReadMock::bmeWrites && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
 printf("EXECUTED native SAME full851/actual first-layer arithmetic+single genuine cancel request CONSUMED/ROOT immutable terminal+all119 actual STOP+2048 inert instructions each/fresh native read-only terminal observer/ROOTinstructions%u peerInstructions%llu;NO model token/current-state reuse/HOLD/restore/END/physical/Work/TPS grant\n",
  cpu.steps,(unsigned long long)peerInstructions);
#ifdef PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE
 CheckNativeHoldLayer(f);
#endif
}
