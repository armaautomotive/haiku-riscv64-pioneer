/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_MATRIX_PHASE_READ_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_MATRIX_PHASE_READ_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_MATRIX_PHASE_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_MATRIX_PHASE_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_READ_NATIVE) || PIONEER_Q38_TOKEN_MATRIX_PHASE_READ_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE != 1 \
 || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE)
#error Exact fresh-slot reader TEST requires SAME original-model first-layer/cancel/HOLD/drain/full-END predecessor
#endif
#include "../PioneerQ38TokenMatrixPhaseReadRole.h"
static void CheckNativeMatrixPhaseUndrained(Fixture& f){
 auto& c=*f.c;auto& io=f.io;const unsigned before=io.touches;
 PioneerQ38TokenMatrixPhaseResult r;uint32_t output[16]={};
 PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
 assert(!ReadQ38TokenNativeMatrixPhase(c,1,output,r) && io.touches==before && !r.started && !r.unsafe);
}
static void CheckNativeMatrixPhaseRead(Fixture& f){
 namespace Q=PioneerQ38TokenMatrixPhaseData;
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;
 assert(!lockHeld && state->TerminalHeld() && !state->Closed() && !state->Unsafe());
 const unsigned packets=io.runtimePackets,resets=io.resets.size();unsigned intervals=0,missing=0,invalid=0;
 uint32_t output[Q::Words]={};PioneerQ38TokenMatrixPhaseResult r;unsigned before=io.touches;
 assert(!ReadQ38TokenNativeMatrixPhase(c,1,output,r) && io.touches==before);
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  const auto pristine=r;auto* fields=reinterpret_cast<uint32_t*>(&r);
  for(unsigned i=0;i<16;i++){
   fields[i]^=1;assert(!ReadQ38TokenNativeMatrixPhase(c,1,output,r) && io.touches==before);r=pristine;
  }
  for(unsigned worker:{0U,120U,UINT32_MAX})assert(!ReadQ38TokenNativeMatrixPhase(c,worker,output,r) && io.touches==before);
  assert(!ReadQ38TokenNativeMatrixPhase(c,1,nullptr,r) && io.touches==before);
  for(void* alias:{static_cast<void*>(&c),static_cast<void*>(&io),static_cast<void*>(state),static_cast<void*>(&r)})
   assert(!ReadQ38TokenNativeMatrixPhase(c,1,static_cast<uint32_t*>(alias),r) && io.touches==before);
  auto* misaligned=reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(output)+1);
  assert(!ReadQ38TokenNativeMatrixPhase(c,1,misaligned,r) && io.touches==before);
  output[7]=1;assert(!ReadQ38TokenNativeMatrixPhase(c,1,output,r) && io.touches==before);output[7]=0;
  for(unsigned worker=1;worker<120;worker++){
   uint32_t observed[Q::Words];for(unsigned i=0;i<Q::Words;i++)observed[i]=io.Raw(worker,Q::Address+i*4);
   const unsigned stop=io.Raw(0,PioneerQ38GlobalChainLayout::R::Nonces+(worker-1)*4);
   const unsigned expected=Q::Classify(observed,worker,stop);
   assert(expected!=Q::Malformed); // actual known publisher,not supplied metric DATA
   const bool okay=ReadQ38TokenNativeMatrixPhase(c,worker,output,r);
   if(!okay)fprintf(stderr,"NATIVE_MATRIX_PHASE_READ_REFUSAL requested%u phase%u worker%u word%u unsafe%u;NO ready or metric supplied\n",
    worker,r.phase,r.worker,r.word,r.unsafe);
   assert(okay && r.started && r.ready && !r.unsafe && r.phase==4 && r.worker==worker
    && r.metricState==expected && r.valid==(expected==Q::Intervals)
    && r.nonce==observed[2] && r.inputs==observed[5] && r.groups==observed[6]);
   for(unsigned i=0;i<Q::Words;i++)assert(output[i]==observed[i]);
   intervals+=expected==Q::Intervals;missing+=expected==Q::Missing;invalid+=expected==Q::InvalidClock;
   const unsigned after=io.touches;PioneerQ38TokenMatrixPhaseResult again;uint32_t untouched[Q::Words]={};
   assert(!ReadQ38TokenNativeMatrixPhase(c,worker,untouched,again) && io.touches==after);
   for(auto& word:output)word=0;r=pristine;
  }
 }
 assert(intervals && intervals+missing+invalid==119 && state->TerminalHeld() && !state->Closed() && !state->Unsafe()
  && io.runtimePackets==packets && io.resets.size()==resets);
 printf("EXECUTED SAME native original851/first-layer/cancel/119STOP/120HOLD/real zero-record drain/fresh prelaunch slots/ALL119 real held+doublechecked16word samples+same complete code+boot/source/counter seals/intervals%u missing%u invalidClock%u/NO native lifecycle transition,NoC packet,reset or invented latest-job identity;normal full-END follows;clock+OS endpoints FABRICATED,NOT physical timing,whole-model,Work or TPS\n",intervals,missing,invalid);
}
