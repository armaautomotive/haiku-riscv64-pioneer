/* SPDX-License-Identifier: MIT */
#include <cstdio>
#include <cstdint>
#include <cassert>
using uint32=uint32_t;using area_id=int;static constexpr int B_OK=0;
#include "pioneer_q38_token_clock_raw.h"
struct Eligibility{bool live=true;unsigned checks=0;};
static bool Eligible(const void* owner){auto& s=*const_cast<Eligibility*>(static_cast<const Eligibility*>(owner));s.checks++;return s.live;}
static void Refused(PioneerQ38ClockIO& io){
 const unsigned before=io.rawEvents,deletes=io.deletes;const auto maps=io.maps;
 assert(io.TlbRead(0)==UINT32_MAX && io.CsmRead(0x10001000)==UINT32_MAX && io.MsiStatusRead()==UINT32_MAX);
 io.TlbWrite(0,0);io.QueueWrite(0x10001000,0);io.SetQueueBounds(0,0,0);io.NotifyArc();io.Pause();
 assert(!io.Map() && !io.Unmap() && io.rawEvents==before && io.deletes==deletes && io.maps==maps);
}
int main(){
 const tt_pioneer_profile profile;auto arc=StrictQueue::Arc();auto health=StrictQueue::Health();tt_arc_queue_result unused={};
 unsigned cases=0;
 {
  PioneerQ38ClockIO io(profile,arc,health,unused);PioneerQ38ClockLease<PioneerQ38ClockIO> lease(io);Eligibility owner;
  assert(lease.Begin(arc,health) && io.maps==1 && io.notifications==3 && io.clock==1350 && io.TokenClockViewsRetained());
  assert(io.tokenClockGuard.Bind(&owner,Eligible) && !io.tokenClockGuard.Bind(&owner,Eligible));
  assert(lease.End(arc,health,true) && io.clock==800 && io.notifications==5 && io.deletes==6 && !io.hardwareCleanup
   && !lease.RequiresExplicitEnd() && !lease.Unsafe() && io.TokenClockViewsReleased() && owner.checks);
  assert(io.tokenClockGuard.Seal(&owner,Eligible) && io.tokenClockGuard.Sealed() && !io.tokenClockGuard.Stopped());Refused(io);cases++;
 }
 for(unsigned fault=0;fault<10;fault++){
  PioneerQ38ClockIO io(profile,arc,health,unused);PioneerQ38ClockLease<PioneerQ38ClockIO> lease(io);Eligibility owner;
  assert(lease.Begin(arc,health) && io.tokenClockGuard.Bind(&owner,Eligible));
  owner.live=false;const unsigned before=io.rawEvents;
  if(fault==0)(void)io.TlbRead(0);else if(fault==1)io.TlbWrite(0,0);else if(fault==2)(void)io.CsmRead(0x10001000);
  else if(fault==3)io.SetQueueBounds(0,0,0);else if(fault==4)io.QueueWrite(0x10001000,0);else if(fault==5)(void)io.MsiStatusRead();
  else if(fault==6)io.NotifyArc();else if(fault==7)io.Pause();else if(fault==8)(void)io.Unmap();else (void)lease.End(arc,health,true);
  assert(io.tokenClockGuard.Stopped() && io.rawEvents==before && !io.deletes);
  owner.live=true;Refused(io);assert(!io.tokenClockGuard.Bind(&owner,Eligible));cases++;
 }
 for(unsigned afterDelete=1;afterDelete<=6;afterDelete++){
  PioneerQ38ClockIO io(profile,arc,health,unused);Eligibility owner;
  assert(io.Map() && io.tokenClockGuard.Bind(&owner,Eligible));
  struct Loss{Eligibility* owner;unsigned count=0,at;} loss{&owner,0,afterDelete};
  io.observer=&loss;io.after=[](void* p,unsigned kind,unsigned){auto& s=*static_cast<Loss*>(p);if(kind==9 && ++s.count==s.at)s.owner->live=false;};
  assert(!io.Unmap() && io.deletes==afterDelete && io.tokenClockGuard.Stopped() && !io.hardwareCleanup);
  owner.live=true;Refused(io);cases++;
 }
 {
  PioneerQ38ClockIO io(profile,arc,health,unused);PioneerQ38ClockLease<PioneerQ38ClockIO> lease(io);Eligibility owner;
  assert(lease.Begin(arc,health) && io.tokenClockGuard.Bind(&owner,Eligible));
  io.observer=&owner;io.after=[](void* p,unsigned kind,unsigned){if(kind==2)static_cast<Eligibility*>(p)->live=false;};
  const unsigned before=io.rawEvents;
  assert(!lease.End(arc,health,true) && io.tokenClockGuard.Stopped() && lease.Unsafe() && lease.RequiresExplicitEnd()
   && io.rawEvents==before+7 && !io.deletes); // six original-control reads then one actual mapping write
  owner.live=true;Refused(io);cases++;
 }
 printf("PASS actual extracted native clock IO+real ClockLease/normal-clock/strict queue/800->1350->800/one-time guard bind+seal/%u transport eligibility-loss+post-write+each-deletion cases/ZERO following raw IO or retries after uncertainty;TEST ARC+VM endpoints,NOT full-token clock-END/hardware/Work/TPS qualification\n",cases);
}
