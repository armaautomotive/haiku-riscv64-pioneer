/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
bool PioneerSessionIO::TokenViewDelete(area_id view){return view>=B_OK && delete_area(view)==B_OK;}
__attribute__((noinline)) bool PioneerSessionIO::TokenViewsScope(){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 if(!s || !s->viewsCloseAttempted || (s->phase!=PioneerQ38TokenNativeState::ViewsClosing && s->phase!=PioneerQ38TokenNativeState::ViewsClosed)
  || !TokenFinalLifetimeScope() || q38TokenViews.Failed())return false;
 if(s->phase==PioneerQ38TokenNativeState::ViewsClosed)
  return s->viewsClosed && s->ViewsReleased() && q38TokenViews.Complete() && q38TokenViews.Matches(*this)
   && !runtimeMapped && !fleetEnabled && !runtimeEntry && !runtimePc && !scratchData;
 return !s->viewsClosed && runtimeMapped && fleetEnabled && runtimeEntry && runtimePc && scratchData && payloadBytes==8192
  && (q38TokenViews.Prepared() || q38TokenViews.Started()) && q38TokenViews.Matches(*this);
} // CPU ONLY,same actual hardware-restored owner;no aliases used or PCI/host release
[[maybe_unused]] static __attribute__((noinline)) bool CloseQ38TokenNativeViews(PioneerStoreContext& c,PioneerQ38TokenViewsResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenViewsResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::HardwareSealed || s->viewsCloseAttempted || s->viewsClosed
  || !c.io.q38TokenViews.Prepared() || !Q38TokenExecutionOutput(c,&result,sizeof(result)) || !c.io.TokenFinalOwnershipScope())return false;
 s->viewsCloseAttempted=true;s->phase=PioneerQ38TokenNativeState::ViewsClosing;result.started=1;
 bool okay=c.io.q38TokenViews.Close(c.io);result.deleted=c.io.q38TokenViews.Deleted();
 if(okay && c.io.q38TokenViews.Complete()){
  c.io.TokenClearViewPointers();s->viewsClosed=true;s->phase=PioneerQ38TokenNativeState::ViewsClosed;
  okay=c.io.TokenViewsScope();
 }
 if(!okay){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);result.unsafe=1;result.ready=0;return false;}
 result.ready=1;return true;
} // one attempt;failed delete/lifetime loss retains all remaining views/hosts/PCI/reservations
#endif
