/* SPDX-License-Identifier: MIT */
// OFFLINE ONLY: no installed driver or existing Qwen Work image selects this.
#define PIONEER_DRAM_SWEEP_FREESTANDING
#include "PioneerDramReadSweep.h"
struct DramReadSweepIO {
 uint32_t Read(uint32_t a){return *reinterpret_cast<volatile uint32_t*>(a);}
 void Write(uint32_t a,uint32_t v){*reinterpret_cast<volatile uint32_t*>(a)=v;}
 void Fence(){asm volatile("fence" ::: "memory");}
 __attribute__((noinline)) bool Cycles(uint64_t& value){
  // Official Blackhole BabyRISCV CSRs: CSRRS x?,mcycle[h],x0, READ ONLY.
  // Bounded high/low/high sampling, never reset/inhibit/change any counter.
  for(unsigned attempt=0;attempt<4;attempt++){
   uint32_t high,low,confirm;
   asm volatile("csrrs %0,0xb80,x0\ncsrrs %1,0xb00,x0\ncsrrs %2,0xb80,x0"
    : "=r"(high),"=r"(low),"=r"(confirm) :: "memory");
   if(high==confirm){value=(uint64_t(high)<<32)|low;return true;}
  }
  return false;
 }
};
extern "C" void dram_sweep_fault_park() __attribute__((noreturn));
extern "C" unsigned dram_sweep_main(){
 DramReadSweepIO io;
 if(!PioneerDramReadSweep::Run(io))dram_sweep_fault_park();
 return 1;
}
