/* SPDX-License-Identifier: MIT */
static void CheckNativeBootstrap(Fixture& f,PioneerQ38TokenMemoryResult& held){
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;
 assert(io.rootStarts==1010 && io.rootStops==1010 && io.rootHolds==1010);
 const unsigned rootResets=io.resets.size(),packets=io.runtimePackets;
 for(const auto& p:io.bootstrapPeer)assert(!p->steps && p->pc==0x6000 && !p->deviceWrites);
 PioneerQ38TokenBootstrapResult result;const auto pristine=result;auto* words=reinterpret_cast<uint32*>(&result);
 const unsigned before=io.touches;
 for(unsigned i=0;i<16;i++){
  words[i]^=1;assert(!BootstrapQ38TokenNativeWorkers(c,result) && io.touches==before && !state->Unsafe());result=pristine;
 }
 // Full shared command exclusion remains active even with all cores held.
 assert(c.Unsafe() && !c.Unsafe(false,false,true));
 io.bootstrapMode=true;const bool okay=BootstrapQ38TokenNativeWorkers(c,result);
 if(!okay && !io.bootstrapObserved)fprintf(stderr,"BOOTSTRAP_DIAGNOSTIC phase%u worker%u word%u armed%u released%u workers%u polls%u PCs%x/%x unsafe%u last%s:%x=%x;SIMULATED endpoints ONLY\n",
  result.phase,result.worker,result.word,result.armed,result.released,result.workers,result.polls,result.firstPC,result.secondPC,result.unsafe,
  io.lastOp,io.lastAddress,io.lastValue);
 if(io.bootstrapFault){
  assert(!okay && result.started && result.unsafe && !result.ready && io.bootstrapObserved && state->Unsafe());
  if(io.bootstrapFault==1)assert(io.touches==io.bootstrapObserved && result.phase==3 && io.bootstrapArms==1 && !io.bootstrapStarts);
  else assert(result.phase==4 && io.bootstrapArms==119 && io.bootstrapStarts==119);
  const unsigned touches=io.touches;PioneerQ38TokenBootstrapResult again;
  assert(!BootstrapQ38TokenNativeWorkers(c,again) && !CloseQ38TokenNativeHeld(c,held) && io.touches==touches);
  f.Unknown();printf("PASS native QGW%u bootstrap fault%u/phase%u/actual preceding1010 zero jobs+120 held stages+4616 metadata packets/ALL owners retained/no reARM/restore/retry/cleanup/following IO after observed uncertainty;SIMULATED endpoints,NOT ROOT/model RUN/physical/TPS\n",PioneerQ38SelectedWorkerPrecise+PioneerQ38SelectedWorkerPushed+1,io.bootstrapFault,result.phase);
  return;
 }
 assert(okay && result.started && !result.unsafe && result.ready && result.phase==7 && result.armed==119 && result.released==119
  && result.workers==119 && io.bootstrapArms==119 && io.bootstrapStarts==119 && !state->Unsafe() && !state->Closed());
 assert(c.Unsafe() && !c.Unsafe(false,false,true)); // ordinary callers still cannot borrow running peers
 uint64 steps=0;unsigned stack=0x70000;
 for(unsigned w=1;w<120;w++){
  auto& cpu=*io.bootstrapPeer[w-1];auto& core=io.Core(w);
  assert(cpu.steps && cpu.Idle() && cpu.statusWrites==17 && !cpu.matrixInstructions && !cpu.requests && !cpu.responses
   && !cpu.pending && !cpu.moveInstructions && !cpu.floatOps && cpu.deviceWrites && cpu.minStack>=0x6e000
   && core.coreReset==0x46800 && core.resets==std::vector<uint32>({0x87ffff,0x46800}));
  for(unsigned i=0;i<16;i++)assert(io.Raw(w,PioneerQ38SelectedGlobalWorker::Status+i*4)==(i==3?unsigned(PioneerQ38SelectedGlobalWorker::Magic):0));
  steps+=cpu.steps;stack=std::min(stack,cpu.minStack);
 }
 assert(io.Core(0).coreReset==0x47800 && !c.q38Dispatch->rootRunning && !c.q38Dispatch->bootAttempted && !c.q38Dispatch->booted
  && io.rootStarts==1010 && io.rootStops==1010 && io.rootHolds==1010 && io.resets.size()==rootResets && io.runtimePackets==packets
  && !io.pciWrites && !io.HostReadMock::bmeWrites && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
 const unsigned touches=io.touches;PioneerQ38TokenBootstrapResult again;
 assert(!BootstrapQ38TokenNativeWorkers(c,again) && !CloseQ38TokenNativeHeld(c,held) && io.touches==touches);
 printf("PASS SAME native full851 owner+1010 zero jobs+installed120 L1 histories+4616 metadata packets/ALL119 VIRGIN QGW%u instruction boots DIRECT shared L1+CSR backing/%llu actual instructions/maxWrittenStack%u/ALL ARM before ANY release/final status+two WAIT PCs+unchanged NoC counters/ROOT HELD/legacy flags FALSE/held cleanup and ordinary commands REFUSED;SIMULATED OS+VM+NoC,NOT ROOT/model RUN/STOP/END/physical/Work/TPS\n",
  PioneerQ38SelectedWorkerPrecise+PioneerQ38SelectedWorkerPushed+1,(unsigned long long)steps,0x70000-stack);
}
