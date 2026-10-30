/* SPDX-License-Identifier: MIT */
// QAC1 isolated LOCAL arithmetic image, not a native/Work/worker image.
#define PIONEER_ATTENTION_FREESTANDING
#define PIONEER_Q38_PRECISE_ACTIVATION_CHUNK_IMAGE 1
#define PIONEER_Q38_SIGMOID_PRECISION_IMAGE 1
#include "PioneerQ38PreciseActivationChunk.h"
static volatile uint32_t* Words(unsigned address){return reinterpret_cast<volatile uint32_t*>(address);}
extern "C" void scalar_fault_park() __attribute__((noreturn));
extern "C" unsigned scalar_main(){
 if(Words(0x74600)[0]!=0x31434151 || Words(0x74600)[1]!=256 || Words(0x74600)[2]!=1
  || Words(0x74600)[4]!=576)return 0;
 const unsigned type=Words(0x74600)[3];
 if(type!=8 && type!=12 && type!=14)return 0;
 for(unsigned i=5;i<32;i++)if(Words(0x74600)[i])return 0;
 if(!PioneerQ38PreciseActivationChunk(type,reinterpret_cast<const float*>(0x80000),
  reinterpret_cast<const float*>(0x80400),256,reinterpret_cast<float*>(0x81000),
  reinterpret_cast<PioneerQ38ProjectionInput*>(0x81800)))scalar_fault_park();
 asm volatile("fence":::"memory");return 256;
}
