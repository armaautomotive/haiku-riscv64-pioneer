/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_VIEWS_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_VIEWS_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE != 1 \
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
#error Explicit native hardware-sealed VM-closure TEST required
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_PCI_FIXTURE
#include "pioneer_q38_token_native_pci_checks.inc"
#endif
static void CheckNativeViewsClose(Fixture& f){
 auto& c=*f.c;auto& io=f.io;auto* s=c.q38TokenNative;auto& clock=c.q38ClockIO;
 assert(!lockHeld && s->HardwareLeasesSealed() && !s->ViewsReleased() && !s->Closed()
  && io.nativeViewDeletes.empty() && io.q38TokenViews.Prepared());
 std::array<area_id,29> handles;for(unsigned i=0;i<29;i++)handles[i]=*io.TokenViewHandle(i);
 const auto hosts=std::array<area_id,3>{io.TokenHostView(0),io.TokenHostView(1),io.TokenHostView(2)};
 const unsigned before=io.touches,raw=clock.rawEvents,packets=io.runtimePackets;
 PioneerQ38TokenViewsResult result;const auto pristine=result;auto* words=reinterpret_cast<uint32*>(&result);
 assert(!CloseQ38TokenNativeViews(c,result) && io.touches==before && io.nativeViewDeletes.empty());
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c && io.TokenFinalOwnershipScope());
  for(unsigned i=0;i<16;i++){
   words[i]^=1;assert(!CloseQ38TokenNativeViews(c,result) && io.touches==before && io.nativeViewDeletes.empty());result=pristine;
  }
  const bool okay=CloseQ38TokenNativeViews(c,result);
  if(io.nativeViewFault){
   assert(!okay && result.started && result.unsafe && !result.ready && s->Unsafe() && !s->ViewsReleased()
    && !s->Closed() && io.nativeViewDeletes.size()==1 && io.touches==io.nativeViewFaultTouches
    && result.deleted==(io.nativeViewFault==1?0U:1U) && !io.TokenViewsScope());
   for(unsigned i=0;i<29;i++)assert(*io.TokenViewHandle(i)==(i==0 && io.nativeViewFault==2?-1:handles[i]));
  }else{
   if(!okay)fprintf(stderr,"VIEWS_CLOSE_DIAGNOSTIC started%u unsafe%u deleted%u syscalls%zu;NO caller clean receipt supplied\n",
    result.started,result.unsafe,result.deleted,io.nativeViewDeletes.size());
   assert(okay && result.started && result.ready && !result.unsafe && result.deleted==29 && io.nativeViewDeletes.size()==29
    && s->ViewsReleased() && !s->Closed() && !s->Unsafe() && io.TokenViewsScope() && !io.TokenFinalOwnershipScope()
    && io.q38TokenViews.Complete() && !io.runtimeMapped && !io.fleetEnabled && !io.runtimeEntry && !io.runtimePc && !io.scratchData);
   for(unsigned i=0;i<29;i++)assert(*io.TokenViewHandle(i)==-1 && io.nativeViewDeletes[i]==handles[i]);
   for(auto view:io.q38TokenMemoryView)assert(!view);
   const unsigned touches=io.touches;
   auto refuse=[&](auto& field,auto bad){const auto saved=field;field=bad;
    assert(!io.TokenViewsScope() && io.touches==touches && io.nativeViewDeletes.size()==29);field=saved;};
   refuse(currentThread,99);refuse(currentTeam,99);refuse(sReloadGeneration,sReloadGeneration+1);
   refuse(io.nativeViewAreas[24],999);refuse(io.outputArea,999);refuse(c.bulkHost.pending,true);
   refuse(c.wideHost.pending,true);refuse(c.lease.ready,true);
   assert(io.TokenViewsScope() && io.touches==touches);
  }
  const unsigned touches=io.touches,calls=io.nativeViewDeletes.size();PioneerQ38TokenViewsResult again;
  assert(!CloseQ38TokenNativeViews(c,again) && io.touches==touches && io.nativeViewDeletes.size()==calls);
 }
 for(unsigned i=0;i<3;i++)assert(io.TokenHostView(i)==hosts[i]);
 assert(clock.rawEvents==raw && clock.deletes==6 && io.runtimePackets==packets && !io.pciWrites && !io.HostReadMock::bmeWrites
  && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
 if(io.nativeViewFault)printf("EXECUTED native VM closure fault%u/actual full predecessor+real clock END+hardware seal/one actual simulated OS deletion or post-delete generation loss/NO following syscall or card IO/retry/all remaining views+PCI+hosts+reservations retained;NO full END/physical/Work/TPS grant\n",io.nativeViewFault);
 else puts("EXECUTED native SAME full predecessor+real clock END+hardware seal/29 actual simulated OS-view deletions/per-step native lifetime+unchanged manifest/all alias pointers fenced/8 altered metadata refusals/no retry/NO card IO or PCI/host/reservation release;VM-only closure,NOT full END/physical/Work/TPS qualification");
#ifdef PIONEER_Q38_TOKEN_NATIVE_PCI_FIXTURE
 if(!io.nativeViewFault)CheckNativePciClose(f);
#endif
}
