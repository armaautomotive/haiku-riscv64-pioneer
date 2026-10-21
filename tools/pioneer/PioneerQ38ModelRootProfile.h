/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MODEL_ROOT_PROFILE_H
#define PIONEER_Q38_MODEL_ROOT_PROFILE_H
#ifndef PIONEER_Q38_MODEL_ROOT_PROFILE
#error Separate fixed-public THREE32 host-copy diagnostic ONLY
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38RootCycleData.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
static inline bool PioneerQ38ModelRootProfileTag(const tt_pioneer_driver_reload& r) {
 return r.version==1 && r.driver==55 && r.guard_version==1 && r.generation
  && !r.quiesced && !r.cold && !r.active_owner && r.open_count==1
  && (r.build_tag==UINT64_C(0x2026102100010012) || r.build_tag==UINT64_C(0x2026102100010017)
   || r.build_tag==UINT64_C(0x2026102100010019) || r.build_tag==UINT64_C(0x2026102100010020)
   || r.build_tag==UINT64_C(0x2026102100010021));
}
// ALREADY returned CPU-memory counters ONLY. No IO,learned-reference,
// timeout/completion/cleanup authority,or change to graph success exists.
// Bad metrics are sticky-invalid but MUST NOT change model/card execution.
class PioneerQ38ModelRootProfile {
 unsigned sequence=0,position=0,currentWaves=0,measuredPositions=0;
 bool active=false,measured=false,valid=true;
public:
 struct Bucket {uint64_t samples=0,workers=0,cycles[3]={},packets[6]={};};
 Bucket bucket[9];
 static int Shape(unsigned type,unsigned inputs) {
  unsigned t=type==8?0:type==12?1:type==14?2:3,k=inputs==5120?0:inputs==6144?1:inputs==17408?2:3;
  return t<3 && k<3?int(t*3+k):-1;
 }
 void Begin(unsigned seq,unsigned pos) {
  if(!valid)return;
  if(active || seq!=sequence || pos!=position || seq>=3 || pos>=32){valid=false;return;}
  active=true;measured=pos>=5;currentWaves=0;
 }
 void Add(unsigned type,unsigned inputs,unsigned workers,const uint32_t* stats) {
  if(!valid)return;
  int index=Shape(type,inputs);
  if(!active || index<0 || !workers || workers>119 || currentWaves==UINT32_MAX){valid=false;return;}
  if(!measured){currentWaves++;return;}
  auto& b=bucket[index];uint64_t delta[3];
  if(!PioneerQ38RootCycleDataValid(stats) || b.samples==UINT64_MAX || workers>UINT64_MAX-b.workers){valid=false;return;}
  for(unsigned i=0;i<3;i++) {
   delta[i]=PioneerQ38RootCyclePhase(stats,i);
   if(delta[i]>UINT64_MAX-b.cycles[i]){valid=false;return;}
  }
  for(unsigned i=0;i<6;i++)if(uint64_t(stats[6+i])>UINT64_MAX-b.packets[i]){valid=false;return;}
  for(unsigned i=0;i<3;i++)b.cycles[i]+=delta[i];
  for(unsigned i=0;i<6;i++)b.packets[i]+=stats[6+i];
  b.samples++;b.workers+=workers;currentWaves++;
 }
 void End(bool success) {
  if(!valid)return;
  if(!active || !success || !currentWaves){valid=false;return;}
  if(measured)measuredPositions++;
  active=false;measured=false;
  if(++position==32){position=0;sequence++;}
 }
 bool PublishAllowed(bool modelOkay,bool cleanEnd,bool stopped)const {
  return valid && !active && sequence==3 && !position && measuredPositions==81 && modelOkay && cleanEnd && !stopped;
 }
};
#endif
