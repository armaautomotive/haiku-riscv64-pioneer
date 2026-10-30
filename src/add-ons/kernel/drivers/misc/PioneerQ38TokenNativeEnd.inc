/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
__attribute__((noinline)) bool PioneerQ38TokenCallScope::Finish(PioneerQ38TokenEndResult& result){
 const PioneerQ38TokenEndResult pristine;const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 auto* c=Context();auto* s=c?c->q38TokenNative:NULL;
 if(!c || !s || destroyed || !cookie || cookie->context!=c || sRuntimeOwner!=cookie || c->q38GdnCallThread!=thread || find_thread(NULL)!=thread
  || !s->HostBuffersReleased() || s->reservationReleaseAttempted || s->reservationEndpointAttempted || s->reservationReleased
  || !Q38TokenExecutionOutput(*c,&result,sizeof(result)) || !c->io.TokenHostReleaseScope())return false;
 result.started=1;result.hosts=3;s->reservationReleaseAttempted=true;s->phase=PioneerQ38TokenNativeState::ReservationReleasing;
 const status_t released=c->io.TokenUnreserveDeviceChecked();result.unreserve_status=released;
 if(released!=B_OK){tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);result.unsafe=1;return false;}
 // Record actual successful unreserve BEFORE checking lifetime again. From
 // here onward all proof/disposal is CPU bookkeeping;NO config/MMIO/OS IO.
 s->reservationReleased=true;s->phase=PioneerQ38TokenNativeState::ClosedState;result.reservation=1;
 if(!s->Closed() || !c->io.TokenHostReleaseScope()){
  tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);result.unsafe=1;return false;
 }
 c->q38GdnCallThread=-1;c->io.q38MutableContext=NULL;cookie->context=NULL;
 sRuntimeOwner=NULL;sBankStorageOnly=false;sQ38LocalDeltaOwner=NULL;
 context=NULL;cookie=NULL;destroyed=true;
 delete c; // retires sticky whole-model bank reservations+CPU journals;all VM/hosts already gone
 result.context=1;result.ready=1;return true;
} // private compile-only path;legacy END remains REFUSED until actual end-to-end/hardware qualification
#endif
