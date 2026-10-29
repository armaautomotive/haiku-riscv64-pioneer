/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_RECORD_SHAPE_H
#define PIONEER_Q38_TOKEN_RECORD_SHAPE_H
#include "PioneerQ38TokenLayout.h"
// Published-record DATA grammar ONLY. The caller independently owes the
// genuine terminal/progress,source identity,owned STOP/HOLD,immutable GDDR,
// current NoC receipts and sequential prompt/greedy input chain. Historical
// record counters are DATA,never a current hardware baseline or END grant.
static inline bool PioneerQ38TokenRecordShape(const uint32_t* r,const uint32_t* boot,const uint32_t* terminal,
 unsigned position,unsigned input,unsigned limit,unsigned prompts,bool stopEos,unsigned eos0,unsigned eos1){
 namespace T=PioneerQ38TokenLayout;
 if(!r || !boot || !terminal || !limit || limit>T::Context || !prompts || prompts>limit
  || input>=248320 || eos0>=248320 || eos1>=248320 || position>=limit
  || terminal[0]!=T::TerminalMagic || terminal[1]!=1 || terminal[2]<1 || terminal[2]>3
  || !terminal[3] || terminal[3]>limit || position>=terminal[3] || terminal[4]>=limit
  || r[0]!=T::TokenMagic || r[1]!=1 || r[2]!=position || r[3]!=input || r[4]>=248320
  || (r[5]&0x7f800000U)==0x7f800000U || r[6]!=position+1 || r[7]!=((position+1)&1)
  || r[8]!=unsigned(position+1<prompts) || r[10]!=64 || r[11]!=973
  || r[38]!=r[42] || r[44]!=position+1 || r[45]!=48 || r[46]!=119)return false;
 const bool limitEnd=position+1==limit;
 const bool eosEnd=stopEos && position+1>=prompts && (r[4]==eos0 || r[4]==eos1);
 const bool end=limitEnd || eosEnd,last=position+1==terminal[3];
 if(r[9]!=unsigned(!end) || (end && !last))return false;
 if(last){
  if(terminal[2]==1){if(!limitEnd || terminal[4]!=position)return false;}
  else if(terminal[2]==2){if(limitEnd || !eosEnd || terminal[4]!=position)return false;}
  else{
   if(!terminal[10] || terminal[10]>3)return false;
   if(terminal[10]==3){if(terminal[4]!=position)return false;}
   else if(end || terminal[4]!=position+1)return false;
  }
 }
 for(unsigned i=0;i<8;i++)if(r[12+i]!=boot[8+i])return false;
 for(unsigned i=0;i<16;i++)if(r[20+i]!=boot[16+i])return false;
 for(unsigned i=47;i<64;i++)if(r[i])return false;
 return true;
}
#endif
