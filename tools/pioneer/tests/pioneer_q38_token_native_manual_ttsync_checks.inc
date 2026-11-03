/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_token_native_manual_ttsync_role.h"
#include <unistd.h>
// Fork only AFTER genuine initialization,119 virgin workers and actual
// original first-layer arithmetic. Corrupt an observed register,never
// inject ownership,PC,status,arithmetic output or a completion receipt.
static void CheckNativeManualOverrideLoss(Fixture& f){
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;
 assert(!lockHeld && !state->Unsafe() && !state->Closed() && !io.manualOverrideFault
  && io.executionCpu->fullLayerComplete && !io.Core(0).pending);
 unsigned refused=0;
 for(unsigned bit=0;bit<=32;bit++){
  fflush(nullptr);const pid_t child=fork();assert(child>=0);
  if(!child){
   io.manualOverrideFault=bit==32?UINT32_MAX:uint32(1U)<<bit;
   const auto resets=io.resets;const unsigned starts=io.rootStarts,stops=io.rootStops,holds=io.rootHolds;
   const unsigned requests=io.runtimePackets;auto* owner=c.q38Dispatch;
   {
    PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
    PioneerQ38TokenFrontierResult result;
    assert(!ObserveQ38TokenNativeFrontier(c,result) && result.started && result.unsafe && !result.ready
     && io.manualOverrideFaultSeen==1 && io.touches==io.manualOverrideFaultTouches);
   }
   assert(c.q38TokenNative==state && c.q38Dispatch==owner && f.cookie.context==&c
    && state->Unsafe() && !state->Closed() && io.resets==resets
    && io.rootStarts==starts && io.rootStops==stops && io.rootHolds==holds && io.runtimePackets==requests);
   const unsigned stoppedAt=io.touches;
   f.Unknown();assert(io.touches==stoppedAt);_exit(0);
  }
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));refused++;
 }
 assert(!state->Unsafe() && !io.manualOverrideFault && !io.manualOverrideFaultSeen);
 printf("PASS_MANUAL_NATIVE_OVERRIDE_LOSS genuineOriginalFirstLayer1 badRegisterWords%u actualScopedFrontierGuard1 firstBadReadThenNoIO1 allOwnersQuarantinedAndRetained1 noResetRetryCleanupRelease1;PARENT original STOP/HOLD/full-END STILL REQUIRED;SYNTHETIC_MMIO_NOT_HARDWARE_OR_TPS\n",refused);
}
