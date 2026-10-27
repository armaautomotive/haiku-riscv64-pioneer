/* SPDX-License-Identifier: MIT */
// Isolated LOCAL arithmetic image; no native/Work/producer selection.
#define PIONEER_ATTENTION_FREESTANDING
#define PIONEER_Q38_SIGMOID_PRECISION_IMAGE 1
#include "PioneerQ38SigmoidPrecision.h"
static volatile uint32_t* Words(unsigned at){return reinterpret_cast<volatile uint32_t*>(at);}
extern "C" void scalar_fault_park() __attribute__((noreturn));
extern "C" unsigned scalar_main(){
 if(Words(0x74600)[0]!=0x31475351 || Words(0x74600)[1]!=64 || Words(0x74600)[2]!=1)return 0;
 for(unsigned i=3;i<32;i++)if(Words(0x74600)[i])return 0;
 float result[64];
 for(unsigned i=0;i<64;i++)if(!PioneerQ38SigmoidPrecision(PioneerAttentionFloat(Words(0x80000)[i]),result[i]))scalar_fault_park();
 for(unsigned i=0;i<64;i++)Words(0x81000)[i]=PioneerAttentionBits(result[i]);
 asm volatile("fence":::"memory");return 64;
}
