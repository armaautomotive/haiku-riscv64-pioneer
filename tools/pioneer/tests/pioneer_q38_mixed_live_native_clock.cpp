/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_CLOCK_LEASE_MODEL_ONLY
#include "pioneer_q38_clock_lease_test.cpp"
#include "pioneer_q38_mixed_live_native_clock.h"
struct MixedLiveNativeClock::Impl{
 ClockLeaseIO io;PioneerQ38ClockLease<ClockLeaseIO> lease{io};
 Impl(){(void)&Inert;}
};
MixedLiveNativeClock::MixedLiveNativeClock():impl(new Impl){}
MixedLiveNativeClock::~MixedLiveNativeClock(){delete impl;}
tt_arc_scratch_result MixedLiveNativeClock::Arc(){return StrictQueue::Arc();}
tt_arc_telemetry_result MixedLiveNativeClock::Health(){return StrictQueue::Health();}
bool MixedLiveNativeClock::Begin(const tt_arc_scratch_result& a,const tt_arc_telemetry_result& h){return impl->lease.Begin(a,h);}
bool MixedLiveNativeClock::End(const tt_arc_scratch_result& a,const tt_arc_telemetry_result& h,bool clean){return impl->lease.End(a,h,clean);}
bool MixedLiveNativeClock::Unsafe()const{return impl->lease.Unsafe();}
bool MixedLiveNativeClock::RequiresExplicitEnd()const{return impl->lease.RequiresExplicitEnd();}
const tt_arc_clock_guard_result& MixedLiveNativeClock::Snapshot()const{return impl->lease.Snapshot();}
void MixedLiveNativeClock::Quarantine(){quarantined=true;impl->lease.Quarantine();}
void MixedLiveNativeClock::Fault(unsigned fault){
 assert(fault<5);if(!fault)impl->io.timeout=true;else if(fault==1)impl->io.badReply=true;
 else if(fault==2)impl->io.failRestore=true;else if(fault==3)impl->io.failUnmap=true;else Quarantine();
} // raw TEST endpoint loss/unknown ONLY,never a successful lease setter
unsigned MixedLiveNativeClock::IO()const{return impl->io.io;}
unsigned MixedLiveNativeClock::Notifications()const{return impl->io.notifications;}
unsigned MixedLiveNativeClock::Unmaps()const{return impl->io.unmaps;}
