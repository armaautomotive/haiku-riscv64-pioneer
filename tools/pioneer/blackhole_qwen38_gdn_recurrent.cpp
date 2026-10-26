/* SPDX-License-Identifier: MIT */
// OFFLINE ONE worker image:resident raw Q/K/V/gates/A/dt -> prep ->
// resident CURRENT state -> SFPU recurrence -> DIFFERENT NEXT state.
// No intermediate terminal,host numeric input,CPU reference,PCI/DMA or
// host stop/reload. NOT selected by any native owner/driver/Work mode.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38MixedCapsuleHook.h"
typedef __UINT32_TYPE__ uint32_t;
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnRecurrentShape.h"
// Size-optimized descriptor checks may copy constant route tables. Fixed
// freestanding byte copy,not host DMA. Volatile prevents a recursive libcall.
extern "C" void* memcpy(void* target,const void* source,__SIZE_TYPE__ bytes){
 volatile unsigned char* out=static_cast<volatile unsigned char*>(target);
 const volatile unsigned char* in=static_cast<const volatile unsigned char*>(source);
 for(__SIZE_TYPE__ i=0;i<bytes;i++)out[i]=in[i];
 return target;
}
extern "C" unsigned q38_resident_gdn_input_stage();
extern "C" unsigned q38_resident_gdn_state_stage();
// Shared exact shape checks avoid embedding two copies in the16KiB saved
// code prefix. BOTH stages still independently call their complete check.
extern "C" __attribute__((noinline)) bool q38_resident_gdn_input_shape(const uint32_t* d){return PioneerQ38GdnInputShape(d);}
extern "C" __attribute__((noinline)) bool q38_resident_gdn_state_shape(const uint32_t* d){return PioneerQ38ResidentGdnShape(d);}
extern "C" void scalar_fault_park() __attribute__((noreturn));
static volatile uint32_t* Words(unsigned at){return reinterpret_cast<volatile uint32_t*>(at);}
static bool Descriptors(const uint32_t* input,const uint32_t* state){
 PIONEER_Q38_MIXED_SEAL();
 for(unsigned i=0;i<64;i++)if(Words(TT_Q38_GDN_INPUT_DESCRIPTOR)[i]!=input[i]
  || Words(TT_Q38_RGDN_DESCRIPTOR)[i]!=state[i])return false;
 return true;
}
static bool BoundaryCounters(){
 const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
 for(unsigned i=0;i<8;i++)if(*Words(0xffb20000+offsets[i])!=Words(TT_Q38_GDN_INPUT_SUMMARY)[16+i])return false;
 return true;
}
extern "C" unsigned scalar_main(){
 uint32_t input[64],state[64];
 for(unsigned i=0;i<64;i++){
  input[i]=Words(TT_Q38_GDN_INPUT_DESCRIPTOR)[i];state[i]=Words(TT_Q38_RGDN_DESCRIPTOR)[i];
 }
 if(!q38_resident_gdn_input_shape(input) || !q38_resident_gdn_state_shape(state)
  || !PioneerQ38GdnRecurrentPairing(input,state))return 0; // NO MMIO before complete paired shape
 if(q38_resident_gdn_input_stage()!=64 || !Descriptors(input,state) || !BoundaryCounters())scalar_fault_park();
 // The input stage's terminal is a function return INSIDE this worker.
 // Raw q/k/gates never leave card-local memory between these operations.
 if(q38_resident_gdn_state_stage()!=64 || !Descriptors(input,state))scalar_fault_park();
 // Each stage independently checks its real packets/counters/commands.
 // Also require unbroken counter continuity across the in-worker call.
 // The future native owner must still verify BOTH full receipt summaries.
 for(unsigned i=0;i<8;i++)if(Words(TT_Q38_GDN_INPUT_SUMMARY)[16+i]!=Words(TT_Q38_RGDN_SUMMARY)[8+i])scalar_fault_park();
 asm volatile("fence" ::: "memory");return 64; // local64 output,private next32KiB;NOT slot/token publication
}
