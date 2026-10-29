/* SPDX-License-Identifier: MIT */
static bool CheckNativeTraffic(Fixture& f){
 auto& c=*f.c;auto& io=f.io;auto& cpu=*io.executionCpu;auto* state=c.q38TokenNative;
 const unsigned hostPackets=io.runtimePackets,rootResets=io.resets.size();
 const uint32 before[]={io.Core(0).counters[0],io.Core(0).counters[1],io.Core(0).accepted,io.Core(0).counters[3]};
 assert(cpu.markerWrites==1 && !cpu.submitted && !cpu.completed && !io.Core(0).pending);
 cpu.traffic=true;
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  for(unsigned round=0;round<16384 && cpu.submitted<4;round++)io.Pause();
  assert(cpu.submitted==4 && cpu.completed==3 && io.Core(0).pending && cpu.markerWrites==1
   && !cpu.matrixInstructions && !cpu.floatOps && !state->Unsafe());
#ifndef PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE
  assert(!cpu.requests && !cpu.responses); // old RuntimeMachine fabric remains unused;full ScalarMachine has none
#endif
 }
 // Observe only bytes ACTUALLY delivered by three genuine ROM NoC reads.
 // These expected views never seed local memory or private ROOT state.
 for(unsigned i=0;i<128;i++){
  assert(io.Raw(0,PioneerQ38TokenLayout::SessionLocal+i*4)==io.runtimeDram.at(PioneerQ38TokenLayout::SessionOffset/4+i));
  assert(io.Raw(0,PioneerQ38GlobalChainLayout::Head+i*4)==io.runtimeDram.at(PioneerQ38GlobalChainLayout::HeadOffset/4+i));
 }
 assert(io.Core(0).counters[0]==before[0]+3 && io.Core(0).counters[1]==before[1]+17
  && io.Core(0).accepted==before[2]+4 && io.Core(0).counters[3]==before[3]+3);
 assert(io.runtimePackets==hostPackets && io.resets.size()==rootResets && !io.pciWrites && !io.HostReadMock::bmeWrites);
 const auto command=std::array<uint32,10>{cpu.request[0],cpu.request[1],cpu.request[2],cpu.request[3],cpu.request[4],
  cpu.request[5],cpu.request[6],cpu.request[7],cpu.request[8],cpu.request[9]};
 PioneerQ38TokenExecutionResult observed;const auto pristine=observed;auto* words=reinterpret_cast<uint32*>(&observed);
 const unsigned outside=io.touches;
 assert(!ObserveQ38TokenNativeRoot(c,observed) && io.touches==outside); // not an ordinary-command or out-of-call permission
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c && c.Unsafe() && !c.Unsafe(false,false,true));
  for(unsigned i=0;i<16;i++){
   words[i]^=1;assert(!ObserveQ38TokenNativeRoot(c,observed) && io.touches==outside && !state->Unsafe());observed=pristine;
  }
  const unsigned steps=cpu.steps;io.trafficObserve=true;
  const bool okay=ObserveQ38TokenNativeRoot(c,observed);
  if(io.trafficFault){
   assert(!okay && observed.started && observed.unsafe && !observed.acknowledged && io.trafficObserved && state->Unsafe()
    && io.Core(0).pending && cpu.steps==steps && cpu.submitted==4 && cpu.completed==3);
   if(io.trafficFault==1)assert(io.touches==io.trafficObserved);
   const unsigned touches=io.touches;PioneerQ38TokenExecutionResult again;PioneerQ38TokenMemoryResult held;
   assert(!ObserveQ38TokenNativeRoot(c,again) && !CancelQ38TokenNativeRoot(c,again) && !CloseQ38TokenNativeHeld(c,held) && io.touches==touches);
   f.Unknown();printf("PASS native ACTIVE NoC observer fault%u/SAME genuine full initialization+119boot+ROOT instruction session/prompt/head reads+first learned packet outstanding/ALLowners retained/no command/counter recapture/reset/retry/cleanup/following IO after uncertainty;SIMULATED endpoints,NOT model arithmetic/STOP/END/physical/TPS\n",io.trafficFault);return false;
  }
  assert(okay && observed.started && observed.phase==8 && observed.acknowledged==1 && !observed.cancelRequested && !observed.unsafe
   && !state->Unsafe() && !state->Closed() && cpu.steps==steps && io.Core(0).pending && cpu.submitted==4 && cpu.completed==3);
 }
 for(unsigned i=0;i<10;i++)assert(io.Core(0).command[i]==command[i]);
 printf("PASS SAME native running ROOT/ACTUAL installed instructions beyond startup/3 genuine NoC reads:session512B+prompt64B+head512B EXACT from native-staged shared GDDR/first learned2304B request OUTSTANDING/fresh native observer accepts legitimate BUSY with source+PC+PCI checks/no host NoC packet,command,counter recapture,products or reset/ROOTsteps%u;PAUSED at raw learned request,NOT embedding/matrix/layer/token/cancel consumption/STOP/END/physical/Work/TPS\n",cpu.steps);
 return true;
}
