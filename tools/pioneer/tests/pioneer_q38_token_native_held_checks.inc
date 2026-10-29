/* SPDX-License-Identifier: MIT */
static void CheckNativeHeld(Fixture& f,unsigned fault){
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;
 const auto learned=f.learned;const uint32 prompt[]={248319,17};PioneerQ38TokenMemoryResult result;
 const unsigned starts=io.rootStarts,stops=io.rootStops,holds=io.rootHolds,resets=io.resets.size();
 io.journalResult=&result;io.fault=fault;
 assert(starts==1010 && state->InitializationsClosed(c.banks,f.proof));
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  auto deny=[&](const uint32* input,unsigned count,unsigned limit,unsigned eos){
   const unsigned touches=io.touches;auto* owner=c.q38TokenNative;
   assert(!PrepareQ38TokenNativeHeld(c,input,count,limit,false,eos,248317,result) && io.touches==touches
    && c.q38TokenNative==owner && !owner->Unsafe() && !c.q38Dispatch && !io.fleetEnabled && !io.maps);
  };
  deny(nullptr,2,3,248318);deny(prompt,0,3,248318);deny(prompt,2,1,248318);deny(prompt,2,8193,248318);deny(prompt,2,3,248320);
  const uint32 invalid[]={248320,17};deny(invalid,2,3,248318);
  deny(reinterpret_cast<const uint32*>(&c),2,3,248318);deny(reinterpret_cast<const uint32*>(state),2,3,248318);
  deny(reinterpret_cast<const uint32*>(&result),2,3,248318);
  const PioneerQ38TokenMemoryResult pristine=result;
  auto* resultWords=reinterpret_cast<uint32*>(&result);
  for(unsigned word=0;word<sizeof(result)/4;word++){
   resultWords[word]^=1;deny(prompt,2,3,248318);result=pristine;
  }
  for(unsigned failure=1;failure<=3;failure++){
   tokenChildFail=failure;tokenChildAttempts=0;deny(prompt,2,3,248318);
   assert(tokenChildAttempts==3 && state->InitializationsClosed(c.banks,f.proof)
    && !c.dispatchOwnerRetained && !c.banks.Quarantined() && !sColdRecoveryRequired
    && f.learned==learned && !io.q38TokenMemoryMapAttempted);
   tokenChildFail=0;
  }
  auto unknown=[&](){
   assert(fault && io.faultSeen==1 && io.touchesAtFault==io.touches && result.started && result.unsafe
    && !result.installed && !result.closed && result.phase==(fault==1?1U:fault==2?3U:6U)
    && c.q38TokenNative==state && c.q38Dispatch && c.q38Dispatch->prepared==120 && c.q38Dispatch->unsafe
    && !c.q38Dispatch->bootAttempted && !c.q38Dispatch->booted && !c.q38Dispatch->rootRunning
    && io.rootStarts==starts && io.rootStops==stops && io.rootHolds==holds && io.resets.size()==resets
    && f.learned==learned && !io.pciWrites && !io.HostReadMock::bmeWrites && io.maps==4);
   for(unsigned w=0;w<120;w++){
    const auto& node=c.q38Dispatch->worker[w];const auto& store=node.master?*node.master:node.store;
    assert(node.image.unsafe && store.recovery_required && io.Core(w).coreReset==0x47800 && (!w || io.Core(w).resets.empty()));
   }
   const unsigned touches=io.touches;PioneerQ38TokenMemoryResult again;
   assert(!PrepareQ38TokenNativeHeld(c,prompt,2,3,false,248318,248317,again) && !CloseQ38TokenNativeHeld(c,result)
    && io.touches==touches);f.Unknown();
   printf("PASS native held fault%u/actual preceding1010 zero jobs+120 participant ownership/native generation loss in phase%u/ALLowners+aliases+leases retained/no retry/restore/release/following IO;SIMULATED endpoints,NOT physical/RUN/END/TPS\n",fault,result.phase);
  };
  const bool prepared=PrepareQ38TokenNativeHeld(c,prompt,2,3,false,248318,248317,result);
  if(!prepared && !io.faultSeen)fprintf(stderr,"HELD_PREPARE_DIAGNOSTIC resultPhase%u/worker%u/word%u/started%u/saved%u/installedWords%u/unsafe%u/cohort%u/prepared%u/core%u/maps%u/routes%u/retained%u/detail%08x:%08x:%08x:%08x/last%s:%x=%x;SIMULATED fixture metadata ONLY\n",
   result.phase,result.worker,result.word,result.started,result.savedWords,result.installedWords,result.unsafe,unsigned(c.q38Dispatch!=nullptr),
   c.q38Dispatch?c.q38Dispatch->prepared:0,io.runtimeCore,io.maps,unsigned(c.q38DispatchRoutes),unsigned(c.dispatchOwnerRetained),
   c.q38Dispatch?c.q38Dispatch->closeDetail[0]:0,c.q38Dispatch?c.q38Dispatch->closeDetail[1]:0,
   c.q38Dispatch?c.q38Dispatch->closeDetail[2]:0,c.q38Dispatch?c.q38Dispatch->closeDetail[3]:0,io.lastOp,io.lastAddress,io.lastValue);
  if(fault && fault<3){assert(!prepared);unknown();return;}
  assert(prepared);
  assert(c.q38TokenNative==state && !state->Unsafe() && !state->Closed() && state->InitializationsClosed(c.banks,f.proof)
   && c.q38Dispatch && c.q38Dispatch->prepared==120 && !c.q38Dispatch->unsafe && !c.q38Dispatch->bootAttempted
   && !c.q38Dispatch->booted && !c.q38Dispatch->rootRunning && c.q38DispatchRoutes && io.maps==4
   && result.started && result.installed && !result.closed && !result.unsafe
   && result.savedWords==PioneerQ38TokenL1::SavedWords && result.verifiedOriginalWords==PioneerQ38TokenL1::SavedWords
   && result.installedWords==PioneerQ38TokenPrograms::CodeWords(0)+119*PioneerQ38TokenPrograms::CodeWords(1));
  if(fault==3){assert(!CloseQ38TokenNativeHeld(c,result));unknown();return;}
#ifdef PIONEER_Q38_TOKEN_NATIVE_RUNTIME_FIXTURE
  if(!CheckNativeRuntime(f))return;
#endif
  assert(CloseQ38TokenNativeHeld(c,result) && result.closed && !result.unsafe
   && result.restoredWords==PioneerQ38TokenL1::SavedWords && result.verifiedOriginalWords==2*PioneerQ38TokenL1::SavedWords
   && !state->Unsafe() && !state->Closed() && state->InitializationsClosed(c.banks,f.proof));
  const unsigned touches=io.touches;PioneerQ38TokenMemoryResult again;
  assert(!PrepareQ38TokenNativeHeld(c,prompt,2,3,false,248318,248317,again)
   && !CloseQ38TokenNativeHeld(c,result) && io.touches==touches);
 }
 for(unsigned w=0;w<120;w++){
  auto& core=io.Core(w);assert(core.coreReset==0x47800 && (w?core.resets.empty():core.resets.size()==resets));
  for(unsigned i=0;i<PioneerQ38TokenL1::SnapshotWords(w);i++)
   assert(io.Raw(w,PioneerQ38TokenL1::SnapshotAddress(w,i))==io.original.at(PioneerQ38TokenL1::SnapshotBase(w)+i));
 }
 assert(io.rootStarts==starts && io.rootStops==stops && io.rootHolds==holds && f.learned==learned
  && !c.bulk.dirty && !io.pciWrites && !io.HostReadMock::bmeWrites && !io.wideReleased
  && !io.PageMock::releases && !io.OutputBufferMock::releases && io.aliasReads && io.aliasWrites);
 printf("PASS SAME genuinely initialized full851 native owner/actual held factory+all120 route probes+119 NEW store leases+1920 captured sources/private prompt/startup/ACTUAL4aliases+native scope+whole journal/saved%u installed%u restored%u/ALL120 remain HELD/no boot/reset/learned mutation/release;SIMULATED hardware+VM,NOT full-model RUN/STOP/END/physical/TPS\n",result.savedWords,result.installedWords,result.restoredWords);
}
