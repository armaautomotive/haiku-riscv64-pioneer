/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
[[maybe_unused]] static __attribute__((noinline)) bool Q38TokenNativeClockScope(const void* owner){
 auto* c=const_cast<PioneerStoreContext*>(static_cast<const PioneerStoreContext*>(owner));
 auto* s=c?c->q38TokenNative:NULL;
 if(!c || !s || s->phase!=PioneerQ38TokenNativeState::ClockEnding || !s->clockEndAttempted || s->clockEnded
  || !s->runtime || !s->runtime->SharedMappingsRestored() || !c->io.TokenModelSourceScope(false)
  || !c->q38ClockIO.tokenClockGuard.Matches(c,Q38TokenNativeClockScope) || c->q38ClockIO.tokenClockGuard.Stopped()
  || c->q38ClockIO.tokenClockGuard.Sealed())return false;
 const uint64_t now=c->io.Q38CohortNow();
 if(now<s->clockPrevious || now<s->clockBegan || now-s->clockBegan>=UINT64_C(300000000))return false;
 s->clockPrevious=now;return true;
} // CPU native FD/thread/team/generation/clock/host checks before EVERY ARC access or clock-view deletion
[[maybe_unused]] static __attribute__((noinline)) bool EndQ38TokenNativeClock(PioneerStoreContext& c,PioneerQ38TokenClockResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenClockResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::SharedTlbRestored || s->clockEndAttempted || s->clockEnded
  || !s->SharedMappingsRestored() || !s->runtime || !Q38TokenExecutionOutput(c,&result,sizeof(result))
  || !c.io.TokenModelSourceScope(false) || !c.q38ClockIO.tokenClockGuard.Pristine() || !c.q38ClockIO.TokenClockViewsRetained())return false;
 const auto& before=c.q38Clock.Snapshot();
 if(!c.q38Clock.RequiresExplicitEnd() || c.q38Clock.Unsafe() || before.version!=1 || before.state!=TT_ARC_CLOCK_BUSY_VERIFIED
  || !before.busy_requested || !before.busy_acknowledged || before.idle_acknowledged || before.restored || before.recovery_required
  || before.before_mhz!=800 || before.before_mode!=3 || before.busy_mhz<=800 || before.busy_mhz>1400 || before.busy_mode!=3
  || before.messages>UINT32_MAX-17)return false;
 const uint32_t messages=before.messages;
 s->clockBegan=s->clockPrevious=c.io.Q38CohortNow();
 if(s->clockBegan>UINT64_MAX-UINT64_C(300000000))return false;
 result.beforeMHz=before.before_mhz;
 s->clockEndAttempted=true;s->phase=PioneerQ38TokenNativeState::ClockEnding;result.started=1;result.phase=1;
 auto fail=[&](){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);result.unsafe=1;result.ready=0;return false;};
 if(!c.q38ClockIO.tokenClockGuard.Bind(&c,Q38TokenNativeClockScope) || !Q38TokenNativeClockScope(&c))return fail();
 // Authority is derived above from this SAME private full-token owner,
 // NEVER a caller bool or the legacy FinishQ38NormalClock closed predicate.
 if(!c.q38Clock.End(c.baseline.baseline.baseline.baseline.control.control.control.arc,
   c.baseline.baseline.baseline.baseline.control.control.telemetry,true))return fail();
 result.phase=2;const auto& after=c.q38Clock.Snapshot();
 if(c.q38ClockIO.tokenClockGuard.Stopped() || !Q38TokenNativeClockScope(&c) || c.q38Clock.Unsafe()
  || c.q38Clock.RequiresExplicitEnd() || !c.q38ClockIO.TokenClockViewsReleased() || after.version!=1
  || after.state!=TT_ARC_CLOCK_RESTORED || !after.restored || !after.idle_acknowledged || after.recovery_required
  || after.after_mhz!=result.beforeMHz || after.after_mode!=3 || after.messages<messages+2 || after.messages>messages+17
  || after.last_message!=0x34 || after.last.state!=TT_ARC_QUERY_PASSED || !after.last.request_restored
  || !after.last.tlb_restored || after.last.recovery_required)return fail();
 result.messages=after.messages-messages;result.afterMHz=after.after_mhz;result.phase=3;
 if(!s->runtime->VerifyClockReturnMappings(c.io) || !Q38TokenNativeClockScope(&c)
  || !c.q38ClockIO.tokenClockGuard.Seal(&c,Q38TokenNativeClockScope))return fail();
 result.tlbWords=6;s->clockEnded=true;s->phase=PioneerQ38TokenNativeState::ClockEnded;
 if(!s->ClockRestored() || !c.io.TokenModelSourceScope(false))return fail();
 result.ready=1;result.phase=4;return true;
} // normal firmware IDLE/query+clock-view disposal ONLY;master/bulk/model VM/hosts/reservations/END STILL owned
#endif
