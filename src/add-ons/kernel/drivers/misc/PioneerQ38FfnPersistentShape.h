/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_PERSISTENT_SHAPE_H
#define PIONEER_Q38_FFN_PERSISTENT_SHAPE_H
#include "PioneerQ38FfnResidentShape.h"
// NEW OFFLINE private generation;NOT compatible with the old peer/root
// registry and NEVER an ioctl,producer capability or lifetime grant.
enum {TT_Q38_FFN_PERSISTENT_MARKER=0x31504651,TT_Q38_FFN_PERSISTENT_FAULT=0x46504651,
 TT_Q38_FFN_PERSISTENT_ACTIVATION=3,TT_Q38_FFN_PERSISTENT_JOB=0x31414651};
static inline bool PioneerQ38FfnPersistentActivationShape(const uint32_t* j,const uint32_t* d,
 const uint32_t* source,unsigned worker,unsigned self)
{
 if(!j || !d || !source || !worker || worker>68 || self!=PioneerQ38FfnResidentPeer(worker-1)
  || j[0]!=TT_Q38_FFN_PERSISTENT_JOB || j[1]!=256 || j[2]!=576 || !j[3] || j[3]>=0x7fffffff
  || j[10]!=TT_Q38_FFN_PERSISTENT_MARKER || j[11]!=worker || !j[12] || j[12]>=0x7fffffff
  || j[12]>j[3] || j[13]!=TT_Q38_FFN_PERSISTENT_ACTIVATION || j[14] || j[15]
  || !PioneerQ38FfnResidentShape(d) || d[5]!=worker-1 || d[6]!=j[3] || d[7]!=worker || d[9]!=self)return false;
 for(unsigned i=4;i<10;i++)if(j[i])return false;
 for(unsigned i=0;i<16;i++)if(d[16+i]!=source[i])return false;
 return true; // DATA relation only,not proof of producer/coordinator ownership
}
static inline bool PioneerQ38FfnPersistentActivationProof(const uint32_t* p,const uint32_t* j,const uint32_t* d,
 const uint32_t* source,unsigned worker,unsigned self,const uint32_t* before,const uint32_t* after)
{
 if(!p || !before || !after || !PioneerQ38FfnPersistentActivationShape(j,d,source,worker,self)
  || before[2]!=before[6] || after[2]!=after[6])return false;
 const unsigned delta[]={2,32,4,2,2,9,4,2};
 for(unsigned i=0;i<8;i++)if(p[i]!=before[i] || p[8+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i])return false;
 if(p[16]!=256 || p[17]!=j[12] || p[18]!=worker || p[19]!=TT_Q38_FFN_PERSISTENT_MARKER
  || p[30]!=source[0] || p[31]!=self || p[48]!=3 || p[49]!=d[6] || p[50]!=d[5] || p[51]!=d[42]
  || p[52]!=d[40] || p[53]!=d[41] || p[62]!=TT_Q38_FFN_RESIDENT_MARKER || p[63]!=1)return false;
 const uint32_t command[]={0x90e00,0,d[9],d[4]+d[13],0,d[8],0,0x212,64,0};
 for(unsigned i=0;i<10;i++)if(p[20+i]!=command[i])return false;
 for(unsigned i=0;i<16;i++)if(p[32+i]!=source[i])return false;
 for(unsigned i=0;i<8;i++)if(p[54+i]!=d[32+i])return false;
 return true; // DATA ONLY:all physical/current participants/owners still owed
}
#endif
