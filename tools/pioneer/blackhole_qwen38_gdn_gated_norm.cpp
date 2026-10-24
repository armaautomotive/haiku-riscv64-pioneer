/* SPDX-License-Identifier: MIT */
// OFFLINE LOCAL QGN1 qualifier. NOT a native/source/ownership/read grant.
#define PIONEER_ATTENTION_FREESTANDING
#define PIONEER_Q38_GDN_GATED_NORM_IMAGE 1
#define PIONEER_Q38_GDN_PREPARE_IMAGE 1
#include "PioneerQ38GdnGatedNorm.h"
static volatile uint32_t* Words(unsigned at){return reinterpret_cast<volatile uint32_t*>(at);}
extern "C" void scalar_fault_park() __attribute__((noreturn));
extern "C" unsigned scalar_main(){
 if(Words(0x74600)[0]!=0x314e4751 || Words(0x74600)[1]!=128 || Words(0x74600)[2]!=64 || Words(0x74600)[3]>=96)return 0;
 for(unsigned i=4;i<32;i++)if(Words(0x74600)[i])return 0;
 Words(0x74000)[0]=0; // invalidate stale output seal BEFORE any arithmetic
 if(!PioneerQ38GdnGatedNormHalf(reinterpret_cast<const float*>(0x80000),reinterpret_cast<const float*>(0x80200),
  reinterpret_cast<const float*>(0x80400),Words(0x74600)[3]&1,reinterpret_cast<float*>(0x8ea00)))scalar_fault_park();
 Words(0x74000)[1]=128;Words(0x74000)[2]=64;Words(0x74000)[3]=Words(0x74600)[3];
 asm volatile("fence" ::: "memory");Words(0x74000)[0]=0x314e4751;
 asm volatile("fence" ::: "memory");return 64;
}
