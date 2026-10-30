/* SPDX-License-Identifier: MIT */
static void CheckNativePreparationFault(Fixture& f,unsigned fault){
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;
 assert(fault>=1 && fault<=8 && state->InitializationsClosed(c.banks,f.proof) && !c.q38Dispatch && !io.maps);
 const auto learned=f.learned;const auto starts=io.rootStarts,stops=io.rootStops,holds=io.rootHolds;
 const unsigned resets=io.resets.size();const uint32 prompt[]={248319,17};PioneerQ38TokenMemoryResult result;
 io.preparationFault=fault;
 for(unsigned worker:{0U,1U,119U,120U,UINT32_MAX}){
  const unsigned before=io.touches;
  assert(!io.PrepareTokenNativeParticipant(worker) && io.touches==before);
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  assert(!io.PrepareTokenNativeParticipant(worker) && io.touches==before);
 }
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  assert(!PrepareQ38TokenNativeHeld(c,prompt,2,3,false,248318,248317,result));
 }
 assert(io.NativePreparationAuditInactive()); // RAII clears EVERY failed participant path
 assert(io.preparationFaultSeen==1 && io.touches==io.preparationFaultTouches && c.q38Dispatch && c.q38Dispatch->unsafe
  && c.q38Dispatch->prepared==((fault>=2 && fault<=4) || fault==6?2U:1U)
  && !c.q38Dispatch->bootAttempted && !c.q38Dispatch->booted && !c.q38Dispatch->rootRunning
  && !io.maps && !io.q38TokenMemoryMapAttempted && !result.started && result.unsafe && !result.installed && !result.closed
  && io.rootStarts==starts && starts==1010 && io.rootStops==stops && io.rootHolds==holds && io.resets.size()==resets
  && f.learned==learned && !io.pciWrites && !io.HostReadMock::bmeWrites && !io.wideReleased
  && !io.PageMock::releases && !io.OutputBufferMock::releases);
 if(fault==5)assert(io.preparationMapWrites==1 && !io.preparationPanelWrites);
 if(fault==6)assert(io.Core(1).config==0x16005 && c.q38Dispatch->worker[1].store.configured && !c.q38Dispatch->worker[1].store.ready);
 if(fault==7 || fault==8)assert(io.preparationPanelWrites==1);
 if(fault<=4){
  const auto& events=io.preparationFifoEvents;assert(!events.empty());
  assert(events.back()==(fault==1?0x64U:fault==4?0x168U:0x164U));
  if(fault==4)assert(events.size()>=2 && events[events.size()-2]==0x164);
 }
 for(unsigned w=0;w<120;w++)assert(io.Core(w).coreReset==0x47800 && (!w || io.Core(w).resets.empty()));
 f.Unknown();
 printf("PASS native preparation fault%u/actual preceding1010 zero jobs+CLOSED initializers/first bad FIFO or ACTUAL mapping/NIU/panel write+readback-or-generation loss/no following IO/counter read/retry/reset/cleanup/release;all owners retained,NO held-stage/compute/END/hardware/Work/TPS grant\n",fault);
}
