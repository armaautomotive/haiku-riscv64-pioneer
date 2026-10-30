/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_READOUT_FLOW_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_READOUT_FLOW_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || defined(PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Explicit native complete-flow committed readout TEST required
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_END_FLOW_FIXTURE
#include "pioneer_q38_token_native_full_end_checks.inc"
#endif
static void CheckNativeReadoutFlow(Fixture& f){
 namespace T=PioneerQ38TokenLayout;
 auto& c=*f.c;auto& io=f.io;auto& cpu=*io.executionCpu;auto* state=c.q38TokenNative;
 assert(!lockHeld && state->TerminalObserved() && !state->TerminalHeld() && !state->Closed()
  && !state->Unsafe() && cpu.pc==0x60004 && cpu.flowLayers==192 && io.terminalHoldOrder.empty());
 std::array<unsigned,120> resets;
 for(unsigned w=0;w<120;w++){assert(io.Core(w).coreReset==0x46800);resets[w]=io.Core(w).resets.size();}
 const unsigned packets=io.runtimePackets,rootPackets=cpu.layerPackets;
 const uint64_t rootInstructions=cpu.ObservedRootInstructions();
 std::array<uint32_t,3*64> output={};
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);PioneerQ38TokenHoldResult held;
  assert(HoldQ38TokenNativeStopped(c,held) && held.ready && held.held==120 && state->TerminalHeld());
  assert(io.runtimePackets==packets && io.terminalHoldOrder.size()==120);
  for(unsigned w=0;w<120;w++)assert(io.terminalHoldOrder[w]==w && io.Core(w).coreReset==0x47800
   && io.Core(w).resets.size()==resets[w]+1 && !c.q38Dispatch->worker[w].image.running);
 }
 for(unsigned first:{0U,2U}){
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);const unsigned count=first?1:2;
  PioneerQ38TokenReadResult result;const bool okay=ReadQ38TokenNativeRecords(c,count,output.data()+first*64,result);
  if(!okay)fprintf(stderr,"FULL_READOUT_DIAGNOSTIC phase%u worker%u word%u first%u records%u packets%u bytes%u unsafe%u;NO record supplied\n",
   result.phase,result.worker,result.word,result.firstRecord,result.recordCount,result.packets,result.bytesRead,result.unsafe);
  assert(okay && result.ready && result.drained==unsigned(first==2) && result.firstRecord==first && result.recordCount==count
   && result.packets==1 && result.bytesRead==count*256 && !result.unsafe && state->TerminalHeld() && !state->Closed());
  if(first){const unsigned touches=io.touches;PioneerQ38TokenReadResult again;
   assert(!ReadQ38TokenNativeRecords(c,1,output.data(),again) && io.touches==touches);}
 } // two genuine fresh native calls;cached greedy input must survive call boundaries
 for(unsigned i=0;i<output.size();i++)assert(output[i]==io.runtimeDram.at(T::OutputOffset/4+i)); // OBSERVER only,never supplied as an input
 assert(io.runtimePackets==packets+2 && cpu.layerPackets==rootPackets && cpu.ObservedRootInstructions()==rootInstructions
  && !io.pciWrites && !io.HostReadMock::bmeWrites && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
 puts("EXECUTED native SAME full851/3tokens*all64 actual model/GDDR committed records/ROOT+119 actual STOP/fresh terminal+120 actual HOLD/native512B+256B NoC reads in2 fresh calls/ALL192 delivered record words EXACT actual immutable GDDR/prefill+cross-call greedy input/duplicate refused;independent full numeric comparison STILL REQUIRED;NO current-state reuse/restore/END/physical/Work/TPS grant");
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_END_FLOW_FIXTURE
 CheckNativeFullEndFlow(f); // actual cleanup may delete context;never dereference c/state after return
#endif
}
