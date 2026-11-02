/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_token_native_phase_profile_role.h"
static void CheckNativePhaseProfile(Fixture& f){
 namespace Q=PioneerQ38TokenPhaseProfileData;
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;const auto& cpu=*io.executionCpu;
 assert(!lockHeld && state->TerminalHeld() && !state->Closed() && !state->Unsafe()
  && cpu.phaseClockReads>=3 && cpu.phaseClockReads%3==0 && cpu.pc==0x60004);
 std::array<uint32_t,64> output={},observed={};
 for(unsigned i=0;i<Q::Words;i++)observed[i]=io.Raw(0,Q::Address+i*4);
 assert(observed[0]==Q::Magic && observed[1]==Q::Version && observed[2]==Q::Words
  && observed[Q::State]==Q::Active && !observed[Q::Error] && observed[Q::Current]==Q::Recurrent
  && observed[Q::Transitions]==2 && !observed[Q::Committed] && !Q::Get(observed.data(),Q::EndCycle)
  && !Q::Closed(observed.data())); // cancelled before any full token,NEVER a fabricated complete timing record
 const unsigned touches=io.touches,packets=io.runtimePackets,resets=io.resets.size();
 PioneerQ38TokenPhaseProfileResult result;
 assert(!ReadQ38TokenNativePhaseProfile(c,output.data(),result) && io.touches==touches);
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  const auto pristine=result;auto* words=reinterpret_cast<uint32_t*>(&result);
  for(unsigned i=0;i<16;i++){
   words[i]^=1;assert(!ReadQ38TokenNativePhaseProfile(c,output.data(),result) && io.touches==touches);result=pristine;
  }
  assert(!ReadQ38TokenNativePhaseProfile(c,nullptr,result) && io.touches==touches);
  const bool okay=ReadQ38TokenNativePhaseProfile(c,output.data(),result);
  if(!okay)fprintf(stderr,"NATIVE_PHASE_READ_REFUSAL phase%u worker%u word%u unsafe%u;NO ready state supplied\n",
   result.phase,result.worker,result.word,result.unsafe);
  assert(okay && result.started && result.ready && !result.unsafe && !result.valid && !result.committed
   && result.phase==4 && !result.worker && result.word==59 && result.words==60
   && result.metricState==Q::Active && !result.metricError && output==observed);
  const unsigned after=io.touches;PioneerQ38TokenPhaseProfileResult again;
  assert(!ReadQ38TokenNativePhaseProfile(c,output.data(),again) && io.touches==after);
 }
 assert(state->TerminalHeld() && !state->Closed() && !state->Unsafe() && io.runtimePackets==packets && io.resets.size()==resets);
 printf("EXECUTED SAME native original first-layer/cancel/119STOP/120HOLD/real zero-record drain/actual timing ROOT CSRreads%llu/ONE real held+doublechecked60word snapshot+4zero padding/incomplete-cancel metric retained WITHOUT cleanup bypass;normal full-END follows;FABRICATED clock+OS endpoints,NOT physical phase time,whole-model,Work or TPS\n",
  (unsigned long long)cpu.phaseClockReads);
}
