/* SPDX-License-Identifier: MIT */
// Close request journals without releasing the model reservation. Every
// failed stage quarantines the owner, even if that stage refused before IO:
// earlier stages may already have changed the hardware or host mappings.
static __attribute__((noinline)) bool
CloseQ38QgwdResidentRequest(PioneerQ38TokenCallScope& scope,PioneerQ38QgwdResident::Reply& out)
{
 auto* c=scope.Context();auto* s=c?c->q38TokenNative:NULL;
 if(!c || !s || c->qgwdResidentRequestClosed || !s->TerminalHeld())return false;
 out.started=1;
#define PIONEER_QGWD_RESIDENT_CLOSE(number,Type,operation) \
 {Type result;out.stage=number;if(!operation(*c,result)){ \
  tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained); \
  out.unsafe=1;out.ready=0;return false;}}
 PIONEER_QGWD_RESIDENT_CLOSE(1,PioneerQ38TokenRestoreResult,RestoreQ38TokenNativeL1)
 PIONEER_QGWD_RESIDENT_CLOSE(2,PioneerQ38TokenDispatchRestoreResult,RestoreQ38TokenNativeDispatchImages)
 PIONEER_QGWD_RESIDENT_CLOSE(3,PioneerQ38TokenPeerRestoreResult,RestoreQ38TokenNativePeerStores)
 PIONEER_QGWD_RESIDENT_CLOSE(4,PioneerQ38TokenRootStoreRestoreResult,RestoreQ38TokenNativeRootStore)
 PIONEER_QGWD_RESIDENT_CLOSE(5,PioneerQ38TokenSharedTlbRestoreResult,RestoreQ38TokenNativeSharedMappings)
 PIONEER_QGWD_RESIDENT_CLOSE(6,PioneerQ38TokenClockResult,EndQ38TokenNativeClock)
 PIONEER_QGWD_RESIDENT_CLOSE(7,PioneerQ38TokenHardwareSealResult,SealQ38TokenNativeHardware)
 PIONEER_QGWD_RESIDENT_CLOSE(8,PioneerQ38TokenViewsResult,CloseQ38TokenNativeViews)
 PIONEER_QGWD_RESIDENT_CLOSE(9,PioneerQ38TokenPciResult,CloseQ38TokenNativePci)
 PIONEER_QGWD_RESIDENT_CLOSE(10,PioneerQ38TokenHostsResult,CloseQ38TokenNativeHosts)
#undef PIONEER_QGWD_RESIDENT_CLOSE
 if(!s->HostBuffersReleased() || s->Closed() || !c->io.TokenHostReleaseScope()){
  tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);out.unsafe=1;return false;
 }
 c->qgwdResidentRequestClosed=true;
 out.hostsReleased=3;out.reservationRetained=1;out.ready=1;return true;
}
