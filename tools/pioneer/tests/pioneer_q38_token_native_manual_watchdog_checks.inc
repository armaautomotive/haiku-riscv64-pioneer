/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_token_native_manual_ttsync_role.h"
#if !defined(PIONEER_Q38_TOKEN_NATIVE_MANUAL_WATCHDOG_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_MANUAL_WATCHDOG_FIXTURE != 1 \
 || !defined(PIONEER_Q38_NATIVE_PEER_MANUAL_WATCHDOG_TEST) || PIONEER_Q38_NATIVE_PEER_MANUAL_WATCHDOG_TEST != 1
#error Distinct complete native manual-wait watchdog test required
#endif
#include "pioneer_q38_manual_watchdog_bridge.h"
static void CheckNativeManualWatchdog(Fixture& f){
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;
 assert(!lockHeld && !state->Unsafe() && !c.q38FfnModelPlan && tokenNativePositions==34);
 unsigned cases=0;
 for(unsigned mode=0;mode<3;mode++){
  fflush(nullptr);const pid_t child=fork();assert(child>=0);
  if(!child){
   auto& peer=*io.bootstrapPeer[0]->cpu;peer.LoseNextManualCompletion();
   io.executionCpu->ResumeFirstMatrix();
   for(unsigned round=0;round<3000000 && !peer.ManualWaitStats().blocked;round++)io.Pause();
   const auto stalled=peer.Stats();const auto wait=peer.ManualWaitStats();
   assert(stalled.pc==0x7200c && stalled.active && wait.ordered && wait.stores==wait.loads+1 && wait.blocked
    && !state->Unsafe() && !state->TerminalObserved() && !state->Closed());
   struct Native {Fixture& f;unsigned frontiers=0,polls=0,pauses=0,closes=0;} native{f};
   Q38ManualWatchdogCallbacks cb={&native,
    [](void* p,PioneerQ38TokenFrontierControl::Packet& packet){
     auto& n=*static_cast<Native*>(p);tokenControlCopies=0;
     const auto status=Q38TokenFrontierControl(&n.f.cookie,&packet,sizeof(packet));n.frontiers++;
     assert(!lockHeld && tokenControlCopies==2);
     return status==B_OK && PioneerQ38TokenFrontierControl::ReplyShape(packet) && packet.reply.ready
      && !packet.reply.detail[7] && packet.reply.detail[8]==34 && packet.reply.detail[9]==2;
    },
    [](void* p){
     auto& n=*static_cast<Native*>(p);auto packet=std::make_unique<PioneerQ38TokenControl::Packet>();
     memset(packet.get(),0,sizeof(*packet));auto& request=packet->request;
     request.magic=PioneerQ38TokenControl::Magic;request.version=1;request.operation=PioneerQ38TokenControl::Observe;
     tokenControlCopies=0;const auto status=Q38TokenNativeControl(&n.f.cookie,packet.get(),sizeof(*packet));n.polls++;
     assert(!lockHeld && tokenControlCopies==2);
     return status==B_OK && packet->reply.started && !packet->reply.ready && !packet->reply.unsafe && packet->reply.stage==9;
    },
    [](void* p){auto& n=*static_cast<Native*>(p);return n.f.c->q38TokenNative->TerminalObserved();},
    [](void* p){auto& n=*static_cast<Native*>(p);n.f.io.bootstrapPeer[0]->cpu->Advance(1);n.pauses++;},
    [](void* p){
     auto& n=*static_cast<Native*>(p);auto& f=n.f;auto& io=f.io;auto* owner=f.c->q38TokenNative;
     const auto resets=io.resets;const unsigned before=io.touches,packets=io.runtimePackets;
     const unsigned starts=io.rootStarts,stops=io.rootStops,holds=io.rootHolds;
     assert(ReleaseResidentCookie(&f.cookie)==B_OK);n.closes++;
     assert(f.cookie.context==f.c.get() && f.cookie.magic==0x50313030 && sRuntimeOwner==&f.cookie
      && f.c->q38TokenNative==owner && io.touches==before && io.resets==resets && io.runtimePackets==packets
      && io.rootStarts==starts && io.rootStops==stops && io.rootHolds==holds && !lockHeld);
     f.Unknown();assert(io.touches==before);
     assert(ReleaseResidentCookie(&f.cookie)==B_OK && io.touches==before); // retained close is inert
    }};
   const auto r=Q38RunManualWatchdog(cb,mode);const auto after=peer.Stats();const auto held=peer.ManualWaitStats();
   const unsigned expected=mode==0?3:mode==1?18000:1;
   assert(r.reason==1 && r.observations==expected && !r.finalCount && !r.finishes && r.closes==1
    && native.frontiers==expected && native.polls==expected && native.pauses==expected && native.closes==1
    && after.pc==stalled.pc && after.steps==stalled.steps && after.matrixInstructions==stalled.matrixInstructions
    && after.statusWrites==stalled.statusWrites && after.deviceReads==stalled.deviceReads && after.deviceWrites==stalled.deviceWrites
    && held.stores==wait.stores && held.loads==wait.loads && held.blocked==wait.blocked+expected);
   _exit(0);
  }
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
 }
 assert(!state->Unsafe() && !state->Closed() && !io.bootstrapPeer[0]->cpu->ManualWaitStats().armed);
 printf("PASS_MANUAL_NATIVE_WATCHDOG cases%u actual34positionStartup1 actualBlockedLoadNoRetirement1 originalCallerDeadlinePollClockGuards1 "
  "actualNativeFrontierObserveAndCookieClose1 allOwnersRetainedNoIOResetRetryCleanup1;PARENT original first-layerSTOP_HOLD_fullEND STILL REQUIRED;HOST_ENDPOINTS_NOT_PHYSICAL_OR_TPS\n",cases);
}
