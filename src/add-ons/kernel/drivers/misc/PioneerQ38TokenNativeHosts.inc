/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
__attribute__((noinline)) bool PioneerSessionIO::TokenHostReleaseScope(){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 if(!s || !s->hostsCloseAttempted || !s->pciCloseAttempted || !s->pciClosed || !s->pci.Complete()
  || !s->viewsCloseAttempted || !s->viewsClosed || !q38TokenViews.DeviceClosureMatches(*this) || !TokenFinalCoreScope()
  || runtimeMapped || fleetEnabled || runtimeEntry || runtimePc || scratchData
  || !s->hosts.Matches(*this,c->host,c->bulkHost,c->wideHost,q38TokenViews))return false;
 if(s->phase==PioneerQ38TokenNativeState::HostsClosing)
  return !s->hostsClosed && !s->reservationReleaseAttempted && !s->reservationReleased && (s->hosts.Prepared() || s->hosts.Closing());
 if(s->phase==PioneerQ38TokenNativeState::HostsClosed)
  return s->HostBuffersReleased() && !s->reservationReleaseAttempted && !s->reservationReleased;
 if(s->phase==PioneerQ38TokenNativeState::ReservationReleasing)
  return s->hostsClosed && s->hosts.Complete() && s->reservationReleaseAttempted && !s->reservationReleased;
 return s->phase==PioneerQ38TokenNativeState::ClosedState && s->Closed();
} // metadata ONLY;every released prefix is checked against the prelaunch journal,never adopted from handles alone
__attribute__((noinline)) int32_t PioneerSessionIO::TokenHostReleaseChecked(unsigned kind){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 if(!s || s->phase!=PioneerQ38TokenNativeState::HostsClosing || !s->hosts.PermitsRelease(kind) || !TokenHostReleaseScope())return B_NOT_ALLOWED;
 return kind==2?ReleaseWideOutput():kind==1?ReleaseOutputBuffer():ReleaseHostPage();
} // actual OS single-attempt endpoints;NO legacy controllerKnownSafe or synthetic release receipt
__attribute__((noinline)) bool PioneerSessionIO::TokenReservationScope(){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 return s && s->phase==PioneerQ38TokenNativeState::ReservationReleasing && s->hostsClosed && s->hosts.Complete()
  && s->hosts.ReleasedCount()==3 && s->reservationReleaseAttempted && !s->reservationReleased && TokenHostReleaseScope();
}
__attribute__((noinline)) status_t PioneerSessionIO::TokenUnreserveDeviceChecked(){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 if(!s || s->reservationEndpointAttempted || !TokenReservationScope())return B_NOT_ALLOWED;
 s->reservationEndpointAttempted=true;return TokenUnreserveDeviceRaw();
} // device-manager status is authoritative;no retry or later IO on uncertainty
[[maybe_unused]] static __attribute__((noinline)) bool CloseQ38TokenNativeHosts(PioneerStoreContext& c,PioneerQ38TokenHostsResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenHostsResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::PciClosed || !s->PciRestored() || s->hostsCloseAttempted || s->hostsClosed
  || s->reservationReleaseAttempted || s->reservationEndpointAttempted || s->reservationReleased || !s->hosts.Prepared()
  || !s->hosts.Matches(c.io,c.host,c.bulkHost,c.wideHost,c.io.q38TokenViews)
  || !Q38TokenExecutionOutput(c,&result,sizeof(result)) || !c.io.TokenPciScope())return false;
 s->hostsCloseAttempted=true;s->phase=PioneerQ38TokenNativeState::HostsClosing;result.started=1;
 bool okay=s->hosts.Close(c.io);result.released=s->hosts.ReleasedCount();
 if(okay && s->hosts.Complete()){
  s->hostsClosed=true;s->phase=PioneerQ38TokenNativeState::HostsClosed;okay=c.io.TokenHostReleaseScope();
 }
 if(!okay){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);result.unsafe=1;result.ready=0;return false;}
 result.ready=1;return true;
} // wide->bulk->input ONLY AFTER genuine PCI+VM+hardware closure;context/model reservations still owned
#endif
