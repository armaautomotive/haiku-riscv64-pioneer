/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FAST_DECODE_PROFILE_METRIC_FIXTURE_H
#define PIONEER_Q38_FAST_DECODE_PROFILE_METRIC_FIXTURE_H
// Fabricated v3 DATA for held-read/packet tests, not clock/model evidence.
static std::array<uint32_t,60> FastDecodeMetricFixture(){
 namespace Q=PioneerQ38SelectedPhaseProfileData;
 std::array<uint32_t,60> p={};Q::Initialize(p.data());
 p[Q::State]=Q::Complete;p[Q::Current]=Q::FinalStop;p[Q::WindowStart]=31;
 p[Q::LastPosition]=33;p[Q::Committed]=34;
 const uint64_t begin=UINT64_C(1)<<40;uint64_t cycles=0;
 for(unsigned phase=0;phase<Q::Phases;phase++){
  const auto count=Q::Expected(phase,3);p[Q::Buckets+phase*3]=count;p[Q::Transitions]+=count;
  const uint64_t delta=uint64_t(count)*19;Q::Put(p.data(),Q::Buckets+phase*3+1,delta);cycles+=delta;
 }
 Q::Put(p.data(),Q::BeginCycle,begin);Q::Put(p.data(),Q::EndCycle,begin+cycles);
 Q::Put(p.data(),Q::Previous,begin+cycles);assert(Q::Closed(p.data()));return p;
}
#endif
