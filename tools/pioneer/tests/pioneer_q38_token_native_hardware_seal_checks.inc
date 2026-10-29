/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE)
#error Explicit native clock-ended hardware-lease retirement TEST required
#endif
static void CheckNativeHardwareSeal(Fixture& f){
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;auto& clock=c.q38ClockIO;
 assert(!lockHeld && state->ClockRestored() && !state->HardwareLeasesSealed() && !state->Closed()
  && c.lease.ready && c.bulk.ready && !c.q38Dispatch->closed && !io.hardwareSealReads);
 const unsigned raw=clock.rawEvents,before=io.touches,packets=io.runtimePackets;
 std::array<unsigned,120> resets;for(unsigned w=0;w<120;w++)resets[w]=io.Core(w).resets.size();
 const auto maps=io.maps;PioneerQ38TokenHardwareSealResult result;const auto pristine=result;auto* words=reinterpret_cast<uint32*>(&result);
 io.nativeHardwareSealResult=&result;
 assert(!SealQ38TokenNativeHardware(c,result) && io.touches==before && clock.rawEvents==raw && !io.TokenFinalOwnershipScope());
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  for(unsigned i=0;i<16;i++){
   words[i]^=1;assert(!SealQ38TokenNativeHardware(c,result) && io.touches==before && clock.rawEvents==raw);result=pristine;
  }
  const bool okay=SealQ38TokenNativeHardware(c,result);
  if(io.hardwareSealFault){
   assert(!okay && result.started && result.unsafe && !result.ready && io.hardwareSealReads==1 && io.hardwareSealFaultSeen==1
    && io.touches==io.hardwareSealFaultTouches && state->Unsafe() && !state->HardwareLeasesSealed()
    && c.lease.ready && c.bulk.ready && !c.q38Dispatch->closed && !io.TokenFinalOwnershipScope());
  }else{
   if(!okay)fprintf(stderr,"HARDWARE_SEAL_DIAGNOSTIC phase%u word%u unsafe%u reads%u masterReady%u bulkReady%u cohortClosed%u;NO caller seal supplied\n",
    result.phase,result.word,result.unsafe,io.hardwareSealReads,unsigned(c.lease.ready),unsigned(c.bulk.ready),unsigned(c.q38Dispatch->closed));
   assert(okay && result.ready && !result.unsafe && result.images==120 && result.stores==120 && result.journals==1 && result.tlbWords==6
    && io.hardwareSealReads==6 && state->HardwareLeasesSealed() && !state->ClockRestored() && !state->Closed() && !state->Unsafe()
    && !c.lease.ready && !c.bulk.ready && c.q38Dispatch->closed && !c.q38Dispatch->unsafe && !io.TokenModelSourceScope(false)
    && !io.TokenMemoryAccessScope() && io.TokenFinalOwnershipScope());
   const unsigned touches=io.touches;
   auto refuse=[&](auto& field,auto bad){const auto saved=field;field=bad;
    assert(!io.TokenFinalOwnershipScope() && io.touches==touches && clock.rawEvents==raw);field=saved;};
   refuse(currentThread,99);refuse(currentTeam,99);refuse(sReloadGeneration,sReloadGeneration+1);
   refuse(c.lease.ready,true);refuse(c.bulk.ready,true);refuse(c.q38Dispatch->closed,false);
   refuse(c.q38Dispatch->worker[1].store.ready,true);refuse(c.q38Dispatch->worker[119].image.ready,true);
   refuse(c.q38Dispatch->worker[0].master,static_cast<PioneerResidentLease*>(nullptr));
   refuse(c.q38FfnModelIdentity.session,UINT64_C(1));refuse(c.q38GdnIdentity.driverGeneration,UINT64_C(1));
   refuse(c.bulkHost.pending,true);refuse(c.wideHost.pending,true);refuse(io.q38TokenMemoryView[0],static_cast<volatile uint32*>(nullptr));
   assert(io.TokenFinalOwnershipScope() && io.touches==touches && clock.rawEvents==raw);
  }
  const unsigned touches=io.touches;PioneerQ38TokenHardwareSealResult again;PioneerQ38TokenClockResult old;
  assert(!SealQ38TokenNativeHardware(c,again) && !EndQ38TokenNativeClock(c,old) && io.touches==touches && clock.rawEvents==raw);
 }
 io.nativeHardwareSealResult=nullptr;
 for(unsigned w=0;w<120;w++)assert(io.Core(w).coreReset==0x47800 && io.Core(w).resets.size()==resets[w]);
 assert(io.maps==maps && io.maps==4 && io.runtimePackets==packets && clock.deletes==6 && clock.rawEvents==raw
  && !io.pciWrites && !io.HostReadMock::bmeWrites && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
 if(io.hardwareSealFault)puts("EXECUTED native hardware-seal first control-readback fault/actual SAME full predecessor+real clock END/first bad actual observation/NO following IO or publication/ALL remaining owners retained;NO full END/physical/Work/TPS grant");
 else puts("EXECUTED native SAME full851/original layer+cancel/STOP/HOLD/drain/token+dispatch+119peer+ROOT+shared originals+real clock END/6 fresh original-control reads/CPU-only120 image+120 store+launched-journal retirement/master+bulk ready cleared/cohort closed/14 corrupted-final-metadata refusals ZERO IO/launched history retained;modelVM/PCI/hosts/reservations STILL OWNED,NO full END/physical/Work/TPS grant");
}
