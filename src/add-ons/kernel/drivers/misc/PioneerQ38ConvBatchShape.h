/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_CONV_BATCH_SHAPE_H
#define PIONEER_Q38_CONV_BATCH_SHAPE_H
#include "PioneerQ38ConvShape.h"
// QCB1 DATA only. ALL640groups partitioned over120physical coordinates;
// one card invocation consumes5or6jobs. No host relaunch between groups.
// Never source/init/private-producer/native ownership/launch authority.
enum {TT_Q38_CONV_BATCH_MARKER=0x31424351,TT_Q38_CONV_BATCH_FAULT=0x46424351,
 TT_Q38_CONV_BATCH_TABLE=0x74800,TT_Q38_CONV_BATCH_RECORDS=6,TT_Q38_CONV_BATCH_WORDS=384};
static inline unsigned PioneerQ38ConvBatchCount(unsigned worker){
 if(worker>=120)return 0;
 unsigned count=0;for(unsigned group=worker;group<640;group+=120)count++;
 return count; // bounded subtraction/addition,NO integer divide
}
static inline bool PioneerQ38ConvBatchDisjoint(unsigned a,unsigned bytes,unsigned b,unsigned other){
 return a+bytes<=b || b+other<=a; // addresses already range-checked by QCV1
}
static inline bool PioneerQ38ConvBatchShape(const uint32_t* table){
 if(!table || !PioneerQ38ConvShape(table) || table[7]!=table[10]*16)return false;
 unsigned count=PioneerQ38ConvBatchCount(table[10]);if(count<5 || count>6)return false;
 for(unsigned j=0;j<6;j++){
  const auto* d=table+j*64;
  if(j>=count){for(unsigned i=0;i<64;i++)if(d[i])return false;continue;}
  if(!PioneerQ38ConvShape(d) || d[7]!=(table[10]+j*120)*16)return false;
  for(unsigned i=0;i<64;i++){
   bool varies=i==7 || (i>=11 && i<=15) || i==41 || i==42 || i==43 || i==45;
   if(!varies && d[i]!=table[i])return false;
  }
  for(unsigned k=0;k<j;k++){
   const auto* p=table+k*64;
   if((d[11]==p[11] && !PioneerQ38ConvBatchDisjoint(d[12],256,p[12],256))
    || (d[14]==p[14] && !PioneerQ38ConvBatchDisjoint(d[15],192,p[15],192))
    || (d[11]==p[14] && !PioneerQ38ConvBatchDisjoint(d[12],256,p[15],192))
    || (d[14]==p[11] && !PioneerQ38ConvBatchDisjoint(d[15],192,p[12],256)))return false;
  }
 }
 return true;
}
static inline bool PioneerQ38ConvBatchSummary(const uint32_t* s,const uint32_t* table,const uint32_t* before,const uint32_t* after){
 if(!s || !before || !after || !PioneerQ38ConvBatchShape(table) || before[2]!=before[6] || after[2]!=after[6])return false;
 unsigned n=PioneerQ38ConvBatchCount(table[10]);const unsigned delta[]={3*n,8*n,5*n,3*n,2*n,4*n,5*n,2*n};
 const unsigned first[]={3*n,8*n,2*n,4*n,16*n,256*n,table[6],TT_Q38_CONV_BATCH_MARKER};
 for(unsigned i=0;i<8;i++)if(s[i]!=first[i] || s[8+i]!=before[i] || s[16+i]!=after[i]
  || uint32_t(after[i]-before[i])!=delta[i])return false;
 const unsigned tail[]={table[10],table[5],table[40],n,table[7],table[(n-1)*64+7],TT_Q38_CONV_NEXT,TT_Q38_CONV_OUTPUT};
 for(unsigned i=0;i<8;i++)if(s[24+i]!=tail[i])return false;
 return true; // not independent physical completion/hold/owner proof
}
#endif
