/* SPDX-License-Identifier: MIT */
static bool CheckNativeEmbedding(Fixture& f){
 namespace F=PioneerQ38GlobalFeedbackLayout;
 auto& c=*f.c;auto& io=f.io;auto& cpu=*io.executionCpu;auto* state=c.q38TokenNative;
 assert(!lockHeld && !state->Unsafe() && cpu.original && cpu.original->Ready());
 const unsigned hostPackets=io.runtimePackets,rootResets=io.resets.size();
 cpu.ResumeEmbedding();cpu.corruptBack=io.embeddingFault==2;
 if(io.embeddingFault==1)cpu.original->Revoke(); // one-way RAW source fault;NO synthetic fallback
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  for(unsigned round=0;round<120000 && cpu.submitted<28 && cpu.pc!=0x60000 && cpu.pc!=0x60004;round++)io.Pause();
  if(io.embeddingFault){
   assert(cpu.pc==0x60000 && !state->Unsafe()); // ACTUAL firmware timeout/readback refusal,not a seeded PC/status
   if(io.embeddingFault==1)assert(cpu.sourceLost && cpu.sourceRefusals==1 && !cpu.rawTiles && cpu.submitted==4 && cpu.completed==3 && io.Core(0).pending);
   else assert(!cpu.sourceLost && cpu.rawTiles==20 && cpu.embeddingWrites==2 && cpu.embeddingBacks==2 && cpu.submitted==27 && cpu.completed==27);
   const unsigned resets=io.resets.size(),pcReads=io.executionPcReads;PioneerQ38TokenExecutionResult observed;
   assert(!ObserveQ38TokenNativeRoot(c,observed) && observed.started && observed.unsafe && !observed.acknowledged
    && state->Unsafe() && !state->Closed() && io.resets.size()==resets && io.runtimePackets==hostPackets
    && io.executionPcReads==pcReads+1 && io.executionFaultPcObserved==io.touches);
   const unsigned touches=io.touches;PioneerQ38TokenExecutionResult again;PioneerQ38TokenMemoryResult held;
   assert(!ObserveQ38TokenNativeRoot(c,again) && !CancelQ38TokenNativeRoot(c,again) && !CloseQ38TokenNativeHeld(c,held) && io.touches==touches);
   f.Unknown();printf("PASS native original embedding fault%u/ACTUAL ROOT instruction timeout or readback mismatch parks at60000/real observer quarantines ALLowners/no learned retry/fallback/reset/cleanup/following IO after uncertainty/ROOT instructions%u;SIMULATED endpoints,NOT layer/token/STOP/END/physical/TPS\n",io.embeddingFault,cpu.steps);return false;
  }
  if(cpu.submitted!=28)fprintf(stderr,"EMBEDDING_DIAGNOSTIC PC%x steps%u submitted%u completed%u tiles%u writes%u backs%u sourceLost%u;NO completion supplied\n",
   cpu.pc,cpu.steps,cpu.submitted,cpu.completed,cpu.rawTiles,cpu.embeddingWrites,cpu.embeddingBacks,unsigned(cpu.sourceLost));
  assert(cpu.submitted==28 && cpu.completed==27 && cpu.pc!=0x60000 && cpu.pc!=0x60004 && cpu.rawTiles==20
   && cpu.embeddingWrites==2 && cpu.embeddingBacks==2 && !cpu.sourceLost && cpu.floatOps && io.Core(0).pending
   && !cpu.matrixInstructions && !state->Unsafe() && !state->Closed());
  PioneerQ38TokenExecutionResult observed;assert(ObserveQ38TokenNativeRoot(c,observed) && observed.acknowledged && !observed.unsafe);
 }
 // Independent CPU decoder is an observer ONLY,AFTER genuine ROOT decode,
 // two GDDR writes and two readback comparisons reach the next real packet.
 // It NEVER supplies an embedding,activation,completion or ROOT state.
 unsigned values=0;
 for(unsigned block=0;block<20;block++){
  uint8_t packed[144];float reference[256];
  assert(cpu.original->OriginalBlock(cpu.geometry[0],block,packed,144) && PioneerDecodeQ4K(packed,reference));
  for(unsigned i=0;i<256;i++){
   const unsigned at=block*256+i;const uint32 wanted=Bits(reference[i]);
   assert(io.Raw(0,F::Local+at*4)==wanted && io.Raw(0,F::Shadow+at*4)==wanted
    && io.runtimeDram.at(F::Offset/4+at)==wanted);values++;
  }
 }
 assert(values==5120 && cpu.original->Ready() && io.runtimePackets==hostPackets && io.resets.size()==rootResets
  && !io.pciWrites && !io.HostReadMock::bmeWrites && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
 printf("PASS SAME full851 native owner+119 genuine worker histories/ROOT ACTUAL original27B Q4_K embedding/20 authenticated RAW tiles+2 real GDDR writes+2 verified readbacks/ALL5120 LOCAL+SHADOW+GDDR values bit-exact vs separate original-row decoder OBSERVER ONLY/%u ROOT instructions/%u scalar FP instructions/max writtenStack%u/next layer0 metadata request outstanding/no CPU product upload or private-history transplant;SIMULATED endpoints,NOT model layer/token/cancel consumption/STOP/END/physical/Work/TPS\n",
  cpu.steps,cpu.floatOps,0x70000-cpu.minStack);
 return true;
}
