/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_GDN_SERVICE_SHAPE_H
#define PIONEER_Q38_RESIDENT_GDN_SERVICE_SHAPE_H
#include "PioneerQ38GdnRendezvousShape.h"
#include "PioneerQ38FfnPersistentShape.h"
// OFFLINE returning-service DATA. Not a native/source/initialized-state,
// paired-cohort/clock/producer or token-commit capability.
enum {TT_Q38_RESIDENT_GDN_SERVICE_OPERATION=8,TT_Q38_RESIDENT_GDN_SERVICE_JOB=0x31534451};
static inline bool PioneerQ38ResidentGdnServiceJob(const uint32_t* j,const uint32_t* d,const uint32_t* input,
 const uint32_t* state,const uint32_t* source,unsigned worker,unsigned self){
 if(!j || !source || !worker || worker>96 || self!=PioneerQ38ResidentGdnPeer(worker-1)
  || !PioneerQ38GdnRendezvousShape(d,input,state) || d[7]!=worker-1 || d[9]!=self
  || j[0]!=TT_Q38_RESIDENT_GDN_SERVICE_JOB || j[1]!=64 || j[2]!=768 || j[3]!=d[6]
  || j[10]!=TT_Q38_FFN_PERSISTENT_MARKER || j[11]!=worker || !j[12] || j[12]>=0x7fffffff
  || j[12]>j[3] || j[13]!=TT_Q38_RESIDENT_GDN_SERVICE_OPERATION || j[14] || j[15])return false;
 for(unsigned i=4;i<10;i++)if(j[i])return false;
 for(unsigned i=0;i<16;i++)if(d[16+i]!=source[i])return false;
 return true;
}
static inline bool PioneerQ38ResidentGdnServiceSummaries(const uint32_t* d,const uint32_t* input,const uint32_t* state,
 const uint32_t* first,const uint32_t* recurrent,const uint32_t* gated,const uint32_t* clock,
 const uint32_t* before,const uint32_t* after){
 if(!first || !recurrent || !gated || !clock || !before || !after
  || !PioneerQ38GdnInputSummary(first,input,before,first+16)
  || !PioneerQ38ResidentGdnSummary(recurrent,state,first+16,recurrent+16)
  || !PioneerQ38GdnRendezvousSummary(gated,d,input,state,recurrent+16,after)
  || !PioneerQ38GdnRendezvousClockReceipt(clock,d,gated[5]))return false;
 return true; // exact counter continuity across all THREE actual functions
}
static inline bool PioneerQ38ResidentGdnServiceProof(const uint32_t* p,const uint32_t* j,const uint32_t* d,
 const uint32_t* input,const uint32_t* state,const uint32_t* source,unsigned worker,unsigned self,
 const uint32_t* before,const uint32_t* after,const uint32_t* first,const uint32_t* recurrent,
 const uint32_t* gated,const uint32_t* clock){
 if(!p || !PioneerQ38ResidentGdnServiceJob(j,d,input,state,source,worker,self)
  || !PioneerQ38ResidentGdnServiceSummaries(d,input,state,first,recurrent,gated,clock,before,after))return false;
 for(unsigned i=0;i<8;i++)if(p[i]!=before[i] || p[8+i]!=after[i])return false;
 const uint32_t command[]={0x8ea00,0,self,d[4]+d[43],0,d[8],0,0x212,256,0};
 for(unsigned i=0;i<10;i++)if(p[20+i]!=command[i])return false;
 if(p[16]!=64 || p[17]!=j[12] || p[18]!=worker || p[19]!=TT_Q38_FFN_PERSISTENT_MARKER
  || p[30]!=source[0] || p[31]!=self || p[48]!=TT_Q38_RESIDENT_GDN_SERVICE_OPERATION
  || p[49]!=d[6] || p[50]!=d[5] || p[51]!=d[7] || p[52]!=d[40] || p[53]!=state[11]
  || p[62]!=TT_Q38_GDN_RENDEZVOUS_MARKER || p[63]!=gated[5])return false;
 for(unsigned i=0;i<16;i++)if(p[32+i]!=source[i])return false;
 for(unsigned i=0;i<8;i++)if(p[54+i]!=d[32+i])return false;
 return true; // receipt DATA only,never native/current-slot/token publication
}
#endif
