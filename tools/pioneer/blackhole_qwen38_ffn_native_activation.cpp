/* SPDX-License-Identifier: MIT */
// OFFLINE fixed-local candidate ONLY,not installed or driver-selected.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38FfnNativeInput.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
extern "C" unsigned scalar_main()
{
 const volatile uint32_t* job=reinterpret_cast<const volatile uint32_t*>(0x74000);
 if(job[0]!=0x4e384651 || job[1]!=256 || job[2]!=576 || job[3])return 0;
 if(!PioneerQ38FfnNativeActivation(reinterpret_cast<const float*>(0x90000),
  reinterpret_cast<const float*>(0x90400),256,reinterpret_cast<float*>(0x90800),
  reinterpret_cast<PioneerQ38FfnNativeInput*>(0x90c00)))scalar_fault_park();
 asm volatile("fence" ::: "memory");return 1;
}
