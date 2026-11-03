/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_MANUAL_WATCHDOG_HOST_TEST) || PIONEER_Q38_MANUAL_WATCHDOG_HOST_TEST != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Separate HOST-only caller watchdog TEST,not native,CPU or Work authority
#endif
#include <cassert>
#include <cstdio>
#include <cstring>
#include "pioneer_q38_manual_watchdog_bridge.h"
#include "../PioneerQ38TokenRateSamples.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenDecodeBenchmarkProtocol.h"
using bigtime_t=int64_t;
static constexpr int B_OK=0;
static bigtime_t virtualClock;
static unsigned selectedMode;
static const Q38ManualWatchdogCallbacks* current;
static bigtime_t system_time(){return virtualClock;}
static int snooze(bigtime_t delay){
 assert(delay==100000 && current);current->pause(current->context);
 if(selectedMode==0)virtualClock+=600000000;
 if(selectedMode==2)virtualClock--;
 return B_OK;
} // virtual HOST clock/scheduler only;production deadline/poll policy unchanged
Q38ManualWatchdogResult Q38RunManualWatchdog(const Q38ManualWatchdogCallbacks& callbacks,unsigned mode){
 assert(!current && mode<3 && callbacks.context && callbacks.frontier && callbacks.poll
  && callbacks.terminal && callbacks.pause && callbacks.close);
 current=&callbacks;selectedMode=mode;virtualClock=1;
 Q38ManualWatchdogResult result={};
 struct Session {
  const Q38ManualWatchdogCallbacks& cb;Q38ManualWatchdogResult& r;
  bool ObserveFrontier(PioneerQ38TokenFrontierControl::Packet& p){return cb.frontier(cb.context,p);}
  bool Poll(){return cb.poll(cb.context);}
  bool TerminalReady(){return cb.terminal(cb.context);}
  bool Finish(){r.finishes++;assert(false && "blocked worker cannot authorize Finish");return false;}
  ~Session(){cb.close(cb.context);r.closes++;}
 };
 {
  Session owned{callbacks,result};auto* session=&owned;
  namespace V=PioneerQ38TokenDecodeBenchmark;
  PioneerQ38TokenRateSamples samples(V::Prompts,V::Positions);
#include "manual_watchdog_actual_caller.inc"
  assert(!okay && why && !strcmp(why,"deadline_or_clock") && !finalCount);
  result.reason=1;result.observations=observations;result.finalCount=finalCount;
 }
 assert(!result.finishes && result.closes==1);current=nullptr;return result;
} // actual extracted caller loop+DATA sampler;NOT full USER Begin/source/upload execution
