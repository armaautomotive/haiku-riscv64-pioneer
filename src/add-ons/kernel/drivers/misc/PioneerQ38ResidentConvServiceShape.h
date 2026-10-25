/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_CONV_SERVICE_SHAPE_H
#define PIONEER_Q38_RESIDENT_CONV_SERVICE_SHAPE_H
#include "PioneerQ38ConvBatchShape.h"
#include "PioneerQ38FfnPersistentShape.h"
// OFFLINE service DATA only. Reuses the FFN sequence envelope but adds a
// separately pinned op7. Existing peer/root images MUST NOT select it.
// This is NOT source/init/cohort/producer/physical execution authority.
enum {TT_Q38_RESIDENT_CONV_SERVICE_OPERATION=7,TT_Q38_RESIDENT_CONV_SERVICE_JOB=0x31435051};
static inline bool PioneerQ38ResidentConvServiceJob(const uint32_t* j,const uint32_t* table,
 const uint32_t* source,unsigned worker,unsigned self){
 if(!j || !source || !worker || worker>=120 || self!=PioneerQ38ConvPeer(worker)
  || !PioneerQ38ConvBatchShape(table) || table[10]!=worker || table[9]!=self
  || j[0]!=TT_Q38_RESIDENT_CONV_SERVICE_JOB || j[1]!=PioneerQ38ConvBatchCount(worker)
  || j[2]!=1536 || !j[3] || j[3]>=0x7fffffff || j[3]!=table[6]
  || j[10]!=TT_Q38_FFN_PERSISTENT_MARKER || j[11]!=worker || !j[12] || j[12]>=0x7fffffff
  || j[12]>j[3] || j[13]!=TT_Q38_RESIDENT_CONV_SERVICE_OPERATION || j[14] || j[15])return false;
 for(unsigned i=4;i<10;i++)if(j[i])return false;
 for(unsigned i=0;i<16;i++)if(table[16+i]!=source[i])return false;
 return true;
}
static inline bool PioneerQ38ResidentConvServiceProof(const uint32_t* p,const uint32_t* j,const uint32_t* table,
 const uint32_t* source,unsigned worker,unsigned self,const uint32_t* before,const uint32_t* after,const uint32_t* summary){
 if(!p || !before || !after || !PioneerQ38ResidentConvServiceJob(j,table,source,worker,self)
  || !PioneerQ38ConvBatchSummary(summary,table,before,after))return false;
 unsigned n=PioneerQ38ConvBatchCount(worker);const auto* last=table+(n-1)*64;
 for(unsigned i=0;i<8;i++)if(p[i]!=before[i] || p[8+i]!=after[i])return false;
 const uint32_t command[]={0x81000,0,self,last[4]+last[43],0,last[8],0,0x212,64,0};
 for(unsigned i=0;i<10;i++)if(p[20+i]!=command[i])return false;
 if(p[16]!=16*n || p[17]!=j[12] || p[18]!=worker || p[19]!=TT_Q38_FFN_PERSISTENT_MARKER
  || p[30]!=source[0] || p[31]!=self || p[48]!=TT_Q38_RESIDENT_CONV_SERVICE_OPERATION
  || p[49]!=table[6] || p[50]!=table[5] || p[51]!=table[7] || p[52]!=table[40] || p[53]
  || p[62]!=TT_Q38_CONV_BATCH_MARKER || p[63]!=n)return false;
 for(unsigned i=0;i<16;i++)if(p[32+i]!=source[i])return false;
 for(unsigned i=0;i<8;i++)if(p[54+i]!=table[32+i])return false;
 return true; // DATA only,never a native completion/cleanup or model grant
}
#endif
