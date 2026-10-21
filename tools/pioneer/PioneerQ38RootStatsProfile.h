/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_ROOT_STATS_PROFILE_H
#define PIONEER_Q38_ROOT_STATS_PROFILE_H
#ifndef PIONEER_Q38_ROOT_STATS_BENCH
#error Separate host-copy-only projection diagnostic,not ordinary Work/Assistant
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38RootCycleData.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
// Exact known original-TC drivers ONLY. Called on CPU metadata BEFORE BEGIN;
// no support for a different weight format or inference/Assistant selection.
static inline bool PioneerQ38RootStatsDebugTag(const tt_pioneer_driver_reload& r) {
 return r.version==1 && r.driver==55 && r.guard_version==1 && r.generation
  && !r.quiesced && !r.cold && !r.active_owner && r.open_count==1
  && (r.build_tag==UINT64_C(0x2026102100010012) || r.build_tag==UINT64_C(0x2026102100010016));
}
// ALREADY returned host-memory data ONLY. Invalid/missing/overflowed metrics
// disable this profile,NEVER change execution success,END,cleanup or safety.
// Values are raw counter deltas,NOT pure arithmetic time or token speed.
struct PioneerQ38RootStatsProfile {
 uint64_t samples=0,cycles[3]={},packets[6]={};bool valid=true;
 void Reset(){*this=PioneerQ38RootStatsProfile{};}
 void Add(const uint32_t* stats) {
  if(!valid)return;
  if(!PioneerQ38RootCycleDataValid(stats) || samples==UINT64_MAX){valid=false;return;}
  uint64_t delta[3];
  for(unsigned i=0;i<3;i++) {
   delta[i]=PioneerQ38RootCyclePhase(stats,i);
   if(delta[i]>UINT64_MAX-cycles[i]){valid=false;return;}
  }
  for(unsigned i=0;i<6;i++)if(uint64_t(stats[6+i])>UINT64_MAX-packets[i]){valid=false;return;}
  for(unsigned i=0;i<3;i++)cycles[i]+=delta[i];
  for(unsigned i=0;i<6;i++)packets[i]+=stats[6+i];
  samples++;
 }
};
#endif
