/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_RECURRENT_SHAPE_H
#define PIONEER_Q38_GDN_RECURRENT_SHAPE_H
#include "PioneerQ38GdnInputShape.h"
#include "PioneerQ38ResidentGdnShape.h"
// OFFLINE paired descriptor DATA only. Neither canonical geometry nor
// matching epochs grant initialized-state/native/compute authority.
enum { TT_Q38_GDN_RECURRENT_MARKER=0x314a5251,TT_Q38_GDN_RECURRENT_FAULT=0x464a5251 };
static inline bool PioneerQ38GdnRecurrentSeparate(unsigned bank,unsigned at,unsigned bytes,
 unsigned otherBank,unsigned otherAt,unsigned otherBytes){
 return bank!=otherBank || at+bytes<=otherAt || otherAt+otherBytes<=at;
} // callers first prove bounded,nonwrapping shapes
static inline bool PioneerQ38GdnRecurrentPairing(const uint32_t* input,const uint32_t* state){
 // PRIVATE DATA helper:caller MUST first validate BOTH complete shapes.
 if(input[5]!=state[10] || input[6]!=state[8] || input[7]!=state[9] || input[9]!=state[15]
  || input[40]!=state[40] || state[41])return false;
 for(unsigned i=16;i<40;i++)if(input[i]!=state[i])return false;
 // BOTH current/next state are disjoint from the raw-input workspace and
 // learned A/dt scalars. A private next write can never corrupt this seam's
 // inputs,even when all extents share a bank. No address is caller authority.
 for(unsigned side=0;side<2;side++){
  unsigned bank=state[4+side*2],at=state[5+side*2];
  if(!PioneerQ38GdnRecurrentSeparate(bank,at,32768,input[3],input[4],67108864)
   || !PioneerQ38GdnRecurrentSeparate(bank,at,32768,input[11],input[12],64)
   || !PioneerQ38GdnRecurrentSeparate(bank,at,32768,input[14],input[15],64))return false;
 }
 return true;
}
static inline bool PioneerQ38GdnRecurrentShape(const uint32_t* input,const uint32_t* state){
 return PioneerQ38GdnInputShape(input) && PioneerQ38ResidentGdnShape(state) && PioneerQ38GdnRecurrentPairing(input,state);
}
#endif
