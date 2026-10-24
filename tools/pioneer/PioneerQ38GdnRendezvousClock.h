/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_RENDEZVOUS_CLOCK_H
#define PIONEER_Q38_GDN_RENDEZVOUS_CLOCK_H
#if !defined(PIONEER_Q38_GDN_RENDEZVOUS_IMAGE) || PIONEER_Q38_GDN_RENDEZVOUS_IMAGE != 1 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || defined(PIONEER_Q38_GDN_GATED_NATIVE)
#error Exact separate OFFLINE timed-rendezvous card image required,no host/kernel/reference timer
#endif
// Official Blackhole baby-RISC CSRs.md:mcycle/h at0xb00/0xb80.
// ONLY read-only CSRRS with x0. No counter/inhibit/config/control writes.
// Existing physical ROOT cycle qualification uses this exact sampling form.
static __attribute__((noinline)) PioneerQ38RendezvousCycles PioneerQ38GdnRendezvousClockRead(){
 for(unsigned attempt=0;attempt<4;attempt++){
  uint32_t high,low,confirm;
  asm volatile("csrrs %0,0xb80,x0\ncsrrs %1,0xb00,x0\ncsrrs %2,0xb80,x0"
   :"=r"(high),"=r"(low),"=r"(confirm)::"memory");
  if(high==confirm)return (PioneerQ38RendezvousCycles(high)<<32)|low;
 }
 scalar_fault_park();
}
#endif
