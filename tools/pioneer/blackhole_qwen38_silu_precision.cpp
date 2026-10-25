/* SPDX-License-Identifier: MIT */
// OFFLINE LOCAL arithmetic image only;NO native selector/device ownership.
#define PIONEER_ATTENTION_FREESTANDING
#if !defined(PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE) && !defined(PIONEER_Q38_SILU_PRECISION_IMAGE)
#define PIONEER_Q38_SILU_PRECISION_IMAGE 1
#endif
#include "PioneerQ38SiluIsolation.h"
#ifdef PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE
#if PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE != 1
#error Exact LOCAL legacy arithmetic control required
#endif
#include "PioneerGraphMath.h"
#else
#include "PioneerQ38SiluPrecision.h"
#endif
static volatile uint32_t* Words(unsigned at){return reinterpret_cast<volatile uint32_t*>(at);}
extern "C" void scalar_fault_park() __attribute__((noreturn));
extern "C" unsigned scalar_main(){
 if(Words(0x74600)[0]!=0x31505351 || Words(0x74600)[1]!=64 || Words(0x74600)[2]!=1)return 0;
 for(unsigned i=3;i<32;i++)if(Words(0x74600)[i])return 0;
 float result[64];
 for(unsigned i=0;i<64;i++){
  float input=PioneerAttentionFloat(Words(0x80000)[i]);
#ifdef PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE
  uint32_t magnitude=PioneerAttentionBits(input)&0x7fffffff;
  if(magnitude>=0x7f800000 || (magnitude && magnitude<0x00800000))scalar_fault_park();
  result[i]=PioneerGraphSilu(input); // exact existing arithmetic,diagnostic control ONLY
#else
  if(!PioneerQ38SiluPrecision(input,result[i]))scalar_fault_park();
#endif
 }
 // ALL arithmetic/domain checks BEFORE any output. LOCAL bytes only,not
 // a validated native producer or a retained source/NoC/clock capability.
 for(unsigned i=0;i<64;i++)Words(0x81000)[i]=PioneerAttentionBits(result[i]);
 asm volatile("fence" ::: "memory");return 64;
}
