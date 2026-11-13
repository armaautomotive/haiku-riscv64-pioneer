/* SPDX-License-Identifier: MIT */
// Standalone offline RV32 controller DATA experiment, not a device owner.
typedef __UINT32_TYPE__ uint32_t;
#include "PioneerQ38RecurrentSmallMatrixSchedule.h"
extern "C" void* memset(void* destination,int value,__SIZE_TYPE__ bytes){
 if((reinterpret_cast<__UINTPTR_TYPE__>(destination)|bytes)&3)for(;;){}
 const uint32_t b=unsigned(value)&255U,word=b|(b<<8)|(b<<16)|(b<<24);auto* p=static_cast<uint32_t*>(destination);
 for(__SIZE_TYPE__ i=0;i<bytes/4;i++)p[i]=word;
 return destination;
}
extern "C" void* memcpy(void* destination,const void* source,__SIZE_TYPE__ bytes){
 if((reinterpret_cast<__UINTPTR_TYPE__>(destination)|reinterpret_cast<__UINTPTR_TYPE__>(source)|bytes)&3)for(;;){}
 auto* d=static_cast<uint32_t*>(destination);const auto* s=static_cast<const uint32_t*>(source);
 for(__SIZE_TYPE__ i=0;i<bytes/4;i++)d[i]=s[i];
 return destination;
}
extern "C" __attribute__((noinline,used)) unsigned q38_small_plan(unsigned layer,unsigned position,unsigned invocation,
 const uint32_t* bases,const uint32_t* banks,const uint32_t boot[4][64],const uint32_t* previous,
 PioneerQ38RecurrentSmallMatrixSchedule::Part* output){
 return PioneerQ38RecurrentSmallMatrixSchedule::Build(layer,position,invocation,bases,banks,boot,previous,output);
}
extern "C" __attribute__((noinline,used)) unsigned q38_small_copy(const PioneerQ38RecurrentSmallMatrixSchedule::Part* part,
 const uint32_t* source,unsigned count,uint32_t* output,unsigned capacity){
 return part && PioneerQ38RecurrentSmallMatrixSchedule::CopyRealRows(*part,source,count,output,capacity);
}
