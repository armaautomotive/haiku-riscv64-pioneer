/* SPDX-License-Identifier: MIT */
// OFFLINE fixed-local native projection producer;NO device/driver selector.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38ProjectionInput.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
extern "C" unsigned scalar_main()
{
 const volatile uint32_t* job=reinterpret_cast<const volatile uint32_t*>(0x74000);
 if(job[0]!=0x31495051 || job[1]!=256 || (job[2]!=8 && job[2]!=12 && job[2]!=14) || job[3]!=576)return 0;
 if(!PioneerQ38ProjectionQuantize(job[2],reinterpret_cast<const float*>(0x90000),256,
  reinterpret_cast<PioneerQ38ProjectionInput*>(0x90c00)))scalar_fault_park();
 asm volatile("fence" ::: "memory");return 1;
}
