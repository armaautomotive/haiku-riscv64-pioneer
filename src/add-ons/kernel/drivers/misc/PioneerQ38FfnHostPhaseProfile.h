/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_HOST_PHASE_PROFILE_H
#define PIONEER_Q38_FFN_HOST_PHASE_PROFILE_H
#include <stdint.h>
// Diagnostic CPU-monotonic intervals ONLY. No card reads,completion or
// source authority. RootPoll includes host guards/poll latency,NOT isolated
// device kernel time. Timing faults invalidate diagnostics,never grant work.
enum PioneerQ38FfnHostPhase {
 Q38_FFN_PREFLIGHT,Q38_FFN_INPUT_GUARD,Q38_FFN_INPUT_STAGE,Q38_FFN_MANIFEST,
 Q38_FFN_ROOT_POLL,Q38_FFN_ROOT_PROOF,Q38_FFN_ROOT_HOLD,Q38_FFN_OUTPUT,
 Q38_FFN_HOST_PHASES
};
struct PioneerQ38FfnHostPhaseProfile {
 uint64_t us[Q38_FFN_HOST_PHASES]={};
 uint32_t calls[Q38_FFN_HOST_PHASES]={},polls=0,invalid=0;
 void Clear(){for(auto& n:us)n=0;for(auto& n:calls)n=0;polls=invalid=0;}
 bool Pristine()const{
  if(polls || invalid)return false;
  for(unsigned i=0;i<Q38_FFN_HOST_PHASES;i++)if(us[i] || calls[i])return false;
  return true;
 }
 void Add(unsigned phase,uint64_t before,uint64_t after){
  if(phase>=Q38_FFN_HOST_PHASES || after<before){invalid=1;return;}
  uint64_t delta=after-before;
  if(us[phase]>UINT64_MAX-delta || calls[phase]==UINT32_MAX){invalid=1;return;}
  us[phase]+=delta;calls[phase]++;
 }
 uint64_t Total()const{
  if(invalid)return 0;
  uint64_t sum=0;for(auto n:us){if(sum>UINT64_MAX-n)return 0;sum+=n;}return sum;
 }
 bool Complete()const{
  if(invalid || !polls)return false;
  uint64_t sum=0;
  for(unsigned i=0;i<Q38_FFN_HOST_PHASES;i++){
   if(calls[i]!=1 || sum>UINT64_MAX-us[i])return false;
   sum+=us[i];
  }
  return true;
 }
};
#endif
