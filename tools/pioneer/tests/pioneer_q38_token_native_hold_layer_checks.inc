/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Explicit native first-layer terminal HOLD TEST required
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE
#include "pioneer_q38_token_native_readout_empty_checks.inc"
#endif
static void CheckNativeHoldLayer(Fixture& f){
 auto& c=*f.c;auto& io=f.io;auto& cpu=*io.executionCpu;auto* state=c.q38TokenNative;
 assert(!lockHeld && state->TerminalObserved() && !state->TerminalHeld() && !state->Closed()
  && !state->Unsafe() && cpu.pc==0x60004 && io.terminalHoldOrder.empty());
 std::array<std::array<uint32,18>,120> registers;std::array<unsigned,120> resets;
 for(unsigned w=0;w<120;w++){
  assert(io.Core(w).coreReset==0x46800 && !io.Core(w).pending);resets[w]=io.Core(w).resets.size();
  for(unsigned i=0;i<10;i++)registers[w][i]=io.Core(w).ResidentNocRead(i*4);
  for(unsigned i=0;i<8;i++)registers[w][10+i]=io.Core(w).ResidentNocRead(PioneerQ38PanelCounter(i));
 }
 const unsigned packets=io.runtimePackets,rootPackets=cpu.layerPackets,before=io.touches,aliasWrites=io.aliasWrites;
 PioneerQ38TokenHoldResult held;const auto pristine=held;auto* words=reinterpret_cast<uint32*>(&held);
 assert(!HoldQ38TokenNativeStopped(c,held) && io.touches==before);
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  for(unsigned i=0;i<16;i++){
   words[i]^=1;assert(!HoldQ38TokenNativeStopped(c,held) && io.touches==before);held=pristine;
  }
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTROL_OPERATIONS_FIXTURE
 }
 const bool okay=CheckNativeControlOperation(f,PioneerQ38TokenControl::Hold,true,held);
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
#else
  const bool okay=HoldQ38TokenNativeStopped(c,held);
#endif
  if(!okay)fprintf(stderr,"TERMINAL_HOLD_DIAGNOSTIC phase%u worker%u word%u held%u firstPC%x secondPC%x unsafe%u;NO HOLD supplied\n",
   held.phase,held.worker,held.word,held.held,held.firstPC,held.secondPC,held.unsafe);
  assert(okay && held.started && held.ready && held.held==120 && !held.unsafe && state->TerminalHeld()
   && state->TerminalObserved() && !state->Closed() && !state->Unsafe() && !c.q38Dispatch->rootRunning
   && io.TokenModelSourceScope(false));
  const unsigned touches=io.touches;PioneerQ38TokenHoldResult again;PioneerQ38TokenTerminalResult observed;
  PioneerQ38TokenExecutionResult execution;PioneerQ38TokenMemoryResult old;
  assert(!HoldQ38TokenNativeStopped(c,again) && !ObserveQ38TokenNativeTerminal(c,observed)
   && !CancelQ38TokenNativeRoot(c,execution) && !StartQ38TokenNativeRoot(c,execution)
   && !CloseQ38TokenNativeHeld(c,old) && io.touches==touches); // no legacy restore/restart/duplicate HOLD grant
 }
 assert(io.terminalHoldOrder.size()==120 && io.runtimePackets==packets && cpu.layerPackets==rootPackets && io.aliasWrites==aliasWrites);
 for(unsigned w=0;w<120;w++){
  assert(io.terminalHoldOrder[w]==w && io.Core(w).coreReset==0x47800 && io.Core(w).resets.size()==resets[w]+1
   && io.Core(w).resets.back()==0x47800 && !c.q38Dispatch->worker[w].image.running);
  for(unsigned i=0;i<10;i++)assert(io.Core(w).ResidentNocRead(i*4)==registers[w][i]);
  for(unsigned i=0;i<8;i++)assert(io.Core(w).ResidentNocRead(PioneerQ38PanelCounter(i))==registers[w][10+i]);
 }
 assert(!io.pciWrites && !io.HostReadMock::bmeWrites && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
 puts("EXECUTED native SAME full851/actual original layer0+consumed cancellation+ROOT/all119 actual STOP+fresh native terminal observation/rechecked SAME terminal+closed journals/ROOT FIRST then119 exact HOLD writes/all counters+commands unchanged/fresh FD scope remains owned;NO L1 or CSR restore/token readout/END/disposal/current-state reuse/physical/Work/TPS grant");
#ifdef PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE
 CheckNativeEmptyReadout(f);
#endif
}
