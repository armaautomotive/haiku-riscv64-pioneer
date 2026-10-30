/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
bool PioneerSessionIO::TokenHostLeaseIdentity(const PioneerQ38TokenHostLease& hosts)const{
 const auto* c=q38MutableContext;return c && &c->io==this && c->q38TokenNative && &c->q38TokenNative->hosts==&hosts;
}
__attribute__((noinline)) bool PioneerSessionIO::TokenHostCaptureScope(){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 return s && s->phase==PioneerQ38TokenNativeState::MemoryPreparing && s->hosts.Pristine()
  && s->memory && s->startup && s->participants==c->q38Dispatch && q38TokenViews.Prepared() && q38TokenViews.Matches(*this)
  && TokenModelSourceScope(false);
} // capture SAME actual wired buffers BEFORE any token image installation or launch
#endif
