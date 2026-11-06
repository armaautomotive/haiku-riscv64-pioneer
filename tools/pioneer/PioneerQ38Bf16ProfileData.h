/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BF16_PROFILE_DATA_H
#define PIONEER_Q38_BF16_PROFILE_DATA_H
#if defined(__STDC_HOSTED__) && !__STDC_HOSTED__
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;
#else
#include <stdint.h>
#endif
// Observation DATA only. No frequency assumption,inference acceptance,
// execution grant or cleanup authority. Same reserved60 progress words.
namespace PioneerQ38Bf16ProfileData {
enum {Address=0x75710,Words=60,Magic=0x32485051,Version=2,
 Bootstrap=0,Metadata,Recurrent,Dense,HeadNormalize,HeadMatrix,Feedback,
 StateCommit,Publication,FinalStop,MatrixPrepare,MatrixDispatch,MatrixGatherWait,MatrixFinish,Phases,
 Active=1,Complete=2,Invalid=3,
 ClockUnstable=1,ClockBackward=2,Overflow=3,BadHeader=4,BadPhase=5,BadCommit=6,
 State=3,Current=4,Previous=5,BeginCycle=7,EndCycle=9,Transitions=11,
 Committed=12,Error=13,Buckets=16,Used=Buckets+Phases*3};
static_assert(Phases==14 && Used==58 && Address+Words*4==0x75800,"reserved progress DATA only");
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
#ifdef PIONEER_Q38_BF16_PROFILE_IMAGE
static __attribute__((noinline,unused,section(".bf16.profile")))
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
static inline unsigned Expected(unsigned phase,unsigned committed){
 return phase==Bootstrap || phase==FinalStop?1:
  committed*(phase==Metadata?64:phase==Recurrent?48*9:phase==Dense?16*8:phase>=MatrixPrepare?496:1);
}
static inline bool Closed(const uint32_t* p){
 if(!p || p[0]!=Magic || p[1]!=Version || p[2]!=Words || p[State]!=Complete
  || p[Current]!=FinalStop || !p[Committed] || p[Committed]>8192 || p[Error] || p[14] || p[15]
  || Get(p,EndCycle)!=Get(p,Previous) || Get(p,EndCycle)<=Get(p,BeginCycle))return false;
 for(unsigned i=Used;i<Words;i++)if(p[i])return false;
 uint64_t cycles=0;uint32_t segments=0;
 for(unsigned phase=0;phase<Phases;phase++){
  const unsigned bucket=Buckets+phase*3;const uint64_t delta=Get(p,bucket+1);
  if(p[bucket]!=Expected(phase,p[Committed]) || !delta || delta>~uint64_t(0)-cycles
   || p[bucket]>~uint32_t(0)-segments)return false;
  segments+=p[bucket];cycles+=delta;
 }
 return segments==p[Transitions] && cycles==Get(p,EndCycle)-Get(p,BeginCycle);
}
}
#endif
