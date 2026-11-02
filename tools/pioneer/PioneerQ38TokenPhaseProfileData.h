/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_PHASE_PROFILE_DATA_H
#define PIONEER_Q38_TOKEN_PHASE_PROFILE_DATA_H
#if defined(__STDC_HOSTED__) && !__STDC_HOSTED__
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;
#else
#include <stdint.h>
#endif
// Observation DATA only. No clock-frequency assumption, owner, completion,
// numerical acceptance, address selection or native launch capability.
namespace PioneerQ38TokenPhaseProfileData {
enum {Address=0x75710,Words=60,Magic=0x31485051,Version=1,
 Bootstrap=0,Metadata,Recurrent,Dense,HeadNormalize,HeadMatrix,Feedback,
 StateCommit,Publication,FinalStop,Phases,
 Active=1,Complete=2,Invalid=3,
 ClockUnstable=1,ClockBackward=2,Overflow=3,BadHeader=4,BadPhase=5,BadCommit=6,
 State=3,Current=4,Previous=5,BeginCycle=7,EndCycle=9,Transitions=11,
 Committed=12,Error=13,Buckets=16,Used=46};
static_assert(Buckets+Phases*3==Used && Used<=Words,"bounded timing record");
static_assert(Address==0x75700+16 && Address+Words*4==0x75800,
 "only unused progress words4..63;frontier and following Parts untouched");
static inline uint64_t Get(const volatile uint32_t* p,unsigned i){return uint64_t(p[i])|(uint64_t(p[i+1])<<32);}
static inline void Put(volatile uint32_t* p,unsigned i,uint64_t v){p[i]=uint32_t(v);p[i+1]=uint32_t(v>>32);}
static inline void Invalidate(volatile uint32_t* p,unsigned reason){p[Error]=reason;p[State]=Invalid;}
static inline void Initialize(volatile uint32_t* p,bool valid,uint64_t now){
 for(unsigned i=0;i<Words;i++)p[i]=0;
 p[0]=Magic;p[1]=Version;p[2]=Words;p[Current]=Bootstrap;
 Put(p,Previous,now);Put(p,BeginCycle,now);
 if(valid)p[State]=Active;else Invalidate(p,ClockUnstable);
}
static inline bool Running(volatile uint32_t* p){
 if(p[State]!=Active)return false;
 if(p[0]!=Magic || p[1]!=Version || p[2]!=Words || p[Current]>=Phases){Invalidate(p,BadHeader);return false;}
 return true;
}
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE
// Use space vacated by the unchanged StateSeal relocation; do not expand
// either existing ROM range or the owner's snapshot/journal geometry.
// The ROOT TU includes the enum/layout too, but only the sampler uses this
// routine. 'unused' permits that declaration without forcing dead code in.
static __attribute__((noinline,unused,section(".conv.q38_phase_advance")))
#else
static inline
#endif
void Advance(volatile uint32_t* p,bool valid,uint64_t now,unsigned next,unsigned committed=0){
 if(!Running(p))return;
 if(!valid){Invalidate(p,ClockUnstable);return;}
 if(next>Phases){Invalidate(p,BadPhase);return;}
 if(next==Phases && (p[Current]!=FinalStop || !committed || committed>8192)){Invalidate(p,BadCommit);return;}
 const uint64_t previous=Get(p,Previous);
 if(now<previous){Invalidate(p,ClockBackward);return;}
 const unsigned bucket=Buckets+p[Current]*3;
 const uint64_t delta=now-previous,total=Get(p,bucket+1);
 if(p[bucket]==~uint32_t(0) || p[Transitions]==~uint32_t(0) || delta>~uint64_t(0)-total){Invalidate(p,Overflow);return;}
 p[bucket]++;Put(p,bucket+1,total+delta);p[Transitions]++;Put(p,Previous,now);
 if(next==Phases){Put(p,EndCycle,now);p[Committed]=committed;p[State]=Complete;}
 else p[Current]=next;
}
// Strict closed telemetry grammar, not an inference acceptance predicate.
static inline bool Closed(const uint32_t* p){
 if(!p || p[0]!=Magic || p[1]!=Version || p[2]!=Words || p[State]!=Complete
  || p[Current]!=FinalStop || !p[Committed] || p[Committed]>8192 || p[Error] || p[14] || p[15]
  || Get(p,EndCycle)!=Get(p,Previous) || Get(p,EndCycle)<=Get(p,BeginCycle))return false;
 for(unsigned i=Used;i<Words;i++)if(p[i])return false;
 uint64_t cycles=0;uint32_t segments=0;
 for(unsigned phase=0;phase<Phases;phase++){
  const unsigned bucket=Buckets+phase*3;
  const unsigned expected=phase==Bootstrap || phase==FinalStop?1:
   p[Committed]*(phase==Metadata?64:phase==Recurrent?48:phase==Dense?16:1);
  const uint64_t delta=Get(p,bucket+1);
  if(p[bucket]!=expected || !delta || delta>~uint64_t(0)-cycles || p[bucket]>~uint32_t(0)-segments)return false;
  segments+=p[bucket];cycles+=delta;
 }
 return segments==p[Transitions] && cycles==Get(p,EndCycle)-Get(p,BeginCycle);
}
}
#endif
