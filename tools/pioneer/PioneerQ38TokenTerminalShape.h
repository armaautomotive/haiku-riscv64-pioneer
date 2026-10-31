/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_TERMINAL_SHAPE_H
#define PIONEER_Q38_TOKEN_TERMINAL_SHAPE_H
#include "PioneerQ38TokenLayout.h"
// Equality of two already-read terminal DATA records only. In a native
// recheck the previous record must belong to the SAME private owner and
// actual accepted observation;this helper cannot supply that authority.
// A regressed/unpublished or changed record is never a pending-PC excuse.
static inline bool PioneerQ38TokenTerminalRecheckData(const uint32_t* current,const uint32_t* previous,uint32_t& word){
 if(!current || !previous)return false;
 if(current[0]!=PioneerQ38TokenLayout::TerminalMagic || previous[0]!=PioneerQ38TokenLayout::TerminalMagic){word=0;return false;}
 if(current[1]!=1 || previous[1]!=1){word=1;return false;}
 for(unsigned i=0;i<64;i++)if(current[i]!=previous[i]){word=i;return false;}
 return true;
}
// DATA grammar only. The caller must separately authenticate the session,
// source identity, SAME running native owner, coherent firmware publication,
// live counters, ROOT STOP and all119 actual worker STOP receipts. This
// function grants NO initialization/token/STOP/restore/END/Work capability.
static inline bool PioneerQ38TokenTerminalShape(const uint32_t* t,const uint32_t* boot,
 const uint32_t* progress,unsigned limit,unsigned prompts,bool stopEos,bool cancelRequested){
 namespace T=PioneerQ38TokenLayout;
 if(!t || !boot || !progress || !limit || limit>T::Context || !prompts || prompts>limit
  || t[0]!=T::TerminalMagic || t[1]!=1 || t[2]<1 || t[2]>3 || t[3]>limit || t[4]>=limit
  || t[5]!=119 || !t[6] || t[6]>64 || t[8]>1 || t[9]!=1 || t[11]
  || progress[0]!=T::Magic || progress[1]!=t[4] || progress[2]!=t[6])return false;
 if(t[2]==3){
  if(!cancelRequested || !t[10] || t[10]>3 || t[3]!=t[4]+unsigned(t[10]==3)
   || t[8]!=(t[3]&1) || (t[10]==1?t[7]!=0:(t[6]!=64 || t[7]!=973)))return false;
 }else{
  if(t[10] || t[3]!=t[4]+1 || t[6]!=64 || t[7]!=973 || t[8]!=(t[3]&1)
   || (t[2]==1?t[3]!=limit:(!stopEos || t[3]<prompts || t[3]>=limit)))return false;
 }
 for(unsigned i=0;i<8;i++)if(t[12+i]!=boot[8+i])return false;
 for(unsigned i=0;i<16;i++)if(t[20+i]!=boot[16+i])return false;
 if(t[38]!=t[42])return false; // physical counter aliases,never a new baseline
 for(unsigned i=44;i<64;i++)if(t[i])return false;
 return true;
}
#endif
