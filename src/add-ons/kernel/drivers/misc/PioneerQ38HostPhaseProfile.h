/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HOST_PHASE_PROFILE_H
#define PIONEER_Q38_HOST_PHASE_PROFILE_H
#include <stdint.h>
// CPU diagnostic storage ONLY. No transport,clock,completion/cleanup
// authority. 119-worker public-model shapes only;other shapes counted as
// ignored. Invalid/overflowed measurements are sticky,NEVER affect RUN.
class PioneerQ38HostPhaseProfile {
public:
 struct Bucket {uint64_t count=0,first[8]={},warm[8]={};bool valid=true;};
private:
 Bucket bucket[9];
 uint64_t ignored=0;
public:
 void Add(unsigned type,unsigned inputs,unsigned peers,bool valid,const uint64_t* times) {
  unsigned t=type==8?0:type==12?1:type==14?2:3;
  unsigned k=inputs==5120?0:inputs==6144?1:inputs==17408?2:3;
  if(t==3 || k==3 || peers!=119) {if(ignored!=UINT64_MAX)ignored++;return;}
  auto& b=bucket[t*3+k];if(!b.valid)return;
  if(!valid || !times || b.count==UINT64_MAX){b.valid=false;return;}
  if(b.count)for(unsigned i=0;i<8;i++)if(times[i]>UINT64_MAX-b.warm[i]){b.valid=false;return;}
  for(unsigned i=0;i<8;i++)if(b.count)b.warm[i]+=times[i];else b.first[i]=times[i];
  b.count++;
 }
 const Bucket& At(unsigned type,unsigned k)const{return bucket[type*3+k];} // private fixed loops ONLY
 uint64_t Ignored()const{return ignored;}
};
#endif
