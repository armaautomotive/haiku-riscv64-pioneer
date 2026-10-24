/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_RENDEZVOUS_SHAPE_H
#define PIONEER_Q38_GDN_RENDEZVOUS_SHAPE_H
#include "PioneerQ38GdnGatedShape.h"
// Separate timed-rendezvous DATA only,not a clock/source/native grant.
// At the SAME native owner's verified <=1400MHz clock,84G cycles are at
// least60seconds. Host MUST bound the ENTIRE release+compute at30seconds.
// No caller can supply/extend the fixed clock budget or clock-register IDs.
enum {TT_Q38_GDN_RENDEZVOUS_MARKER=0x31544751,TT_Q38_GDN_RENDEZVOUS_FAULT=0x46544751,
 TT_Q38_GDN_RENDEZVOUS_POLLS=0x7fffffff,TT_Q38_GDN_RENDEZVOUS_CLOCK=0x74d00};
typedef __UINT64_TYPE__ PioneerQ38RendezvousCycles;
static inline PioneerQ38RendezvousCycles PioneerQ38GdnRendezvousBudget(){return 84000000000ULL;}
static inline bool PioneerQ38GdnRendezvousShape(const uint32_t* d,const uint32_t* input,const uint32_t* state){
 if(!d || d[0]!=TT_Q38_GDN_RENDEZVOUS_MARKER || d[51]!=TT_Q38_GDN_RENDEZVOUS_POLLS
  || d[52]!=uint32_t(PioneerQ38GdnRendezvousBudget()) || d[53]!=uint32_t(PioneerQ38GdnRendezvousBudget()>>32)
  || d[54]!=4 || d[55]!=TT_Q38_GDN_RENDEZVOUS_CLOCK)return false;
 uint32_t old[64];for(unsigned i=0;i<64;i++)old[i]=d[i];
 old[0]=TT_Q38_GDN_GATED_MARKER;old[51]=TT_Q38_GDN_GATED_POLLS;for(unsigned i=52;i<56;i++)old[i]=0;
 return PioneerQ38GdnGatedShape(old,input,state); // identical canonical routing/epochs/extents
}
static inline uint32_t PioneerQ38GdnRendezvousReadyWord(const uint32_t* d,unsigned i,bool peer){
 return i?PioneerQ38GdnGatedReadyWord(d,i,peer):uint32_t(TT_Q38_GDN_RENDEZVOUS_MARKER);
}
static inline bool PioneerQ38GdnRendezvousClockValue(PioneerQ38RendezvousCycles begin,PioneerQ38RendezvousCycles previous,PioneerQ38RendezvousCycles now){
 return now>previous && previous>=begin && now-begin<PioneerQ38GdnRendezvousBudget();
} // clock uncertainty/rollover is a refusal,never a deadline extension
static inline bool PioneerQ38GdnRendezvousSummary(const uint32_t* s,const uint32_t* d,const uint32_t* input,
 const uint32_t* state,const uint32_t* before,const uint32_t* after){
 if(!s || !before || !after || !PioneerQ38GdnRendezvousShape(d,input,state)
  || !s[5] || s[5]>TT_Q38_GDN_RENDEZVOUS_POLLS || before[2]!=before[6] || after[2]!=after[6])return false;
 unsigned polls=s[5];const unsigned first[]={polls+3,polls+20,1,4,64,polls,d[6],TT_Q38_GDN_RENDEZVOUS_MARKER};
 const unsigned delta[]={polls+3,polls+20,polls+4,polls+3,1,4,polls+4,1};
 for(unsigned i=0;i<8;i++)if(s[i]!=first[i] || s[8+i]!=before[i] || s[16+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i])return false;
 const unsigned tail[]={d[7],d[5],d[40],d[15],d[4]+d[43],d[3],d[11],d[12]};
 for(unsigned i=0;i<8;i++)if(s[24+i]!=tail[i])return false;
 return true;
}
static inline bool PioneerQ38GdnRendezvousClockReceipt(const uint32_t* c,const uint32_t* d,unsigned polls){
 if(!c || !d || c[0]!=TT_Q38_GDN_RENDEZVOUS_MARKER || c[5]!=polls || c[6]!=d[6] || c[7]!=d[7])return false;
 PioneerQ38RendezvousCycles begin=PioneerQ38RendezvousCycles(c[2])<<32|c[1],end=PioneerQ38RendezvousCycles(c[4])<<32|c[3];
 if(!PioneerQ38GdnRendezvousClockValue(begin,begin,end))return false;
 for(unsigned i=8;i<16;i++)if(c[i]!=d[24+i])return false;
 return true; // timing DATA,never terminal/owner/ready/cleanup authority
}
#endif
